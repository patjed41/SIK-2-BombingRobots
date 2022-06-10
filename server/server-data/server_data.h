#ifndef SERVER_DATA_H
#define SERVER_DATA_H

#include <poll.h>
#include <stddef.h>
#include <string>
#include <random>
#include <sys/time.h>

#include "../../common/types.h"

#define MAX_CLIENTS 25

// Messages from client.
#define JOIN 0
#define PLACE_BOMB 1
#define PLACE_BLOCK 2
#define MOVE 3
#define NO_MSG 10

// Structure containing data used by server.
struct ServerData {
    // Communication with clients.
    int active_clients = 0;
    pollfd poll_descriptors[MAX_CLIENTS + 1];
    std::string clients_addresses[MAX_CLIENTS + 1];
    List<Deque<uint8_t>> clients_buffers;
    uint8_t clients_last_messages[MAX_CLIENTS + 1];

    // Game data.
    Map<PlayerId, Player> players;
    Map<PlayerId, size_t> poll_ids;
    Set<PlayerId> disconnected_players;
    Map<PlayerId, Position> player_positions;
    Set<Position> blocks;
    Map<BombId, Bomb> bombs;
    uint32_t next_bomb_id;
    Set<PlayerId> robots_destroyed;     // Robots destroyed by single bomb.
    Set<Position> blocks_destroyed;     // Blocks destroyed by single bomb.
    Set<PlayerId> all_robots_destroyed; // Robots destroyed by all bombs in one round.
    Set<Position> all_blocks_destroyed; // Blocks destroyed by all bombs in one round.
    Map<PlayerId, Score> scores;

    // Server state.
    bool in_lobby = true;
    double time_to_next_round; // in milliseconds
    timeval last_time;         // Last time when update_time_during_game() was called.
    uint16_t turn;

    // Saved messages for clients that connect late.
    List<uint8_t> all_accepted_player_messages;
    List<uint8_t> all_turn_messages;

    std::minstd_rand random;

    ServerData(uint32_t seed, uint8_t players_count);

    // Sets poll_descriptors[i].id to -1 for every i.
    void clear_poll_descriptors();

    // Sets clients_last_messages[i] to NO_MSG for every i.
    void clear_clients_last_messages();

    // Updates [time_to_next_round] and [last_time].
    void update_time_during_game();

    // Sets up all attributes so game can start in a correct state.
    void set_up_new_game();

    // Sets up attributes for next turn.
    void next_turn();

    // Clears game state after the game finished.
    void clear_state();
};

#endif // SERVER_DATA_H
