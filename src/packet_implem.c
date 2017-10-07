#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <zlib.h>

#include "packet_interface.h"

struct __attribute__((__packed__)) header {
	unsigned int window : 5;
	unsigned int tr : 1;
	unsigned int type : 2;
	uint8_t seqnum;
	uint16_t length;
	uint32_t timestamp;
};

struct __attribute__((__packed__)) pkt {
	struct header *header;
	uint32_t crc1;

	char payload[MAX_PAYLOAD_SIZE];
	uint32_t crc2;
};

pkt_t* pkt_new() {
	pkt_t *pkt = malloc(sizeof (pkt_t));
	if (pkt == NULL) {
		return NULL;
	}

	pkt->header = malloc(sizeof (*pkt->header));
	if (pkt->header == NULL) {
		pkt_del(pkt);
		return NULL;
	}

	pkt->header->type = PTYPE_DATA;
	pkt->header->tr = 0;
	pkt->header->window = 0;
	pkt->header->seqnum = 0;
	pkt->header->length = 0;
	pkt->header->timestamp = 0;
	pkt->crc1 = 0;

	memset(pkt->payload, 0, MAX_PAYLOAD_SIZE * sizeof (*pkt->payload));
	pkt->crc2 = 0;

	return pkt;
}

void pkt_del(pkt_t *pkt) {
	free(pkt->header);
	free(pkt);
}

/**
 * Computes the CRC32 of a header with its TR field set to 0.
 */
uint32_t compute_header_crc32(const struct header *h) {
	struct header copy = *h;
	copy.tr = 0;
	return crc32(0, (unsigned char *) &copy, sizeof (copy));
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt) {
	size_t header_size = sizeof (*pkt->header) + sizeof (pkt->crc1);
	if (len < header_size) { //Si le nb de byte recus est plus petit que la taille d'un header
		return E_NOHEADER; //C'est qu'il y a pas de header
	}

	size_t read = 0;

	memcpy(pkt->header, data + read, sizeof (*pkt->header)); //on met le début de data (pkt recu) dans le header
	read += sizeof (*pkt->header);

	if (pkt_get_type(pkt) == 0) return E_TYPE;
	if (pkt_get_type(pkt) != PTYPE_DATA && pkt_get_tr(pkt) != 0) return E_TR;
	if (pkt_get_window(pkt) > MAX_WINDOW_SIZE) return E_WINDOW;
	if (pkt_get_length(pkt) > MAX_PAYLOAD_SIZE) return E_LENGTH;

	// If there's no payload, we should have len == header_size.
	uint16_t length = pkt_get_length(pkt);
	if (length == 0 && len > header_size) { // len est le nb de bytes recu
		return E_UNCONSISTENT; //Donc si le nb de bytes recus est > que le header mais que length (taille du payload) vaut 0, il y a erreur
	}

	memcpy(&pkt->crc1, data + read, sizeof (pkt->crc1)); //on met ce qui suit le header (le crc1) dans l'espace crc1 de pkt
	read += sizeof (pkt->crc1);

	if (pkt_get_crc1(pkt) != compute_header_crc32(pkt->header)) { //Si le crc qu'on a recu et mis dans le pkt correspond pas au crc du header
		return E_CRC;
	}

	size_t payload_size = length * sizeof (*pkt->payload);
	memcpy(pkt->payload, data + read, payload_size); //on met ce qui suit le crc1 (le payload) dans l'espace payload de pkt
	read += payload_size;

	if (payload_size > 0) { // Si il y a un payload il faut calculer un crc2
		memcpy(&pkt->crc2, data + read, sizeof (pkt->crc2));
		uint32_t computed_crc2 = crc32(0, (unsigned char *) pkt->payload, payload_size);
		if (pkt_get_crc2(pkt) != computed_crc2) { // Si le crc2 du pkt correspond pas a celui qu'on a RECALCULE sur le payload
			return E_CRC;
		}
	}

	return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t *pkt, char *buf, size_t *len) {
	uint16_t length = pkt_get_length(pkt);
	size_t payload_size = length * sizeof (*pkt->payload);

	size_t tot_size = sizeof (*pkt->header) + sizeof (pkt->crc1);
	tot_size += payload_size + ((payload_size > 0) ? sizeof (pkt->crc2) : 0); //si il y a un payload on met le crc2

	if (tot_size > *len) { //si la taille depasse la taille dispo
		*len = 0;
		return E_NOMEM;
	}

	size_t written = 0;

	memcpy(buf + written, pkt->header, sizeof (*pkt->header)); //on met le header dans le debut du buf
	written += sizeof (*pkt->header);

	uint32_t header_crc = htonl(compute_header_crc32(pkt->header));
	memcpy(buf + written, &header_crc, sizeof (header_crc)); //on met le crc dans le buf apres le header car on a incremente written
	written += sizeof (header_crc);

	memcpy(buf + written, pkt->payload, payload_size);
	written += payload_size;

	if (payload_size > 0) { //Si il y a un payload il y a un crc pour ce payload
		uint32_t payload_crc = htonl(crc32(0, (unsigned char *) pkt->payload, payload_size));
		memcpy(buf + written, &payload_crc, sizeof (payload_crc));
		written += sizeof (payload_crc);
	}

	*len = written; //on remplace len par le nb de bytes ecrits
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
	pkt->header->type = type;
	return PKT_OK;
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

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length) {
	uint16_t actual = length;
	if (data == NULL) {
		actual = 0;
	}

	pkt_status_code code = pkt_set_length(pkt, actual);
	if (code != PKT_OK) {
		return code;
	}

	memset(pkt->payload, 0, MAX_PAYLOAD_SIZE * sizeof (*pkt->payload));
	memcpy(pkt->payload, data, actual);
	return PKT_OK;
}
