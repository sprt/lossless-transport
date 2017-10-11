#ifndef __QUEUE_H_
#define __QUEUE_H_


#include <stddef.h>

#include "packet_interface.h"

/**
 * A circular queue with constant-time operations.
 */
typedef struct queue queue_t;

/**
 * Allocates a new queue with the specified maximum size.
 * Returns NULL if there was an error.
 */
queue_t *queue_create(size_t max_size);

/**
 * Enqueues pkt into q.
 * Returns -1 if the operation failed (e.g. because the queue was full).
 */
int queue_add(queue_t *q, const pkt_t *pkt);

/**
 * Dequeues the first packet from q and returns it,
 * or NULL if the queue was empty.
 */
pkt_t *queue_remove(queue_t *q);

/**
 * Free the queue only, i.e. NOT the packets.
 */
void queue_free(queue_t *q);


#endif  /* __QUEUE_H_ */
