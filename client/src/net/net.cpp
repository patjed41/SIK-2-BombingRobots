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

int connect(const std::string &host, uint16_t port, bool tcp) {
    struct addrinfo hints{};
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = tcp ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = tcp ? IPPROTO_TCP : IPPROTO_UDP;

    struct addrinfo *result, *next;
    int socket_fd;

    if (getaddrinfo(host.c_str(), std::to_string((int) port).c_str(), &hints, &result) != 0) {
        return -1;
    }

    for (next = result; next != nullptr; next = next->ai_next) {
        socket_fd = socket(next->ai_family, next->ai_socktype, next->ai_protocol);
        if (socket_fd == -1) {
            continue;
        }

        // Correct address was found and connection succeeded.
        if (connect(socket_fd, next->ai_addr, next->ai_addrlen) != -1) {
            break;
        }

        // Connection failed.
        close(socket_fd);
    }

    freeaddrinfo(result);

    // No address was found.
    if (next == nullptr) {
        return -1;
    }

    return socket_fd;
}

int bind_udp_socket(uint16_t port) {
    struct addrinfo hints{};
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo *result, *next;
    int socket_fd;

    if (getaddrinfo(nullptr, std::to_string((int) port).c_str(), &hints, &result) != 0) {
        fatal("Could not bind UDP socket to port %d.", port);
    }

    for (next = result; next != nullptr; next = next->ai_next) {
        socket_fd = socket(next->ai_family, next->ai_socktype, next->ai_protocol);
        if (socket_fd == -1) {
            continue;
        }

        // Correct address was found and binding succeeded.
        if (bind(socket_fd, next->ai_addr, next->ai_addrlen) == 0) {
            break;
        }

        // Binding failed.
        close(socket_fd);
    }

    freeaddrinfo(result);

    // No address was found.
    if (next == nullptr) {
        fatal("Could not bind UDP socket to port %d.", port);
    }

    return socket_fd;
}
