#include "net.h"

#include <unistd.h>
#include <netinet/tcp.h>

int bind_tcp_socket(uint16_t port) {
    int socket_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    ENSURE(socket_fd > 0);

    sockaddr_in6 address{};
    address.sin6_family = AF_INET6;
    address.sin6_flowinfo = 0;
    address.sin6_addr = IN6ADDR_ANY_INIT; // IPv4 and IPv6 should work.
    address.sin6_port = htons(port);
    address.sin6_scope_id = 0;
    CHECK_ERRNO(bind(socket_fd, (sockaddr *) &address, (socklen_t) sizeof(address)));

    return socket_fd;
}

void start_listening(int socket_fd, int queue_length) {
    CHECK_ERRNO(listen(socket_fd, queue_length));
}

bool turn_off_nagle(int socked_fd) {
    struct ip_mreq ipv{};
    return setsockopt(socked_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&ipv, sizeof(ipv)) == 0;
}

int accept_connection(int socket_fd, sockaddr_in6 *client_address) {
    socklen_t client_address_length = (socklen_t) sizeof(*client_address);

    int client_fd = accept(socket_fd, (sockaddr *) client_address, &client_address_length);
    if (client_fd < 0) {
        return -1;
    }

    return client_fd;
}

std::string get_address(const sockaddr_in6 &address) {
    char address_str[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, &address.sin6_addr, address_str, INET6_ADDRSTRLEN)) {
        std::string result(address_str);
        result.insert(0, "[");
        result += ']';

        // Append port.
        result += ':';
        result.append(std::to_string(ntohs(address.sin6_port)));
        return result;
    }
    else {
        return "fail";
    }
}

bool send_message(int socket_fd, const List<uint8_t> &message, int flags) {
    errno = 0;
    ssize_t sent_length = send(socket_fd, message.data(), message.size(), flags);
    return sent_length == (ssize_t) message.size();
}
