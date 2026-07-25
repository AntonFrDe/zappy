#ifndef GUI_RENDERER_HPP_
    #define GUI_RENDERER_HPP_

#include <SFML/Graphics.hpp>

#include "GameState.hpp"

// Rendu SFML, theme Pac-Man : les joueurs sont des Pac-Man (bouche orientee
// selon leur direction), les oeufs sont des fantomes, les ressources sont
// des pastilles (grosses et clignotantes pour les ressources rares). Aucun
// mur de labyrinthe n'est dessine : le protocole Zappy n'en envoie pas.
class Renderer
{
    public:
        explicit Renderer(int initialMapWidth = 10, int initialMapHeight = 10);

        bool isOpen() const;
        void pollEvents();
        // GameState non-const : le renderer possede l'horloge (dt) et fait
        // avancer/nettoie lui-meme les animations dont la duree est reelle
        // (ex: tenir le resultat d'une incantation ~1s avant de la retirer,
        // voir GameState::advanceIncantationAnimations).
        void draw(GameState &state, float dt);

    private:
        sf::RenderWindow _window;
        int _lastWidth;
        int _lastHeight;
        float _animPhase;

        void ensureWindowSize(int mapWidth, int mapHeight);
        void drawGrid(int mapWidth, int mapHeight);
        void drawTiles(const GameState &state);
        void drawIncantations(const GameState &state);
        void drawEggs(const GameState &state);
        void drawPlayers(const GameState &state);
};

#endif /* !GUI_RENDERER_HPP_ */
