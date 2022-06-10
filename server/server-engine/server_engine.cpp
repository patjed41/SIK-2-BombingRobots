#include "server_engine.h"
#include "../net/net.h"

#include <unistd.h>

// Sets up server's listening socket.
static void set_up_listener(ServerData &data, uint16_t port) {
    data.poll_descriptors[0].fd = bind_tcp_socket(port);
    turn_off_nagle(data.poll_descriptors[0].fd);
    start_listening(data.poll_descriptors[0].fd, QUEUE_LENGTH);
}

// Disconnects client with poll id [poll_id]. Client has poll id equal to x
// when socket [data.poll_descriptors[x].fd] is responsible for communication
// with him.
static void disconnect_client(ServerData &data, size_t poll_id) {
    close(data.poll_descriptors[poll_id].fd);
    data.poll_descriptors[poll_id].fd = -1;
    data.clients_buffers[poll_id].clear();
    data.clients_last_messages[poll_id] = NO_MSG;
    data.active_clients--;

    for (const auto & player_id : data.poll_ids) {
        if (player_id.second == poll_id) {
            data.disconnected_players.emplace(player_id.first);
        }
    }
}

// Disconnects client with poll id [poll_id] if [send_succeeded] = false.
static void disconnect_if_not(bool send_succeeded, ServerData &data, size_t poll_id) {
    if (!send_succeeded) {
        disconnect_client(data, poll_id);
    }
}

// Sends starting messages to new client with poll id [poll_id].
static void welcome(const ServerParameters &parameters, ServerData &data, size_t poll_id) {
    bool sends_succeeded = true;
    sends_succeeded &= send_message(data.poll_descriptors[poll_id].fd,
                                    build_hello(parameters), NO_FLAGS);
    if (data.in_lobby) {
        sends_succeeded &= send_message(data.poll_descriptors[poll_id].fd,
                                        data.all_accepted_player_messages, NO_FLAGS);
    }
    else {
        sends_succeeded &= send_message(data.poll_descriptors[poll_id].fd,
                                        build_game_started(data), NO_FLAGS);
        sends_succeeded &= send_message(data.poll_descriptors[poll_id].fd,
                                        data.all_turn_messages, NO_FLAGS);
    }
    disconnect_if_not(sends_succeeded, data, poll_id);
}

// Accepts new client and sends to him starting messages if there is new client
// who wants to join and accepting him is possible.
static void try_accepting_new_client(const ServerParameters &parameters, ServerData &data) {
    if (!(data.poll_descriptors[0].revents & POLLIN) || data.active_clients >= MAX_CLIENTS) {
        return;
    }

    sockaddr_in6 client_address;
    int client_fd = accept_connection(data.poll_descriptors[0].fd, &client_address);
    if (client_fd == -1) {
        return;
    }

    if (!turn_off_nagle(client_fd)) {
        close(client_fd);
        return;
    }

    size_t poll_id;
    for (poll_id = 1; poll_id <= MAX_CLIENTS; poll_id++) {
        if (data.poll_descriptors[poll_id].fd == -1) {
            std::string address_str = get_address(client_address);
            if (address_str == "fail") {
                close(client_fd);
                return;
            }
            data.clients_addresses[poll_id] = address_str;
            data.poll_descriptors[poll_id].fd = client_fd;
            data.active_clients++;
            welcome(parameters, data, poll_id);
            return;
        }
    }
}

// Checks if client with poll id [poll_id] is already a player.
static bool is_player(ServerData &data, size_t poll_id) {
    for (const auto & player_id : data.poll_ids) {
        if (player_id.second == poll_id) {
            return true;
        }
    }

    return false;
}

// Makes client with poll id [poll_id] a player with name [name].
static void create_new_player(ServerData &data, size_t poll_id, const std::string &name) {
    data.poll_ids[(PlayerId) data.players.size()] = poll_id;
    Player new_player(name, data.clients_addresses[poll_id]);
    data.players[(PlayerId) data.players.size()] = new_player;
}

