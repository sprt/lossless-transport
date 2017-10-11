#include <getopt.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "packet_interface.h"
#include "util.h"

static char *hostname;
static uint16_t port;
static char *filename;

/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket.
 * @sockfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
// static void read_write_loop(int sockfd) {
// 	while (true) {
// 		// We have to reinitialize read_fds at each iteration as select
// 		// mutates it.
// 		fd_set read_fds;
// 		FD_ZERO(&read_fds);
// 		FD_SET(STDIN, &read_fds);
// 		FD_SET(sockfd, &read_fds);

// 		if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) == -1) {
// 			perror("select");
// 			return;
// 		}

// 		fprintf(stderr, "waiting\n");

// 		if (FD_ISSET(STDIN, &read_fds)) {
// 			char buf[1024] = {0};
// 			size_t len = 0;
// 			int c;
// 			while ((c = fgetc(stdin)) != EOF) {
// 				buf[len++] = c;
// 			}
// 			if (ferror(stdin) != 0) {
// 				fprintf(stderr, "Error reading from stdin\n");
// 				clearerr(stdin);
// 				return;
// 			}
// 			if (send(sockfd, buf, len, 0) == -1) {
// 				perror("send");
// 				return;
// 			}
// 			fprintf(stderr, "stdin -> socket: %s\n", buf);
// 			return;
// 		}

// 		if (FD_ISSET(sockfd, &read_fds)) {
// 			char buf[1024] = {0};
// 			int len = recv(sockfd, buf, 1024, 0);
// 			if (len == -1) {
// 				perror("recv");
// 				return;
// 			}
// 			fprintf(stderr, "socket -> stdout: %s\n", buf);
// 		}
// 	}
// }

/**
 * Block the caller until a message is received on sockfd,
 * and connect the socket to the source addresse of the received message.
 * @sockfd: a file descriptor to a bound socket but not yet connected.
 * @return: 0 in case of success, -1 otherwise.
 * @POST: This call is idempotent, it does not 'consume' the data of the
 *        message, and could be repeated several times blocking only at the
 *        first call.
 */
static int wait_for_client(int sockfd) {
	// static bool connected = false;
	// if (connected) {
	// 	return 0;
	// }
	char buf[MAX_PACKET_SIZE];
	struct sockaddr_in6 addr;
	socklen_t addrlen = sizeof (addr);
	if (recvfrom(sockfd, buf, MAX_PACKET_SIZE, 0 /* TODO: MSG_PEEK */, (struct sockaddr *) &addr, &addrlen) == -1) {
		perror("recvfrom");
		return -1;
	}
	if (connect(sockfd, (struct sockaddr *) &addr, addrlen) == -1) {
		perror("connect");
		return -1;
	}
	// connected = true;
	return 0;
}

int main(int argc, char **argv) {
	parse_args(argc, argv, &hostname, &port, &filename);

	struct sockaddr_in6 addr;
	const char *err = real_address(hostname, &addr);
	if (err != NULL) {
		fprintf(stderr, "real_address: %s\n", err);
		exit(1);
	}

	// Bind the socket
	int sockfd = create_socket(&addr, port, NULL, -1);
	if (sockfd == -1) {
		exit(1);
	}

	while (true) {
		fprintf(stderr, "waiting for first packet...\n");
		if (wait_for_client(sockfd) == 0) {
			fprintf(stderr, "received first packet, connected\n");
		}
	}

	return 0;
}
