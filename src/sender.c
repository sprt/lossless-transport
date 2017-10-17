#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "packet_interface.h"
#include "util.h"
#include "window.h"

static const uint32_t TIMER = 4500000; /* retransmission timer (in microseconds) */

static char *hostname; /* host we connect to */
static uint16_t port; /* port we send to */
static char *filename; /* file we read data from */

static bool reached_eof = false; /* whether we're done reading from the file */
static window_t *w; /* sending window, buffer contains in-flight packets */

/**
 * Returns the amount of time in microseconds eapsed since an arbitrary point
 * in time in a STRICTLY monotonic fashion.
 * The returned values being strictly monotonic is crucial to the correct
 * identification of ACKs received for out-of-sequence packets, as it relies on
 * the timestamp field.
 * The first call returns 0.
 */
static uint32_t get_monotime() {
	static bool first_call = true; /* whether this is the first call */
	static struct timeval first; /* unnormalized value returned by the first call */
	static struct timeval last; /* unnormalized value returned by the last call */

	struct timespec now_tp;
	if (clock_gettime(CLOCK_MONOTONIC, &now_tp) == -1) {
		exit_perror("clock_gettime");
	}

	/* Convert the timespec to a timeval */
	struct timeval now;
	now.tv_sec = now_tp.tv_sec;
	now.tv_usec = now_tp.tv_nsec / 1000;

	if (first_call) {
		/* Remember the value of the first call to normalize to 0 later */
		first = now;
		first_call = false;
	} else if (timercmp(&now, &last, <) || !timercmp(&now, &last, !=)) {
		/* Increment if not strictly greater than the last time returned */

		struct timeval us;
		us.tv_sec = 0;
		us.tv_usec = 1;

		struct timeval res;
		timeradd(&last, &us, &res);
		now = res;
	}

	/* Save the time we're returning for later calls */
	last = now;

	/* Normalize to start at 0 */
	struct timeval res;
	timersub(&now, &first, &res);
	now = res;

	/* Convert to microseconds */
	uint32_t now_ms = now.tv_sec * 1000000 + now.tv_usec;

	return now_ms;
}

/**
 * Returns how long the next call to select should wait (in microseconds).
 * If the buffer is full or EOF has been reached, returns the time until the
 * closest timer expiration. Otherwise, returns zero.
 */
inline static uint32_t get_timeout() {
	if (window_full(w) || reached_eof) {
		uint32_t timer = pkt_get_timestamp(window_peek_min_timestamp(w));
		return timer - get_monotime();
	}
	return 0;
}

/**
 * Converts microseconds to a struct timeval.
 */
inline static struct timeval micro_to_timeval(uint32_t us) {
	struct timeval tv;
	tv.tv_sec = us / 1000000;
	tv.tv_usec = us % 100000;
	return tv;
}

static void read_write_loop(int sockfd) {
	while (true) {
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(sockfd, &read_fds);

		/* If we're done reading and all packets have been acknowledged,
		 * we can just quit. */
		if (reached_eof && window_empty(w)) {
			log_msg("Reached EOF and window empty, breaking out\n");
			break;
		}

		log_msg("waiting\n");
		struct timeval timeout = micro_to_timeval(get_timeout());
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

			char pkt_raw[MAX_PACKET_SIZE] = {0};
			size_t pkt_len = MAX_PACKET_SIZE;

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
			err = err || pkt_encode(pkt, pkt_raw, &pkt_len);

			if (err != PKT_OK) {
				exit_msg("Error creating packet: %d\n", err);
			}

			if (send(sockfd, pkt_raw, pkt_len, 0) == -1) {
				exit_perror("send");
			}

			log_msg("Sent %3d-byte packet #%03d (time=%d)\n",
				pkt_len, pkt_get_seqnum(pkt), pkt_get_timestamp(pkt));
		}
	}
}

int main(int argc, char **argv) {
	parse_args(argc, argv, &hostname, &port, &filename);

	w = window_create(10, MAX_WINDOW_SIZE);
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
	int sockfd = create_socket(NULL, -1, &dst_addr, port);
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

	read_write_loop(sockfd);

	return 0;
}
