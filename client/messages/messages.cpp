#include "messages.h"
#include "../net/net.h"
#include "../../common/err.h"

#include <iostream>

#define DATAGRAM_LIMIT 65507
#define GUI_WRONG_MSG 10
#define NO_FLAGS 0

// Reads number of type T from socket [socket_fd] and returns it.
template<class T>
static T read_uint(int socket_fd) {
    T value;
    size_t read_length = receive_message(socket_fd, &value, sizeof(T), MSG_WAITALL);
    ENSURE(read_length == sizeof(T));
    return value;
}

// Sends number of type T and value [value] using socket [socket_fd].
template<class T>
static void send_uint(int socket_fd, T value) {
    send_message(socket_fd, &value, sizeof(T), NO_FLAGS);
}

// Reads string from socket [socket_fd] and returns it.
static std::string read_string(int socket_fd) {
    auto string_length = read_uint<uint8_t>(socket_fd);
    char buffer[string_length + 1];
    size_t read_length = receive_message(socket_fd, &buffer, string_length, MSG_WAITALL);
    ENSURE(read_length == string_length);
    buffer[string_length] = '\0';
    return std::string(buffer);
}

// Sends Join message to server.
static void send_join(const ClientData &data) {
    uint8_t buffer[data.player_name.size() + 2];
    buffer[0] = 0;
    buffer[1] = (uint8_t) data.player_name.size();
    memcpy(buffer + 2, data.player_name.c_str(), data.player_name.size());
    send_message(data.server_fd, buffer, sizeof(buffer), NO_FLAGS);
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

uint8_t read_message_from_gui(const ClientData &data) {
    uint8_t buffer[2];
    size_t read_length = receive_message(data.gui_rec_fd, buffer, 2, NO_FLAGS);
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
        send_message(data.server_fd, buffer, 2, NO_FLAGS);
    }
}

// Reads PlayerId from socket [socket_fd] and returns it.
static PlayerId read_player_id(int socket_fd) {
    return read_uint<PlayerId>(socket_fd);
}

// Reads Socket from socket [socket_fd] and returns it.
static Score read_score(int socket_fd) {
    return ntohl(read_uint<Score>(socket_fd));
}

// Reads BombId from socket [socket_fd] and returns it.
static BombId read_bomb_id(int socket_fd) {
    return read_uint<BombId>(socket_fd);
}

// Reads Position from socket [socket_fd] and returns it.
static Position read_position(int socket_fd) {
    Position position{};
    position.x = read_uint<uint16_t>(socket_fd);
    position.y = read_uint<uint16_t>(socket_fd);
    return position;
}

// Reads Player from socket [socket_fd] and returns it.
static Player read_player(int socket_fd) {
    Player player;
    player.name = read_string(socket_fd);
    player.address = read_string(socket_fd);
    return player;
}

// Reads AcceptedPlayer message from server.
static void read_accepted_player(ClientData &data) {
    PlayerId player_id = read_player_id(data.server_fd);
    Player player = read_player(data.server_fd);
    data.players[player_id] = player;
}

// Reads GameStarted message from server.
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

// Reads BombPlaced message from server.
static void read_bomb_placed(ClientData &data) {
    BombId bomb_id = read_bomb_id(data.server_fd);
    Position position = read_position(data.server_fd);
    Bomb bomb(position, ntohs(data.bomb_timer));
    data.bombs[bomb_id] = bomb;
}

// Converts [position] from net order to host order and vice versa.
static Position convertPosition(const Position &position) {
    return Position(htons(position.x), htons(position.y));
}

