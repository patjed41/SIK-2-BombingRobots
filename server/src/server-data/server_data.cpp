#include "server_data.h"

ServerData::ServerData() {
    for (size_t i = 0; i <= MAX_CLIENTS; ++i) {
        poll_descriptors[i].fd = -1;
        poll_descriptors[i].events = POLLIN;
        poll_descriptors[i].revents = 0;
    }

    clients_buffers = List<Deque<uint8_t>>(MAX_CLIENTS + 1);
}

void ServerData::clear_poll_descriptors() {
    for (int i = 0; i <= MAX_CLIENTS; ++i) {
        poll_descriptors[i].revents = 0;
    }
}
