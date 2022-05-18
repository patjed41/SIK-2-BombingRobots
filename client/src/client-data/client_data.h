#ifndef ROBOTSCLIENT_CLIENT_DATA_H
#define ROBOTSCLIENT_CLIENT_DATA_H

#include <pthread.h>
#include <set>

#include "../types.h"

struct ClientData {
    bool is_in_lobby;
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
    List<Position> blocks;
    Map<BombId, Bomb> bombs; // Bomb.timer - host order
    List<Position> explosions;
    Map<PlayerId, Score> scores; // Score - host order
    std::set<PlayerId> died_this_round;

    int server_fd;
    int gui_rec_fd;
    int gui_send_fd;

    pthread_mutex_t lock;
};

void init(ClientData* data);

void clear(ClientData* data);

void destroy(ClientData* data);

#endif //ROBOTSCLIENT_CLIENT_DATA_H
