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
// Repose sur un hash du nom : au-dela de 8 equipes (taille de la palette
// interne), deux equipes peuvent recevoir la meme couleur. Prefer
// teamColorByIndex() partout ou un index d'ordre d'apparition stable est
// disponible (GameState::Team::colorIndex) ; ne garder teamColor() que
// pour un cas ou seul le nom est connu.
RgbColor teamColor(const std::string &teamName);

// Couleur d'equipe a partir d'un index d'ordre d'apparition (0, 1, 2...),
// par rotation d'angle dore (~137.5 degres) : contrairement a un hash de
// nom, deux index differents ne produisent jamais la meme teinte, quel
// que soit le nombre d'equipes. index < 0 est ramene a 0.
RgbColor teamColorByIndex(int index);

// Ordre protocole des ressources : food=0, linemate=1, deraumere=2,
// sibur=3, mendiane=4, phiras=5, thystame=6. Les 4 dernieres sont rendues
// en "grosse pastille" (power pellet), les 3 premieres en pastille simple.
bool isRareResource(int resourceIndex);

// Couleur propre a chaque type de ressource (une teinte distincte par
// index), pour que les 7 ressources soient identifiables au premier
// coup d'oeil sur une tuile au lieu de se fondre en 2 couleurs.
RgbColor resourceColor(int resourceIndex);

// Multiplicateur de taille pour une pastille de ressource selon la
// quantite presente sur la case (1 ressource => 1.0, taille normale) :
// une case avec beaucoup d'une meme ressource se voit donc au premier
// coup d'oeil, sans avoir besoin d'un compteur numerique. Clampe pour
// qu'une quantite enorme ne fasse pas deborder la tuile.
float resourceQuantityScale(int quantity);

// Angle (degres, sens horaire, 0 = fait face a droite) correspondant a une
// valeur brute d'Orientation (North=1, East=2, South=3, West=4).
float orientationAngleDeg(int orientation);

// Demi-angle (en degres) de la bouche du Pac-Man a l'instant donne :
// oscille entre kMouthMinDeg et kMouthMaxDeg pour l'effet "chomp chomp".
float mouthHalfAngleDeg(float phaseSeconds);

// Centre en pixels d'une tuile (tileX, tileY) sur l'ecran.
PixelPos tileCenterPx(int tileX, int tileY);

// Variante flottante de tileCenterPx, pour une position en cours
// d'interpolation (deplacement anime) plutot qu'une case entiere.
PixelPos tileCenterPxF(float tileX, float tileY);

// Delta signe le plus court entre une position flottante "from" et une
// case cible "to" sur un axe torique de taille "size" (le monde Zappy
// boucle sur les bords) : ex. shortestWrappedDelta(0.f, size-1, size)
// vaut -1, pas size-1, pour animer le deplacement par le bord le plus
// proche plutot que de traverser toute la carte. size <= 0 renvoie la
// distance brute (pas de bouclage possible).
float shortestWrappedDelta(float from, int to, int size);

// Ramene une coordonnee de tuile (potentiellement hors bornes apres une
// interpolation qui traverse un bord) dans l'intervalle [0, size).
// size <= 0 renvoie la valeur inchangee.
float wrapTileCoord(float value, int size);

// Decalage (en pixels, a ajouter au centre de la tuile) attribue a un type
// de ressource, pour qu'une tuile contenant plusieurs ressources a la fois
// les affiche toutes cote a cote au lieu de les superposer exactement.
PixelPos resourceSlotOffsetPx(int resourceIndex);

}

#endif /* !GUI_RENDER_THEME_HPP_ */
