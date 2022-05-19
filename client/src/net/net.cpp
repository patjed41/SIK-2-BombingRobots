#include "net.h"
#include "../err.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>
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
        struct sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(port);
        CHECK_ERRNO(bind(socket_fd, (struct sockaddr *) &address,
                         (socklen_t) sizeof(address)));
    }
    else {
        struct sockaddr_in6 address{};
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
        struct sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        ENSURE(inet_pton(AF_INET, host.c_str(), &address.sin_addr) == 1);
        CHECK_ERRNO(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_in)));
    }
    else {
        struct sockaddr_in6 address{};
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
    struct sockaddr_in address{};
    return inet_pton(AF_INET, host.c_str(), &address.sin_addr) == 1;
}

bool is_address_ipv6(const std::string &host) {
    struct sockaddr_in6 address{};
    return (inet_pton(AF_INET6, host.c_str(), &address.sin6_addr) == 1);
}

void turn_off_nagle(int socked_fd) {
    struct ip_mreq ipv{};
    CHECK_ERRNO(setsockopt(socked_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&ipv, sizeof(ipv)));
}

size_t receive_message(int socket_fd, void *buffer, size_t max_length, int flags) {
    errno = 0;
    ssize_t received_length = recv(socket_fd, buffer, max_length, flags);
    if (received_length < 0) {
        PRINT_ERRNO();
    }
    return (size_t) received_length;
}

void send_message(int socket_fd, const void *message, size_t length, int flags) {
    errno = 0;
    ssize_t sent_length = send(socket_fd, message, length, flags);
    if (sent_length < 0) {
        PRINT_ERRNO();
    }
    ENSURE(sent_length == (ssize_t) length);
}

int connect_to(const std::string &host, uint16_t port, bool tcp) {
    struct addrinfo hints{};
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = tcp ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = tcp ? IPPROTO_TCP : IPPROTO_UDP;          /* Any protocol */

    struct addrinfo *result, *next;
    int server_fd = -1;

    if (getaddrinfo(host.c_str(), std::to_string((int) port).c_str(), &hints, &result) != 0) {
        return -1;
    }

    for (next = result; next != nullptr; next = next->ai_next) {
        server_fd = socket(next->ai_family, next->ai_socktype, next->ai_protocol);
        if (server_fd == -1) {
            continue;
        }

        if (connect(server_fd, next->ai_addr, next->ai_addrlen) != -1) {
            break;
        }

        close(server_fd);
    }

    freeaddrinfo(result);

    return server_fd;
}
