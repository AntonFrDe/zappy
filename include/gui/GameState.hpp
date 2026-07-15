#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

constexpr int RESOURCE_COUNT = 7;

enum class Orientation {
    North = 0,
    East = 1,
    South = 2,
    West = 3,
};

struct Tile {
    std::array<int, RESOURCE_COUNT> resources{};
};

struct Player {
    int id = -1;
    int x = -1;
    int y = -1;
    Orientation orientation = Orientation::North;
    int level = 1;
    std::string team;
    std::array<int, RESOURCE_COUNT> inventory{};
    bool alive = false;
};

struct Team {
    std::string name;
    std::vector<int> playerIds;
    std::vector<int> eggIds;
};

struct Incantation {
    int x = -1;
    int y = -1;
    int level = 0;
    bool active = false;
    std::vector<int> playerIds;
};

struct Egg {
    int id = -1;
    int playerId = -1;
    int x = -1;
    int y = -1;
    bool hatched = false;
};

struct GameState {
    int mapWidth = 0;
    int mapHeight = 0;
    int timeUnit = 0;
    bool gameOver = false;
    std::string winningTeam;
    std::vector<Tile> tiles;
    std::unordered_map<int, Player> players;
    std::unordered_map<std::string, Team> teams;
    std::vector<Incantation> incantations;
    std::unordered_map<int, Egg> eggs;

    void resizeMap(int width, int height)
    {
        mapWidth = width;
        mapHeight = height;
        tiles.assign(static_cast<std::size_t>(width * height), Tile{});
    }

    Tile &tileAt(int x, int y)
    {
        return tiles[static_cast<std::size_t>(y * mapWidth + x)];
    }

    const Tile &tileAt(int x, int y) const
    {
        return tiles[static_cast<std::size_t>(y * mapWidth + x)];
    }

    Player *findPlayer(int id)
    {
        auto it = players.find(id);
        return it == players.end() ? nullptr : &it->second;
    }

    const Player *findPlayer(int id) const
    {
        auto it = players.find(id);
        return it == players.end() ? nullptr : &it->second;
    }

    Egg *findEgg(int id)
    {
        auto it = eggs.find(id);
        return it == eggs.end() ? nullptr : &it->second;
    }

    const Egg *findEgg(int id) const
    {
        auto it = eggs.find(id);
        return it == eggs.end() ? nullptr : &it->second;
    }

    Incantation *findIncantationAt(int x, int y)
    {
        for (auto &incantation : incantations) {
            if (incantation.x == x && incantation.y == y)
                return &incantation;
        }
        return nullptr;
    }

    const Incantation *findIncantationAt(int x, int y) const
    {
        for (const auto &incantation : incantations) {
            if (incantation.x == x && incantation.y == y)
                return &incantation;
        }
        return nullptr;
    }
};
