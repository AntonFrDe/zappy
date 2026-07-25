#include "RenderTheme.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>

namespace render_theme {

namespace {

constexpr RgbColor kTeamPalette[] = {
    {231, 76, 60},   // rouge
    {52, 152, 219},  // bleu
    {46, 204, 113},  // vert
    {241, 196, 15},  // orange fonce (le jaune pur reste reserve au Pac-Man)
    {155, 89, 182},  // violet
    {26, 188, 156},  // turquoise
    {230, 126, 34},  // orange
    {236, 112, 191}, // rose
};
constexpr std::size_t kTeamPaletteSize = sizeof(kTeamPalette) / sizeof(kTeamPalette[0]);

}

RgbColor teamColor(const std::string &teamName)
{
    std::size_t h = std::hash<std::string>{}(teamName);
    return kTeamPalette[h % kTeamPaletteSize];
}

bool isRareResource(int resourceIndex)
{
    return resourceIndex >= 3 && resourceIndex <= 6;
}

namespace {

// food, linemate, deraumere, sibur, mendiane, phiras, thystame : une
// teinte distincte par ressource (le jaune pur reste reserve au Pac-Man).
constexpr RgbColor kResourcePalette[7] = {
    {255, 233, 176}, // food
    {110, 210, 255}, // linemate
    {200, 130, 255}, // deraumere
    {110, 255, 170}, // sibur
    {255, 110, 150}, // mendiane
    {255, 150, 60},  // phiras
    {170, 255, 235}, // thystame
};

}

RgbColor resourceColor(int resourceIndex)
{
    if (resourceIndex < 0 || resourceIndex >= 7)
        return RgbColor{255, 255, 255};
    return kResourcePalette[resourceIndex];
}

float resourceQuantityScale(int quantity)
{
    constexpr int kQuantityCap = 6;
    constexpr float kScalePerExtraUnit = 0.18f;

    if (quantity <= 1)
        return 1.f;

    int extra = std::min(quantity, kQuantityCap) - 1;
    return 1.f + kScalePerExtraUnit * static_cast<float>(extra);
}

float orientationAngleDeg(int orientation)
{
    switch (orientation) {
    case 2:
        return 0.f;   // East
    case 3:
        return 90.f;  // South
    case 4:
        return 180.f; // West
    case 1:
    default:
        return 270.f; // North
    }
}

float mouthHalfAngleDeg(float phaseSeconds)
{
    float t = std::fmod(phaseSeconds, kMouthPeriodSeconds);

    if (t < 0.f)
        t += kMouthPeriodSeconds;

    float half = kMouthPeriodSeconds / 2.f;
    float ratio = (t < half) ? (t / half) : (1.f - (t - half) / half);

    return kMouthMinDeg + ratio * (kMouthMaxDeg - kMouthMinDeg);
}

PixelPos tileCenterPx(int tileX, int tileY)
{
    return tileCenterPxF(static_cast<float>(tileX), static_cast<float>(tileY));
}

PixelPos tileCenterPxF(float tileX, float tileY)
{
    return PixelPos{(tileX + 0.5f) * kCellSize, (tileY + 0.5f) * kCellSize};
}

float shortestWrappedDelta(float from, int to, int size)
{
    float raw = static_cast<float>(to) - from;

    if (size <= 0)
        return raw;

    float sizeF = static_cast<float>(size);
    raw = std::fmod(raw, sizeF);
    if (raw > sizeF / 2.f)
        raw -= sizeF;
    else if (raw < -sizeF / 2.f)
        raw += sizeF;
    return raw;
}

float wrapTileCoord(float value, int size)
{
    if (size <= 0)
        return value;

    float sizeF = static_cast<float>(size);
    value = std::fmod(value, sizeF);
    if (value < 0.f)
        value += sizeF;
    return value;
}

// Grille 3x3 interne a la tuile (le centre reste libre pour un joueur
// eventuellement present dessus) : food=0, linemate=1, deraumere=2,
// sibur=3, mendiane=4, phiras=5, thystame=6.
PixelPos resourceSlotOffsetPx(int resourceIndex)
{
    constexpr float d = kCellSize * 0.28f;
    static constexpr PixelPos offsets[7] = {
        {-d, -d}, {0.f, -d}, {d, -d},
        {-d, 0.f},           {d, 0.f},
        {-d, d},  {0.f, d},
    };

    if (resourceIndex < 0 || resourceIndex >= 7)
        return PixelPos{0.f, 0.f};
    return offsets[resourceIndex];
}

}