// Finds explosions caused by explosion in position [bomb_position] and
// puts them into [data.explosions].
static void findExplosions(ClientData &data, const Position &bomb_position) {
    uint16_t host_explosion_radius = ntohs(data.explosion_radius);

    // Explosions above the bomb.
    Position position = convertPosition(bomb_position);
    for (uint16_t i = 0; i <= host_explosion_radius; i++) {
        data.explosions.insert(convertPosition(position));
        if (data.blocks.contains(convertPosition(position)) || position.y == ntohs(data.size_y) - 1) {
            break;
        }
        position.y++;
    }

    // Explosions right to the bomb.
    position = convertPosition(bomb_position);
    for (uint16_t i = 0; i <= host_explosion_radius; i++) {
        data.explosions.insert(convertPosition(position));
        if (data.blocks.contains(convertPosition(position)) || position.x == ntohs(data.size_x) - 1) {
            break;
        }
        position.x++;
    }

    // Explosions under the bomb.
    position = convertPosition(bomb_position);
    for (uint16_t i = 0; i <= host_explosion_radius; i++) {
        data.explosions.insert(convertPosition(position));
        if (data.blocks.contains(convertPosition(position)) || position.y == 0) {
            break;
        }
        position.y--;
    }

    // Explosions left to the bomb.
    position = convertPosition(bomb_position);
    for (uint16_t i = 0; i <= host_explosion_radius; i++) {
        data.explosions.insert(convertPosition(position));
        if (data.blocks.contains(convertPosition(position)) || position.x == 0) {
            break;
        }
        position.x--;
    }
}

// Reads BombExploded message from server.
static void read_bomb_exploded(ClientData &data) {
    BombId bomb_id = read_bomb_id(data.server_fd);

    findExplosions(data, data.bombs[bomb_id].position);
    data.bombs.erase(bomb_id);

    u_int32_t list_length = ntohl(read_uint<uint32_t>(data.server_fd));
    for (uint32_t i = 0; i < list_length; i++) {
        PlayerId player_id = read_player_id(data.server_fd);
        data.died_this_round.insert(player_id);
    }

    list_length = ntohl(read_uint<uint32_t>(data.server_fd));
    for (uint32_t i = 0; i < list_length; i++) {
        Position position = read_position(data.server_fd);
        data.blocks_destroyed_this_round.insert(position);
    }
}

// Reads PlayerMoved message from server.
static void read_player_moved(ClientData &data) {
    PlayerId player_id = read_player_id(data.server_fd);
    Position position = read_position(data.server_fd);
    data.player_positions[player_id] = position;
}

// Reads BlockPlaced message from server.
static void read_block_placed(ClientData &data) {
    Position position = read_position(data.server_fd);
    data.blocks.emplace(position);
}

