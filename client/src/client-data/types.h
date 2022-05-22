#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <cstdint>
#include <map>
#include <vector>

// Definitions of some types used by client.

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

inline bool operator==(const Position &p1, const Position &p2) {
    return p1.x == p2.x && p1.y == p2.y;
}

using BombId = uint32_t;

struct Bomb {
    Position position;
    uint16_t timer;
};

using Score = uint32_t;

#endif // TYPES_H
