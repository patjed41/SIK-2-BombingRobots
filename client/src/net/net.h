#ifndef NET_H
#define NET_H

#include <sys/types.h>
#include <arpa/inet.h>
#include <string>
#include <netdb.h>

// Turns of Nagle's algorithm from TCP socket.
void turn_off_nagle(int socked_fd);

// Receives message of max length [max_length] and puts it into [buffer].
size_t receive_message(int socket_fd, void *buffer, size_t max_length, int flags);

// Sends message located in [buffer] of length [length].
void send_message(int socket_fd, const void *message, size_t length, int flags);

// Connects to (host):(port). If [tcp] is set tu true, connection uses
// TCP protocol, otherwise it uses UDP protocol. Returns descriptor to newly
// created socket. If connection failed, returns -1.
int connect(const std::string &host, uint16_t port, bool tcp);

// Binds UDP socket to port [port]. If ipv4 is false, protocol ipv6 is used.
int bind_udp_socket(uint16_t port, bool ipv4);

#endif // NET_H