// Reads Event message from server.
static void read_event(ClientData &data) {
    auto event_type = read_uint<uint8_t>(data.server_fd);
    if (event_type >= 4) {
        fatal("Invalid event (%d).", (int) event_type);
    }

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

// Reads Turn message from server.
static void read_turn(ClientData &data) {
    data.explosions.clear();
    data.died_this_round.clear();
    data.blocks_destroyed_this_round.clear();
    for (auto &bomb : data.bombs) {
        bomb.second.timer--;
    }

    data.turn = read_uint<uint16_t>(data.server_fd);

    u_int32_t list_length = ntohl(read_uint<uint32_t>(data.server_fd));
    for (uint32_t i = 0; i < list_length; i++) {
        read_event(data);
    }

    for (PlayerId player_id : data.died_this_round) {
        data.scores[player_id]++;
    }
    for (const Position &position : data.blocks_destroyed_this_round) {
        data.blocks.erase(position);
    }
}

// Reads GameEnded message from server.
static void read_game_ended(ClientData &data) {
    Map<PlayerId, Score> server_scores;

    u_int32_t map_length = ntohl(read_uint<uint32_t>(data.server_fd));
    for (uint32_t i = 0; i < map_length; i++) {
        PlayerId player_id = read_player_id(data.server_fd);
        Score server_score = read_score(data.server_fd);
        server_scores[player_id] = server_score;
    }

    data.clear();
}

bool read_message_from_server(ClientData &data) {
    auto message_type = read_uint<uint8_t>(data.server_fd);
    if (message_type == 0 || message_type >= 5) {
        fatal("Invalid message (%d) from server.", (int) message_type);
    }

    bool send_to_gui = false;

    switch (message_type) {
        case 1:
            read_accepted_player(data);
            send_to_gui = true;
            break;
        case 2:
            read_game_started(data);
            send_to_gui = false;
            break;
        case 3:
            read_turn(data);
            send_to_gui = true;
            break;
        case 4:
            read_game_ended(data);
            send_to_gui = true;
            break;
        default:
            break;
    }

    return send_to_gui;
}

// Puts number [value] of type T into [buffer] starting at position [next_index].
template<class T>
static void put_uint_into_buffer(T value, uint8_t *buffer, size_t &next_index) {
    memcpy(buffer + next_index, &value, sizeof(T));
    next_index += sizeof(T);
}

// Puts string [str] into [buffer] starting at position [next_index].
static void put_string_into_buffer(const std::string &str, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint8_t>((uint8_t) str.size(), buffer, next_index);
    for (char i : str) {
        put_uint_into_buffer<uint8_t>((uint8_t) i, buffer, next_index);
    }
}

// Puts [data.players] into [buffer] starting at position [next_index].
static void put_players_into_buffer(const ClientData &data, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint32_t>(htonl((uint32_t) data.players.size()), buffer, next_index);
    for (const auto &player : data.players) {
        put_uint_into_buffer<PlayerId>(player.first, buffer, next_index);
        put_string_into_buffer(player.second.name, buffer, next_index);
        put_string_into_buffer(player.second.address, buffer, next_index);
    }
}

// Puts [position] into [buffer] starting at position [next_index].
static void put_position_into_buffer(const Position &position, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint16_t>(position.x, buffer, next_index);
    put_uint_into_buffer<uint16_t>(position.y, buffer, next_index);
}

// Puts [data.player_positions] into [buffer] starting at position [next_index].
static void put_player_positions_into_buffer(const ClientData &data, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint32_t>(htonl((uint32_t) data.player_positions.size()), buffer, next_index);
    for (const auto &player_position : data.player_positions) {
        put_uint_into_buffer<PlayerId>(player_position.first, buffer, next_index);
        put_position_into_buffer(player_position.second, buffer, next_index);
    }
}

// Puts [data.blocks] into [buffer] starting at position [next_index].
static void put_blocks_into_buffer(const ClientData &data, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint32_t>(htonl((uint32_t) data.blocks.size()), buffer, next_index);
    for (const Position &block_position : data.blocks) {
        put_position_into_buffer(block_position, buffer, next_index);
    }
}

// Puts positions and timers of [data.blocks] into [buffer] starting at position [next_index].
static void put_bombs_into_buffer(const ClientData &data, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint32_t>(htonl((uint32_t) data.bombs.size()), buffer, next_index);
    for (const auto &bomb : data.bombs) {
        put_position_into_buffer(bomb.second.position, buffer, next_index);
        put_uint_into_buffer<uint16_t>(htons(bomb.second.timer), buffer, next_index);
    }
}

// Puts [data.explosion] into [buffer] starting at position [next_index].
static void put_explosions_into_buffer(const ClientData &data, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint32_t>(htonl((uint32_t) data.explosions.size()), buffer, next_index);
    for (const Position &explosion_position : data.explosions) {
        put_position_into_buffer(explosion_position, buffer, next_index);
    }
}

// Puts [data.scores] into [buffer] starting at position [next_index].
static void put_scores_into_buffer(const ClientData &data, uint8_t *buffer, size_t &next_index) {
    put_uint_into_buffer<uint32_t>(htonl((uint32_t) data.scores.size()), buffer, next_index);
    for (const auto &score : data.scores) {
        put_uint_into_buffer<PlayerId>(score.first, buffer, next_index);
        put_uint_into_buffer<Score>(htonl(score.second), buffer, next_index);
    }
}

// Sends message of length [length] located in [buffer] to GUI.
static void send_to_gui(int gui_send_fd, const uint8_t *buffer, size_t length) {
    ssize_t sent_length = send(gui_send_fd, buffer, length, NO_FLAGS);
    if (sent_length != (ssize_t) length) {
        fatal("Connection with GUI failed.");
    }
}

// Sends Lobby message to GUI.
static void send_lobby(const ClientData &data) {
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

// Sends Game message to GUI.
static void send_game(const ClientData &data) {
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
    bool is_in_lobby = data.is_in_lobby;
    ENSURE(pthread_mutex_unlock(&data.lock) == 0);

    if (is_in_lobby) {
        send_lobby(data);
    }
    else {
        send_game(data);
    }
}
