cmake_minimum_required(VERSION 3.0)
project(RobotsServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion -Werror -O2 -std=gnu++20")

# in case of compiling with g++11.2 on students
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,/opt/gcc-11.2/lib64")

set(SOURCE_FILES
    src/main.cpp
    src/server-data/server_data.cpp
    src/server-parameters/server_parameters.cpp
    src/messages/messages.cpp
    src/net/net.cpp
    )

add_executable(robots-server ${SOURCE_FILES})
