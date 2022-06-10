#include "client_data.h"
#include "../../common/err.h"

void ClientData::init() {
    ENSURE(pthread_mutex_init(&lock, nullptr) == 0);
    is_in_lobby = true;
}

void ClientData::clear() {
    ENSURE(pthread_mutex_lock(&lock) == 0);
    is_in_lobby = true;
    ENSURE(pthread_mutex_unlock(&lock) == 0);

    players.clear();
    player_positions.clear();
    blocks.clear();
    bombs.clear();
    explosions.clear();
    scores.clear();
    died_this_round.clear();
}
