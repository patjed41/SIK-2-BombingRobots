cmake_minimum_required(VERSION 3.0)
project(RobotsServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion -Werror -O2 -std=gnu++20")

set(SOURCE_FILES
    src/main.cpp
    src/server-parameters/server_parameters.cpp
    )

add_executable(robots-server ${SOURCE_FILES})
