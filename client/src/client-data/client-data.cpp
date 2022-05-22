#include "client_data.h"
#include "../err.h"

void ClientData::init() {
    ENSURE(pthread_mutex_init(&lock, 0) == 0);
    is_in_lobby = true;
}

void ClientData::clear() {
    is_in_lobby = true;
    players.clear();
    player_positions.clear();
    blocks.clear();
    bombs.clear();
    explosions.clear();
    scores.clear();
    died_this_round.clear();
}
