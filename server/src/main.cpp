// author - Patryk Jędrzejczak

#include <iostream>

#include "err.h"
#include "server-parameters/server_parameters.h"

int main(int argc, char *argv[]) {
    ServerParameters parameters = read_parameters(argc, argv);
}

