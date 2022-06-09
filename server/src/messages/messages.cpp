#include "messages.h"
#include "../net/net.h"
#include "../err.h"

#include <arpa/inet.h>
#include <algorithm>

// Puts [str] at the end of the [message].
static void put_string_into_message(const std::string &str, List<uint8_t> &message) {
    message.push_back((uint8_t) str.length());
    for (size_t i = 0; i < str.length(); i++) {
        message.push_back((uint8_t) str[i]);
    }
}

// Puts [value] at the end of the [message].
template<class T>
static void put_uint_into_message(T value, List<uint8_t> &message) {
    static uint8_t buffer[sizeof(uint64_t)];
    memcpy(buffer, &value, sizeof(T));
    for (ssize_t i = sizeof(T) - 1; i >= 0; i--) {
        message.push_back(buffer[i]);
    }
}

// Puts [player] at the end of the [message].
static void put_player_into_message(const Player &player, List<uint8_t> &message) {
    put_string_into_message(player.name, message);
    put_string_into_message(player.address, message);
}

// Return id of player with poll id [poll_id].
static PlayerId get_player_id_by_poll_id(const ServerData &data, size_t poll_id) {
    for (const auto & player : data.players) {
        if (player.second.poll_id == poll_id) {
            return player.first;
        }
    }

    return MAX_CLIENTS + 1;
}

List<uint8_t> build_hello(const ServerParameters &parameters) {
    List<uint8_t> message(1, 0);
    put_string_into_message(parameters.server_name, message);
    put_uint_into_message<uint8_t>(parameters.players_count, message);
    put_uint_into_message<uint16_t>(parameters.size_x, message);
    put_uint_into_message<uint16_t>(parameters.size_y, message);
    put_uint_into_message<uint16_t>(parameters.game_length, message);
    put_uint_into_message<uint16_t>(parameters.explosion_radius, message);
    put_uint_into_message<uint16_t>(parameters.bomb_timer, message);

    return message;
}

List<uint8_t> build_accepted_player(const ServerData &data, size_t poll_id) {
    List<uint8_t> message(1, 1);
    PlayerId player_id = get_player_id_by_poll_id(data, poll_id);
    put_uint_into_message<PlayerId>(player_id, message);
    put_player_into_message(data.players.at(player_id), message);
    return message;
}

List<uint8_t> build_game_started(const ServerData &data) {
    List<uint8_t> message(1, 2);
    put_uint_into_message<uint32_t>((uint32_t) data.players.size(), message);
    for (const auto &player : data.players) {
        put_uint_into_message<PlayerId>(player.first, message);
        put_player_into_message(player.second, message);
    }
    return message;
}

static void put_position_into_message(const Position &position, List<uint8_t> &message) {
    put_uint_into_message<uint16_t>(position.x, message);
    put_uint_into_message<uint16_t>(position.y, message);
}

static void spawn_bomb(ServerData &data, const Position &position,
                       uint16_t timer, List<uint8_t> &message) {
    data.bombs[data.next_bomb_id] = Bomb(position, timer);
    put_uint_into_message<uint8_t>(0, message);
    put_uint_into_message<BombId>(data.next_bomb_id, message);
    data.next_bomb_id++;
    put_position_into_message(position, message);
}

static void put_bomb_exploded_into_message(ServerData &data, BombId id, List<uint8_t> &message) {
    put_uint_into_message<uint8_t>(1, message);
    put_uint_into_message<BombId>(id, message);

    put_uint_into_message<uint32_t>((uint32_t) data.robots_destroyed.size(), message);
    for (const auto &player_id : data.robots_destroyed) {
        put_uint_into_message<PlayerId>(player_id, message);
    }

    put_uint_into_message<uint32_t>((uint32_t) data.blocks_destroyed.size(), message);
    for (const auto &block_position : data.blocks_destroyed) {
        put_position_into_message(block_position, message);
    }

    data.all_robots_destroyed.insert(data.robots_destroyed.begin(), data.robots_destroyed.end());
    data.all_blocks_destroyed.insert(data.blocks_destroyed.begin(), data.blocks_destroyed.end());
}

static void spawn_player(ServerData &data, PlayerId id, const Position &position, List<uint8_t> &message) {
    data.player_positions[id] = position;
    put_uint_into_message<uint8_t>(2, message);
    put_uint_into_message<PlayerId>(id, message);
    put_position_into_message(position, message);
}