// Sends [message] to all clients.
static void send_message_to_all(ServerData &data, const List<uint8_t> &message) {
    for (int i = 1; i <= MAX_CLIENTS; i++) {
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

// Sends Turn message with turn != 0 to all clients.
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

// Processes Join message read from client with poll id [poll_id].
static void process_join_from_client(ServerData &data, uint8_t players_count, size_t poll_id) {
    std::string name = read_join(data.clients_buffers[poll_id]);
    if (data.in_lobby && data.players.size() < players_count && !is_player(data, poll_id)) {
        create_new_player(data, poll_id, name);
        send_accepted_player_to_all(data, poll_id);
    }
}

// Processes PlaceBomb message read from client with poll id [poll_id].
static void process_place_bomb_from_client(ServerData &data, size_t poll_id) {
    read_place_bomb(data.clients_buffers[poll_id]);
    if (!data.in_lobby) {
        data.clients_last_messages[poll_id] = PLACE_BOMB;
    }
}

// Processes PlaceBlock message read from client with poll id [poll_id].
static void process_place_block_from_client(ServerData &data, size_t poll_id) {
    read_place_block(data.clients_buffers[poll_id]);
    if (!data.in_lobby) {
        data.clients_last_messages[poll_id] = PLACE_BLOCK;
    }
}

// Processes Move message read from client with poll id [poll_id].
static void process_move_from_client(ServerData &data, size_t poll_id) {
    uint8_t direction = read_move(data.clients_buffers[poll_id]);
    if (!data.in_lobby) {
        data.clients_last_messages[poll_id] = MOVE + direction;
    }
}

// Reads complete messages stored in [data.clients_buffers[poll_id]] it is all
// full messages received from all clients.
static void clear_clients_buffer(const ServerParameters &parameters, ServerData &data, size_t poll_id) {
    bool finished_clearing = false;

    while (!finished_clearing) {
        if (client_sent_join(data.clients_buffers[poll_id])) {
            process_join_from_client(data, parameters.players_count, poll_id);
        }
        else if (client_sent_place_bomb(data.clients_buffers[poll_id])) {
            process_place_bomb_from_client(data, poll_id);
        }
        else if (client_sent_place_block(data.clients_buffers[poll_id])) {
            process_place_block_from_client(data, poll_id);
        }
        else if (client_sent_move(data.clients_buffers[poll_id])) {
            process_move_from_client(data, poll_id);
        }
        else {
            finished_clearing = true;
            if (client_sent_incorrect_message(data.clients_buffers[poll_id])) {
                disconnect_client(data, poll_id);
            }
        }
    }
}

// Reads all bytes received from client with poll id [poll_id].
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

// Reads all bytes received from all clients.
static void read_from_all_clients(const ServerParameters &parameters, ServerData &data) {
    for (int i = 1; i <= MAX_CLIENTS; i++) {
        if (data.poll_descriptors[i].fd != -1
            && (data.poll_descriptors[i].revents & (POLLIN | POLLERR))) {
            read_from_client(parameters, data, i);
        }
    }
}

// Starts new game with parameters [parameters].
static void start_new_game(const ServerParameters &parameters, ServerData &data) {
    send_game_started_to_all(data);
    send_turn_0_to_all(parameters, data);
    data.set_up_new_game();
    data.clear_clients_last_messages();
}

// Processes next turn.
static void process_next_turn(const ServerParameters &parameters, ServerData &data) {
    data.next_turn();
    send_turn_to_all(parameters, data);

    if (data.turn == parameters.game_length) {
        send_game_ended_to_all(data);
        data.clear_state();
    }

    data.clear_clients_last_messages();
}

[[noreturn]] void run(const ServerParameters &parameters) {
    ServerData data(parameters.seed, parameters.players_count);

    set_up_listener(data, parameters.port);

    while (true) {
        data.clear_poll_descriptors();

        int poll_status = poll(data.poll_descriptors, MAX_CLIENTS + 1, 0);
        if (poll_status == -1) {
            continue; // Calling poll() failed, we try again.
        }
        else if (poll_status > 0) {
            try_accepting_new_client(parameters, data);
            read_from_all_clients(parameters, data);
        }

        if (data.in_lobby && data.players.size() == parameters.players_count) {
            start_new_game(parameters, data);
        }
        else if (!data.in_lobby) {
            data.update_time_during_game();
            if ((uint64_t) data.time_to_next_round >= parameters.turn_duration) {
                process_next_turn(parameters, data);
            }
        }
    }
}
