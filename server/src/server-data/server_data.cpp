#include "server_data.h"

#include <cstring>

ServerData::ServerData(uint32_t seed, uint8_t players_count) {
    random = std::minstd_rand(seed);

    for (size_t i = 0; i <= MAX_CLIENTS; ++i) {
        poll_descriptors[i].fd = -1;
        poll_descriptors[i].events = POLLIN;
        poll_descriptors[i].revents = 0;
    }

    clients_buffers = List<Deque<uint8_t>>(MAX_CLIENTS + 1);

    for (PlayerId id = 0; id < players_count; id++) {
        scores[id] = 0;
    }
}

void ServerData::clear_poll_descriptors() {
    for (int i = 0; i <= MAX_CLIENTS; ++i) {
        poll_descriptors[i].revents = 0;
    }
}

void ServerData::clear_client_last_messages() {
    memset(clients_last_messages, NO_MSG, MAX_CLIENTS + 1);
}

void ServerData::set_up_new_game() {
    in_lobby = false;
    for (const auto &player : players) {
        scores[player.first] = 0;
    }
    turn = 0;
    time_to_next_round = 0;
    next_bomb_id = 0;
    gettimeofday(&last_time, nullptr);
}

void ServerData::clear_state() {
    in_lobby = true;
    players.clear();
    player_positions.clear();
    blocks.clear();
    bombs.clear();
    all_accepted_player_messages.clear();
    all_turn_messages.clear();
}
