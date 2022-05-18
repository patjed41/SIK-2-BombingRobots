#include "messages.h"
#include "../net/net.h"
#include "../err.h"

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
