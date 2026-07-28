#pragma once

namespace renderThemeIso {

struct PixelPos {
    float x;
    float y;
};

constexpr float kIsoTileWidth = 64.f;
constexpr float kIsoTileHeight = 32.f;
PixelPos isoProject(float tileX, float tileY, PixelPos origin = {0.f, 0.f});
PixelPos isoUnproject(float screenX, float screenY, PixelPos origin = {0.f, 0.f});
PixelPos tileCenterPx(int tileX, int tileY);
PixelPos tileCenterPxF(float tileX, float tileY);

}
