#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "packet_interface.h"
#include "util.h"
#include "window.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))

const uint32_t TIMER = 4500000; /* retransmission timer (in microseconds) */

char *hostname; /* host we connect to */
uint16_t port; /* port we send to */
char *filename; /* file we read data from */

int sockfd = -1; /* socket we're operating on */
FILE *infile; /* file we're reading data from */
window_t *w; /* sending window, buffer contains in-flight packets */
size_t next = 0; /* sequence number of the next packet to be sent */

/**
 * Returns how long the next call to select should wait (in microseconds).
 * If the buffer is full or EOF has been reached, returns the time until the
 * closest timer expiration (no less than zero). Otherwise, returns zero.
 */
uint32_t get_timeout(void) {
	/* Select waits until either the socket is ready for reading or the
	 * timeout expires. If the window is full or if we've reached EOF on the
	 * file, we can't write any more data right now and we should block,
	 * albeit at *most* until the closest timer in the window expires. */
	if (window_full(w) || feof(infile)) {
		uint32_t timer = pkt_get_timestamp(window_peek_min_timestamp(w));
		uint32_t now = get_monotime();
		/* select doesn't like negative timeouts, and we can't return
		 * a negative number through an unsigned type anyway */
		if (timer > now) {
			return timer - now;
		}
	}
	return 0;
}

void handle_ack(pkt_t *ack) {
	/* The seqnum field has the next sequence number expected by the receiver.
	 * The timestamp field corresponds to the last packet received by the receiver. */

	/* Whether the packet with the given timestamp has been popped from the buffer. */
	bool popped_timestamp = false;

	/* ACKs are cumulative so we can remove from the buffer all packets that
	 * have a smaller sequence number. */
	pkt_t *min_seqnum = window_peek_min_seqnum(w);
	while (min_seqnum != NULL && pkt_get_seqnum(min_seqnum) < pkt_get_seqnum(ack)) {
		window_pop_min_seqnum(w);
		if (pkt_get_timestamp(min_seqnum) == pkt_get_timestamp(ack)) {
			popped_timestamp = true;
		}

		window_slide(w);

		/* Resize the sending window according to the receiving window
		 * so as not to overload the receiver. */
		size_t swin = window_get_size(w);
		size_t rwin = pkt_get_window(ack);
		window_resize(w, MIN(swin, rwin));

		pkt_del(min_seqnum);
		min_seqnum = window_peek_min_seqnum(w);
	}

	if (!popped_timestamp) {
		pkt_t *last = window_pop_timestamp(w, pkt_get_timestamp(ack));
		if (last == NULL) {
			exit_msg("No packet with matching timestamp in buffer\n");
		}
		pkt_del(last);
	}
}

void handle_nack(pkt_t *nack) {
	if (!window_has(w, pkt_get_seqnum(nack))) {
		log_msg("Out of window, discarding\n");
		return;
	}

	pkt_t *match = window_find_seqnum(w, pkt_get_seqnum(nack));
	if (match == NULL) {
		exit_msg("No packet with matching timestamp in buffer\n");
	}

	/* Set the timer of the corresponding packet to the current time
	 * so that it will be resent immediately (retransmit_packets calls
	 * get_monotime after this). */
	pkt_set_timestamp(match, get_monotime());
}

/**
 * Resend each packet for which the retransmission timer has expired.
 * Exits on error.
 */
void retransmit_packets(void) {
	uint32_t now = get_monotime();
	pkt_t *pkt = window_peek_min_timestamp(w);

	while (pkt != NULL && pkt_get_timestamp(pkt) <= now) {
		/* Reschedule the retransmission of the packet in case it fails again */
		if (window_update_timestamp(w, pkt_get_timestamp(pkt), now + TIMER) == -1) {
			exit_msg("Cannot update timestamp of packet\n");
		}

		/* Resend it now */
		if (send_packet(sockfd, pkt) == -1) {
			exit_perror("send");
		}

		/* The packet was in the buffer already so nothing else to do */

		log_msg("Resent packet:\n");
		log_pkt(pkt);

		pkt = window_peek_min_timestamp(w);
	}
}

/**
 * Called inside an infinite loop. Exits on error.
 */
