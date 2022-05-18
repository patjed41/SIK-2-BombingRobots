#include "messages.h"
#include "../net/net.h"
#include "../err.h"

#include <iostream>
#include <algorithm>

template<class T>
static T read_uint(int socket_fd) {
    T value;
    size_t read_length = receive_message(socket_fd, &value, sizeof(T), MSG_WAITALL);
    ENSURE(read_length == sizeof(T));
    return value;
}

template<class T>
static void send_uint(int socket_fd, T value) {
    send_message(socket_fd, &value, sizeof(T), 0);
}

static std::string read_string(int socket_fd) {
    uint8_t string_length = read_uint<uint8_t>(socket_fd);
    char buffer[string_length + 1];
    size_t read_length = receive_message(socket_fd, &buffer, string_length, MSG_WAITALL);
    ENSURE(read_length == string_length);
    buffer[string_length] = '\0';
    return std::string(buffer);
}

void read_hello(ClientData &data) {
    ENSURE(read_uint<uint8_t>(data.server_fd) == 0);

    data.server_name = read_string(data.server_fd);
    data.players_count = read_uint<uint8_t>(data.server_fd);
    data.size_x = read_uint<uint16_t>(data.server_fd);
    data.size_y = read_uint<uint16_t>(data.server_fd);
    data.game_length = read_uint<uint16_t>(data.server_fd);
    data.explosion_radius = read_uint<uint16_t>(data.server_fd);
    data.bomb_timer = read_uint<uint16_t>(data.server_fd);
}

uint8_t read_message_from_gui(ClientData &data) {
    uint8_t buffer[2];
    size_t read_length = receive_message(data.gui_rec_fd, buffer, 2, 0);
    ENSURE((read_length == 1 && buffer[0] < 2) || (read_length == 2 && buffer[1] < 4));

    switch (buffer[0]) {
        case 0:
            return 0;
        case 1:
            return 1;
        case 2:
            return 2 + buffer[1];
        default:
            return 0; // This won't happen.
    }
}

void send_message_to_server(ClientData &data, uint8_t message_type) {
    ENSURE(pthread_mutex_lock(&data.lock) == 0);
    bool in_lobby = data.is_in_lobby;
    ENSURE(pthread_mutex_unlock(&data.lock) == 0);

    if (in_lobby) {
        send_uint<uint8_t>(data.server_fd, 0);
    }
    else if (message_type < 2) {
        send_uint<uint8_t>(data.server_fd, message_type + 1);
    }
    else {
        send_uint<uint8_t>(data.server_fd, 3);
        send_uint<uint8_t>(data.server_fd, message_type - 2);
    }
}

static PlayerId read_player_id(int socket_fd) {
    return read_uint<PlayerId>(socket_fd);
}

static Score read_score(int socket_fd) {
    return ntohl(read_uint<Score>(socket_fd));
}

static BombId read_bomb_id(int socket_fd) {
    return read_uint<BombId>(socket_fd);
}

static Position read_position(int socket_fd) {
    Position position;
    position.x = read_uint<uint16_t>(socket_fd);
    position.y = read_uint<uint16_t>(socket_fd);
    return position;
}

static Player read_player(int socket_fd) {
    Player player;
    player.name = read_string(socket_fd);
    player.address = read_string(socket_fd);
    return player;
}

static void read_accepted_player(ClientData &data) {
    PlayerId player_id = read_player_id(data.server_fd);
    Player player = read_player(data.server_fd);
    data.players[player_id] = player;
}

static void read_game_started(ClientData &data) {
    data.players_count = 0;
    data.players.clear();

    u_int32_t map_length = ntohl(read_uint<uint32_t>(data.server_fd));
    for (uint32_t i = 0; i < map_length; i++) {
        PlayerId player_id = read_player_id(data.server_fd);
        Player player = read_player(data.server_fd);
        data.players[player_id] = player;
    }

    data.is_in_lobby = false;
}

static void read_game_ended(ClientData &data) {
    Map<PlayerId, Score> server_scores;

    u_int32_t map_length = ntohl(read_uint<uint32_t>(data.server_fd));
    for (uint32_t i = 0; i < map_length; i++) {
        PlayerId player_id = read_player_id(data.server_fd);
        Score server_score = read_score(data.server_fd);
        server_scores[player_id] = server_score;
    }

    if (data.scores.size() != server_scores.size()
        || !std::equal(data.scores.begin(), data.scores.end(), server_scores.begin())) {
        fatal("Incorrect score in client.");
    }

    clear(&data);
}

static void read_bomb_placed(ClientData &data) {
    BombId bomb_id = read_bomb_id(data.server_fd);
    Position position = read_position(data.server_fd);
    Bomb bomb = (Bomb) { bomb.position = position, bomb.timer = ntohs(data.bomb_timer) };
    data.bombs[bomb_id] = bomb;
}

static void read_bomb_exploded(ClientData &data) {
    BombId bomb_id = read_bomb_id(data.server_fd);
    data.explosions.push_back(data.bombs[bomb_id].position);
    data.bombs.erase(bomb_id);

    u_int32_t list_length = ntohl(read_uint<uint32_t>(data.server_fd));
    for (uint32_t i = 0; i < list_length; i++) {
        PlayerId player_id = read_player_id(data.server_fd);
        data.died_this_round.insert(player_id);
    }

    list_length = ntohl(read_uint<uint32_t>(data.server_fd));
    for (uint32_t i = 0; i < list_length; i++) {
        Position position = read_position(data.server_fd);
        data.blocks.erase(std::find(data.blocks.begin(), data.blocks.end(), position));
    }
}

static void read_player_moved(ClientData &data) {
    PlayerId player_id = read_player_id(data.server_fd);
    Position position = read_position(data.server_fd);
    data.player_positions[player_id] = position;
}

static void read_block_placed(ClientData &data) {
    Position position = read_position(data.server_fd);
    data.blocks.push_back(position);
}

static void read_event(ClientData &data) {
    u_int8_t event_type = read_uint<uint8_t>(data.server_fd);
    ENSURE(event_type < 4);

    switch (event_type) {
        case 0:
            read_bomb_placed(data);
            break;
        case 1:
            read_bomb_exploded(data);
            break;
        case 2:
            read_player_moved(data);
            break;
        case 3:
            read_block_placed(data);
            break;
    }
}

static void read_turn(ClientData &data) {
    data.explosions.clear();
    data.died_this_round.clear();
    data.turn = read_uint<uint16_t>(data.server_fd);

    u_int32_t list_length = ntohl(read_uint<uint32_t>(data.server_fd));
    for (uint32_t i = 0; i < list_length; i++) {
        read_event(data);
    }

    for (PlayerId player_id : data.died_this_round) {
        data.scores[player_id]++;
    }
    for (auto &bomb : data.bombs) {
        bomb.second.timer--;
    }
}

bool read_message_from_server(ClientData &data) {
    uint8_t message_type = read_uint<uint8_t>(data.server_fd);
    ENSURE(message_type > 0 && message_type < 5);

    switch (message_type) {
        case 1:
            read_accepted_player(data);
            return true;
        case 2:
            read_game_started(data);
            return false;
        case 3:
            read_turn(data);
            return true;
        case 4:
            read_game_ended(data);
            return false; // ?????????
        default:
            return false; // This won't happen.
    }
}