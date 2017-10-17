#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "packet_interface.h"
#include "util.h"

static char *hostname;
static uint16_t port;
static char *filename;

static void read_write_loop(int sockfd) {
	fd_set master;
	FD_ZERO(&master);
	FD_SET(STDIN_FILENO, &master);
	FD_SET(sockfd, &master);

	while (true) {
		/* Make a copy of the master set at each iteration as later
		 * we clear stdin from the set when we hit EOF. */
		fd_set read_fds = master;

		log_msg("waiting\n");
		if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) == -1) {
			exit_perror("select");
		}

		if (FD_ISSET(STDIN_FILENO, &read_fds)) {
			log_msg("got data from input\n");
			char buf[MAX_PACKET_SIZE];
			ssize_t len = read(STDIN_FILENO, buf, MAX_PACKET_SIZE);
			if (len == -1) {
				exit_perror("read");
			}
			if (len == 0) {
				log_msg("reach eof\n");
				FD_CLR(STDIN_FILENO, &master);
			} else {
				if (send(sockfd, buf, len, 0) == -1) {
					exit_perror("send");
				}
				log_msg("sent\n");
			}
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
	}
}

int main(int argc, char **argv) {
	parse_args(argc, argv, &hostname, &port, &filename);

	struct sockaddr_in6 dst_addr;
	const char *err = real_address(hostname, &dst_addr);
	if (err != NULL) {
		exit_msg("real_address: %s\n", err);
	}

	/* Connect the socket */
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
