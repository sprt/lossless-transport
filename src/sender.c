#include <fcntl.h>
#include <unistd.h>

#include "packet_interface.h"
#include "util.h"
#include "window.h"

const uint32_t TIMER = 4500000; /* retransmission timer (in microseconds) */

char *hostname; /* host we connect to */
uint16_t port; /* port we send to */
char *filename; /* file we read data from */

int sockfd = -1; /* socket we're operating on */
bool reached_eof = false; /* whether we're done reading from the file */
window_t *w; /* sending window, buffer contains in-flight packets */
// size_t next = 0; /* sequence number of the next packet to be sent */

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
	if (window_full(w) || reached_eof) {
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

/**
 * Sends a packet over the specified socket. Calls send and returns its value.
 * Exits if the packet couldn't be encoded.
 */
int send_packet(pkt_t *pkt) {
	char buf[MAX_PACKET_SIZE];
	size_t len = MAX_PACKET_SIZE;
	if (pkt_encode(pkt, buf, &len) != PKT_OK) {
		exit_msg("Error encoding packet: %d\n");
	}
	return send(sockfd, buf, len, 0);
}

/**
 * Resend each packet for which the retransmission timer has expired.
 * Exits on error.
 */
void retransmit_packets(void) {
	uint32_t now = get_monotime();
	pkt_t *delayed_pkt;
	while ((delayed_pkt = window_peek_min_timestamp(w)) != NULL) {
		uint32_t pkt_timer = pkt_get_timestamp(delayed_pkt);
		if (now < pkt_timer) {
			break;
		}
		/* Reschedule the retransmission of the packet in case it fails again */
		if (window_update_timestamp(w, pkt_timer, now + TIMER) == -1) {
			exit_msg("Cannot update timestamp of packet\n");
		}
		/* Resend it now */
		if (send_packet(delayed_pkt) == -1) {
			exit_perror("send");
		}
		/* The packet was in the buffer already so nothing else to do */
		log_msg("Resent packet #%03d (time=%d)\n",
			pkt_get_seqnum(delayed_pkt), pkt_get_timestamp(delayed_pkt));
	}
}

void read_write_loop(void) {
	while (true) {
		/* Initialize the set inside the loop as it is mutated by select */
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(sockfd, &read_fds);

		/* If we're done reading and all packets have been acknowledged,
		 * we can just quit */
		if (reached_eof && window_empty(w)) {
			log_msg("Reached EOF and window empty, breaking out\n");
			break;
		}

		/* Timeout will be zero unless the buffer is full or EOF was reached */
		struct timeval timeout = micro_to_timeval(get_timeout());
		log_msg("select waiting for %lu.%lus\n", timeout.tv_sec, timeout.tv_usec);

		/* Wait until either we receive data on the socket
		 * or the timeout expires */
		if (select(sockfd + 1, &read_fds, NULL, NULL, &timeout) == -1) {
			exit_perror("select");
		}

		if (FD_ISSET(sockfd, &read_fds)) {
			log_msg("got data on socket\n");
			char buf[MAX_PACKET_SIZE];
			int len = recv(sockfd, buf, MAX_PACKET_SIZE, 0);
			if (len == -1) {
				exit_perror("recv");
			}
			log_msg("received\n");
		}

		retransmit_packets();

		/* If the window isn't full and we still have data to read,
		 * just keep filling up the buffer */
		if (!window_full(w) && !reached_eof) {
			char buf[MAX_PAYLOAD_SIZE] = {0};
			ssize_t len = read(STDIN_FILENO, buf, MAX_PAYLOAD_SIZE);
			if (len == -1) {
				exit_perror("read");
			}

			if (len == 0) {
				log_msg("Reached EOF\n");
				reached_eof = true;
				continue;
			}

			pkt_t *pkt = pkt_new();
			if (pkt == NULL) {
				exit_msg("Error creating packet\n");
			}
			pkt_status_code err = PKT_OK;
			err = err || pkt_set_type(pkt, PTYPE_DATA);
			err = err || pkt_set_seqnum(pkt, 10);
			/* The sender has no receiving window */
			err = err || pkt_set_window(pkt, 0);
			err = err || pkt_set_timestamp(pkt, get_monotime() + TIMER);
			err = err || pkt_set_payload(pkt, buf, len);

			if (send_packet(pkt) == -1) {
				exit_perror("send");
			}

			/* Packet is in-flight and non-acknowledged,
			 * hence add it to the buffer */
			if (window_push(w, pkt) == -1) {
				exit_msg("Could not add packet to buffer\n");
			}

			log_msg("Sent   packet #%03d (time=%d)\n",
				pkt_get_seqnum(pkt), pkt_get_timestamp(pkt));
		}
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

	if (filename != NULL) {
		int fd = open(filename, O_RDONLY);
		if (fd == -1) {
			exit_perror("open");
		}
		if (dup2(fd, STDIN_FILENO) == -1) {
			exit_perror("dup2");
		}
	}

	read_write_loop();

	window_free(w);

	return 0;
}
