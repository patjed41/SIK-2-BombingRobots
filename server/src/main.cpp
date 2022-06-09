// author - Patryk Jędrzejczak

#include <iostream>
#include <unistd.h>

#include "err.h"
#include "messages/messages.h"
#include "net/net.h"

// Disconnects client with poll id [poll_id].
static void disconnect_client(ServerData &data, size_t poll_id) {
    close_socket(data.poll_descriptors[poll_id].fd);
    data.poll_descriptors[poll_id].fd = -1;
    data.clients_buffers[poll_id].clear();
    data.clients_last_messages[poll_id] = 0;
    data.active_clients--;

    for (auto & player : data.players) {
        if (player.second.poll_id == poll_id) {
            data.disconnected_players.emplace(player.first);
        }
    }
}

// Disconnects client with poll id [poll_id] if [send_succeeded] = false.
static void disconnect_if_not(bool send_succeeded, ServerData &data, size_t poll_id) {
    if (!send_succeeded) {
        disconnect_client(data, poll_id);
    }
}

// Accepts new client and sends to him starting messages.
static void accept_new_client(const ServerParameters &parameters, ServerData &data) {
    sockaddr_in6 client_address;
    int client_fd = accept_connection(data.poll_descriptors[0].fd, &client_address);
    turn_off_nagle(client_fd);

    size_t poll_id;
    for (poll_id = 1; poll_id <= MAX_CLIENTS; poll_id++) {
        if (data.poll_descriptors[poll_id].fd == -1) {
            data.poll_descriptors[poll_id].fd = client_fd;
            data.clients_addresses[poll_id] = get_address(client_address);
            data.active_clients++;
            break;
        }
    }

    bool sends_succeeded = true;
    sends_succeeded &= send_message(client_fd, build_hello(parameters), NO_FLAGS);
    if (data.in_lobby) {
        sends_succeeded &= send_message(client_fd, data.all_accepted_player_messages, NO_FLAGS);
    }
    else {
        sends_succeeded &= send_message(client_fd, build_game_started(data), NO_FLAGS);
        sends_succeeded &= send_message(client_fd, data.all_turn_messages, NO_FLAGS);
    }
    disconnect_if_not(sends_succeeded, data, poll_id);
}

// Checks if client with poll id [poll_id] is already a player.
static bool is_player(ServerData &data, size_t poll_id) {
    for (auto & player : data.players) {
        if (player.second.poll_id == poll_id) {
            return true;
        }
    }

    return false;
}

// Makes client with poll id [poll_id] a player with name [name].
static void create_new_player(ServerData &data, size_t poll_id, const std::string &name) {
    Player new_player;
    new_player.name = name;
    new_player.address = data.clients_addresses[poll_id];
    new_player.poll_id = poll_id;
    data.players[(uint8_t) data.players.size()] = new_player;
}

// Sends [message] to all clients.
static void send_message_to_all(ServerData &data, const List<uint8_t> &message) {
    for (int i = 1; i <= MAX_CLIENTS; ++i) {
        if (data.poll_descriptors[i].fd != -1) {
            disconnect_if_not(send_message(data.poll_descriptors[i].fd, message, NO_FLAGS), data, i);
        }
    }
}

// Sends AcceptedPlayer message to all clients. Client with poll id [poll_id]
// is the accepted player.
static void send_accepted_player_to_all(ServerData &data, size_t poll_id) {
    List<uint8_t> message = build_accepted_player(data, poll_id);
    data.all_accepted_player_messages.insert(data.all_accepted_player_messages.end(),
                                             message.begin(), message.end());
    send_message_to_all(data, message);
}

// Sends GameStarted message to all clients.
static void send_game_started_to_all(ServerData &data) {
    List<uint8_t> message = build_game_started(data);
    send_message_to_all(data, message);
}

// Sends Turn message with turn = 0 to all clients.
static void send_turn_0_to_all(const ServerParameters &parameters, ServerData &data) {
    List<uint8_t> message = build_turn_0(parameters, data);
    data.all_turn_messages.insert(data.all_turn_messages.end(), message.begin(), message.end());
    send_message_to_all(data, message);
}

// Sends Turn message to all clients.
static void send_turn_to_all(const ServerParameters &parameters, ServerData &data) {
    List<uint8_t> message = build_turn(parameters, data);
    data.all_turn_messages.insert(data.all_turn_messages.end(), message.begin(), message.end());
    send_message_to_all(data, message);
}

// Sends GameEnded message to all clients.
static void send_game_ended_to_all(ServerData &data) {
    List<uint8_t> message = build_game_ended(data);
    send_message_to_all(data, message);
}

