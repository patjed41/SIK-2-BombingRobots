#ifndef CLIENT_DATA_H
#define CLIENT_DATA_H

#include <pthread.h>

#include "../../common/types.h"

// Structure containing the data required by client.
struct ClientData {
    // State of the game/lobby.
    bool is_in_lobby;
    std::string player_name;
    std::string server_name;
    uint8_t players_count;
    uint16_t size_x;
    uint16_t size_y;
    uint16_t game_length;
    uint16_t explosion_radius;
    uint16_t bomb_timer;
    uint16_t turn;
    Map<PlayerId, Player> players;
    Map<PlayerId, Position> player_positions;
    Set<Position> blocks;
    Map<BombId, Bomb> bombs;
    Set<Position> explosions;
    Map<PlayerId, Score> scores;
    Set<PlayerId> died_this_round;
    Set<Position> blocks_destroyed_this_round;

    // Socket descriptors.
    int server_fd;
    int gui_rec_fd;
    int gui_send_fd;

    // Mutex protecting is_in_lobby.
    pthread_mutex_t lock;

    // Initiates new ClientData instance.
    void init();

    // Clears ClientData instance after finished game.
    void clear();
};

#endif // CLIENT_DATA_H
