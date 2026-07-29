#pragma once

#include <algorithm>
#include <utility>
#include <vector>

namespace renderThemeIso {

// Position en pixels dans le repère écran
struct PixelPos {
    float x;
    float y;
};

// Segment de ligne de la grille isométrique (deux extrémités en pixels)
struct IsoLineSegment {
    PixelPos from;
    PixelPos to;
};

constexpr float kIsoTileWidth = 64.f;
constexpr float kIsoTileHeight = 32.f;

// Convertit une coordonnée de tuile en position pixel écran (projection isométrique)
PixelPos isoProject(float tileX, float tileY, PixelPos origin = {0.f, 0.f});

// Convertit une position pixel écran en coordonnée de tuile (inverse de isoProject)
PixelPos isoUnproject(float screenX, float screenY, PixelPos origin = {0.f, 0.f});

// Centre pixel d'une tuile, à partir de coordonnées entières
PixelPos tileCenterPx(int tileX, int tileY);

// Centre pixel d'une tuile, à partir de coordonnées flottantes
PixelPos tileCenterPxF(float tileX, float tileY);

// Origine par défaut pour centrer horizontalement la carte
PixelPos defaultMapOrigin(int mapHeight);

// Construit tous les segments formant le quadrillage isométrique de la carte
std::vector<IsoLineSegment> buildIsoGridLines(int mapWidth, int mapHeight, PixelPos origin = {0.f, 0.f});

// Décalage pixel prédéfini pour placer une icône de ressource autour du centre d'une tuile
PixelPos resourceSlotOffsetPx(int resourceIndex);

// Clé de profondeur (tileX + tileY) utilisée pour trier l'ordre de dessin
float depthKey(float tileX, float tileY);

// Trie une liste d'éléments par profondeur pour un rendu isométrique correct
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
