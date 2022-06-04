#ifndef SERVER_DATA_H
#define SERVER_DATA_H

#include <poll.h>
#include <stddef.h>
#include <string>

#include "types.h"

#define MAX_CLIENTS 25

#define JOIN 0
#define PLACE_BOMB 1
#define PLACE_BLOCK 2
#define MOVE 3

struct ServerData {
    pollfd poll_descriptors[MAX_CLIENTS + 1];
    std::string clients_addresses[MAX_CLIENTS + 1];
    List<Deque<uint8_t>> clients_buffers;
    uint8_t clients_last_messages[MAX_CLIENTS + 1];
    int active_clients = 0;
    Map<PlayerId, Player> players;

    List<uint8_t> all_accepted_player_messages;
    bool in_lobby = true;
    uint64_t time_to_next_round = 0; // in milliseconds

    ServerData();

    void clear_poll_descriptors();
};

#endif // SERVER_DATA_H
