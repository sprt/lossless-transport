#include <assert.h>
#include <stdlib.h>

#include "packet_interface.h"
#include "window.h"

struct window {
	// Example: ... 3 [0 1] 2 ... (max_seqnum=4, size=2, pos=0)
	size_t max_seqnum;
	size_t capacity; // maximum size
	size_t size; // current size
	size_t pos;

	// Buffer
	size_t bufsize;
	struct node *front;
};

struct node {
	pkt_t *pkt;
	struct node *next;
};

window_t *window_create(size_t size, size_t max_size, size_t max_seqnum) {
	if (size > max_size) {
		return NULL;
	}
	window_t *w = calloc(1, sizeof (window_t));
	if (w == NULL) {
		return NULL;
	}
	w->size = size;
	w->capacity = max_size;
	w->max_seqnum = max_seqnum; /* TODO: check valid */
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

size_t window_start(window_t *w) {
	return w->pos;
}

size_t window_end(window_t *w) {
	return (w->pos + (w->size - 1)) % (w->max_seqnum + 1);
}

void window_slide(window_t *w) {
	w->pos = (w->pos + 1) % (w->max_seqnum + 1);
}

void window_slide_to(window_t *w, size_t pos) {
	assert(pos <= w->max_seqnum);
	w->pos = pos;
}

bool window_has(window_t *w, size_t seqnum) {
	if (w->size == 0) {
		return false;
	}
	if (window_start(w) > window_end(w)) { /* Example: 3 [4 5 0 1] 2 */
		return window_start(w) <= seqnum || seqnum <= window_end(w);
	}
	return window_start(w) <= seqnum && seqnum <= window_end(w);
}

int window_resize(window_t *w, size_t new_size) {
	if (new_size > w->capacity) {
		return -1;
	}
	w->size = new_size;
	return 0;
}

size_t window_get_max_size(window_t *w) {
	return w->capacity;
}

size_t window_get_size(window_t *w) {
	return w->size;
}

int window_push(window_t *w, pkt_t *pkt) {
	if (window_full(w)) {
		return -1;
	}

	struct node *front = calloc(1, sizeof (struct node));
	if (front == NULL) {
		return -1;
	}

	front->pkt = pkt;
	front->next = w->front;
	w->front = front;

	w->bufsize++;
	return 0;
}

size_t window_available(window_t *w) {
	/* We may have shrunk the window below the buffer size */
	if (w->size < w->bufsize) {
		return 0;
	}
	return w->size - w->bufsize;
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

pkt_t *window_find_seqnum(window_t *w, size_t seqnum) {
	struct node *cur = w->front;
	while (cur != NULL) {
		if (pkt_get_seqnum(cur->pkt) == seqnum) {
			return cur->pkt;
		}
		cur = cur->next;
	}
	/* Reached only if there's no match */
	return NULL;
}

int window_update_timestamp(window_t *w, uint32_t old_time, uint32_t new_time) {
	struct node *cur = w->front;
	while (cur != NULL) {
		if (pkt_get_timestamp(cur->pkt) == old_time) {
			pkt_set_timestamp(cur->pkt, new_time);
			return 0;
		}
		cur = cur->next;
	}
	/* Reached only if there's no match */
	return -1;
}

/**
 * Removes the specified node from the buffer and returns the corresponding
 * packet. Returns NULL if the node pointed to is NULL.
 */
pkt_t *window_pop_node(window_t *w, struct node **node) {
	if (*node == NULL) {
		return NULL;
	}

	pkt_t *pkt = (*node)->pkt;

	struct node *tmp = *node;
	*node = (*node)->next;
	free(tmp);

	w->bufsize--;
	return pkt;
}

pkt_t *window_pop_timestamp(window_t *w, uint32_t timestamp) {
	struct node **cur = &w->front;
	struct node **match = cur;
	while (*cur != NULL) {
		if (pkt_get_timestamp((*cur)->pkt) == timestamp) {
			match = cur;
			break;
		}
		cur = &(*cur)->next;
	}
	return window_pop_node(w, match);
}

struct node **window_find_min_timestamp(window_t *w) {
	struct node **cur = &w->front;
	struct node **min = cur;
	while (*cur != NULL) {
		if (pkt_get_timestamp((*cur)->pkt) < pkt_get_timestamp((*min)->pkt)) {
			min = cur;
		}
		cur = &(*cur)->next;
	}
	return min;
}

pkt_t *window_peek_min_timestamp(window_t *w) {
	struct node **min = window_find_min_timestamp(w);
	if (*min == NULL) {
		return NULL;
	}
	return (*min)->pkt;
}

pkt_t *window_pop_min_timestamp(window_t *w) {
	struct node **min = window_find_min_timestamp(w);
	return window_pop_node(w, min);
}

struct node **window_find_min_seqnum(window_t *w) {
	struct node **cur = &w->front;
	struct node **min = cur;
	while (*cur != NULL) {
		if (pkt_get_seqnum((*cur)->pkt) < pkt_get_seqnum((*min)->pkt)) {
			min = cur;
		}
		cur = &(*cur)->next;
	}
	return min;
}

pkt_t *window_peek_min_seqnum(window_t *w) {
	struct node **min = window_find_min_seqnum(w);
	if (*min == NULL) {
		return NULL;
	}
	return (*min)->pkt;
}

pkt_t *window_pop_min_seqnum(window_t *w) {
	struct node **min = window_find_min_seqnum(w);
	return window_pop_node(w, min);
}