static void spawn_block(ServerData &data, const Position &position, List<uint8_t> &message) {
    data.blocks.emplace(position);
    put_uint_into_message<uint8_t>(3, message);
    put_position_into_message(position, message);
}

static Position get_random_position(const ServerParameters &parameters, ServerData &data) {
    return Position((uint16_t) (data.random() % parameters.size_x),
                    (uint16_t) (data.random() % parameters.size_y));
}

List<uint8_t> build_turn_0(const ServerParameters &parameters, ServerData &data) {
    List<uint8_t> message(1, 3);
    put_uint_into_message<uint16_t>(0, message);
    put_uint_into_message<uint32_t>((uint32_t) parameters.players_count +
                                    (uint32_t) parameters.initial_blocks, message);

    // Place players.
    for (PlayerId id = 0; id < parameters.players_count; id++) {
        spawn_player(data, id, get_random_position(parameters, data), message);
    }

    // Place blocks.
    for (uint16_t i = 0; i < parameters.initial_blocks; i++) {
        spawn_block(data, get_random_position(parameters, data), message);
    }

    return message;
}

static void findDestroyedRobots(ServerData &data, const Position &explosion_position) {
    for (const auto &player : data.player_positions) {
        if (player.second == explosion_position) {
            data.robots_destroyed.emplace(player.first);
        }
    }
}

static void findDestroyed(const ServerParameters &parameters, ServerData &data, BombId id) {
    data.robots_destroyed.clear();
    data.blocks_destroyed.clear();
    const Bomb &bomb = data.bombs[id];

    // Explosions above the bomb.
    Position position = bomb.position;
    for (uint16_t i = 0; i <= parameters.explosion_radius; i++) {
        findDestroyedRobots(data, position);
        if (data.blocks.contains(position)) {
            data.blocks_destroyed.emplace(position);
            break;
        }
        if (position.y == parameters.size_y - 1) {
            break;
        }
        position.y++;
    }

    // Explosions right to the bomb.
    position = bomb.position;
    for (uint16_t i = 0; i <= parameters.explosion_radius; i++) {
        findDestroyedRobots(data, position);
        if (data.blocks.contains(position)) {
            data.blocks_destroyed.emplace(position);
            break;
        }
        if (position.x == parameters.size_x - 1) {
            break;
        }
        position.x++;
    }

    // Explosions under the bomb.
    position = bomb.position;
    for (uint16_t i = 0; i <= parameters.explosion_radius; i++) {
        findDestroyedRobots(data, position);
        if (data.blocks.contains(position)) {
            data.blocks_destroyed.emplace(position);
            break;
        }
        if (position.y == 0) {
            break;
        }
        position.y--;
    }

    // Explosions left to the bomb.
    position = bomb.position;
    for (uint16_t i = 0; i <= parameters.explosion_radius; i++) {
        findDestroyedRobots(data, position);
        if (data.blocks.contains(position)) {
            data.blocks_destroyed.emplace(position);
            break;
        }
        if (position.x == 0) {
            break;
        }
        position.x--;
    }
}

static void clear_exploded_bombs(ServerData &data) {
    bool finish = true;
    for (auto it = data.bombs.begin(); it != data.bombs.end(); it++) {
        if (it->second.timer == 0) {
            data.bombs.erase(it);
            finish = false;
            break;
        }
    }

    if (!finish) {
        clear_exploded_bombs(data);
    }
}

static bool try_moving_player(const ServerParameters &parameters, ServerData &data,
                              PlayerId id, uint8_t direction, List<uint8_t> &message) {
    Position new_position = data.player_positions[id];
    if (direction == 0) {
        if (new_position.y == parameters.size_y - 1) {
            return false;
        }
        new_position.y++;
    }
    else if (direction == 1) {
        if (new_position.x == parameters.size_x - 1) {
            return false;
        }
        new_position.x++;
    }
    else if (direction == 2) {
        if (new_position.y == 0) {
            return false;
        }
        new_position.y--;
    }
    else {
        if (new_position.x == 0) {
            return false;
        }
        new_position.x--;
    }

    if (data.blocks.contains(new_position)) {
        return false;
    }

    spawn_player(data, id, new_position, message);
    return true;
}

