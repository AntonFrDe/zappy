#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

constexpr int RESOURCE_COUNT = 7;

// Aligne sur le protocole reseau du serveur (1=N, 2=E, 3=S, 4=W), voir
// include/server/map.h::direction_t. Ne pas repartir de 0 : le rendu
// (RenderTheme::orientationAngleDeg) attend directement ces valeurs.
enum class Orientation {
    North = 1,
    East = 2,
    South = 3,
    West = 4,
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

    // Position affichee (en tuiles, fractionnaire), interpolee par
    // Renderer vers (x, y) a chaque frame au lieu de teleporter le joueur
    // sur un ppo. CommandParser l'initialise a (x, y) a la creation du
    // joueur pour ne pas animer depuis (0, 0) a l'apparition.
    float animX = 0.f;
    float animY = 0.f;
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
    int result = -1;                 // -1 = en cours, 0 = echec, 1 = succes (connu apres pie)
    float resultElapsedSeconds = 0.f; // temps ecoule depuis pie, avance par GameState::advanceIncantationAnimations
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

    // Duree (s) pendant laquelle une incantation terminee reste dans l'etat
    // apres pie, le temps que Renderer joue l'animation de resultat.
    // Partagee avec Renderer pour normaliser la progression de l'animation.
    static constexpr float kIncantationResultHoldSeconds = 1.f;

    // Purge les incantations terminees (active=false) dont le resultat est
    // affiche depuis plus de kIncantationResultHoldSeconds. A appeler une
    // fois par frame avec le dt ecoule.
    void advanceIncantationAnimations(float dt)
    {
        for (auto it = incantations.begin(); it != incantations.end(); ) {
            if (!it->active) {
                it->resultElapsedSeconds += dt;
                if (it->resultElapsedSeconds >= kIncantationResultHoldSeconds) {
                    it = incantations.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    // Ne renvoie que l'incantation encore active sur cette case, la plus
    // recente en cas de doublon transitoire (recherche depuis la fin du
    // vector). Une incantation terminee (active=false) reste brievement
    // dans la liste le temps de son animation de resultat (voir
    // advanceIncantationAnimations), mais n'est plus jamais retrouvee ici :
    // elle ne peut donc plus faire obstacle a la suivante sur la meme case.
    Incantation *findIncantationAt(int x, int y)
    {
        for (auto it = incantations.rbegin(); it != incantations.rend(); ++it) {
            if (it->x == x && it->y == y && it->active)
                return &(*it);
        }
        return nullptr;
    }

    const Incantation *findIncantationAt(int x, int y) const
    {
        for (auto it = incantations.rbegin(); it != incantations.rend(); ++it) {
            if (it->x == x && it->y == y && it->active)
                return &(*it);
        }
        return nullptr;
    }
};
