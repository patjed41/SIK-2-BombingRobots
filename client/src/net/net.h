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

#endif //NET_H
