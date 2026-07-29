#include "RendererIso.hpp"

namespace renderThemeIso {

PixelPos isoProject(float tileX, float tileY, PixelPos origin)
{
    float halfW = kIsoTileWidth / 2.f;
    float halfH = kIsoTileHeight / 2.f;

    return PixelPos{
        origin.x + (tileX - tileY) * halfW,
        origin.y + (tileX + tileY) * halfH,
    };
}

PixelPos isoUnproject(float screenX, float screenY, PixelPos origin)
{
    float dx = screenX - origin.x;
    float dy = screenY - origin.y;
    float halfW = kIsoTileWidth / 2.f;
    float halfH = kIsoTileHeight / 2.f;

    return PixelPos{
        (dx / halfW + dy / halfH) / 2.f,
        (dy / halfH - dx / halfW) / 2.f,
    };
}

PixelPos tileCenterPxF(float tileX, float tileY)
{
    return isoProject(tileX + 0.5f, tileY + 0.5f);
}

PixelPos tileCenterPx(int tileX, int tileY)
{
    return tileCenterPxF(static_cast<float>(tileX), static_cast<float>(tileY));
}

PixelPos defaultMapOrigin(int mapHeight)
{
    return PixelPos{ static_cast<float>(mapHeight) * (kIsoTileWidth / 2.f), 0.f };
}

std::vector<IsoLineSegment> buildIsoGridLines(int mapWidth, int mapHeight, PixelPos origin)
{
    std::vector<IsoLineSegment> lines;

    if (mapWidth < 0)
        mapWidth = 0;
    if (mapHeight < 0)
        mapHeight = 0;
    for (int x = 0; x <= mapWidth; ++x) {
        PixelPos a = isoProject(static_cast<float>(x), 0.f, origin);
        PixelPos b = isoProject(static_cast<float>(x), static_cast<float>(mapHeight), origin);
        lines.push_back(IsoLineSegment{a, b});
    }
    for (int y = 0; y <= mapHeight; ++y) {
        PixelPos a = isoProject(0.f, static_cast<float>(y), origin);
        PixelPos b = isoProject(static_cast<float>(mapWidth), static_cast<float>(y), origin);
        lines.push_back(IsoLineSegment{a, b});
    }
    return lines;
}

PixelPos resourceSlotOffsetPx(int resourceIndex)
{
    constexpr float dx = kIsoTileWidth * 0.16f;
    constexpr float dy = kIsoTileHeight * 0.22f;
    static constexpr PixelPos offsets[7] = {
        {-dx, -dy}, {0.f, -dy}, {dx, -dy},
        {-dx, 0.f},             {dx, 0.f},
        {-dx, dy},  {0.f, dy},
    };

    if (resourceIndex < 0 || resourceIndex >= 7)
        return PixelPos{0.f, 0.f};
    return offsets[resourceIndex];
}

float depthKey(float tileX, float tileY)
{
    return tileX + tileY;
}
}
