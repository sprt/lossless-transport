#include <stdio.h>
#include <string.h>
#include "packet_interface.h"

#define BUF_SIZE 1024

int main() {
	pkt_t *pkt = pkt_new();
	pkt_set_type(pkt, 1);
	pkt_set_tr(pkt, 0);
	pkt_set_window(pkt, 28);

	pkt_set_seqnum(pkt, 0x7b);
	char payload[] = "hello world";
	pkt_set_payload(pkt, payload, strlen(payload));
	pkt_set_timestamp(pkt, 0x17);

	char buf[BUF_SIZE];
	size_t n = BUF_SIZE;
	pkt_encode(pkt, buf, &n);

	for (size_t i = 0; i < n; i++) {
		printf("%02x ", (unsigned char) buf[i]);
	}
	printf("\n");

	pkt_del(pkt);
	return 0;
}
