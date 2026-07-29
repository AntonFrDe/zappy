# Zappy — Rendu isométrique (Vue GUI)

*Documentation technique du module de rendu isométrique (IsoView / RendererIso)*

## 1. Vue d'ensemble

Ce module implémente l'affichage isométrique du client graphique Zappy (SFML). Il se compose de trois parties : la géométrie de projection isométrique pure (`RendererIso`), la mise en cache et le dessin de la scène (`IsoView`), et le point d'entrée de l'application (`main.cpp`) qui orchestre la connexion réseau, la mise à jour de l'état du jeu et la boucle de rendu.

Le module suit une séparation claire des responsabilités :

- **RendererIso** : calculs géométriques indépendants de SFML (projection tuile ↔ écran, grille, tri de profondeur).
- **IsoView** : conversion des types métier vers SFML et dessin effectif (grille, ressources, joueurs, œufs).
- **main.cpp** : boucle applicative (réseau, parsing de commandes, fenêtre SFML).

## 2. Architecture des fichiers

| Fichier | Rôle |
|---|---|
| `RendererIso.hpp` / `.cpp` | Namespace `renderThemeIso` — mathématiques de projection isométrique, génération de la grille, offsets de ressources, tri par profondeur. |
| `IsoView.hpp` / `.cpp` | Cache de grille (`IsoGridCache`), conversion vers types SFML, fonction `drawState()` qui dessine toute la scène. |
| `main.cpp` | Point d'entrée : parsing des arguments, connexion réseau, boucle principale SFML. |

## 3. RendererIso — géométrie isométrique

Namespace : `renderThemeIso`. Ce module ne dépend d'aucune bibliothèque graphique ; il travaille uniquement avec des coordonnées flottantes, ce qui le rend testable indépendamment de SFML.

### 3.1 Structures de données

```cpp
struct PixelPos {
    float x;
    float y;
};

struct IsoLineSegment {
    PixelPos from;
    PixelPos to;
};
```

`PixelPos` représente une coordonnée écran (en pixels). `IsoLineSegment` représente un segment de la grille isométrique (deux extrémités en pixels).

### 3.2 Constantes

| Constante | Valeur | Description |
|---|---|---|
| `kIsoTileWidth` | `64.f` | Largeur d'une tuile isométrique en pixels. |
| `kIsoTileHeight` | `32.f` | Hauteur d'une tuile isométrique en pixels (typiquement moitié de la largeur pour un rendu 2:1). |

### 3.3 Fonctions de projection

#### `isoProject(tileX, tileY, origin) → PixelPos`

Convertit des coordonnées de tuile (flottantes, système cartésien) en coordonnées écran isométriques, selon la formule classique :

```
screenX = origin.x + (tileX - tileY) * (kIsoTileWidth / 2)
screenY = origin.y + (tileX + tileY) * (kIsoTileHeight / 2)
```

Le paramètre `origin` (par défaut `{0,0}`) permet de décaler l'ensemble de la projection, typiquement pour centrer la carte dans la fenêtre.

#### `isoUnproject(screenX, screenY, origin) → PixelPos`

Fonction inverse de `isoProject` : convertit une position écran en coordonnées de tuile. Utile pour la sélection à la souris (picking). Elle applique la transformation inverse de la matrice de projection isométrique.

#### `tileCenterPx(tileX, tileY)` / `tileCenterPxF(tileX, tileY)`

Raccourcis pour obtenir le centre pixel d'une tuile (offset de +0.5 sur chaque axe) avant projection. Version entière et version flottante.

#### `defaultMapOrigin(mapHeight) → PixelPos`

Calcule une origine par défaut de sorte que la carte apparaisse centrée horizontalement, en fonction de la hauteur de la carte : `origin.x = mapHeight * kIsoTileWidth / 2`, `origin.y = 0`.

### 3.4 Génération de la grille

#### `buildIsoGridLines(mapWidth, mapHeight, origin) → std::vector<IsoLineSegment>`

Construit l'ensemble des segments de ligne formant le quadrillage isométrique de la carte : une ligne verticale (en coordonnées tuile) pour chaque colonne de 0 à `mapWidth`, et une ligne horizontale pour chaque rangée de 0 à `mapHeight`. Les valeurs négatives de largeur/hauteur sont ramenées à 0 par sécurité.

