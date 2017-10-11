#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "packet_interface.h"
#include "util.h"

static char *hostname;
static uint16_t port;
static char *filename;

/**
 * Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket.
 * @sockfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
static void read_write_loop(int sockfd) {
	while (true) {
		// We have to reinitialize read_fds at each iteration as select
		// mutates it.
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(STDIN, &read_fds);
		FD_SET(sockfd, &read_fds);

		if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			return;
		}

		fprintf(stderr, "waiting\n");

		if (FD_ISSET(STDIN, &read_fds)) {
			fprintf(stderr, "got data on stdin\n");
			char buf[MAX_PACKET_SIZE];
			ssize_t len = read(STDIN, buf, MAX_PACKET_SIZE);
			if (len == -1) {
				perror("read");
				return;
			}
			if (send(sockfd, buf, len, 0) == -1) {
				perror("send");
				return;
			}
			fprintf(stderr, "sent\n");
			// return;
		}

		if (FD_ISSET(sockfd, &read_fds)) {
			fprintf(stderr, "got data on socket\n");
			char buf[MAX_PACKET_SIZE];
			int len = recv(sockfd, buf, MAX_PACKET_SIZE, 0);
			if (len == -1) {
				perror("recv");
				return;
			}
			fprintf(stderr, "received\n");
		}
	}
}

/**
 * Block the caller until a message is received on sockfd,
 * and connect the socket to the source addresse of the received message.
 * @sockfd: a file descriptor to a bound socket but not yet connected.
 * @return: 0 in case of success, -1 otherwise.
 * @POST: This call is idempotent, it does not 'consume' the data of the
 *        message, and could be repeated several times blocking only at the
 *        first call.
 */
// static int wait_for_client(int sockfd) {
// 	char buf[1024] = {0};
// 	struct sockaddr addr = {0};
// 	socklen_t sizeof_addr = sizeof (addr);
// 	if (recvfrom(sockfd, buf, 1024, MSG_PEEK, &addr, &sizeof_addr) == -1) {
// 		perror("recvfrom");
// 		return -1;
// 	}
// 	if (connect(sockfd, &addr, sizeof (addr)) == -1) {
// 		perror("connect");
// 		return -1;
// 	}
// 	return 0;
// }

int main(int argc, char **argv) {
	parse_args(argc, argv, &hostname, &port, &filename);

	struct sockaddr_in6 dst_addr;
	const char *err = real_address(hostname, &dst_addr);
	if (err != NULL) {
		fprintf(stderr, "real_address: %s\n", err);
		exit(1);
	}

	// Connect the socket
	int sockfd = create_socket(NULL, -1, &dst_addr, port);
	if (sockfd == -1) {
		exit(1);
	}

	read_write_loop(sockfd);

	// if (wait_for_client(sender_sock) == -1) {
	// 	exit(1);
	// }

	// read_write_loop(receiver_sock);

	return 0;
}
