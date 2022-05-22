#ifndef NET_H
#define NET_H

#include <sys/types.h>
#include <arpa/inet.h>
#include <string>
#include <netdb.h>

int open_socket(bool is_ipv4, bool is_tcp);

void bind_socket(int socket_fd, uint16_t port, bool is_ipv4);

void connect_socket(int socket_fd, std::string &host, uint16_t port);

void close_socket(int socket_fd);

bool is_address_ipv4(const std::string &host);

bool is_address_ipv6(const std::string &host);

void turn_off_nagle(int socked_fd);

size_t receive_message(int socket_fd, void *buffer, size_t max_length, int flags);

void send_message(int socket_fd, const void *message, size_t length, int flags);

int connect_to(const std::string &host, uint16_t port, bool tcp);

int bind_udp_socket(uint16_t port);

#endif // NET_H
