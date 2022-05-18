cmake_minimum_required(VERSION 3.0)
project(RobotsClient)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion -Werror -O2 -std=gnu++20")

set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)
set(CMAKE_VERBOSE_MAKEFILE OFF)

set(SOURCE_FILES
    src/main.cpp
    src/client-parameters/client_parameters.cpp
    src/net/net.cpp
    src/client-data/client-data.cpp
    src/messages/messages.cpp
)

add_executable(robots-client ${SOURCE_FILES})
target_link_libraries(robots-client pthread)
