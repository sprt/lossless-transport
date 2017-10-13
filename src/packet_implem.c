#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <zlib.h>

#include "packet_interface.h"

struct __attribute__((__packed__)) pkt {
	unsigned int window : 5;
	unsigned int tr : 1;
	unsigned int type : 2;
	uint8_t seqnum;
	uint16_t length;
	uint32_t timestamp;
	uint32_t crc1;
	char payload[MAX_PAYLOAD_SIZE];
	uint32_t crc2;
};

pkt_t* pkt_new() {
	pkt_t *pkt = calloc(1, sizeof (pkt_t));
	if (pkt == NULL) {
		return NULL;
	}
	pkt->type = PTYPE_DATA;
	return pkt;
}

void pkt_del(pkt_t *pkt) {
	free(pkt);
}

/**
 * Computes the CRC32 of the header with its TR field set to 0.
 */
uint32_t pkt_compute_crc1(const pkt_t *pkt) {
	// Copy the packet and set the TR of that copy
	pkt_t p = *pkt;
	p.tr = 0;
	return crc32(0, (unsigned char *) &p, HEADER_SIZE - sizeof (pkt->crc1));
}

/**
 * Computes the CRC32 of the payload. len is the size of the payload.
 */
uint32_t pkt_compute_crc2(const pkt_t *pkt, size_t len) {
	return crc32(0, (unsigned char *) pkt->payload, len);
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt) {
	if (data == NULL || len < HEADER_SIZE) {
		return E_NOHEADER;
	}

	size_t read = 0;

	memcpy(pkt, data + read, HEADER_SIZE);
	read += HEADER_SIZE;

	// The packet includes the CRC2 field iff it has a payload and it wasn't
	// truncated. pkt_get_length returns 0 if it was truncated so we're all set.
	size_t payload_size = pkt_get_length(pkt) * sizeof (*pkt->payload);
	size_t total_size = HEADER_SIZE + payload_size + ((payload_size > 0) ? sizeof (pkt->crc2) : 0);

	// Field errors should be returned in the order those fields are
	// declared in the struct. We could return E_UNCONSISTENT if the packet
	// has a payload and isn't of type PTYPE_DATA, but we don't since it
	// isn't specified and thus doing so might break other implementations
	// (albeit buggy ones).
	if (pkt_get_type(pkt) == 0) {
		return E_TYPE;
	} else if (pkt_get_type(pkt) != PTYPE_DATA && pkt_get_tr(pkt) != 0) {
		return E_TR;
	} else if (pkt_get_window(pkt) > MAX_WINDOW_SIZE) {
		return E_WINDOW;
	} else if (payload_size > MAX_PAYLOAD_SIZE) {
		return E_LENGTH;
	} else if (len != total_size) {
		// Return now in order to prevent a potential buffer overflow
		return E_UNCONSISTENT;
	} else if (pkt_get_crc1(pkt) != pkt_compute_crc1(pkt)) {
		return E_CRC;
	}

	// No-op if payload empty
	memcpy(pkt->payload, data + read, payload_size);
	read += payload_size;

	// Prevent memcpy from overflowing
	if (payload_size > 0) {
		memcpy(&pkt->crc2, data + read, sizeof (pkt->crc2));
		if (pkt_get_crc2(pkt) != pkt_compute_crc2(pkt, payload_size)) {
			return E_CRC;
		}
	}

	return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t *pkt, char *buf, size_t *len) {
	if (pkt_get_total_size(pkt) > *len) {
		*len = 0;
		return E_NOMEM;
	}

	size_t written = 0;

	memcpy(buf + written, pkt, HEADER_SIZE - sizeof (pkt->crc1));
	written += HEADER_SIZE - sizeof (pkt->crc1);

	uint32_t crc1 = htonl(pkt_compute_crc1(pkt));
	memcpy(buf + written, &crc1, sizeof (crc1));
	written += sizeof (crc1);

	// No-op if payload empty
	size_t payload_size = pkt_get_length(pkt) * sizeof (*pkt->payload);
	memcpy(buf + written, pkt->payload, payload_size);
	written += payload_size;

	if (payload_size > 0) {
		uint32_t crc2 = htonl(pkt_compute_crc2(pkt, payload_size));
		memcpy(buf + written, &crc2, sizeof (crc2));
		written += sizeof (crc2);
	}

	*len = written;
	return PKT_OK;
}


/*
 * Getters
 */

ptypes_t pkt_get_type(const pkt_t* pkt) {
	return (ptypes_t) pkt->type;
}

uint8_t pkt_get_tr(const pkt_t* pkt) {
	return (uint8_t) pkt->tr;
}

uint8_t pkt_get_window(const pkt_t* pkt) {
	return (uint8_t) pkt->window;
}

uint8_t pkt_get_seqnum(const pkt_t* pkt) {
	return (uint8_t) pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt) {
	if (pkt_get_tr(pkt)) {
		return 0;
	}
	return ntohs(pkt->length);
}

uint32_t pkt_get_timestamp(const pkt_t* pkt) {
	return pkt->timestamp;
}

uint32_t pkt_get_crc1(const pkt_t* pkt) {
	return ntohl(pkt->crc1);
}

uint32_t pkt_get_crc2(const pkt_t* pkt) {
	return ntohl(pkt->crc2);
}

const char* pkt_get_payload(const pkt_t* pkt) {
	if (pkt_get_length(pkt) == 0) {
		return NULL;
	}
	return pkt->payload;
}


/*
 * Setters
 */

pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type) {
	if (type == 0 || type >> 2) {
		return E_TYPE;
	}
	pkt->type = type;
	return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr) {
	if (tr >> 1) {
		return E_TR;
	}
	pkt->tr = tr;
	return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window) {
	if (window > MAX_WINDOW_SIZE) {
		return E_WINDOW;
	}
	pkt->window = window;
	return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum) {
	pkt->seqnum = seqnum;
	return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length) {
	if (length > MAX_PAYLOAD_SIZE) {
		return E_LENGTH;
	}

	// Maintain the invariant that unused slots are zeroed out
	uint16_t unused = (MAX_PAYLOAD_SIZE - length) * sizeof (*pkt->payload);
	memset(pkt->payload + length, 0, unused);

	pkt->length = htons(length);
	return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp) {
	pkt->timestamp = timestamp;
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

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length) {
	uint16_t actual = length;
	if (data == NULL) {
		actual = 0;
	}

	pkt_status_code code = pkt_set_length(pkt, actual);
	if (code != PKT_OK) {
		return code;
	}

	memcpy(pkt->payload, data, actual);
	return PKT_OK;
}
