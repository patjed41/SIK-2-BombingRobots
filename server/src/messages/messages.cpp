#include "messages.h"
#include "../net/net.h"
#include "../err.h"

#include <arpa/inet.h>

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

void send_hello(int client_fd, const ServerParameters &parameters) {
    List<uint8_t> message(1, 0);
    put_string_into_message(parameters.server_name, message);
    put_uint_into_message<uint8_t>(parameters.players_count, message);
    put_uint_into_message<uint16_t>(parameters.size_x, message);
    put_uint_into_message<uint16_t>(parameters.size_y, message);
    put_uint_into_message<uint16_t>(parameters.game_length, message);
    put_uint_into_message<uint16_t>(parameters.explosion_radius, message);
    put_uint_into_message<uint16_t>(parameters.bomb_timer, message);

    send_message(client_fd, message, NO_FLAGS);
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
    return buffer.size() > 1 && buffer[0] == 1 && buffer[1] < 4;
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