void main_loop(void) {
	/* Initialize the set inside the loop as it is mutated by select */
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(sockfd, &read_fds);

	/* Timeout will be zero unless the buffer is full or EOF was reached */
	uint32_t timeout_us = get_timeout();
	struct timeval timeout_tv = micro_to_timeval(timeout_us);
	log_msg("---------- Waiting for %.3fs...\n", (double) timeout_us / 1000000);

	/* Wait until either we receive data on the socket
	 * or the timeout expires */
	if (select(sockfd + 1, &read_fds, NULL, NULL, &timeout_tv) == -1) {
		exit_perror("select");
	}

	/* Received an ACK or a NACK */
	if (FD_ISSET(sockfd, &read_fds)) {
		log_msg("Received packet:\n");

		char buf[MAX_PACKET_SIZE];
		int len = recv(sockfd, buf, MAX_PACKET_SIZE, 0);
		if (len == -1) {
			exit_perror("recv");
		}

		pkt_t *resp = pkt_new();
		if (resp == NULL) {
			exit_msg("Could not allocate packet\n");
		}

		if (pkt_decode(buf, len, resp) != PKT_OK) {
			log_msg("Error decoding packet: %d\n", resp);
		} else {
			log_pkt(resp);

			switch (pkt_get_type(resp)) {
			case PTYPE_DATA:
				log_msg("DATA packet, ignoring\n");
				return;

			case PTYPE_ACK:
				log_msg("ACK for #%d\n", pkt_get_seqnum(resp) - 1);
				handle_ack(resp);
				break;

			case PTYPE_NACK:
				log_msg("NACK for #%d\n", pkt_get_seqnum(resp));
				handle_nack(resp);
				break;
			}
		}
	}

	retransmit_packets();

	/* If the window isn't full and we still have data to read,
	 * just keep filling up the buffer */
	if (!window_full(w) && !feof(infile)) {
		char buf[MAX_PAYLOAD_SIZE] = {0};
		size_t len = fread(buf, sizeof (*buf), MAX_PAYLOAD_SIZE, infile);

		if (len < MAX_PAYLOAD_SIZE) { /* either error or EOF */
			if (ferror(infile)) {
				exit_msg("Error reading from file\n");
			}
			log_msg("Reached EOF\n");
		}

		pkt_t *pkt = pkt_new();
		if (pkt == NULL) {
			exit_msg("Error creating packet\n");
		}

		pkt_status_code err = PKT_OK;
		err = err || pkt_set_type(pkt, PTYPE_DATA);
		err = err || pkt_set_seqnum(pkt, next);
		/* The sender has no receiving window */
		err = err || pkt_set_window(pkt, 0);
		err = err || pkt_set_timestamp(pkt, get_monotime() + TIMER);
		err = err || pkt_set_payload(pkt, buf, len);

		if (err != PKT_OK) {
			exit_msg("Could not create packet: %d\n", err);
		}

		if (send_packet(sockfd, pkt) == -1) {
			exit_perror("send");
		}

		next = (next + 1) % window_get_max_size(w);

		/* Packet is in-flight and non-acknowledged,
		 * hence add it to the buffer */
		if (window_push(w, pkt) == -1) {
			exit_msg("Could not add packet to buffer\n");
		}

		log_msg("Sent packet:\n");
		log_pkt(pkt);
	}
}

int main(int argc, char **argv) {
	parse_args(argc, argv, &hostname, &port, &filename);

	w = window_create(1, MAX_WINDOW_SIZE);
	if (w == NULL) {
		exit_msg("window_create: Could not create window\n");
	}

	struct sockaddr_in6 dst_addr;
	const char *err = real_address(hostname, &dst_addr);
	if (err != NULL) {
		exit_msg("real_address: %s\n", err);
	}

	/* Connect the socket. This tells the socket both to send to this
	 * address by default and to only receive from this address. */
	sockfd = create_socket(NULL, -1, &dst_addr, port);
	if (sockfd == -1) {
		exit_msg("create_socket: Could not create socket\n");
	}

	if (filename == NULL) {
		infile = stdin;
	} else {
		infile = fopen(filename, "rb");
		if (infile == NULL) {
			exit_perror("fopen");
		}
	}

	while (!feof(infile) || !window_empty(w)) {
		/* This is probably the line you're looking for */
		main_loop();
	}

	window_free(w);

	return 0;
}
