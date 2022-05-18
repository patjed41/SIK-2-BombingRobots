#include "client_data.h"
#include "../err.h"


void init(ClientData* data) {
    ENSURE(pthread_mutex_init(&data->lock, 0) == 0);
    data->is_in_lobby = true;
}

void clear(ClientData* data) {
    data->is_in_lobby = true;
    data->players.clear();
    data->player_positions.clear();
    data->blocks.clear();
    data->bombs.clear();
    data->explosions.clear();
    data->scores.clear();
    data->died_this_round.clear();
}

void destroy(ClientData* data) {
    ENSURE(pthread_mutex_destroy(&data->lock) == 0);
}
