#pragma once

#include <algorithm>
#include <utility>
#include <vector>

namespace renderThemeIso {

struct PixelPos {
    float x;
    float y;
};

struct IsoLineSegment {
    PixelPos from;
    PixelPos to;
};

constexpr float kIsoTileWidth = 64.f;
constexpr float kIsoTileHeight = 32.f;
PixelPos isoProject(float tileX, float tileY, PixelPos origin = {0.f, 0.f});
PixelPos isoUnproject(float screenX, float screenY, PixelPos origin = {0.f, 0.f});
PixelPos tileCenterPx(int tileX, int tileY);
PixelPos tileCenterPxF(float tileX, float tileY);
PixelPos defaultMapOrigin(int mapHeight);
std::vector<IsoLineSegment> buildIsoGridLines(int mapWidth, int mapHeight, PixelPos origin = {0.f, 0.f});
float depthKey(float tileX, float tileY);
template <typename T, typename TilePosFn>
void sortByDepth(std::vector<T> &items, TilePosFn tilePosOf)
{
    std::stable_sort(items.begin(), items.end(), [&](const T &a, const T &b) {
        std::pair<float, float> pa = tilePosOf(a);
        std::pair<float, float> pb = tilePosOf(b);
        return depthKey(pa.first, pa.second) < depthKey(pb.first, pb.second);
    });
}

}
