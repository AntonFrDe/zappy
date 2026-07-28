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
}
