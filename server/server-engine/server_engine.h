#ifndef SERVER_ENGINE_H
#define SERVER_ENGINE_H

#include "../messages/messages.h"

// Runs server with command line parameters [parameters].
[[noreturn]] void run(const ServerParameters &parameters);

#endif // SERVER_ENGINE_H
