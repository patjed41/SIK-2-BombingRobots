#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <cstdint>
#include <map>
#include <vector>
#include <set>
#include <deque>

// Definitions of some types used by server.

template<class K, class V> using Map = std::map<K, V>;
template<class T> using Set = std::set<T>;
template<class T> using List = std::vector<T>;
template<class T> using Deque = std::deque<T>;

using PlayerId = uint8_t;

struct Player {
    std::string name;
    std::string address;

    Player() = default;

    Player(std::string name, std::string address) : name(name), address(address) {}
};

struct Position {
    uint16_t x;
    uint16_t y;

    Position() = default;

    Position(uint16_t x, uint16_t y) : x(x), y(y) {}
};

inline bool operator==(const Position &p1, const Position &p2) {
    return p1.x == p2.x && p1.y == p2.y;
}

inline bool operator<(const Position &p1, const Position &p2) {
    if (p1.x != p2.x) {
        return p1.x < p2.x;
    }
    return p1.y < p2.y;
}

using BombId = uint32_t;

struct Bomb {
    Position position;
    uint16_t timer;

    Bomb() = default;

    Bomb(Position position, uint16_t timer) : position(position), timer(timer) {}
};

using Score = uint32_t;

#endif // TYPES_H
