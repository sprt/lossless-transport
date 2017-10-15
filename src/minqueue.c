#include <stddef.h>
#include <stdlib.h>

#include "packet_interface.h"
#include "minqueue.h"

struct minqueue {
	size_t capacity; // maximum size
	size_t size; // current size
	pkt_t **arr;
};

minqueue_t *queue_create(size_t max_size) {
	minqueue_t *q = calloc(1, sizeof (minqueue_t));
	if (q == NULL) {
		return NULL;
	}
	q->arr = calloc(max_size, sizeof (*q->arr));
	if (q->arr == NULL) {
		queue_free(q);
		return NULL;
	}
	q->capacity = max_size;
	return q;
}

int pkt_cmp(const void *a, const void *b) {
	pkt_t *pkt_a = *((pkt_t **) a);
	pkt_t *pkt_b = *((pkt_t **) b);
	return pkt_get_timestamp(pkt_b) - pkt_get_timestamp(pkt_a);
}

inline void swap(pkt_t **arr, size_t i, size_t j) {
	pkt_t *tmp = arr[i];
	arr[i] = arr[j];
	arr[j] = tmp;
}

int queue_push(minqueue_t *q, pkt_t *pkt) {
	if (q->size == q->capacity) {
		return -1;
	}
	size_t pos = q->size;
	q->arr[pos] = (pkt_t *) pkt;
	for (; pos > 0 && pkt_cmp(&q->arr[pos - 1], &q->arr[pos]) > 0; pos--) {
		swap(q->arr, pos - 1, pos);
	}
	q->size++;
	return 0;
}

pkt_t *queue_peek(const minqueue_t *q) {
	if (q->size == 0) {
		return NULL;
	}
	return q->arr[q->size - 1];
}

int queue_update(minqueue_t *q, uint8_t seqnum, uint32_t timestamp) {
	for (size_t i = 0; i < q->size; i++) {
		if (pkt_get_seqnum(q->arr[i]) == seqnum) {
			pkt_set_timestamp(q->arr[i], timestamp);
			qsort(q->arr, q->size, sizeof (*q->arr), pkt_cmp);
			return 0;
		}
	}
	return -1;
}

pkt_t *queue_pop(minqueue_t *q) {
	if (q->size == 0) {
		return NULL;
	}
	pkt_t *pkt = q->arr[q->size - 1];
	q->arr[q->size - 1] = NULL;
	q->size--;
	return pkt;
}

void queue_free(minqueue_t *q) {
	free(q->arr);
	free(q);
}
