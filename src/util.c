#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "packet_interface.h"

char pkt_fmt_buf[1024];

void print_time(void) {
	struct timespec tp;
	if (clock_gettime(CLOCK_MONOTONIC, &tp) == -1) {
		abort();
	}
	double secs = tp.tv_sec + ((double) tp.tv_nsec) / 1000000000;
	fprintf(stderr, "[%7.3f] ", secs);
}

void log_msg(const char *fmt, ...) {
	print_time();
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

void log_perror(const char *s) {
	print_time();
	perror(s);
}

void exit_msg(const char *fmt, ...) {
	print_time();
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(1);
}

void exit_perror(const char *s) {
	print_time();
	fprintf(stderr, "%s: %s\n", s, strerror(errno));
	exit(1);
}

void exit_usage(char **argv) {
	fprintf(stderr, "Usage: %s <hostname> <port> [-f FILE]\n", argv[0]);
	exit(2);
}

char *pkt_repr(pkt_t *pkt) {
	if (pkt == NULL) {
		return "(nil)";
	}

	char *type;
	switch (pkt_get_type(pkt)) {
		case PTYPE_ACK:  type = "ACK";     break;
		case PTYPE_DATA: type = "DATA";    break;
		case PTYPE_NACK: type = "NACK";    break;
		default:         type = "UNKNOWN";
	}

	int len = sprintf(pkt_fmt_buf, "{Type=%s, TR=%d, Win=%d, Seq=%d, Time=%d, Len=%d}",
		type, pkt_get_tr(pkt), pkt_get_window(pkt), pkt_get_seqnum(pkt),
		pkt_get_timestamp(pkt), pkt_get_length(pkt));
	if (len < 0) {
		abort();
	}

	return pkt_fmt_buf;
}

void parse_args(int argc, char **argv,
                char **hostname, uint16_t *port, char **filename) {
	int c;
	while ((c = getopt(argc, argv, "f:h")) != -1) {
		switch (c) {
		case 'f':
			*filename = optarg;
			break;
		case 'h':
		case '?':
			exit_usage(argv);
			break;
		default:
			abort(); // unreachable
		}
	}

	if (optind + 2 != argc) {
		fprintf(stderr, "%s: wrong number of arguments\n", argv[0]);
		exit_usage(argv);
	}

	*hostname = argv[optind++];
	*port = atoi(argv[optind]);

	if (*port == 0 || *port >> 16) {
		fprintf(stderr, "%s: port must be an integer between 1 and 65535\n", argv[0]);
		exit_usage(argv);
	}
}

const char *real_address(const char *address, struct sockaddr_in6 *rval) {
	struct addrinfo hint = {0};
	hint.ai_family = AF_INET6;
	hint.ai_socktype = SOCK_DGRAM;
	hint.ai_protocol = IPPROTO_UDP;

	struct addrinfo info;
	struct addrinfo *infoptr = &info;
	int err = getaddrinfo(address, NULL, &hint, &infoptr);
	if (err != 0) {
		return gai_strerror(err);
	}

	struct sockaddr_in6 *addr = (struct sockaddr_in6 *) infoptr->ai_addr;
	*rval = *addr;

	freeaddrinfo(infoptr);
	return NULL;
}

int create_socket(struct sockaddr_in6 *source_addr, int src_port,
                  struct sockaddr_in6 *dest_addr, int dst_port) {
	int sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1) {
		log_perror("socket");
		return -1;
	}

	if (source_addr != NULL && src_port > 0) {
		source_addr->sin6_port = htons(src_port);
		if (bind(sockfd, (struct sockaddr *) source_addr, sizeof (*source_addr)) != 0) {
			log_perror("bind");
			return -1;
		}
	}

	if (dest_addr != NULL && dst_port > 0) {
		dest_addr->sin6_port = htons(dst_port);
		if (connect(sockfd, (struct sockaddr *) dest_addr, sizeof (*dest_addr)) != 0) {
			log_perror("connect");
			return -1;
		}
	}

	return sockfd;
}

int send_packet(int sockfd, pkt_t *pkt) {
	char buf[MAX_PACKET_SIZE];
	size_t len = MAX_PACKET_SIZE;
	if (pkt_encode(pkt, buf, &len) != PKT_OK) {
		exit_msg("Error encoding packet: %d\n");
	}
	return send(sockfd, buf, len, 0);
}

uint32_t get_monotime(void) {
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
	return now.tv_sec * 1000000 + now.tv_usec;
}

struct timeval micro_to_timeval(uint32_t us) {
	struct timeval tv;
	tv.tv_sec = us / 1000000;
	tv.tv_usec = us % 1000000;
	return tv;
}
