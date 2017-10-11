#include <stddef.h>
#include <stdlib.h>

#include "packet_interface.h"
#include "queue.h"

typedef struct node {
	pkt_t *pkt;
	struct node *next;
} node_t;

struct queue {
	size_t maxsize; // maximum size
	size_t size; // current size
	node_t *tail;
};

queue_t *queue_create(size_t maxsize) {
	queue_t *q = malloc(sizeof (queue_t));
	if (q == NULL) {
		return NULL;
	}
	q->maxsize = maxsize;
	q->size = 0;
	q->tail = NULL;
	return q;
}

int queue_add(queue_t *q, const pkt_t *pkt) {
	if (q->size == q->maxsize) {
		return -1;
	}

	node_t *node = malloc(sizeof (node_t));
	if (node == NULL) {
		return -1;
	}

	node->pkt = (pkt_t *) pkt;

	// Special case: empty queue
	if (q->size == 0) {
		node->next = node;
		goto update_tail;
	}

	// Make the new node point to the head
	node->next = q->tail->next;
	// Make the tail point to the new node
	q->tail->next = node;

update_tail:
	// Update the tail to be the new node
	q->tail = node;
	q->size++;

	return 0;
}

pkt_t *queue_remove(queue_t *q) {
	if (q->size == 0) {
		return NULL;
	}

	// Save the head
	node_t *first = q->tail->next;
	// Make the tail point to the second node
	q->tail->next = q->tail->next->next;
	// Extract from the former head
	pkt_t *pkt = first->pkt;

	free(first);
	q->size--;

	return pkt;
}

void queue_free(queue_t *q) {
	node_t *node = q->tail;
	for (size_t i = 0; i < q->size; i++) {
		node_t *next = node->next;
		free(node);
		node = next;
	}
	free(q);
}