// Reads complete messages stored in [data.clients_buffers[poll_id]].
static void clear_clients_buffer(const ServerParameters &parameters, ServerData &data, size_t poll_id) {
    bool finished = false;
    while (!finished) {
        if (client_sent_join(data.clients_buffers[poll_id])) {
            std::string name = read_join(data.clients_buffers[poll_id]);
            if (data.in_lobby && data.players.size() < parameters.players_count && !is_player(data, poll_id)) {
                create_new_player(data, poll_id, name);
                send_accepted_player_to_all(data, poll_id);
            }
        }
        else if (client_sent_place_bomb(data.clients_buffers[poll_id])) {
            read_place_bomb(data.clients_buffers[poll_id]);
            if (!data.in_lobby) {
                data.clients_last_messages[poll_id] = PLACE_BOMB;
            }
        }
        else if (client_sent_place_block(data.clients_buffers[poll_id])) {
            read_place_block(data.clients_buffers[poll_id]);
            if (!data.in_lobby) {
                data.clients_last_messages[poll_id] = PLACE_BLOCK;
            }
        }
        else if (client_sent_move(data.clients_buffers[poll_id])) {
            uint8_t direction = read_move(data.clients_buffers[poll_id]);
            if (!data.in_lobby) {
                data.clients_last_messages[poll_id] = MOVE + direction;
            }
        }
        else if (client_sent_incorrect_message(data.clients_buffers[poll_id])) {
            disconnect_client(data, poll_id);
            finished = true;
        }
        else {
            finished = true;
        }
    }
}

// Reads from client [data.poll_descriptors][poll_id].
static void read_from_client(const ServerParameters &parameters, ServerData &data, size_t poll_id) {
    static uint8_t buffer[PACKET_LIMIT];

    ssize_t read_bytes = read(data.poll_descriptors[poll_id].fd, buffer, PACKET_LIMIT);
    if (read_bytes <= 0) {
        disconnect_client(data, poll_id);
    }
    else {
        for (ssize_t i = 0; i < read_bytes; i++) {
            data.clients_buffers[poll_id].push_back(buffer[i]);
        }

        clear_clients_buffer(parameters, data, poll_id);
    }
}

static double time_dif_in_millis(const timeval &t1, const timeval &t2) {
    double seconds_dif = (float) (t2.tv_sec - t1.tv_sec) * 1000.f;
    double millis_dif = (float) (t2.tv_usec - t1.tv_usec) / 1000.f;
    return seconds_dif + millis_dif;
}

static void update_time_during_turn(ServerData &data) {
    timeval current_time;
    gettimeofday(&current_time, nullptr);
    data.time_to_next_round += time_dif_in_millis(data.last_time, current_time);
    data.last_time = current_time;
}

// Runs server.
[[noreturn]] static void run(const ServerParameters &parameters, ServerData &data) {
    while (true) {
        data.clear_poll_descriptors();

        int poll_status = poll(data.poll_descriptors, MAX_CLIENTS + 1, 0);
        if (poll_status == -1) {
            continue;
        }
        else if (poll_status > 0) {
            if (data.poll_descriptors[0].revents & POLLIN && data.active_clients < MAX_CLIENTS) {
                accept_new_client(parameters, data);
            }

            for (int i = 1; i <= MAX_CLIENTS; i++) {
                if (data.poll_descriptors[i].fd != -1 &&
                    (data.poll_descriptors[i].revents & (POLLIN | POLLERR))) {
                    read_from_client(parameters, data, i);
                }
            }
        }

        if (data.in_lobby && data.players.size() == parameters.players_count) {
            send_game_started_to_all(data);
            send_turn_0_to_all(parameters, data);
            data.set_up_new_game();
            data.clear_client_last_messages();
        }
        else if (!data.in_lobby) {
            update_time_during_turn(data);
            if ((uint64_t) data.time_to_next_round >= parameters.turn_duration) {
                send_turn_to_all(parameters, data);
                if (data.turn == parameters.game_length) {
                    send_game_ended_to_all(data);
                    data.clear_state();
                }
                data.clear_client_last_messages();
            }
        }
    }
}

int main(int argc, char *argv[]) {
    ServerParameters parameters = read_parameters(argc, argv);

    ServerData data(parameters.seed, parameters.players_count);
    data.poll_descriptors[0].fd = bind_tcp_socket(parameters.port);
    turn_off_nagle(data.poll_descriptors[0].fd);
    start_listening(data.poll_descriptors[0].fd, QUEUE_LENGTH);

    run(parameters, data);
}

