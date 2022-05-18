#ifndef ROBOTSCLIENT_MESSAGES_H
#define ROBOTSCLIENT_MESSAGES_H

#include "../client-data/client_data.h"

void read_hello(ClientData &data);

// returns message_type
uint8_t read_message_from_gui(ClientData &data);

void send_message_to_server(ClientData &data, uint8_t message_type);

#endif //ROBOTSCLIENT_MESSAGES_H
