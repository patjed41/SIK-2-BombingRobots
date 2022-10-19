cmake_minimum_required(VERSION 3.0)
project(Robots)

set(CMAKE_CXX_FLAGS "-O2 -std=c++17")

# in case of compiling with g++11.2 on students
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,/opt/gcc-11.2/lib64")

set(CLIENT_SOURCE_FILES
    client/main.cpp
    client/client-parameters/client_parameters.cpp
    client/net/net.cpp
    client/client-data/client-data.cpp
    client/messages/messages.cpp
)

add_executable(robots-client ${CLIENT_SOURCE_FILES})
target_link_libraries(robots-client pthread)

set(SERVER_SOURCE_FILES
    server/main.cpp
    server/server-data/server_data.cpp
    server/server-parameters/server_parameters.cpp
    server/messages/messages.cpp
    server/net/net.cpp
    server/server-engine/server_engine.cpp
)

add_executable(robots-server ${SERVER_SOURCE_FILES})

