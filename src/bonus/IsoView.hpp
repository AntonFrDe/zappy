#pragma once

#include <SFML/Graphics.hpp>
#include "../../include/gui/GameState.hpp"
#include "RenderTheme.hpp"
#include "RendererIso.hpp"

// Convertit un PixelPos (renderThemeIso) en sf::Vector2f (SFML)
sf::Vector2f toSfVec(renderThemeIso::PixelPos p);

// Convertit une couleur RGB du thème en sf::Color (SFML)
sf::Color toSfColor(render_theme::RgbColor c);

// Cache de la grille isométrique, reconstruite seulement si la taille de la carte change
struct IsoGridCache {
    int mapWidth = -1;
    int mapHeight = -1;
    renderThemeIso::PixelPos origin{0.f, 0.f};
    sf::VertexArray vertices;

    // Régénère la grille (segments + vertices) si newWidth/newHeight ont changé
    void rebuildIfNeeded(int newWidth, int newHeight);
};

// Représentation d'un joueur prête pour le dessin et le tri par profondeur
struct DrawablePlayer {
    int id;
    float tileX, tileY;
    sf::Color color;
};

// Dessine toute la scène : grille, ressources, joueurs (triés par profondeur), œufs
void drawState(sf::RenderWindow &window, const GameState &state, const IsoGridCache &grid);
