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

	// if (window_full(w)) {
	// 	log_msg("Window full, discarding\n");
	// 	return;
	// }

	/* Read the datagram received into a buffer */
	char buf[MAX_PACKET_SIZE];
	int len = recv(sockfd, buf, MAX_PACKET_SIZE, 0);
	if (len == -1) {
		exit_perror("recv");
	}

	/* Decode the datagram into a packet */
	pkt_t *pkt = pkt_new();
	if (pkt == NULL) {
		exit_msg("Could not allocate packet\n");
	}
	if (pkt_decode(buf, len, pkt) != PKT_OK) {
		log_msg("Error decoding packet (%d), ignoring", pkt_decode);
		return;
	}

	log_msg("Seqnum=%03d, TR=%d, Length=%03d, Timestamp=%010d\n",
		pkt_get_seqnum(pkt), pkt_get_tr(pkt), pkt_get_length(pkt),
		pkt_get_timestamp(pkt));
}

int main(int argc, char **argv) {
	parse_args(argc, argv, &hostname, &port, &filename);

	if (filename == NULL) {
		outfile = stdin;
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
