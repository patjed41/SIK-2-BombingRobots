#ifndef ROBOTSCLIENT_TYPES_H
#define ROBOTSCLIENT_TYPES_H

#include <string>
#include <cstdint>
#include <map>
#include <vector>

template<class T> using List = std::vector<T>;
template<class K, class V> using Map = std::map<K, V>;

using PlayerId = uint8_t;

struct Player {
    std::string name;
    std::string address;
};

struct Position {
    uint16_t x;
    uint16_t y;
};

using BombId = uint32_t;

struct Bomb {
    Position position;
    uint16_t timer;
};

using Score = uint32_t;

#endif //ROBOTSCLIENT_TYPES_H
