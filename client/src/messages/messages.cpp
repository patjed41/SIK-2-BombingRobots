#include "messages.h"
#include "../net/net.h"
#include "../err.h"

#include <iostream>

#define DATAGRAM_LIMIT 65507
#define GUI_WRONG_MSG 10

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
    auto string_length = read_uint<uint8_t>(socket_fd);
    char buffer[string_length + 1];
    size_t read_length = receive_message(socket_fd, &buffer, string_length, MSG_WAITALL);
    ENSURE(read_length == string_length);
    buffer[string_length] = '\0';
    return std::string(buffer);
}

static void send_join(ClientData &data) {
    uint8_t buffer[data.player_name.size() + 2];
    buffer[0] = 0;
    buffer[1] = (uint8_t) data.player_name.size();
    memcpy(buffer + 2, data.player_name.c_str(), data.player_name.size());
    send_message(data.server_fd, buffer, sizeof(buffer), 0);
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
    if (!(read_length == 1 && buffer[0] < 2) && !(read_length == 2 && buffer[1] < 4)) {
        return GUI_WRONG_MSG;
    }

    switch (buffer[0]) {
        case 0:
            return 0;
        case 1:
            return 1;
        case 2:
            return 2 + buffer[1];
        default:
            return 0;
    }
}

void send_message_to_server(ClientData &data, uint8_t message_type) {
    if (message_type == GUI_WRONG_MSG) {
        return;
    }

    ENSURE(pthread_mutex_lock(&data.lock) == 0);
    bool in_lobby = data.is_in_lobby;
    ENSURE(pthread_mutex_unlock(&data.lock) == 0);

    if (in_lobby) { // Join
        send_join(data);
    }
    else if (message_type < 2) { // PlaceBomb or PlaceBlock
        send_uint<uint8_t>(data.server_fd, message_type + 1);
    }
    else { // Move
        uint8_t buffer[2];
        buffer[0] = 3;
        buffer[1] = message_type - 2;
        send_message(data.server_fd, buffer, 2, 0);
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
    Position position{};
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
    data.players.clear();

    u_int32_t map_length = ntohl(read_uint<uint32_t>(data.server_fd));
    for (uint32_t i = 0; i < map_length; i++) {
        PlayerId player_id = read_player_id(data.server_fd);
        Player player = read_player(data.server_fd);
        data.players[player_id] = player;
    }

    data.scores.clear();
    for (const auto &player : data.players) {
        data.scores[player.first] = 0;
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

    data.clear();
}

static void read_bomb_placed(ClientData &data) {
    BombId bomb_id = read_bomb_id(data.server_fd);
    Position position = read_position(data.server_fd);
    Bomb bomb = (Bomb) { .position = position, .timer = ntohs(data.bomb_timer) };
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
    auto event_type = read_uint<uint8_t>(data.server_fd);
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
        default:
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

void read_message_from_server(ClientData &data) {
    auto message_type = read_uint<uint8_t>(data.server_fd);
    ENSURE(message_type > 0 && message_type < 5);

    ENSURE(pthread_mutex_lock(&data.lock) == 0);

    switch (message_type) {
        case 1:
            read_accepted_player(data);
            break;
        case 2:
            read_game_started(data);
            break;
        case 3:
            read_turn(data);
            break;
        case 4:
            read_game_ended(data);
            break;
        default:
            break;
    }

    ENSURE(pthread_mutex_unlock(&data.lock) == 0);
}

template<class T>
void put_uint_into_buffer(T value, uint8_t *buffer, size_t &next_index) {
    memcpy(buffer + next_index, &value, sizeof(T));
    next_index += sizeof(T);
}

void put_string_into_buffer(const std::string &str, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint8_t>((uint8_t) str.size(), buffer, next_index);
    for (char i : str) {
        put_uint_into_buffer<uint8_t>((uint8_t) i, buffer, next_index);
    }
}

static void put_players_into_buffer(ClientData &data, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint32_t>(htonl((uint32_t) data.players.size()), buffer, next_index);
    for (const auto &player : data.players) {
        put_uint_into_buffer<PlayerId>(player.first, buffer, next_index);
        put_string_into_buffer(player.second.name, buffer, next_index);
        put_string_into_buffer(player.second.address, buffer, next_index);
    }
}

static void put_position_into_buffer(const Position &position, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint16_t>(position.x, buffer, next_index);
    put_uint_into_buffer<uint16_t>(position.y, buffer, next_index);
}

static void put_player_positions_into_buffer(ClientData &data, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint32_t>(htonl((uint32_t) data.player_positions.size()), buffer, next_index);
    for (const auto &player_position : data.player_positions) {
        put_uint_into_buffer<PlayerId>(player_position.first, buffer, next_index);
        put_position_into_buffer(player_position.second, buffer, next_index);
    }
}

static void put_blocks_into_buffer(ClientData &data, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint32_t>(htonl((uint32_t) data.blocks.size()), buffer, next_index);
    for (const Position &block_position : data.blocks) {
        put_position_into_buffer(block_position, buffer, next_index);
    }
}

static void put_bombs_into_buffer(ClientData &data, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint32_t>(htonl((uint32_t) data.bombs.size()), buffer, next_index);
    for (const auto &bomb : data.bombs) {
        put_position_into_buffer(bomb.second.position, buffer, next_index);
        put_uint_into_buffer<uint16_t>(htons(bomb.second.timer), buffer, next_index);
    }
}

static void put_explosions_into_buffer(ClientData &data, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint32_t>(htonl((uint32_t) data.explosions.size()), buffer, next_index);
    for (const Position &explosion_position : data.explosions) {
        put_position_into_buffer(explosion_position, buffer, next_index);
    }
}

static void put_scores_into_buffer(ClientData &data, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint32_t>(htonl((uint32_t) data.scores.size()), buffer, next_index);
    for (const auto &score : data.scores) {
        put_uint_into_buffer<PlayerId>(score.first, buffer, next_index);
        put_uint_into_buffer<Score>(htonl(score.second), buffer, next_index);
    }
}

static void send_to_gui(int gui_send_fd, const uint8_t *buffer, size_t length) {
    ssize_t sent_length = send(gui_send_fd, buffer, length, 0);
    if (sent_length != (ssize_t) length) {
        fatal("Connection with GUI refused.");
    }
}

static void send_lobby(ClientData &data) {
    uint8_t buffer[DATAGRAM_LIMIT];
    size_t next_index = 0;

    put_uint_into_buffer<uint8_t>(0, buffer, next_index);
    put_string_into_buffer(data.server_name, buffer, next_index);
    put_uint_into_buffer<uint8_t>(data.players_count, buffer, next_index);
    put_uint_into_buffer<uint16_t>(data.size_x, buffer, next_index);
    put_uint_into_buffer<uint16_t>(data.size_y, buffer, next_index);
    put_uint_into_buffer<uint16_t>(data.game_length, buffer, next_index);
    put_uint_into_buffer<uint16_t>(data.explosion_radius, buffer, next_index);
    put_uint_into_buffer<uint16_t>(data.bomb_timer, buffer, next_index);
    put_players_into_buffer(data, buffer, next_index);

    send_to_gui(data.gui_send_fd, buffer, next_index);
}

static void send_game(ClientData &data) {
    uint8_t buffer[DATAGRAM_LIMIT];
    size_t next_index = 0;

    put_uint_into_buffer<uint8_t>(1, buffer, next_index);
    put_string_into_buffer(data.server_name, buffer, next_index);
    put_uint_into_buffer<uint16_t>(data.size_x, buffer, next_index);
    put_uint_into_buffer<uint16_t>(data.size_y, buffer, next_index);
    put_uint_into_buffer<uint16_t>(data.game_length, buffer, next_index);
    put_uint_into_buffer<uint16_t>(data.turn, buffer, next_index);
    put_players_into_buffer(data, buffer, next_index);
    put_player_positions_into_buffer(data, buffer, next_index);
    put_blocks_into_buffer(data, buffer, next_index);
    put_bombs_into_buffer(data, buffer, next_index);
    put_explosions_into_buffer(data, buffer, next_index);
    put_scores_into_buffer(data, buffer, next_index);

    send_to_gui(data.gui_send_fd, buffer, next_index);
}

void send_message_to_gui(ClientData &data) {
    ENSURE(pthread_mutex_lock(&data.lock) == 0);

    if (data.is_in_lobby) {
        send_lobby(data);
    }
    else {
        send_game(data);
    }

    ENSURE(pthread_mutex_unlock(&data.lock) == 0);
}
