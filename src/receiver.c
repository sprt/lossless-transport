#include <stdio.h>
#include <stdlib.h>

#include "packet_interface.h"
#include "util.h"
#include "window.h"

char *hostname; /* host we bind to */
uint16_t port; /* port we receive on */
char *filename; /* file on which we write out data */

int sockfd = -1; /* socket we're listening on */
FILE *outfile; /* file we're writing out the data to */
window_t *w; /* receiving window, buffer contains out-of-sequence packets */
size_t next = 0; /* sequence number of the next expected packet */

/**
 * Blocks until we receive the first packet and then establishes the
 * connection to the sender. Does not consume the message (ie. a subsequent call
 * to recv will see the same data). On error, prints on stderr and returns -1;
 * otherwise, returns 0. Assumes the socket isn't connected.
 */
int wait_for_client(void) {
	char buf[MAX_PACKET_SIZE];
	struct sockaddr_in6 addr;
	socklen_t addrlen = sizeof (addr);

	/* Wait for a packet from anyone */
	if (recvfrom(sockfd, buf, MAX_PACKET_SIZE, MSG_PEEK, (struct sockaddr *) &addr, &addrlen) == -1) {
		log_perror("recvfrom");
		return -1;
	}

	/* For UDP, connecting merely implies setting the address to which data
	 * is sent by default, and the only address from which data is received. */
	if (connect(sockfd, (struct sockaddr *) &addr, addrlen) == -1) {
		log_perror("connect");
		return -1;
	}

	return 0;
}

/**
 * Called inside an infinite loop. Exits on error.
 */
void main_loop(void) {
	/* Initialize the set inside the loop as it is mutated by select */
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(sockfd, &read_fds);

	log_msg("Waiting for a packet\n");
	if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) == -1) {
		exit_perror("select");
	}

	/* If this is reached, select returned and we know we have a packet
	 * since read_fds only contains one file descriptor. */

	/* Read the datagram received into a buffer */
	char buf[MAX_PACKET_SIZE];
	int len = recv(sockfd, buf, MAX_PACKET_SIZE, 0);
	if (len == -1) {
		exit_perror("recv");
	}

	/* Decode the datagram into a packet */
	pkt_t *pkt = pkt_new();
	if (pkt_decode(buf, len, pkt) != PKT_OK) { /* guards from pkt being NULL */
		log_msg("Error decoding packet (%d), ignoring", pkt_decode);
		return;
	}

	log_msg("Seqnum=%03d, TR=%d, Length=%03d, Timestamp=%010d\n",
		pkt_get_seqnum(pkt), pkt_get_tr(pkt), pkt_get_length(pkt),
		pkt_get_timestamp(pkt));

	if (!window_has(w, pkt_get_seqnum(pkt))) {
		log_msg("Out of window, discarding");
		pkt_del(pkt);
		return;
	}

	/* We're gonna send a (N)ACK so let's allocate a new packet to reply */
	pkt_t *reply = pkt_new();
	if (reply == NULL) {
		exit_msg("Could not allocate reply packet\n");
	}

	if (pkt_get_tr(pkt)) {
		pkt_del(pkt);

		/* Send a NACK if we receive a truncated packet */
		pkt_status_code err = PKT_OK;
		err = err || pkt_set_type(reply, PTYPE_NACK);
		/* We don't store truncated packets so the window size doesn't change */
		err = err || pkt_set_window(reply, window_available(w));
		err = err || pkt_set_seqnum(reply, pkt_get_seqnum(pkt));

		if (err != PKT_OK) {
			exit_msg("Could not create packet: %d\n", err);
		}

		if (send_packet(sockfd, reply) == -1) {
			exit_perror("Could not send NACK: send:");
		}
	} else {
		/* We're gonna send an ACK, but first we store the received
		 * packet in the buffer, and then we try to write out packets to
		 * the file, so that we can reply with an accurate window size. */

		if (window_push(w, pkt) == -1) {
			exit_msg("Could not add packet to buffer (full=%d)", window_full(w));
		}

		/* Write out the in-sequence packets that are at the beginning
		 * of the window.
		 *
		 * If the sequence number of the first packet in the buffer
		 * corresponds to the first sequence number in the window, then
		 * the first packet in the buffer is in-sequence, and we can
		 * write it out to the file. */
		pkt_t *min_seqnum = window_peek_min_seqnum(w);
		while (min_seqnum != NULL && pkt_get_seqnum(min_seqnum) == window_start(w)) {
			/* Try to write the packet to the file. Iff this
			 * succeeds, we can pop it from the buffer and slide the
			 * window. */

			char fbuf[MAX_PACKET_SIZE];
			size_t flen = MAX_PACKET_SIZE;

			pkt_status_code err = pkt_encode(min_seqnum, fbuf, &flen);
			if (err != PKT_OK) {
				exit_msg("Could not encode packet to store in buffer: %d", err);
			}

			if (fwrite(fbuf, sizeof (*fbuf), flen, outfile) < flen) {
				exit_msg("Error writing to file\n");
			}

			window_pop_min_seqnum(w);
			window_slide(w);

			pkt_del(min_seqnum);
			min_seqnum = window_peek_min_seqnum(w);
		}

		/* There's no duplication so we can just send an ACK */

		pkt_status_code err = PKT_OK;
		err = err || pkt_set_type(reply, PTYPE_ACK);
		err = err || pkt_set_window(reply, window_available(w));
		err = err || pkt_set_seqnum(reply, window_start(w));
		err = err || pkt_set_timestamp(reply, pkt_get_timestamp(pkt));

		if (err != PKT_OK) {
			exit_msg("Could not create packet: %d\n", err);
		}

		if (send_packet(sockfd, reply) == -1) {
			exit_perror("Could not send ACK: send:");
		}
	}

	pkt_del(reply);
}

int main(int argc, char **argv) {
	parse_args(argc, argv, &hostname, &port, &filename);

	if (filename == NULL) {
		outfile = stdout;
	} else {
		outfile = fopen(filename, "wb+");
		if (outfile == NULL) {
			exit_perror("fopen");
		}
	}

	w = window_create(MAX_WINDOW_SIZE, MAX_WINDOW_SIZE);
	if (w == NULL) {
		exit_msg("Could not create window\n");
	}

	struct sockaddr_in6 addr;
	const char *err = real_address(hostname, &addr);
	if (err != NULL) {
		exit_msg("real_address: %s\n", err);
	}

	/* Bind the socket */
	sockfd = create_socket(&addr, port, NULL, -1);
	if (sockfd == -1) {
		exit(1);
	}

	log_msg("Waiting for sender\n");
	if (wait_for_client() == -1) {
		exit_msg("Error waiting for sender\n");
	}

	log_msg("Received first packet, connected\n");

	while (true) {
		/* This is probably the line you're looking for */
		main_loop();
	}

	return 0;
}
