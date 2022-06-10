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

void ServerData::clear_clients_last_messages() {
    memset(clients_last_messages, NO_MSG, MAX_CLIENTS + 1);
}

// Returns [t1] - [t2] in milliseconds.
static double time_dif_in_millis(const timeval &t1, const timeval &t2) {
    double seconds_dif = (float) (t2.tv_sec - t1.tv_sec) * 1000.f;
    double millis_dif = (float) (t2.tv_usec - t1.tv_usec) / 1000.f;
    return seconds_dif + millis_dif;
}

void ServerData::update_time_during_game() {
    timeval current_time;
    gettimeofday(&current_time, nullptr);
    time_to_next_round += time_dif_in_millis(last_time, current_time);
    last_time = current_time;
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

void ServerData::next_turn() {
    turn++;
    time_to_next_round = 0;
    all_robots_destroyed.clear();
    all_blocks_destroyed.clear();
}

void ServerData::clear_state() {
    in_lobby = true;
    disconnected_players.clear();
    players.clear();
    poll_ids.clear();
    player_positions.clear();
    blocks.clear();
    bombs.clear();
    all_accepted_player_messages.clear();
    all_turn_messages.clear();
}
