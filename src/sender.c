#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "packet_interface.h"

char *hostname;
uint16_t port;
char *filename;

void exit_usage(char **argv) {
	fprintf(stderr, "\nUsage: %s <hostname> <port> [-f FILE]\n", argv[0]);
	exit(1);
}

void parse_args(int argc, char **argv) {
	int c;
	while ((c = getopt(argc, argv, "f:")) != -1) {
		switch (c) {
		case 'f':
			filename = optarg;
			break;
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

	hostname = argv[optind++];
	port = atoi(argv[optind]);

	if (port == 0 || port >> 16) {
		fprintf(stderr, "%s: port must be an integer between 1 and 65535\n", argv[0]);
		exit_usage(argv);
	}
}

int main(int argc, char **argv) {
	parse_args(argc, argv);

	return 0;
}
