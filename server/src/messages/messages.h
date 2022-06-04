#ifndef MESSAGES_H
#define MESSAGES_H

#include "../server-data/server_data.h"
#include "../server-parameters/server_parameters.h"

#define NO_FLAGS 0

// Sends Hello message to client [client_fd].
void send_hello(int client_fd, const ServerParameters &parameters);

// Builds AcceptedPlayer message and returns it. Client with poll id [poll_id]
// is the accepted player.
List<uint8_t> build_accepted_player(const ServerData &data, size_t poll_id);

// Builds GameStarted message and returns it.
List<uint8_t> build_game_started(const ServerData &data);

// Returns true if client sent correct and complete Join message.
bool client_sent_join(const Deque<uint8_t> &buffer);

// Returns true if client sent correct and complete PlaceBomb message.
bool client_sent_place_bomb(const Deque<uint8_t> &buffer);

// Returns true if client sent correct and complete PlaceBlock message.
bool client_sent_place_block(const Deque<uint8_t> &buffer);

// Returns true if client sent correct and complete Move message.
bool client_sent_move(const Deque<uint8_t> &buffer);

// Returns true if client sent incorrect message.
bool client_sent_incorrect_message(const Deque<uint8_t> &buffer);

// Reads Join message from [buffer]. Returns client's name.
std::string read_join(Deque<uint8_t> &buffer);

// Reads PlaceBomb message from [buffer].
void read_place_bomb(Deque<uint8_t> &buffer);

// Reads PlaceBlock message from [buffer].
void read_place_block(Deque<uint8_t> &buffer);

// Reads Move message from [buffer]. Returns direction.
uint8_t read_move(Deque<uint8_t> &buffer);

#endif // MESSAGES_H
