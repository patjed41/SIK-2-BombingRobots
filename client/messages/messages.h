#ifndef MESSAGES_H
#define MESSAGES_H

#include "../client-data/client_data.h"

// Reads Hello from server. Updates [data].
void read_hello(ClientData &data);

// Reads message from GUI. Returns message type that is equal to message id
// if message from gui is different from Move. Returns 2 + direction if
// message is Move. If message is incorrect, returns GUI_WRONG_MSG.
uint8_t read_message_from_gui(const ClientData &data);

// Sends message of type [message_type] to server. If message type is correct
// and client is in lobby, sends Join.
void send_message_to_server(ClientData &data, uint8_t message_type);

// Reads message from server. Updates [data]. Returns true if client should
// immediately send message to GUI.
bool read_message_from_server(ClientData &data);

// Sends message to GUI with data in [data].
void send_message_to_gui(ClientData &data);

#endif // MESSAGES_H