List<uint8_t> build_turn(const ServerParameters &parameters, ServerData &data) {
    List<uint8_t> message(1, 3);
    List<uint8_t> events_message;
    data.turn++;
    put_uint_into_message<uint16_t>(data.turn, message);
    uint32_t events = 0;
    data.time_to_next_round = 0;
    data.all_robots_destroyed.clear();
    data.all_blocks_destroyed.clear();

    for (auto &bomb : data.bombs) {
        bomb.second.timer--;
        if (bomb.second.timer == 0) {
            findDestroyed(parameters, data, bomb.first);
            put_bomb_exploded_into_message(data, bomb.first, events_message);
            events++;
        }
    }

    clear_exploded_bombs(data);

    Set<Position> survived_blocks;
    std::set_difference(data.blocks.begin(), data.blocks.end(),
                        data.all_blocks_destroyed.begin(), data.all_blocks_destroyed.end(),
                        std::inserter(survived_blocks, survived_blocks.end()));
    data.blocks = survived_blocks;

    for (PlayerId id = 0; id < parameters.players_count; id++) {
        if (data.all_robots_destroyed.contains(id)) {
            spawn_player(data, id, get_random_position(parameters, data), events_message);
            data.scores[id]++;
            events++;
        }
        else if (data.disconnected_players.contains(id)) {
            continue;
        }
        else if (data.clients_last_messages[data.players[id].poll_id] == PLACE_BOMB) {
            spawn_bomb(data, data.player_positions[id], parameters.bomb_timer, events_message);
            events++;
        }
        else if (data.clients_last_messages[data.players[id].poll_id] == PLACE_BLOCK
            && !data.blocks.contains(data.player_positions[id])) {
            spawn_block(data, data.player_positions[id], events_message);
            events++;
        }
        else if (data.clients_last_messages[data.players[id].poll_id] != NO_MSG) {
            uint8_t direction = data.clients_last_messages[data.players[id].poll_id] - MOVE;
            if (try_moving_player(parameters, data, id, direction, events_message)) {
                events++;
            }
        }
    }

    put_uint_into_message<uint32_t>(events, message);
    message.insert(message.end(), events_message.begin(), events_message.end());
    return message;
}

List<uint8_t> build_game_ended(const ServerData &data) {
    List<uint8_t> message(1, 4);
    put_uint_into_message<uint32_t>((uint32_t) data.scores.size(), message);
    for (const auto &score : data.scores) {
        put_uint_into_message<PlayerId>(score.first, message);
        put_uint_into_message<Score>(score.second, message);
    }
    return message;
}

bool client_sent_join(const Deque<uint8_t> &buffer) {
    return buffer.size() > 2 && buffer[0] == 0 &&
           buffer[1] > 0 && buffer.size() > (size_t) buffer[1] + 1;
}

bool client_sent_place_bomb(const Deque<uint8_t> &buffer) {
    return buffer.size() > 0 && buffer[0] == 1;
}

bool client_sent_place_block(const Deque<uint8_t> &buffer) {
    return buffer.size() > 0 && buffer[0] == 2;
}

bool client_sent_move(const Deque<uint8_t> &buffer) {
    return buffer.size() > 1 && buffer[0] == 3 && buffer[1] < 4;
}

bool client_sent_incorrect_message(const Deque<uint8_t> &buffer) {
    return (buffer.size() > 0 && buffer[0] > 3) ||
           (buffer.size() > 1 && buffer[0] == 3 && buffer[1] > 3) ||
           (buffer.size() > 1 && buffer[0] == 0 && buffer[1] == 0);
}

std::string read_join(Deque<uint8_t> &buffer) {
    buffer.pop_front();
    size_t name_length = buffer.front();
    buffer.pop_front();

    std::string result;
    for (size_t i = 0; i < name_length; i++) {
        result += (char) buffer.front();
        buffer.pop_front();
    }
    return result;
}

void read_place_bomb(Deque<uint8_t> &buffer) {
    buffer.pop_front();
}

void read_place_block(Deque<uint8_t> &buffer) {
    buffer.pop_front();
}

uint8_t read_move(Deque<uint8_t> &buffer) {
    char result = (char) buffer[1];
    buffer.pop_front();
    buffer.pop_front();
    return (uint8_t) result;
}
