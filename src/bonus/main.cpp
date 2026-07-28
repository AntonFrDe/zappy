#include <cstdio>

#include "RendererIso.hpp"

int main()
{
    using renderThemeIso::PixelPos;
    using renderThemeIso::isoProject;
    using renderThemeIso::isoUnproject;
    using renderThemeIso::tileCenterPx;

    int coords[][2] = { {0, 0}, {5, 0}, {0, 5}, {3, 3}, {9, 4} };

    std::printf("tuile -> pixel (projection isometrique)\n");
    for (auto &c : coords) {
        PixelPos px = tileCenterPx(c[0], c[1]);
        std::printf("  (%d,%d) -> (%.1f, %.1f)\n", c[0], c[1], px.x, px.y);
    }

    std::printf("\nverification aller-retour (pixel -> tuile)\n");
    for (auto &c : coords) {
        PixelPos px = isoProject(static_cast<float>(c[0]), static_cast<float>(c[1]));
        PixelPos back = isoUnproject(px.x, px.y);
        std::printf("  (%d,%d) -> px(%.1f,%.1f) -> (%.3f, %.3f)\n",
            c[0], c[1], px.x, px.y, back.x, back.y);
    }

    return 0;
}