### 3.5 Ressources

#### `resourceSlotOffsetPx(resourceIndex) → PixelPos`

Retourne un décalage pixel prédéfini (parmi 7 emplacements disposés en grille 3×3 sans le centre) permettant de positionner jusqu'à 7 icônes de ressources différentes autour du centre d'une tuile, sans chevauchement. Retourne `{0,0}` si l'index est hors de la plage `[0, 7)`.

### 3.6 Tri par profondeur

#### `depthKey(tileX, tileY) → float`

Clé de profondeur simple : `tileX + tileY`. En isométrique, plus cette somme est grande, plus l'élément est proche de la caméra (bas de l'écran) et doit être dessiné après les éléments qui ont une somme plus faible.

#### `sortByDepth<T, TilePosFn>(items, tilePosOf)`

Template générique qui trie un vecteur d'éléments quelconques par ordre de dessin, à l'aide d'une fonction fournie par l'appelant (`tilePosOf`) qui extrait la position (tileX, tileY) de chaque élément. Utilise `std::stable_sort` pour préserver l'ordre relatif des éléments à profondeur égale, ce qui évite un scintillement (flicker) entre deux rendus successifs.

## 4. IsoView — cache et dessin

Fichier : `IsoView.hpp` / `IsoView.cpp`. Ce module fait le pont entre les structures génériques de `renderThemeIso` et SFML, et contient toute la logique de dessin de la scène de jeu.

### 4.1 Fonctions de conversion

| Fonction | Signature | Rôle |
|---|---|---|
| `toSfVec` | `sf::Vector2f toSfVec(PixelPos p)` | Convertit un `PixelPos` (renderThemeIso) en `sf::Vector2f` (SFML). |
| `toSfColor` | `sf::Color toSfColor(render_theme::RgbColor c)` | Convertit une couleur RGB du thème en `sf::Color` SFML (sans canal alpha explicite → opaque). |

### 4.2 IsoGridCache

Structure qui met en cache la grille isométrique afin de ne pas la reconstruire à chaque frame.

```cpp
struct IsoGridCache {
    int mapWidth = -1;
    int mapHeight = -1;
    renderThemeIso::PixelPos origin{0.f, 0.f};
    sf::VertexArray vertices;

    void rebuildIfNeeded(int newWidth, int newHeight);
};
```

#### `rebuildIfNeeded(newWidth, newHeight)`

Ne reconstruit la grille que si les dimensions de la carte ont changé (comparaison avec `mapWidth` / `mapHeight` stockés). Lors d'une reconstruction :

- Calcule une nouvelle origine via `defaultMapOrigin()`, puis lui ajoute un décalage fixe de +40px sur x et y (marge visuelle par rapport au bord de la fenêtre).
- Génère les segments de grille via `buildIsoGridLines()`.
- Remplit un `sf::VertexArray` de type `sf::Lines` (2 sommets par segment) avec une couleur de grille fixe RGB(58, 62, 110).

Ce mécanisme évite un coût de reconstruction O(mapWidth × mapHeight) à chaque frame quand la carte ne change pas de taille (cas le plus fréquent).

### 4.3 DrawablePlayer

```cpp
struct DrawablePlayer {
    int id;
    float tileX, tileY;
    sf::Color color;
};
```

Structure intermédiaire regroupant les informations nécessaires au dessin d'un joueur, indépendamment de la structure `Player` du `GameState`, afin de faciliter le tri par profondeur.

### 4.4 drawState(window, state, grid)

Fonction principale de dessin, appelée une fois par frame depuis la boucle principale. Ordre de dessin (déterminant l'empilement visuel) :

1. La grille isométrique (`grid.vertices`).
2. Les ressources posées sur chaque tuile (`drawTileResources`, fonction interne au fichier).
3. Les joueurs, triés par profondeur (`sortByDepth`) puis dessinés sous forme de disques colorés.
4. Les œufs non éclos, dessinés en petits disques gris clair.

#### `drawTileResources` (interne, anonymous namespace)

Parcourt chaque tuile de la carte (double boucle sur y puis x) et, pour chaque type de ressource présent en quantité positive :

- Calcule le centre pixel de la tuile via `isoProject`.
- Applique le décalage propre au type de ressource via `resourceSlotOffsetPx`.
- Détermine la couleur (`render_theme::resourceColor`) et une échelle de taille dépendante de la quantité (`render_theme::resourceQuantityScale`).
- Utilise un rayon de base plus grand pour les ressources rares (`render_theme::isRareResource`), avec en plus un contour blanc semi-transparent pour les distinguer visuellement.
- Dessine un `sf::CircleShape` centré sur la position calculée.

#### Rendu des joueurs

Les joueurs sont d'abord convertis en `DrawablePlayer` (position et couleur d'équipe résolue via `state.teams`), puis triés par `sortByDepth` avant d'être dessinés, garantissant que les joueurs "plus proches" de la caméra isométrique s'affichent par-dessus ceux qui sont plus loin. Chaque joueur est représenté par un disque de rayon `kIsoTileWidth * 0.3`.

