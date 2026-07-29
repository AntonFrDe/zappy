#include "IsoView.hpp"
#include "RenderTheme.hpp"

#include <utility>
#include <vector>

using renderThemeIso::PixelPos;

sf::Vector2f toSfVec(PixelPos p)
{
    return sf::Vector2f(p.x, p.y);
}

sf::Color toSfColor(render_theme::RgbColor c)
{
    return sf::Color(c.r, c.g, c.b);
}

void IsoGridCache::rebuildIfNeeded(int newWidth, int newHeight)
{
    if (newWidth == mapWidth && newHeight == mapHeight)
        return;

    mapWidth = newWidth;
    mapHeight = newHeight;
    origin = renderThemeIso::defaultMapOrigin(mapHeight);
    origin.x += 40.f;
    origin.y += 40.f;

    auto lines = renderThemeIso::buildIsoGridLines(mapWidth, mapHeight, origin);
    vertices = sf::VertexArray(sf::Lines, lines.size() * 2);
    sf::Color gridColor(58, 62, 110);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        vertices[i * 2 + 0] = sf::Vertex(toSfVec(lines[i].from), gridColor);
        vertices[i * 2 + 1] = sf::Vertex(toSfVec(lines[i].to), gridColor);
    }
}

namespace {

void drawTileResources(sf::RenderWindow &window, const GameState &state, const IsoGridCache &grid)
{
    for (int y = 0; y < state.mapHeight; ++y) {
        for (int x = 0; x < state.mapWidth; ++x) {
            const Tile &tile = state.tileAt(x, y);
            PixelPos center = renderThemeIso::isoProject(
                static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, grid.origin);

            for (int r = 0; r < RESOURCE_COUNT; ++r) {
                if (tile.resources[r] <= 0)
                    continue;

                PixelPos slotOffset = renderThemeIso::resourceSlotOffsetPx(r);
                sf::Vector2f slotPos(center.x + slotOffset.x, center.y + slotOffset.y);

                sf::Color color = toSfColor(render_theme::resourceColor(r));
                float quantityScale = render_theme::resourceQuantityScale(tile.resources[r]);
                float baseRadius = render_theme::isRareResource(r)
                    ? renderThemeIso::kIsoTileWidth * 0.09f
                    : renderThemeIso::kIsoTileWidth * 0.05f;

                sf::CircleShape marker(baseRadius * quantityScale);
                marker.setFillColor(color);
                if (render_theme::isRareResource(r)) {
                    marker.setOutlineThickness(1.f);
                    marker.setOutlineColor(sf::Color(255, 255, 255, 180));
                }
                marker.setOrigin(marker.getRadius(), marker.getRadius());
                marker.setPosition(slotPos);
                window.draw(marker);
            }
        }
    }
}

}

void drawState(sf::RenderWindow &window, const GameState &state, const IsoGridCache &grid)
{
    window.draw(grid.vertices);
    drawTileResources(window, state, grid);

    std::vector<DrawablePlayer> drawablePlayers;
    drawablePlayers.reserve(state.players.size());
    for (const auto &[id, player] : state.players) {
        auto teamIt = state.teams.find(player.team);
        int colorIndex = teamIt != state.teams.end() ? teamIt->second.colorIndex : 0;
        drawablePlayers.push_back(DrawablePlayer{
            id,
            static_cast<float>(player.x),
            static_cast<float>(player.y),
            toSfColor(render_theme::teamColorByIndex(colorIndex)),
        });
    }
    renderThemeIso::sortByDepth(drawablePlayers, [](const DrawablePlayer &p) {
        return std::pair<float, float>(p.tileX, p.tileY);
    });

    for (const auto &p : drawablePlayers) {
        PixelPos px = renderThemeIso::isoProject(p.tileX + 0.5f, p.tileY + 0.5f, grid.origin);
        float radius = renderThemeIso::kIsoTileWidth * 0.3f;

        sf::CircleShape disc(radius);
        disc.setOrigin(radius, radius);
        disc.setPosition(toSfVec(px));
        disc.setFillColor(p.color);
        window.draw(disc);
    }
    for (const auto &[id, egg] : state.eggs) {
        if (egg.hatched)
            continue;
        PixelPos px = renderThemeIso::isoProject(
            static_cast<float>(egg.x) + 0.5f, static_cast<float>(egg.y) + 0.5f, grid.origin);
        float radius = renderThemeIso::kIsoTileWidth * 0.12f;

        sf::CircleShape disc(radius);
        disc.setOrigin(radius, radius);
        disc.setPosition(toSfVec(px));
        disc.setFillColor(sf::Color(230, 230, 230));
        window.draw(disc);
    }
}
