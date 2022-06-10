#ifndef MESSAGES_H
#define MESSAGES_H

#include "../server-data/server_data.h"
#include "../server-parameters/server_parameters.h"

#define NO_FLAGS 0

/******************************** TO CLIENTS **********************************/

// Builds Hello message and returns it.
List<uint8_t> build_hello(const ServerParameters &parameters);

// Builds AcceptedPlayer message and returns it. Client with poll id [poll_id]
// is the accepted player.
List<uint8_t> build_accepted_player(const ServerData &data, size_t poll_id);

// Builds GameStarted message and returns it.
List<uint8_t> build_game_started(const ServerData &data);

// Builds Turn message with turn equal to 0. Changes [data] by spawning players
// and placing blocks.
List<uint8_t> build_turn_0(const ServerParameters &parameters, ServerData &data);

// Builds Turn message. Updates [data].
List<uint8_t> build_turn(const ServerParameters &parameters, ServerData &data);

// Builds GameEnded message and returns it.
List<uint8_t> build_game_ended(const ServerData &data);

/******************************* FROM CLIENTS *********************************/

// Returns true if client sent correct and complete Join message.
bool client_sent_join(const Deque<uint8_t> &buffer);

// Returns true if client sent correct and complete PlaceBomb message.
bool client_sent_place_bomb(const Deque<uint8_t> &buffer);

// Returns true if client sent correct and complete PlaceBlock message.
bool client_sent_place_block(const Deque<uint8_t> &buffer);

// Returns true if client sent correct and complete Move message.
bool client_sent_move(const Deque<uint8_t> &buffer);

// Returns true if client sent incorrect message. If client sent message that
// is incomplete but may be correct, false is returned.
bool client_sent_incorrect_message(const Deque<uint8_t> &buffer);

// Reads Join message from [buffer]. Returns client's name. Message should be
// correct.
std::string read_join(Deque<uint8_t> &buffer);

// Reads PlaceBomb message from [buffer]. Message should be correct.
void read_place_bomb(Deque<uint8_t> &buffer);

// Reads PlaceBlock message from [buffer]. Message should be correct.
void read_place_block(Deque<uint8_t> &buffer);

// Reads Move message from [buffer]. Returns direction. Message should be
// correct.
uint8_t read_move(Deque<uint8_t> &buffer);

#endif // MESSAGES_H
