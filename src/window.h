#ifndef __WINDOW_H_
#define __WINDOW_H_


#include <stdbool.h>

#include "packet_interface.h"

/**
 * A wrapping sliding window with a built-in packet buffer.
 */
typedef struct window window_t;

/**
 * Allocates a new window with the specified starting size and maximum size.
 * Returns NULL on error or if size > max_size.
 */
window_t *window_create(size_t size, size_t max_size, size_t max_seqnum);

/**
 * Free the resources allocated to the window only (i.e. NOT the packets).
 */
void window_free(window_t *w);

/**
 * Returns the first sequence number in the window.
 * Example: calling with 3 0 [1 2] returns 1.
 */
size_t window_start(window_t *w);

/**
 * Returns the last sequence number in the window.
 * Example: calling with 3 0 [1 2] returns 2.
 */
size_t window_end(window_t *w);

/**
 * Slide the window by one unit without touching the internal buffer.
 * Example: 3 0 [1 2] becomes 3] 0 1 [2.
 */
void window_slide(window_t *w);

/**
 * Slide the window to the specified position.
 * Example: sliding window 3 0 [1 2] to position 2 gives 3] 0 1 [2.
 */
void window_slide_to(window_t *w, size_t pos);

/**
 * Reports whether the given sequence number is within the window.
 */
bool window_has(window_t *w, size_t seqnum);

/**
 * Resizes the window without touching the buffer.
 * Returns -1 if new_size > max_size, and 0 otherwise.
 */
int window_resize(window_t *w, size_t new_size);

/**
 * Returns the maximum size set when the window was created.
 */
size_t window_get_max_size(window_t *w);

/**
 * Returns the size of the window as set by window_create and window_resize.
 */
size_t window_get_size(window_t *w);

/**
 * Inserts a packet into the buffer.
 * Returns -1 on error or if the window is full, and 0 otherwise.
 */
int window_push(window_t *w, pkt_t *pkt);

/**
 * Returns the number of free slots in the buffer.
 */
size_t window_available(window_t *w);

/**
 * Returns the number of packets in the buffer.
 */
size_t window_buffer_size(window_t *w);

/**
 * Reports whether the internal *buffer* is empty.
 */
bool window_empty(window_t *w);

/**
 * Reports whether the internal *buffer* can receive more packets.
 */
bool window_full(window_t *w);

/**
 * Returns a packet from the buffer with the specified sequence.
 */
pkt_t *window_find_seqnum(window_t *w, size_t seqnum);

/**
 * Updates the timestamp of the packet which current timestamp is old_time to
 * new_time. Returns -1 if there is no such packet.
 */
int window_update_timestamp(window_t *w, uint32_t old_time, uint32_t new_time);

/**
 * Returns the packet with the specified timestamp and removes it from the
 * buffer. Returns NULL if there is no such packet.
 */
pkt_t *window_pop_timestamp(window_t *w, uint32_t timestamp);

/**
 * Returns the packet in the buffer with the minimum timestamp,
 * or NULL if the buffer is empty.
 */
pkt_t *window_peek_min_timestamp(window_t *w);

/**
 * Returns the packet with the minimum timestamp and removes it from the buffer.
 * Returns NULL if the buffer is empty.
 */
pkt_t *window_pop_min_timestamp(window_t *w);

/**
 * Returns the packet in the buffer with the minimum sequence number,
 * or NULL if the buffer is empty.
 */
pkt_t *window_peek_min_seqnum(window_t *w);

/**
 * Returns the packet with the minimum sequence number and removes it from the
 * buffer. Returns NULL if the buffer is empty.
 */
pkt_t *window_pop_min_seqnum(window_t *w);

/*
 * Sender
 * ======
 *
 * Window: max, size, pos
 * Unbounded min-queue (O(n) insertion, O(1) removal):
 * 	Each item has: pkt (esp. seqnum, timestamp)
 * next: seqnum of the next packet to be sent (initial value: 0)
 *
 * Queue contains unACKed packets
 *
 * Select on: recv
 *
 * Select timeout:
 * - If (eof reached) and (queue empty):
 *   - Break out
 * - ElseIf (queue full) or (eof reached):
 *   - Can't send more right now, first timer (wakes up on either recv or timer)
 * - Else:
 *   - Don't wait, fill buffer
 *
 * After select returns:
 *
 * - On recv has data:
 *   - On ACK:
 *     -> Timestamp field corresponds to the last packet received by the receiver
 *     -> Seqnum field has the next seqnum expected to be received
 *     - ACK all packets such that seqnum < seqnum field (w_pop_min_seqnum)
 *     - ACK packet that matches timestamp field (w_pop_timestamp)
 *     - Slide window (w_slide)
 *     - Resize window according to window field (w_resize)
 *   - On NACK:
 *     - Set timer to -1 and will resend on next iteration (w_update_timestamp)
 *   - In both case:
 *     - Discard if not within window or if seqnum <= next (w_has)
 *
 * - If front of queue is packet to resend (check timer):
 *   - For all expired: pop, resend, update timer, push (w_peek_min_timestamp,
 *     w_pop_min_timestamp)
 *
 * - If (window not full) AND not (done sending):
 *   - Read 512 bytes, send, push packet to queue (w_push)
 *
 * Receiver
 * ========
 *
 * Window: max, size, pos
 * Unbounded min-queue (O(n) insertion, O(1) removal):
 * 	Each item has: pkt, seqnum, outseq
 * next: seqnum of the next packet to be received (initial value: 0)
 * [0 1 (2) 3 4]
 *
 * Queue contains out of seq packets (and thus not yet written to file)
 *
 * Enqueue
 * -------
 *
 * On receive:
 * Add to queue if inside window and window not full.
 *
 * Dequeue
 * -------
 *
 * On write to file:
 * write if first item in buf is first in window
 *
 */


#endif  /* __WINDOW_H_ */
