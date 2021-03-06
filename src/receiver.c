#include <assert.h>
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
	log_msg("---------- Waiting for a packet...\n");
	log_msg("Window: [%zu, %zu], buffer: %zu/%zu\n", window_start(w),
		window_end(w), window_buffer_size(w), window_get_size(w));

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

	pkt_status_code decerr = pkt_decode(buf, len, pkt);
	if (decerr != PKT_OK) {
		log_msg("Error decoding packet (%s), ignoring\n", pkt_code_to_str(decerr));
		pkt_del(pkt);
		return;
	}

	log_msg("< %s\n", pkt_repr(pkt));

	if (!window_has(w, pkt_get_seqnum(pkt))) {
		log_msg("Out of window, ignoring\n");
		pkt_del(pkt);
		return;
	}

	/* We're gonna send a (N)ACK so let's allocate a new packet to reply */
	pkt_t *reply = pkt_new();
	if (reply == NULL) {
		exit_msg("Could not allocate reply packet\n");
	}

	if (pkt_get_tr(pkt)) {
		/* Send a NACK if we receive a truncated packet */
		pkt_status_code err = PKT_OK;
		err = err || pkt_set_type(reply, PTYPE_NACK);
		/* We don't store truncated packets so the window size doesn't change */
		err = err || pkt_set_window(reply, window_available(w));
		err = err || pkt_set_seqnum(reply, pkt_get_seqnum(pkt));

		if (err != PKT_OK) {
			exit_msg("Could not create packet: %s\n",
				pkt_code_to_str(err));
		}

		if (send_packet(sockfd, reply) == -1) {
			exit_perror("Could not send NACK: send:");
		}

		pkt_del(pkt);
		log_msg("> %s\n", pkt_repr(reply));
	} else {
		/* We're gonna send an ACK, but first we store the received
		 * packet in the buffer, and then we try to write out packets to
		 * the file, so that we can reply with an accurate window size. */

		if (window_find_seqnum(w, pkt_get_seqnum(pkt)) != NULL) {
			log_msg("Already in buffer\n");
		} else {
			if (window_push(w, pkt) == -1) {
				if (window_full(w)) {
					log_msg("Buffer full, not adding\n");
					// pkt_del(pkt);
					// pkt_del(reply);
					// return;
				} else {
					exit_msg("Could not add packet to buffer\n");
				}
			} else {
				log_msg("%d\n", pkt_get_seqnum(pkt));
			}
		}

		/* Save the timestamp of the packet we just received in case
		 * it's immediately removed from the buffer and freed. */
		uint32_t ack_timestamp = pkt_get_timestamp(pkt);

		/* Find the next in-sequence packet. If it's not in the buffer,
		 * we can't acknowledge any packet. */
		pkt_t *next_pkt = window_find_seqnum(w, window_start(w));
		while (next_pkt != NULL) {
			/* Try to write the packet to the file. Iff this succeeds,
			 * we can pop it from the buffer and slide the window. */

			const char *payload = pkt_get_payload(next_pkt);
			size_t payload_len = pkt_get_length(next_pkt);

			if (fwrite(payload, sizeof (*payload), payload_len, outfile) < payload_len) {
				exit_msg("Error writing to file\n");
			}

			log_msg("Wrote packet #%d\n", pkt_get_seqnum(next_pkt));
			assert(window_pop_timestamp(w, pkt_get_timestamp(next_pkt)) == next_pkt);

			/* We don't store truncated packets in the buffer so no
			* need to check for that */
			if (pkt_get_length(next_pkt) == 0) {
				log_msg("Received EOF packet, ready to quit\n");
			} else {
				/* Don't slide the window when we receive the
				* EOF packet. Rationale: the ACK may get lost
				* and when the sender retransmits the EOF packet,
				* it would fall outside our window. */
				window_slide(w);
			}

			pkt_del(next_pkt);
			next_pkt = window_find_seqnum(w, window_start(w));
		}

		assert(fflush(outfile) == 0);

		/* There's no duplication so we can just send an ACK */

		pkt_status_code err = PKT_OK;
		err = err || pkt_set_type(reply, PTYPE_ACK);
		err = err || pkt_set_window(reply, window_available(w));
		err = err || pkt_set_seqnum(reply, window_start(w));
		err = err || pkt_set_timestamp(reply, ack_timestamp);

		if (err != PKT_OK) {
			exit_msg("Could not create packet: %s\n",
				pkt_code_to_str(err));
		}

		if (send_packet(sockfd, reply) == -1) {
			exit_perror("Could not send ACK: send:");
		}

		log_msg("> %s\n", pkt_repr(reply));

		log_msg("Window: [%zu, %zu], buffer: %zu/%zu\n", window_start(w),
			window_end(w), window_buffer_size(w), window_get_size(w));
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

	w = window_create(MAX_WINDOW_SIZE, MAX_WINDOW_SIZE, 255);
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

	log_msg("Waiting for sender...\n");
	if (wait_for_client() == -1) {
		exit_msg("Error waiting for sender\n");
	}

	log_msg("Received first packet, connected\n");

	while (true) {
		/* This is probably the line you're looking for */
		main_loop();
	}

	window_free(w);
	fclose(outfile);

	return 0;
}
