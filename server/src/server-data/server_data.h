#ifndef SERVER_DATA_H
#define SERVER_DATA_H

#include <poll.h>
#include <stddef.h>
#include <string>
#include <random>
#include <sys/time.h>

#include "types.h"

#define MAX_CLIENTS 25

#define JOIN 0
#define PLACE_BOMB 1
#define PLACE_BLOCK 2
#define MOVE 3
#define NO_MSG 10

struct ServerData {
    pollfd poll_descriptors[MAX_CLIENTS + 1];
    std::string clients_addresses[MAX_CLIENTS + 1];
    List<Deque<uint8_t>> clients_buffers;
    uint8_t clients_last_messages[MAX_CLIENTS + 1];
    int active_clients = 0;
    Set<PlayerId> disconnected_players;

    Map<PlayerId, Player> players;
    Map<PlayerId, Position> player_positions;
    Set<Position> blocks;
    Map<BombId, Bomb> bombs;
    uint32_t next_bomb_id;
    Set<PlayerId> robots_destroyed;
    Set<Position> blocks_destroyed;
    Set<PlayerId> all_robots_destroyed;
    Set<Position> all_blocks_destroyed;
    Map<PlayerId, Score> scores;

    List<uint8_t> all_accepted_player_messages;
    List<uint8_t> all_turn_messages;

    bool in_lobby = true;
    double time_to_next_round; // in milliseconds
    timeval last_time;
    uint16_t turn;

    std::minstd_rand random;

    ServerData(uint32_t seed, uint8_t players_count);

    void clear_poll_descriptors();

    void clear_client_last_messages();

    void set_up_new_game();

    void clear_state();
};

#endif // SERVER_DATA_H
