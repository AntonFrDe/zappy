#include <SFML/Graphics.hpp>
#include <vector>

#include "RendererIso.hpp"

using renderThemeIso::PixelPos;

namespace {

sf::Vector2f toSfVec(PixelPos p)
{
    return sf::Vector2f(p.x, p.y);
}

struct DemoEntity {
    const char *name;
    float tileX, tileY;
    sf::Color color;
};

}

int main()
{
    const int mapWidth = 8;
    const int mapHeight = 6;

    PixelPos origin = renderThemeIso::defaultMapOrigin(mapHeight);
    origin.x += 40.f;
    origin.y += 40.f;

    auto gridLines = renderThemeIso::buildIsoGridLines(mapWidth, mapHeight, origin);

    sf::VertexArray gridVertices(sf::Lines, gridLines.size() * 2);
    sf::Color gridColor(58, 62, 110);
    for (std::size_t i = 0; i < gridLines.size(); ++i) {
        gridVertices[i * 2 + 0] = sf::Vertex(toSfVec(gridLines[i].from), gridColor);
        gridVertices[i * 2 + 1] = sf::Vertex(toSfVec(gridLines[i].to), gridColor);
    }

    std::vector<DemoEntity> entities = {
        {"D", 5.f, 3.f, sf::Color(235, 90, 90)},
        {"A", 2.f, 1.f, sf::Color(90, 200, 235)},
        {"C", 4.f, 3.f, sf::Color(235, 200, 90)},
        {"B", 3.f, 2.f, sf::Color(140, 235, 90)},
    };

    renderThemeIso::sortByDepth(entities, [](const DemoEntity &e) {
        return std::pair<float, float>(e.tileX, e.tileY);
    });

    sf::RenderWindow window(sf::VideoMode(700, 450), "Zappy iso (depth sort)");
    window.setVerticalSyncEnabled(true);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        window.clear(sf::Color(16, 16, 24));
        window.draw(gridVertices);
        for (const auto &e : entities) {
            PixelPos p = renderThemeIso::isoProject(e.tileX + 0.5f, e.tileY + 0.5f, origin);
            float radius = renderThemeIso::kIsoTileWidth * 0.55f;

            sf::CircleShape disc(radius);
            disc.setOrigin(radius, radius);
            disc.setPosition(toSfVec(p));
            disc.setFillColor(e.color);
            window.draw(disc);
        }

        window.display();
    }

    return 0;
}
