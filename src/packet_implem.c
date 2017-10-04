#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <stdbool.h>
#include <netinet/in.h>
#include "packet_interface.h"

struct __attribute__((__packed__)) pkt {
	struct {
		unsigned int type : 2;
		unsigned int tr : 1;
		unsigned int window : 5;
		uint8_t seqnum;
		uint16_t length;
		uint32_t timestamp;
		uint32_t crc1;
	} *header;

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

	pkt->header->type = 0;
	pkt->header->tr = 0;
	pkt->header->window = 0;
	pkt->header->seqnum = 0;
	pkt->header->length = 0;
	pkt->header->timestamp = 0;
	pkt->header->crc1 = 0;

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

	size_t total = sizeof (*pkt->header);
	if (length > 0) {
		total += length * sizeof (pkt->payload[0]);
		total += sizeof (pkt->crc2);
	}

	fprintf(stderr, "pkt_encode:\n");
	fprintf(stderr, "pkt_get_length: %d\n", length);
	fprintf(stderr, "len: %zu\n", *len);
	fprintf(stderr, "total: %zu\n\n", total);

	if (total > *len) {
		*len = 0;
		return E_NOMEM;
	}

	size_t n = 0;

	memcpy(buf + n, pkt->header, sizeof (*pkt->header));
	n += sizeof (*pkt->header);

	if (length > 0) {
		memcpy(buf + n, pkt->payload, length * sizeof (pkt->payload[0]));
		n += length * sizeof (pkt->payload[0]);

		memcpy(buf + n, &pkt->crc2, sizeof (pkt->crc2));
		n += sizeof (pkt->crc2);
	}

	*len = n;
	return PKT_OK;
}


/*
 * Getters
 */

ptypes_t pkt_get_type(const pkt_t* pkt) {
	return (ptypes_t) pkt->header->type;
}

uint8_t pkt_get_tr(const pkt_t* pkt) {
	return (uint8_t) pkt->header->tr;
}

uint8_t pkt_get_window(const pkt_t* pkt) {
	return (uint8_t) pkt->header->window;
}

uint8_t pkt_get_seqnum(const pkt_t* pkt) {
	return (uint8_t) pkt->header->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt) {
	if (pkt_get_tr(pkt)) {
		return 0;
	}
	return ntohs(pkt->header->length);
}

uint32_t pkt_get_timestamp(const pkt_t* pkt) {
	return pkt->header->timestamp;
}

uint32_t pkt_get_crc1(const pkt_t* pkt) {
	return ntohl(pkt->header->crc1);
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
	switch (type) {
	case PTYPE_DATA:
	case PTYPE_ACK:
	case PTYPE_NACK:
		pkt->header->type = type;
		return PKT_OK;
	default:
		return E_TYPE;
	}
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr) {
	if (tr >> 1) {
		return E_TR;
	}
	pkt->header->tr = tr;
	return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window) {
	if (window > MAX_WINDOW_SIZE) {
		return E_WINDOW;
	}
	pkt->header->window = window;
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
	pkt->header->crc1 = htonl(crc1);
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