#### Rendu des œufs

Les œufs dont le champ `hatched` est vrai (déjà éclos) sont ignorés. Les autres sont dessinés comme de petits disques (rayon `kIsoTileWidth * 0.12`) de couleur gris clair RGB(230, 230, 230), sans tri de profondeur particulier.

## 5. main.cpp — point d'entrée

Orchestre le cycle de vie complet de l'application cliente :

1. Analyse des arguments de la ligne de commande (`parseArgs`) ; affiche l'usage et quitte avec le code 84 en cas d'erreur, ou 0 si l'aide est demandée.
2. Connexion réseau au serveur Zappy (`NetworkClient::connectToServer`) puis handshake protocolaire (`performHandshake`).
3. Lecture de la taille initiale de la carte (`readInitialMapSize`) et initialisation du `GameState` (`state.resizeMap`).
4. Création du `CommandParser` (qui met à jour le `GameState` à partir des lignes reçues du serveur) et de l'`IsoGridCache`.
5. Création de la fenêtre SFML (900×650, V-Sync activée) intitulée « Zappy iso ».
6. Boucle principale, tant que le réseau et la fenêtre sont actifs :
   - Traitement des événements SFML (fermeture de fenêtre).
   - Réception des lignes réseau (`network.receiveLines`) et dispatch de chaque commande au `CommandParser`.
   - Reconstruction conditionnelle de la grille (`grid.rebuildIfNeeded`) si les dimensions de la carte ont changé.
   - Effacement de la fenêtre (fond RGB(16, 16, 24)), appel à `drawState()`, puis affichage (`window.display()`).
7. À la sortie de la boucle (déconnexion ou fermeture), affichage d'un message et retour 0.

## 6. Flux de données

Résumé du flux, de la réception réseau jusqu'à l'affichage d'une frame :

```
Serveur Zappy
    │  (lignes texte du protocole)
    ▼
NetworkClient::receiveLines()
    │
    ▼
CommandParser::dispatch(line)   → met à jour GameState
    │
    ▼
IsoGridCache::rebuildIfNeeded() → régénère la grille si la carte a changé
    │
    ▼
drawState(window, state, grid)
    ├─ grid.vertices                 (grille)
    ├─ drawTileResources(...)        (ressources par tuile)
    ├─ sortByDepth(joueurs) + dessin (joueurs)
    └─ dessin des œufs non éclos
    │
    ▼
window.display()
```

## 7. Points d'attention et pistes d'amélioration

- Couleurs de grille et d'œufs codées en dur (« magic numbers ») dans `IsoView.cpp` — pourraient être déplacées dans `RenderTheme` pour centraliser la palette.
- `resourceSlotOffsetPx` suppose exactement 7 types de ressources (`RESOURCE_COUNT`) ; toute évolution du nombre de ressources nécessite une mise à jour manuelle du tableau `offsets`.
- Les œufs ne sont pas triés par profondeur avant d'être dessinés, contrairement aux joueurs ; cela peut être acceptable s'ils ne se chevauchent jamais visuellement, mais reste une incohérence potentielle avec le reste du pipeline de rendu.
- `isoUnproject` n'est pas encore utilisé dans le code fourni (aucun picking souris visible dans `main.cpp`) ; probablement prévu pour une fonctionnalité future (sélection de tuile/joueur).
- `IsoGridCache::rebuildIfNeeded` ne gère pas de dimensions négatives en amont (elles sont clampées plus bas dans `buildIsoGridLines`), ce qui est cohérent mais pourrait être documenté explicitement dans l'en-tête.
