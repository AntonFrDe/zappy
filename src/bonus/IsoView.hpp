#pragma once

#include <SFML/Graphics.hpp>
#include "../../include/gui/GameState.hpp"
#include "RenderTheme.hpp"
#include "RendererIso.hpp"

sf::Vector2f toSfVec(renderThemeIso::PixelPos p);
sf::Color toSfColor(render_theme::RgbColor c);

struct IsoGridCache {
    int mapWidth = -1;
    int mapHeight = -1;
    renderThemeIso::PixelPos origin{0.f, 0.f};
    sf::VertexArray vertices;

    void rebuildIfNeeded(int newWidth, int newHeight);
};

struct DrawablePlayer {
    int id;
    float tileX, tileY;
    sf::Color color;
};
void drawState(sf::RenderWindow &window, const GameState &state, const IsoGridCache &grid);
