#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <arpa/inet.h>
#include <string>

#include "../err.h"
#include "../server-data/types.h"

#define QUEUE_LENGTH 25
#define PACKET_LIMIT 65535

// Binds TCP socket to port [port] and returns descriptor.
int bind_tcp_socket(uint16_t port);

// Starts listening on socket [socket_fd] with queue [queue_length].
void start_listening(int socket_fd, int queue_length);

// Turns of Nagle's algorithm from TCP socket.
void turn_off_nagle(int socked_fd);

// Accepts new connection on socket [socket_fd] and returns a descriptor
// to new socket. Puts client address into [*client_address].
int accept_connection(int socket_fd, sockaddr_in6 *client_address);

// Closes socket.
void close_socket(int socked_fd);

// Extracts address from sockaddr_in6 structure.
std::string get_address(const sockaddr_in6 &address);

// Sends [message] via [socket_fd].
void send_message(int socket_fd, const List<uint8_t> &message, int flags);

#endif // NET_H
