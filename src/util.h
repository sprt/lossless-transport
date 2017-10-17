#ifndef __UTIL_H_
#define __UTIL_H_

/**
 * Various utility functions used by both sender and receiver.
 */

#include <stdint.h>

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

#endif  /* __UTIL_H_ */
