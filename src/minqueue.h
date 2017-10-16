#ifndef __MINQUEUE_H_
#define __MINQUEUE_H_


#include <stddef.h>

#include "packet_interface.h"

/**
 * A bounded min-queue of packets with O(n) insertion.
 * The priority is the timestamp field.
 */
typedef struct minqueue minqueue_t;

/**
 * Allocates a new queue with the specified maximum size.
 * Returns NULL if there was an error.
 */
minqueue_t *queue_create(size_t max_size);

/**
 * Enqueues pkt into q.
 * Returns -1 if the operation failed (e.g. because the queue was full).
 */
int queue_push(minqueue_t *q, pkt_t *pkt);

/**
 * Returns the first packet in the queue without mutating it,
 * or NULL if the queue is empty.
 */
pkt_t *queue_peek(const minqueue_t *q);

/**
 * Updates the timestamp of the packet with the given sequence number
 * and reorders the queue. Returns -1 if there exists no such packet.
 * Behavior is undefined if there exist multiple such packets.
 */
int queue_update(minqueue_t *q, uint8_t seqnum, uint32_t timestamp);

/**
 * Dequeues the first packet from q and returns it,
 * or NULL if the queue is empty.
 */
pkt_t *queue_pop(minqueue_t *q);

/**
 * Free the queue only, i.e. NOT the packets.
 */
void queue_free(minqueue_t *q);


#endif  /* __MINQUEUE_H_ */
