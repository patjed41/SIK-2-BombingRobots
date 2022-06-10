#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <arpa/inet.h>
#include <string>

#include "../../common/err.h"
#include "../../common/types.h"

#define QUEUE_LENGTH 25     // Max length of queue of clients trying to connect.
#define PACKET_LIMIT 65535  // Max number of bytes send in one TCP packet.

// Binds TCP socket to port [port] and returns descriptor to newly created socket.
int bind_tcp_socket(uint16_t port);

// Starts listening on socket [socket_fd] with queue [queue_length].
void start_listening(int socket_fd, int queue_length);

// Turns of Nagle's algorithm from TCP socket. Returns false if function failed.
bool turn_off_nagle(int socked_fd);

// Accepts new connection on socket [socket_fd] and returns a descriptor
// to new socket. Puts client address into [*client_address]. IPv4 addresses
// are mapped to IPv6. Returns -1 if connection failed.
int accept_connection(int socket_fd, sockaddr_in6 *client_address);

// Extracts address from address structure. Returns address in format [ip]:port.
// Returns "fail" if function failed.
std::string get_address(const sockaddr_in6 &address);

// Sends [message] via [socket_fd]. Returns false if sending failed.
bool send_message(int socket_fd, const List<uint8_t> &message, int flags);

#endif // NET_H
