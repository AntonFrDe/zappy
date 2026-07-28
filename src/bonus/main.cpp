#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>

#include "RendererIso.hpp"

static void dumpGridPpm(const std::vector<renderThemeIso::IsoLineSegment> &lines,
    int mapWidth, int mapHeight, const char *path)
{
    using renderThemeIso::kIsoTileWidth;

    const int margin = 10;
    int w = static_cast<int>(static_cast<float>(mapWidth + mapHeight) * kIsoTileWidth / 2.f) + margin * 2;
    int h = static_cast<int>(static_cast<float>(mapWidth + mapHeight) * kIsoTileWidth / 4.f) + margin * 2;

    std::vector<unsigned char> buf(static_cast<std::size_t>(w) * h * 3, 0);
    auto plot = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= w || y >= h)
            return;
        std::size_t idx = (static_cast<std::size_t>(y) * w + x) * 3;
        buf[idx] = 90; buf[idx + 1] = 140; buf[idx + 2] = 240;
    };
    auto drawLine = [&](float fx0, float fy0, float fx1, float fy1) {
        int x0 = static_cast<int>(fx0) + margin, y0 = static_cast<int>(fy0) + margin;
        int x1 = static_cast<int>(fx1) + margin, y1 = static_cast<int>(fy1) + margin;
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        while (true) {
            plot(x0, y0);
            if (x0 == x1 && y0 == y1)
                break;
            int e2 = 2 * err;
            if (e2 >= dy) { 
                err += dy;
                x0 += sx; }
            if (e2 <= dx) { 
                err += dx;
                y0 += sy; }
        }
    };
    for (const auto &seg : lines)
        drawLine(seg.from.x, seg.from.y, seg.to.x, seg.to.y);
    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << w << " " << h << "\n255\n";
    out.write(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
}

int main()
{
    using renderThemeIso::PixelPos;
    using renderThemeIso::isoProject;
    using renderThemeIso::isoUnproject;
    using renderThemeIso::tileCenterPx;
    using renderThemeIso::defaultMapOrigin;
    using renderThemeIso::buildIsoGridLines;

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
    int mapWidth = 8;
    int mapHeight = 6;
    PixelPos origin = defaultMapOrigin(mapHeight);
    auto gridLines = buildIsoGridLines(mapWidth, mapHeight, origin);

    std::printf("\ngrille iso %dx%d : %zu segments (attendu %d)\n",
        mapWidth, mapHeight, gridLines.size(), (mapWidth + 1) + (mapHeight + 1));
    dumpGridPpm(gridLines, mapWidth, mapHeight, "iso_grid.ppm");
    std::printf("PPM ecrit dans iso_grid.ppm\n");
    return 0;
}
