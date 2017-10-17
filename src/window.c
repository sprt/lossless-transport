#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "packet_interface.h"
#include "window.h"

struct window {
	// Example: ... 3 [0 1] 2 ... (capacity=4, size=2, pos=0)
	size_t capacity; // maximum size
	size_t size; // current size
	size_t pos;

	// Buffer
	size_t bufsize;
	struct node *front;
	struct node *rear;
};

struct node {
	pkt_t *pkt;
	struct node *next;
};

window_t *window_create(size_t size, size_t max_size) {
	if (size > max_size) {
		return NULL;
	}
	window_t *w = calloc(1, sizeof (window_t));
	if (w == NULL) {
		return NULL;
	}
	w->size = size;
	w->capacity = max_size;
	return w;
}

void window_free(window_t *w) {
	struct node *cur = w->front;
	while (cur != NULL) {
		struct node *next = cur->next;
		free(cur);
		cur = next;
	}
	free(w);
}

void window_slide(window_t *w) {
	w->pos = (w->pos + 1) % w->capacity;
}

bool window_has(window_t *w, size_t seqnum) {
	size_t start = w->pos;
	size_t end = start + ((w->size - 1) % w->capacity);
	return (start <= seqnum) && (seqnum <= end);
}

int window_resize(window_t *w, size_t new_size) {
	if (new_size > w->capacity) {
		return -1;
	}
	w->size = new_size;
	return 0;
}

size_t window_get_size(window_t *w) {
	return w->size;
}

int window_push(window_t *w, pkt_t *pkt) {
	if (window_full(w)) {
		return -1;
	}

	struct node *rear = calloc(1, sizeof (struct node));
	if (rear == NULL) {
		return -1;
	}

	if (w->front == NULL) {
		/* Special case: buffer empty */
		w->front = rear;
		w->rear = rear;
	} else {
		w->rear->next = rear;
		w->rear = rear;
	}

	rear->pkt = pkt;
	w->bufsize++;

	return 0;
}

size_t window_buffer_size(window_t *w) {
	return w->bufsize;
}

bool window_empty(window_t *w) {
	return w->bufsize == 0;
}

bool window_full(window_t *w) {
	// We may have shrunk the window below the size of the buffer
	// so check for that as well.
	return (w->bufsize == w->capacity) || (w->bufsize >= w->size);
}

int window_update_timestamp(window_t *w, uint32_t old_time, uint32_t new_time) {
	pkt_t *match = NULL;
	struct node *cur = w->front;
	while (cur != NULL) {
		if (pkt_get_timestamp(cur->pkt) == old_time) {
			match = cur->pkt;
			break;
		}
		cur = cur->next;
	}
	if (match == NULL) {
		return -1;
	}
	pkt_set_timestamp(match, new_time);
	return 0;
}

// // Unsure if needed
// pkt_t *window_pop_timestamp(window_t *w, uint32_t timestamp) {
// 	struct node *match = NULL;
// 	struct node *cur = w->front;
// 	while (cur != NULL) {
// 		if (pkt_get_timestamp(cur->pkt) == timestamp) {
// 			match = cur;
// 			break;
// 		}
// 		cur = cur->next;
// 	}
// 	return window_pop_node(w, &match);
// }

/**
 * Removes the specified node from the buffer and returns the corresponding
 * packet. Returns NULL if the node pointed to is NULL.
 */
pkt_t *window_pop_node(window_t *w, struct node **node) {
	if (*node == NULL) {
		return NULL;
	}

	pkt_t *pkt = (*node)->pkt;

	// Special case: exactly one element
	if (w->front->next == NULL) {
		free(w->front);
		w->front = NULL;
		w->rear = NULL;
		w->bufsize = 0;
		return pkt;
	}

	struct node *tmp = *node;
	*node = (*node)->next;
	free(tmp);

	w->bufsize--;
	return pkt;
}

struct node *window_find_min_timestamp(window_t *w) {
	struct node *min = NULL;
	struct node *cur = w->front;
	while (cur != NULL) {
		if (min == NULL || pkt_get_timestamp(cur->pkt) < pkt_get_timestamp(min->pkt)) {
			min = cur;
		}
		cur = cur->next;
	}
	return min;
}

pkt_t *window_peek_min_timestamp(window_t *w) {
	struct node *min = window_find_min_timestamp(w);
	if (min == NULL) {
		return NULL;
	}
	return min->pkt;
}

pkt_t *window_pop_min_timestamp(window_t *w) {
	struct node *min = window_find_min_timestamp(w);
	return window_pop_node(w, &min);
}

struct node *window_find_min_seqnum(window_t *w) {
	struct node *min = NULL;
	struct node *cur = w->front;
	while (cur != NULL) {
		if (min == NULL || pkt_get_seqnum(cur->pkt) < pkt_get_seqnum(min->pkt)) {
			min = cur;
		}
	}
	return min;
}

pkt_t *window_peek_min_seqnum(window_t *w) {
	struct node *min = window_find_min_seqnum(w);
	if (min == NULL) {
		return NULL;
	}
	return min->pkt;
}

pkt_t *window_pop_min_seqnum(window_t *w) {
	struct node *min = window_find_min_seqnum(w);
	return window_pop_node(w, &min);
}
