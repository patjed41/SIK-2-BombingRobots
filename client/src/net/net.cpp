#include "net.h"
#include "../err.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

int open_socket(bool is_ipv4, bool is_tcp) {
    int domain = is_ipv4 ? PF_INET : PF_INET6;
    int type = is_tcp ? SOCK_STREAM : SOCK_DGRAM;
    int protocol = is_tcp ? IPPROTO_TCP : IPPROTO_UDP;
    int socket_fd = socket(domain, type, protocol);

    if (socket_fd < 0) {
        PRINT_ERRNO();
    }

    return socket_fd;
}

void bind_socket(int socket_fd, uint16_t port, bool is_ipv4) {
    if (is_ipv4) {
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(port);
        CHECK_ERRNO(bind(socket_fd, (struct sockaddr *) &address,
                         (socklen_t) sizeof(address)));
    }
    else {
        struct sockaddr_in6 address;
        address.sin6_family = AF_INET6;
        address.sin6_flowinfo = 0;
        address.sin6_addr = in6addr_any;
        address.sin6_port = htons(port);
        address.sin6_scope_id = 0;
        CHECK_ERRNO(bind(socket_fd, (struct sockaddr *) &address,
                         (socklen_t) sizeof(address)));
    }
}

void connect_socket(int socket_fd, std::string &host, uint16_t port) {
    if (is_address_ipv4(host)) {
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        ENSURE(inet_pton(AF_INET, host.c_str(), &address.sin_addr) == 1);
        CHECK_ERRNO(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_in)));
    }
    else {
        struct sockaddr_in6 address;
        address.sin6_family = AF_INET6;
        address.sin6_flowinfo = 0;
        ENSURE(inet_pton(AF_INET6, host.c_str(), &address.sin6_addr) == 1);
        address.sin6_port = htons(port);
        address.sin6_scope_id = 0;
        CHECK_ERRNO(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_in6)));
    }
}

void close_socket(int socket_fd) {
    CHECK_ERRNO(close(socket_fd));
}

bool is_address_ipv4(const std::string &host) {
    struct sockaddr_in address;
    return inet_pton(AF_INET, host.c_str(), &address.sin_addr) == 1;
}

bool is_address_ipv6(const std::string &host) {
    struct sockaddr_in6 address;
    return (inet_pton(AF_INET6, host.c_str(), &address.sin6_addr) == 1);
}
