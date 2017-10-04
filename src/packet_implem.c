#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <zlib.h>

#include "packet_interface.h"

struct __attribute__((__packed__)) pkt {
	struct __attribute__((__packed__)) {
		/* From the LSB to the MSB:
		 * Type (2 bits), TR (1 bit), Window (5 bits).
		 *
		 * Didn't use bit fields as their order in memory isn't
		 * well-defined. */
		uint8_t ttw;

		uint8_t seqnum;
		uint16_t length;
		uint32_t timestamp;
	} *header;
	uint32_t crc1;

	char *payload;
	uint32_t crc2;
};

pkt_t* pkt_new() {
	pkt_t *pkt = malloc(sizeof (pkt_t));
	if (pkt == NULL) {
		return NULL;
	}

	pkt->payload = NULL;
	pkt->crc2 = 0;

	pkt->header = malloc(sizeof (*pkt->header));
	if (pkt->header == NULL) {
		pkt_del(pkt);
		return NULL;
	}

	pkt->header->ttw = 0;
	pkt->header->seqnum = 0;
	pkt->header->length = 0;
	pkt->header->timestamp = 0;
	pkt->crc1 = 0;

	return pkt;
}

void pkt_del(pkt_t *pkt) {
	free(pkt->header);
	free(pkt->payload);
	free(pkt);
}

/* Ignore GCC warning. */
#define UNUSED(x) (void) (x)

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt) {
	UNUSED(data);
	UNUSED(len);
	UNUSED(pkt);
	return E_UNCONSISTENT;
}

pkt_status_code pkt_encode(const pkt_t *pkt, char *buf, size_t *len) {
	uint16_t length = pkt_get_length(pkt);
	size_t total = sizeof (*pkt->header) + sizeof (pkt->crc1);
	size_t written = 0;

	if (length > 0) {
		total += length * sizeof (pkt->payload[0]);
		total += sizeof (pkt->crc2);
	}

	if (total > *len) {
		*len = 0;
		return E_NOMEM;
	}

	memcpy(buf + written, pkt->header, sizeof (*pkt->header));
	written += sizeof (*pkt->header);

	uint32_t header_crc = htonl(crc32(0, (unsigned char *) pkt->header, sizeof (*pkt->header)));
	memcpy(buf + written, &header_crc, sizeof (header_crc));
	written += sizeof (header_crc);

	if (length > 0) {
		size_t payload_size = length * sizeof (pkt->payload[0]);

		memcpy(buf + written, pkt->payload, payload_size);
		written += payload_size;

		uint32_t payload_crc = htonl(crc32(0, (unsigned char *) pkt->payload, payload_size));
		memcpy(buf + written, &payload_crc, sizeof (payload_crc));
		written += sizeof (payload_crc);
	}

	*len = written;
	return PKT_OK;
}


/*
 * Getters
 */

ptypes_t pkt_get_type(const pkt_t* pkt) {
	return (ptypes_t) (pkt->header->ttw & 0x3);
}

uint8_t pkt_get_tr(const pkt_t* pkt) {
	return (uint8_t) ((pkt->header->ttw & 0x4) >> 2);
}

uint8_t pkt_get_window(const pkt_t* pkt) {
	return (uint8_t) ((pkt->header->ttw & 0xf8) >> 3);
}

uint8_t pkt_get_seqnum(const pkt_t* pkt) {
	return (uint8_t) pkt->header->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt) {
	return ntohs(pkt->header->length);
}

uint32_t pkt_get_timestamp(const pkt_t* pkt) {
	return pkt->header->timestamp;
}

uint32_t pkt_get_crc1(const pkt_t* pkt) {
	return ntohl(pkt->crc1);
}

uint32_t pkt_get_crc2(const pkt_t* pkt) {
	return ntohl(pkt->crc2);
}

const char* pkt_get_payload(const pkt_t* pkt) {
	return pkt->payload;
}


/*
 * Setters
 */

pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type) {
	if (type == 0 || type >> 2) {
		return E_TYPE;
	}
	pkt->header->ttw &= ~0x3;
	pkt->header->ttw |= type;
	return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr) {
	if (tr >> 1) {
		return E_TR;
	}
	pkt->header->ttw &= ~(1 << 2);
	pkt->header->ttw |= tr << 2;
	return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window) {
	if (window > MAX_WINDOW_SIZE) {
		return E_WINDOW;
	}
	pkt->header->ttw &= ~0xf8;
	pkt->header->ttw |= window << 3;
	return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum) {
	pkt->header->seqnum = seqnum;
	return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length) {
	fprintf(stderr, "pkt_set_length: %d\n\n", length);
	if (length > MAX_PAYLOAD_SIZE) {
		return E_LENGTH;
	}
	pkt->header->length = htons(length);
	return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp) {
	pkt->header->timestamp = timestamp;
	return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1) {
	pkt->crc1 = htonl(crc1);
	return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2) {
	pkt->crc2 = htonl(crc2);
	return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
                                const char *data,
                                const uint16_t length) {
	fprintf(stderr, "pkt_set_payload:\n");
	fprintf(stderr, "length: %d\n\n", length);

	pkt_status_code code = pkt_set_length(pkt, length);
	if (code != PKT_OK) {
		return code;
	}

	pkt->payload = realloc(pkt->payload, length * sizeof (pkt->payload[0]));
	if (pkt->payload == NULL) {
		return E_NOMEM;
	}

	memcpy(pkt->payload, data, length);
	return PKT_OK;
}
