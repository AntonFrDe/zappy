#ifndef GUI_RENDER_THEME_HPP_
    #define GUI_RENDER_THEME_HPP_

#include <string>

// Fonctions de theme purement calculatoires (aucune dependance a SFML), pour
// pouvoir les tester unitairement sans avoir besoin d'une fenetre/contexte
// graphique. Renderer.cpp s'appuie dessus pour dessiner reellement.
namespace render_theme {

struct RgbColor {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

struct PixelPos {
    float x;
    float y;
};

constexpr float kCellSize = 40.f;
constexpr float kMouthMinDeg = 14.f;
constexpr float kMouthMaxDeg = 55.f;
constexpr float kMouthPeriodSeconds = 0.42f;

// Couleur d'equipe deterministe (meme nom -> meme couleur), utilisee comme
// liseré autour du Pac-Man ; le corps du Pac-Man reste toujours jaune.
RgbColor teamColor(const std::string &teamName);

// Ordre protocole des ressources : food=0, linemate=1, deraumere=2,
// sibur=3, mendiane=4, phiras=5, thystame=6. Les 4 dernieres sont rendues
// en "grosse pastille" (power pellet), les 3 premieres en pastille simple.
bool isRareResource(int resourceIndex);

// Angle (degres, sens horaire, 0 = fait face a droite) correspondant a une
// valeur brute d'Orientation (North=0, East=1, South=2, West=3).
float orientationAngleDeg(int orientation);

// Demi-angle (en degres) de la bouche du Pac-Man a l'instant donne :
// oscille entre kMouthMinDeg et kMouthMaxDeg pour l'effet "chomp chomp".
float mouthHalfAngleDeg(float phaseSeconds);

// Centre en pixels d'une tuile (tileX, tileY) sur l'ecran.
PixelPos tileCenterPx(int tileX, int tileY);

// Decalage (en pixels, a ajouter au centre de la tuile) attribue a un type
// de ressource, pour qu'une tuile contenant plusieurs ressources a la fois
// les affiche toutes cote a cote au lieu de les superposer exactement.
PixelPos resourceSlotOffsetPx(int resourceIndex);

}

#endif /* !GUI_RENDER_THEME_HPP_ */
