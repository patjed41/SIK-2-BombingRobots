#ifndef MESSAGES_H
#define MESSAGES_H

#include "../client-data/client_data.h"

void read_hello(ClientData &data);

// returns message_type
uint8_t read_message_from_gui(ClientData &data);

void send_message_to_server(ClientData &data, uint8_t message_type);

void read_message_from_server(ClientData &data);

void send_message_to_gui(ClientData &data);

#endif // MESSAGES_H
