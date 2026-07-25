#ifndef GUI_RENDERER_HPP_
    #define GUI_RENDERER_HPP_

#include <SFML/Graphics.hpp>
#include <string>

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
        sf::Font _font;
        bool _hasFont;

        // Camera de la zone carte (zoom + deplacement), separee de la vue
        // par defaut utilisee pour le HUD : le panneau HUD reste fixe a
        // l'ecran quel que soit le zoom/pan applique a la carte. Recalculee
        // a chaque frame (updateMapView) a partir de la taille de fenetre
        // courante, donc reagit d'elle-meme a un redimensionnement.
        sf::View _mapView;
        sf::FloatRect _mapViewportPx; // rectangle (px fenetre) occupe par la vue carte, pour le hit-test des clics
        float _zoomLevel;
        sf::Vector2f _panOffset; // decalage du centre de la camera, en pixels carte

        // Selection au clic gauche (case ou joueur), pour le panneau
        // "inventaire du joueur selectionne" / "contenu de la case
        // selectionnee" du HUD. pollEvents() n'a pas acces au GameState,
        // donc le clic est bufferise puis traite au debut de draw().
        bool _hasPendingClick;
        sf::Vector2i _pendingClickPx;
        int _selectedPlayerId;
        bool _hasSelectedTile;
        int _selectedTileX;
        int _selectedTileY;

        void ensureWindowSize(int mapWidth, int mapHeight);
        // Recalcule _mapView : viewport letterboxe (garde le ratio
        // d'aspect de la carte au lieu de l'etirer) dans la zone a gauche
        // du HUD, taille visible selon _zoomLevel, centre selon _panOffset.
        void updateMapView(int mapWidth, int mapHeight);
        void processPendingClick(const GameState &state);
        void drawText(const std::string &text, sf::Vector2f pos, unsigned int charSize,
            sf::Color color, bool bold = false);
        void drawHud(const GameState &state);
        // Rapproche Player::animX/animY de (x, y) d'une fraction de la
        // distance restante a chaque frame (lissage exponentiel), en
        // choisissant le chemin le plus court sur la carte toroidale.
        void updatePlayerAnimations(GameState &state, float dt);
        void drawGrid(int mapWidth, int mapHeight);
        void drawTiles(const GameState &state);
        void drawIncantations(const GameState &state);
        void drawEggs(const GameState &state);
        void drawPlayers(const GameState &state);
        // Anime les evenements ponctuels sans effet visuel propre dans le
        // protocole (pgt/pdr/pex/pbc/pfk/pdi/plv), voir GameState::effects.
        void drawEffects(const GameState &state);
        // N'affiche du texte que si une police a pu etre chargee (voir
        // constructeur) : le projet n'en embarque pas, donc plusieurs
        // chemins systeme usuels sont tentes, sans jamais planter si aucun
        // n'existe (bandeau colore affiche quand meme, sans le libelle).
        void drawGameOverBanner(const GameState &state);
};

#endif /* !GUI_RENDERER_HPP_ */
