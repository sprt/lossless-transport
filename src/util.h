#ifndef __UTIL_H_
#define __UTIL_H_


/**
 * Various utility functions used by sender and receiver.
 */

#include <netdb.h>

/**
 * Parses arguments from the command line and stores them in the corresponding
 * pointer. On error, prints usage on stderr and exits.
 */
void parse_args(int argc, char **argv,
                char **hostname, uint16_t *port, char **filename);

/**
 * Resolves the resource name to an usable IPv6 address.
 * @address: The name to resolve.
 * @rval: Where the resulting IPv6 address descriptor should be stored.
 * @return: NULL if it succeeded, or a pointer towards a string describing the
 *          error if any.
 */
const char *real_address(const char *address, struct sockaddr_in6 *rval);

/**
 * Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *          or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port,
                  struct sockaddr_in6 *dest_addr, int dst_port);

/**
 * Sends a packet over the specified socket.
 * Calls send and returns its value. If the packet could not be encoded,
 * it prints on stderr and exits.
 */
int send_packet(int sockfd, pkt_t *pkt);

/**
 * Returns the amount of time (in microseconds) elapsed since an arbitrary point
 * in time, in a STRICTLY monotonic fashion, with the first call returning 0.
 *
 * Rationale: from the perspective of the receiver, the only way to acknowledge
 * an out-of-sequence packet is to set the timestamp field of the ACK to the
 * timestamp of that packet (it can't simply set the seqnum field as ACKs are
 * cumulative), hence it is crucial that no two packets share the same timestamp
 * for when the sender removes it from its buffer.
 */
uint32_t get_monotime(void);

/**
 * Converts microseconds to a struct timeval.
 */
struct timeval micro_to_timeval(uint32_t us);

/**
 * Prints a message on stderr.
 */
void log_msg(const char *fmt, ...);

/**
 * Calls perror with the specified string.
 */
void log_perror(const char *s);

/**
 * Prints a message on stderr and exits with a non-zero code.
 */
void exit_msg(const char *fmt, ...);

/**
 * Calls perror with the specified string and exits with a non-zero code.
 */
void exit_perror(const char *s);


#endif  /* __UTIL_H_ */
