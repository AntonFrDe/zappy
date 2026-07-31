#include "Renderer.hpp"
#include "RenderTheme.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

namespace {

constexpr float kPi = 3.14159265f;

sf::Color toSfColor(render_theme::RgbColor c, unsigned char alpha = 255)
{
    return sf::Color(c.r, c.g, c.b, alpha);
}

sf::Vector2f toSfVec(render_theme::PixelPos p)
{
    return sf::Vector2f(p.x, p.y);
}

// Couleur d'equipe sans collision : passe par Team::colorIndex (ordre
// d'apparition, voir CommandParser::handleTna) quand l'equipe est connue,
// et ne retombe sur le hash du nom (qui peut entrer en collision au-dela
// de 8 equipes) que dans le cas defensif ou elle ne l'est pas encore.
render_theme::RgbColor teamColorFor(const GameState &state, const std::string &teamName)
{
    const Team *team = state.findTeam(teamName);
    return team ? render_theme::teamColorByIndex(team->colorIndex) : render_theme::teamColor(teamName);
}

// Simule un vrai degrade (SFML n'a pas de radial gradient natif) en
// empilant plusieurs cercles semi-transparents, du plus grand/plus pale
// au plus petit/plus intense - un halo "neon" plutot qu'un disque plat.
void drawGlow(sf::RenderWindow &window, sf::Vector2f center, float maxRadius,
    sf::Color color, unsigned char peakAlpha)
{
    constexpr int rings = 5;

    for (int i = rings; i >= 1; --i) {
        float t = static_cast<float>(i) / rings;
        sf::CircleShape ring(maxRadius * t);
        sf::Color c = color;

        c.a = static_cast<unsigned char>(peakAlpha * (1.f - t) * (1.f - t));
        ring.setFillColor(c);
        ring.setOrigin(ring.getRadius(), ring.getRadius());
        ring.setPosition(center);
        window.draw(ring);
    }
}

// Range "level" petits carres cote a cote, centres sur "topCenter" : sert
// d'indicateur du niveau vise par une incantation (evite de dependre d'une
// police/sf::Font, absente du projet pour l'instant).
void drawLevelPips(sf::RenderWindow &window, sf::Vector2f topCenter, int level, unsigned char alpha)
{
    constexpr float kPipSize = 4.f;
    constexpr float kPipGap = 3.f;
    float totalWidth = level * kPipSize + (level - 1) * kPipGap;
    float startX = topCenter.x - totalWidth / 2.f;

    for (int i = 0; i < level; ++i) {
        sf::RectangleShape pip(sf::Vector2f(kPipSize, kPipSize));
        pip.setFillColor(sf::Color(255, 216, 10, alpha));
        pip.setPosition(startX + i * (kPipSize + kPipGap), topCenter.y);
        window.draw(pip);
    }
}

// Anneau rituel : cercle pointille tournant + particules en orbite, joue
// pendant toute la duree de l'incantation (jusqu'a reception de pie).
void drawRitualCircle(sf::RenderWindow &window, sf::Vector2f center, float animPhase)
{
    constexpr int kParticleCount = 8;
    float ringRadius = render_theme::kCellSize * 0.46f;

    sf::CircleShape ring(ringRadius);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(2.f);
    ring.setOutlineColor(sf::Color(255, 216, 10, 150));
    ring.setOrigin(ring.getRadius(), ring.getRadius());
    ring.setPosition(center);
    window.draw(ring);

    float rotationDeg = animPhase * 90.f;
    for (int i = 0; i < kParticleCount; ++i) {
        float angleDeg = rotationDeg + i * (360.f / kParticleCount);
        float rad = angleDeg * kPi / 180.f;
        sf::Vector2f pos(center.x + std::cos(rad) * ringRadius, center.y + std::sin(rad) * ringRadius);

        sf::CircleShape particle(2.4f);
        particle.setFillColor(sf::Color(255, 216, 10, 230));
        particle.setOrigin(particle.getRadius(), particle.getRadius());
        particle.setPosition(pos);
        window.draw(particle);
    }
}

// Halo distinct (couleur d'equipe) pour chaque participant a l'incantation,
// reparti en petit cercle autour du centre de la case pour rester lisibles
// meme si les joueurs eux-memes sont dessines superposes sur cette case.
void drawParticipantHalos(sf::RenderWindow &window, sf::Vector2f center,
    const GameState &state, const std::vector<int> &playerIds, float animPhase)
{
    if (playerIds.empty())
        return;

    float radius = render_theme::kCellSize * 0.22f;
    int count = static_cast<int>(playerIds.size());

    for (int i = 0; i < count; ++i) {
        float angleDeg = i * (360.f / count) - 90.f;
        float rad = angleDeg * kPi / 180.f;
        sf::Vector2f pos(center.x + std::cos(rad) * radius, center.y + std::sin(rad) * radius);

        const Player *player = state.findPlayer(playerIds[i]);
        render_theme::RgbColor color = player ? teamColorFor(state, player->team)
                                               : render_theme::RgbColor{255, 255, 255};
        float pulse = 0.6f + 0.4f * std::sin(animPhase * 5.f + static_cast<float>(i));

        sf::CircleShape halo(6.f * pulse);
        halo.setFillColor(sf::Color::Transparent);
        halo.setOutlineThickness(1.5f);
        halo.setOutlineColor(sf::Color(color.r, color.g, color.b, 210));
        halo.setOrigin(halo.getRadius(), halo.getRadius());
        halo.setPosition(pos);
        window.draw(halo);
    }
}

// Resultat "succes" : explosion vert/dore qui s'agrandit en s'estompant sur
// GameState::kIncantationResultHoldSeconds, plus le niveau atteint affiche.
void drawIncantationSuccess(sf::RenderWindow &window, sf::Vector2f center, float t, int reachedLevel)
{
    unsigned char alpha = static_cast<unsigned char>(255.f * (1.f - t));
    float radius = render_theme::kCellSize * (0.3f + 0.9f * t);

    sf::CircleShape burstOuter(radius);
    burstOuter.setFillColor(sf::Color::Transparent);
    burstOuter.setOutlineThickness(3.f);
    burstOuter.setOutlineColor(sf::Color(120, 255, 140, alpha));
    burstOuter.setOrigin(burstOuter.getRadius(), burstOuter.getRadius());
    burstOuter.setPosition(center);
    window.draw(burstOuter);

    sf::CircleShape burstInner(radius * 0.6f);
    burstInner.setFillColor(sf::Color::Transparent);
    burstInner.setOutlineThickness(2.f);
    burstInner.setOutlineColor(sf::Color(255, 216, 10, alpha));
    burstInner.setOrigin(burstInner.getRadius(), burstInner.getRadius());
    burstInner.setPosition(center);
    window.draw(burstInner);

    float riseOffset = t * render_theme::kCellSize * 0.5f;
    sf::Vector2f pipsPos(center.x, center.y - render_theme::kCellSize * 0.62f - riseOffset);
    drawLevelPips(window, pipsPos, reachedLevel, alpha);
}

// Resultat "echec" : flash rouge qui s'estompe puis disparait sur
// GameState::kIncantationResultHoldSeconds.
void drawIncantationFailure(sf::RenderWindow &window, sf::Vector2f center, float t)
{
    unsigned char alpha = static_cast<unsigned char>(200.f * (1.f - t));

    sf::CircleShape flash(render_theme::kCellSize * 0.55f);
    flash.setFillColor(sf::Color(220, 40, 40, alpha));
    flash.setOrigin(flash.getRadius(), flash.getRadius());
    flash.setPosition(center);
    window.draw(flash);

    sf::CircleShape ring(render_theme::kCellSize * (0.3f + 0.5f * t));
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(2.f);
    ring.setOutlineColor(sf::Color(255, 80, 80, alpha));
    ring.setOrigin(ring.getRadius(), ring.getRadius());
    ring.setPosition(center);
    window.draw(ring);
}

// Silhouette de Pac-Man : un disque plein auquel il manque un coin,
// echantillonne de mouthHalfAngleDeg a (360 - mouthHalfAngleDeg). Tourne
// vers la droite (angle 0) par defaut ; setRotation() l'oriente ensuite.
sf::ConvexShape makePacmanShape(float radius, float mouthHalfAngleDeg)
{
    constexpr int segments = 24;
    sf::ConvexShape shape;

    shape.setPointCount(segments + 2);
    shape.setPoint(0, sf::Vector2f(0.f, 0.f));

    float startDeg = mouthHalfAngleDeg;
    float endDeg = 360.f - mouthHalfAngleDeg;
    for (int i = 0; i <= segments; ++i) {
        float t = startDeg + (endDeg - startDeg) * (static_cast<float>(i) / segments);
        float rad = t * kPi / 180.f;
        shape.setPoint(static_cast<std::size_t>(i) + 1,
            sf::Vector2f(std::cos(rad) * radius, std::sin(rad) * radius));
    }
    return shape;
}

// Silhouette de fantome fidele au tout premier aperçu (mockup HTML/SVG
// valide) : dome arrondi en haut, 3 "pieds" en zigzag en bas. Construite
// dans une boite de reference 40x40 (memes proportions que le mockup),
// centree sur (0,0) puis mise a l'echelle par boxSize/40.
sf::ConvexShape makeGhostShape(float boxSize)
{
    constexpr int domeSegments = 12;
    float scale = boxSize / 40.f;
    std::vector<sf::Vector2f> raw;

    raw.push_back(sf::Vector2f(6.f, 36.f));
    for (int i = 0; i <= domeSegments; ++i) {
        float t = 180.f + 180.f * (static_cast<float>(i) / domeSegments);
        float rad = t * kPi / 180.f;
        raw.push_back(sf::Vector2f(20.f + std::cos(rad) * 14.f, 18.f + std::sin(rad) * 14.f));
    }
    raw.push_back(sf::Vector2f(34.f, 36.f));
    raw.push_back(sf::Vector2f(30.f, 32.f));
    raw.push_back(sf::Vector2f(26.f, 36.f));
    raw.push_back(sf::Vector2f(22.f, 32.f));
    raw.push_back(sf::Vector2f(18.f, 36.f));
    raw.push_back(sf::Vector2f(14.f, 32.f));
    raw.push_back(sf::Vector2f(10.f, 36.f));

    sf::ConvexShape shape;
    shape.setPointCount(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i)
        shape.setPoint(i, sf::Vector2f((raw[i].x - 20.f) * scale, (raw[i].y - 20.f) * scale));
    return shape;
}

}

namespace {

sf::ContextSettings makeAntialiasedSettings()
{
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    return settings;
}

}

namespace {

// Aucune police n'est embarquee dans le projet : on tente quelques chemins
// systeme usuels (Linux principalement) et on se contente de ne pas
// afficher de texte si aucun ne fonctionne, plutot que de planter.
bool loadAnySystemFont(sf::Font &font)
{
    static const char *kCandidatePaths[] = {
        "assets/fonts/arial.ttf",
    };

    for (const char *path : kCandidatePaths)
        if (font.loadFromFile(path))
            return true;
    return false;
}

}

namespace {

// Largeur (px) du panneau HUD ajoute a droite de la carte, et hauteur
// minimale de fenetre pour qu'il reste lisible sur une petite carte.
// Limite connue : avec beaucoup d'equipes/joueurs sur une petite carte, le
// contenu du panneau peut quand meme deborder en bas de fenetre (pas de
// scroll ni de redimensionnement dynamique pour l'instant).
constexpr float kHudWidth = 260.f;
constexpr float kHudMinHeight = 500.f;

// Taille maximale de fenetre a l'ouverture (une grande carte ne doit pas
// creer une fenetre plus grande que l'ecran) : la camera (zoom/pan) permet
// ensuite de naviguer dans une carte qui ne tient pas entierement a l'ecran.
constexpr float kMaxWindowWidth = 1280.f;
constexpr float kMaxWindowHeight = 800.f;

constexpr float kMinZoom = 0.2f;
constexpr float kMaxZoom = 5.f;

// Taille totale de fenetre = carte + panneau HUD, plafonnee pour ne pas
// depasser une taille raisonnable meme sur une tres grande carte.
sf::Vector2u computeWindowSize(int mapWidth, int mapHeight)
{
    float mapPxWidth = static_cast<float>(std::max(mapWidth, 1)) * render_theme::kCellSize;
    float mapPxHeight = static_cast<float>(std::max(mapHeight, 1)) * render_theme::kCellSize;

    float width = std::min(mapPxWidth + kHudWidth, kMaxWindowWidth);
    float height = std::min(std::max(mapPxHeight, kHudMinHeight), kMaxWindowHeight);

    return sf::Vector2u(static_cast<unsigned int>(width), static_cast<unsigned int>(height));
}

}

Renderer::Renderer(int initialMapWidth, int initialMapHeight)
    : _window(sf::VideoMode(computeWindowSize(initialMapWidth, initialMapHeight).x,
          computeWindowSize(initialMapWidth, initialMapHeight).y),
          "Zappy GUI", sf::Style::Default, makeAntialiasedSettings()),
      _lastWidth(initialMapWidth),
      _lastHeight(initialMapHeight),
      _animPhase(0.f),
      _hasFont(loadAnySystemFont(_font)),
      _zoomLevel(1.f),
      _panOffset(0.f, 0.f),
      _hasPendingClick(false),
      _pendingClickPx(0, 0),
      _selectedPlayerId(-1),
      _hasSelectedTile(false),
      _selectedTileX(-1),
      _selectedTileY(-1)
{
    _window.setVerticalSyncEnabled(true);
    rebuildGridCache(initialMapWidth, initialMapHeight);
}

bool Renderer::isOpen() const
{
    return _window.isOpen();
}

void Renderer::pollEvents()
{
    sf::Event event;

    while (_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            _window.close();

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            _hasPendingClick = true;
            _pendingClickPx = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
        }

        // La reponse effective au redimensionnement (recalcul du viewport
        // letterboxe pour la carte, et de la vue ecran pour le HUD) se
        // fait chaque frame dans draw(), a partir de _window.getSize()
        // qui reflete deja la nouvelle taille : rien a faire ici.

        if (event.type == sf::Event::MouseWheelScrolled) {
            float factor = event.mouseWheelScroll.delta > 0.f ? 1.1f : (1.f / 1.1f);
            _zoomLevel = std::max(kMinZoom, std::min(_zoomLevel * factor, kMaxZoom));
        }

        if (event.type == sf::Event::KeyPressed) {
            constexpr float kPanStepPx = 40.f;
            if (event.key.code == sf::Keyboard::Left)
                _panOffset.x -= kPanStepPx;
            else if (event.key.code == sf::Keyboard::Right)
                _panOffset.x += kPanStepPx;
            else if (event.key.code == sf::Keyboard::Up)
                _panOffset.y -= kPanStepPx;
            else if (event.key.code == sf::Keyboard::Down)
                _panOffset.y += kPanStepPx;
        }
    }
}

void Renderer::ensureWindowSize(int mapWidth, int mapHeight)
{
    if (mapWidth <= 0 || mapHeight <= 0)
        return;
    if (mapWidth == _lastWidth && mapHeight == _lastHeight)
        return;

    _window.setSize(computeWindowSize(mapWidth, mapHeight));
    _lastWidth = mapWidth;
    _lastHeight = mapHeight;
    // Nouvelle carte (ou premier msz) : repart d'une camera neutre plutot
    // que de garder le zoom/pan de la carte precedente.
    _zoomLevel = 1.f;
    _panOffset = sf::Vector2f(0.f, 0.f);
    rebuildGridCache(mapWidth, mapHeight);
}

void Renderer::rebuildGridCache(int mapWidth, int mapHeight)
{
    mapWidth = std::max(mapWidth, 1);
    mapHeight = std::max(mapHeight, 1);

    float width = static_cast<float>(mapWidth) * render_theme::kCellSize;
    float height = static_cast<float>(mapHeight) * render_theme::kCellSize;
    sf::Color lineColor(30, 34, 84);

    _gridVertices.setPrimitiveType(sf::Lines);
    _gridVertices.clear();

    for (int x = 0; x <= mapWidth; ++x) {
        float px = static_cast<float>(x) * render_theme::kCellSize;
        _gridVertices.append(sf::Vertex(sf::Vector2f(px, 0.f), lineColor));
        _gridVertices.append(sf::Vertex(sf::Vector2f(px, height), lineColor));
    }
    for (int y = 0; y <= mapHeight; ++y) {
        float py = static_cast<float>(y) * render_theme::kCellSize;
        _gridVertices.append(sf::Vertex(sf::Vector2f(0.f, py), lineColor));
        _gridVertices.append(sf::Vertex(sf::Vector2f(width, py), lineColor));
    }
}

void Renderer::updateMapView(int mapWidth, int mapHeight)
{
    sf::Vector2u winSize = _window.getSize();
    float mapAreaWidthPx = std::max(1.f, static_cast<float>(winSize.x) - kHudWidth);
    float mapAreaHeightPx = std::max(1.f, static_cast<float>(winSize.y));

    float mapPxWidth = static_cast<float>(std::max(mapWidth, 1)) * render_theme::kCellSize;
    float mapPxHeight = static_cast<float>(std::max(mapHeight, 1)) * render_theme::kCellSize;

    // Letterbox : la carte garde son ratio d'aspect au lieu d'etre etiree
    // pour remplir toute la zone disponible.
    float fitScale = std::min(mapAreaWidthPx / mapPxWidth, mapAreaHeightPx / mapPxHeight);
    float letterboxWidthPx = mapPxWidth * fitScale;
    float letterboxHeightPx = mapPxHeight * fitScale;
    float letterboxLeftPx = (mapAreaWidthPx - letterboxWidthPx) / 2.f;
    float letterboxTopPx = (mapAreaHeightPx - letterboxHeightPx) / 2.f;

    _mapViewportPx = sf::FloatRect(letterboxLeftPx, letterboxTopPx, letterboxWidthPx, letterboxHeightPx);

    _mapView.setViewport(sf::FloatRect(
        letterboxLeftPx / static_cast<float>(winSize.x),
        letterboxTopPx / static_cast<float>(winSize.y),
        letterboxWidthPx / static_cast<float>(winSize.x),
        letterboxHeightPx / static_cast<float>(winSize.y)));

    _mapView.setSize(mapPxWidth / _zoomLevel, mapPxHeight / _zoomLevel);
    _mapView.setCenter(mapPxWidth / 2.f + _panOffset.x, mapPxHeight / 2.f + _panOffset.y);
}

void Renderer::drawGrid()
{
    // Contenu mis en cache dans _gridVertices par rebuildGridCache() : un
    // seul draw() ici, plutot que 2*(mapWidth+mapHeight+2) formes
    // recreees a chaque frame (cf. rebuildGridCache).
    _window.draw(_gridVertices);
}

void Renderer::drawTiles(const GameState &state)
{
    // Ne parcourt que les tuiles dans le champ de la camera (+1 tuile de
    // marge) plutot que toute la carte : sans effet a l'echelle par
    // defaut (carte entiere visible), mais evite de reconstruire des
    // formes pour des tuiles hors champ des qu'on zoome sur une grande
    // carte.
    float viewLeft = _mapView.getCenter().x - _mapView.getSize().x / 2.f;
    float viewTop = _mapView.getCenter().y - _mapView.getSize().y / 2.f;
    float viewRight = viewLeft + _mapView.getSize().x;
    float viewBottom = viewTop + _mapView.getSize().y;

    int minX = std::max(0, static_cast<int>(std::floor(viewLeft / render_theme::kCellSize)) - 1);
    int minY = std::max(0, static_cast<int>(std::floor(viewTop / render_theme::kCellSize)) - 1);
    int maxX = std::min(state.mapWidth - 1, static_cast<int>(std::ceil(viewRight / render_theme::kCellSize)) + 1);
    int maxY = std::min(state.mapHeight - 1, static_cast<int>(std::ceil(viewBottom / render_theme::kCellSize)) + 1);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const Tile &tile = state.tileAt(x, y);
            render_theme::PixelPos center = render_theme::tileCenterPx(x, y);

            for (int r = 0; r < RESOURCE_COUNT; ++r) {
                if (tile.resources[r] <= 0)
                    continue;

                // Chaque type de ressource a sa propre place dans la tuile
                // (grille 3x3) : sans ca, plusieurs ressources sur la meme
                // tuile se dessinaient exactement les unes sur les autres,
                // et seule la derniere restait visible.
                render_theme::PixelPos slotOffset = render_theme::resourceSlotOffsetPx(r);
                sf::Vector2f slotPos(center.x + slotOffset.x, center.y + slotOffset.y);

                sf::Color resourceColor = toSfColor(render_theme::resourceColor(r));
                float quantityScale = render_theme::resourceQuantityScale(tile.resources[r]);

                if (render_theme::isRareResource(r)) {
                    // pulsation resserree (0.85-1.15) pour ne jamais devenir
                    // trop petite/pale a l'ecran.
                    float pulse = 1.f + 0.15f * std::sin(_animPhase * 4.f);
                    sf::CircleShape power(render_theme::kCellSize * 0.13f * pulse * quantityScale);
                    power.setFillColor(resourceColor);
                    power.setOutlineThickness(1.2f);
                    power.setOutlineColor(sf::Color(255, 255, 255, 180));
                    power.setOrigin(power.getRadius(), power.getRadius());
                    power.setPosition(slotPos);
                    _window.draw(power);
                } else {
                    sf::CircleShape pellet(render_theme::kCellSize * 0.05f * quantityScale);
                    pellet.setFillColor(resourceColor);
                    pellet.setOrigin(pellet.getRadius(), pellet.getRadius());
                    pellet.setPosition(slotPos);
                    _window.draw(pellet);
                }
            }
        }
    }
}

void Renderer::drawIncantations(const GameState &state)
{
    for (const Incantation &incantation : state.incantations) {
        render_theme::PixelPos center = render_theme::tileCenterPx(incantation.x, incantation.y);
        sf::Vector2f centerVec = toSfVec(center);

        if (incantation.active) {
            float pulse = 0.5f + 0.5f * std::sin(_animPhase * 6.f);
            unsigned char peak = static_cast<unsigned char>(110 + 80 * pulse);

            drawGlow(_window, centerVec, render_theme::kCellSize * 0.6f, sf::Color(255, 216, 10), peak);
            drawRitualCircle(_window, centerVec, _animPhase);
            drawParticipantHalos(_window, centerVec, state, incantation.playerIds, _animPhase);
            drawLevelPips(_window, sf::Vector2f(centerVec.x, centerVec.y - render_theme::kCellSize * 0.62f),
                incantation.level, 255);
            continue;
        }

        // Incantation terminee : le resultat reste affiche pendant
        // GameState::kIncantationResultHoldSeconds (voir
        // GameState::advanceIncantationAnimations pour le nettoyage).
        float t = incantation.resultElapsedSeconds / GameState::kIncantationResultHoldSeconds;
        t = std::min(std::max(t, 0.f), 1.f);

        if (incantation.result == 1)
            drawIncantationSuccess(_window, centerVec, t, incantation.level);
        else
            drawIncantationFailure(_window, centerVec, t);
    }
}

void Renderer::drawEggs(const GameState &state)
{
    // Duree de l'animation de ponte/apparition (grossit depuis 0). Le
    // "pop" d'eclosion, lui, dure GameState::kEggHatchHoldSeconds (partage
    // avec la purge de l'etat, pour disparaitre juste apres l'animation).
    constexpr float kSpawnAnimSeconds = 0.35f;

    for (const auto &pair : state.eggs) {
        const Egg &egg = pair.second;

        render_theme::PixelPos center = render_theme::tileCenterPx(egg.x, egg.y);
        float bob = std::sin(_animPhase * 3.f) * render_theme::kCellSize * 0.05f;
        float boxSize = render_theme::kCellSize * 0.62f;
        float alphaRatio = 1.f;

        if (egg.hatched) {
            // Eclosion : un petit "pop" (grossit legerement) puis
            // disparait en fondu, avant d'etre retire de l'etat.
            float hatchT = std::min(egg.hatchedElapsedSeconds / GameState::kEggHatchHoldSeconds, 1.f);
            boxSize *= 1.f + 0.4f * hatchT;
            alphaRatio = 1.f - hatchT;
        } else {
            // Ponte : grossit depuis 0 a l'apparition plutot que de
            // surgir d'un coup a pleine taille.
            float spawnT = std::min(egg.ageSeconds / kSpawnAnimSeconds, 1.f);
            boxSize *= spawnT;
        }

        if (boxSize <= 0.f || alphaRatio <= 0.f)
            continue;

        float scale = boxSize / 40.f;
        sf::Vector2f bodyPos(center.x, center.y + bob);
        unsigned char alpha = static_cast<unsigned char>(255.f * alphaRatio);

        drawGlow(_window, bodyPos, boxSize * 0.85f, sf::Color(255, 92, 179),
            static_cast<unsigned char>(130.f * alphaRatio));

        sf::ConvexShape ghost = makeGhostShape(boxSize);
        ghost.setFillColor(sf::Color(255, 92, 179, alpha));
        ghost.setPosition(bodyPos);
        _window.draw(ghost);

        for (float side : {-1.f, 1.f}) {
            sf::Vector2f eyePos(bodyPos.x + side * 6.f * scale, bodyPos.y - 1.f * scale);

            sf::CircleShape eye(4.2f * scale);
            eye.setFillColor(sf::Color(255, 255, 255, alpha));
            eye.setOrigin(eye.getRadius(), eye.getRadius());
            eye.setPosition(eyePos);
            _window.draw(eye);

            sf::CircleShape pupil(2.f * scale);
            pupil.setFillColor(sf::Color(26, 26, 46, alpha));
            pupil.setOrigin(pupil.getRadius(), pupil.getRadius());
            pupil.setPosition(eyePos.x + side * 1.3f * scale, eyePos.y);
            _window.draw(pupil);
        }
    }
}

void Renderer::drawEffects(const GameState &state)
{
    for (const TransientEffect &effect : state.effects) {
        float t = std::min(effect.elapsedSeconds / effect.durationSeconds, 1.f);
        render_theme::PixelPos center = render_theme::tileCenterPx(effect.x, effect.y);
        unsigned char alpha = static_cast<unsigned char>(255.f * (1.f - t));

        switch (effect.kind) {
        case EffectKind::ResourcePickup:
        case EffectKind::ResourceDrop: {
            // Remonte vers le joueur pour un ramassage, descend vers le
            // sol pour un depot.
            render_theme::RgbColor color = render_theme::resourceColor(effect.resourceIndex);
            float direction = effect.kind == EffectKind::ResourcePickup ? -1.f : 1.f;
            float offset = direction * t * render_theme::kCellSize * 0.5f;

            sf::CircleShape dot(4.f * (1.f - 0.3f * t));
            dot.setFillColor(sf::Color(color.r, color.g, color.b, alpha));
            dot.setOrigin(dot.getRadius(), dot.getRadius());
            dot.setPosition(center.x, center.y + offset);
            _window.draw(dot);
            break;
        }
        case EffectKind::Expulsion: {
            sf::CircleShape ring(render_theme::kCellSize * (0.3f + 0.8f * t));
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(3.f);
            ring.setOutlineColor(sf::Color(255, 140, 40, alpha));
            ring.setOrigin(ring.getRadius(), ring.getRadius());
            ring.setPosition(toSfVec(center));
            _window.draw(ring);
            break;
        }
        case EffectKind::Broadcast: {
            // Deux anneaux "onde sonore" decales dans le temps.
            for (int i = 0; i < 2; ++i) {
                float ringT = std::min(t + static_cast<float>(i) * 0.25f, 1.f);
                unsigned char ringAlpha = static_cast<unsigned char>(180.f * (1.f - ringT));

                sf::CircleShape ring(render_theme::kCellSize * (0.4f + 1.4f * ringT));
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineThickness(2.f);
                ring.setOutlineColor(sf::Color(120, 220, 255, ringAlpha));
                ring.setOrigin(ring.getRadius(), ring.getRadius());
                ring.setPosition(toSfVec(center));
                _window.draw(ring);
            }
            break;
        }
        case EffectKind::EggLaying: {
            sf::CircleShape pulse(render_theme::kCellSize * (0.15f + 0.25f * t));
            pulse.setFillColor(sf::Color(255, 92, 179, alpha));
            pulse.setOrigin(pulse.getRadius(), pulse.getRadius());
            pulse.setPosition(center.x, center.y + render_theme::kCellSize * 0.2f);
            _window.draw(pulse);
            break;
        }
        case EffectKind::Death: {
            sf::CircleShape fade(render_theme::kCellSize * 0.32f * (1.f - 0.6f * t));
            fade.setFillColor(sf::Color(120, 20, 20, alpha));
            fade.setOutlineThickness(2.f);
            fade.setOutlineColor(sf::Color(200, 40, 40, alpha));
            fade.setOrigin(fade.getRadius(), fade.getRadius());
            fade.setPosition(toSfVec(center));
            _window.draw(fade);
            break;
        }
        case EffectKind::LevelUp: {
            sf::CircleShape burst(render_theme::kCellSize * (0.25f + 0.5f * t));
            burst.setFillColor(sf::Color::Transparent);
            burst.setOutlineThickness(2.5f);
            burst.setOutlineColor(sf::Color(120, 255, 140, alpha));
            burst.setOrigin(burst.getRadius(), burst.getRadius());
            burst.setPosition(toSfVec(center));
            _window.draw(burst);

            if (effect.level > 0) {
                sf::Vector2f pipsPos(center.x, center.y - render_theme::kCellSize * 0.55f - t * 10.f);
                drawLevelPips(_window, pipsPos, effect.level, alpha);
            }
            break;
        }
        }
    }
}

void Renderer::drawPlayers(const GameState &state)
{
    float mouthHalf = render_theme::mouthHalfAngleDeg(_animPhase);

    // Regroupe les joueurs vivants par tuile visuelle (position animee
    // arrondie a la case la plus proche) : sans ca, plusieurs joueurs sur
    // la meme case se dessinaient exactement les uns sur les autres.
    std::map<std::pair<int, int>, std::vector<const Player *>> clusters;
    for (const auto &pair : state.players) {
        const Player &player = pair.second;
        if (!player.alive)
            continue;
        int tileX = static_cast<int>(std::lround(player.animX));
        int tileY = static_cast<int>(std::lround(player.animY));
        clusters[{tileX, tileY}].push_back(&player);
    }

    for (const auto &clusterEntry : clusters) {
        const std::vector<const Player *> &group = clusterEntry.second;
        std::size_t count = group.size();

        // Centre la case sur la position moyenne du groupe plutot que sur
        // la case arrondie : des joueurs qui convergent vers la meme case
        // depuis des directions differentes se regroupent en douceur au
        // lieu de sauter d'un coup a la position finale.
        float cx = 0.f, cy = 0.f;
        for (const Player *player : group) {
            cx += player->animX;
            cy += player->animY;
        }
        cx /= static_cast<float>(count);
        cy /= static_cast<float>(count);
        sf::Vector2f clusterCenter = toSfVec(render_theme::tileCenterPxF(cx, cy));

        // Reduit la taille et etale les joueurs en cercle quand ils sont
        // plusieurs sur la meme case, pour qu'ils restent distinguables
        // au lieu de se superposer.
        float scale = count == 1 ? 1.f : std::max(0.45f, 1.f - 0.12f * static_cast<float>(count - 1));
        float radius = render_theme::kCellSize * 0.32f * scale;
        float spreadRadius = count == 1 ? 0.f : render_theme::kCellSize * 0.24f;

        for (std::size_t i = 0; i < count; ++i) {
            const Player &player = *group[i];
            sf::Vector2f center = clusterCenter;

            if (count > 1) {
                float angleDeg = static_cast<float>(i) * (360.f / static_cast<float>(count)) - 90.f;
                float rad = angleDeg * kPi / 180.f;
                center.x += std::cos(rad) * spreadRadius;
                center.y += std::sin(rad) * spreadRadius;
            }

            render_theme::RgbColor team = teamColorFor(state, player.team);

            drawGlow(_window, center, radius * 2.2f, sf::Color(255, 216, 10), count == 1 ? 150 : 90);

            sf::CircleShape ring(radius + 2.5f * scale);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(2.5f * scale);
            ring.setOutlineColor(toSfColor(team));
            ring.setOrigin(ring.getRadius(), ring.getRadius());
            ring.setPosition(center);
            _window.draw(ring);

            sf::ConvexShape pacman = makePacmanShape(radius, mouthHalf);
            pacman.setFillColor(sf::Color(255, 216, 10));
            pacman.setOutlineThickness(1.5f);
            pacman.setOutlineColor(sf::Color(120, 90, 0));
            pacman.setPosition(center);
            pacman.setRotation(render_theme::orientationAngleDeg(static_cast<int>(player.orientation)));
            _window.draw(pacman);
        }
    }
}

void Renderer::updatePlayerAnimations(GameState &state, float dt)
{
    // Duree de lissage : plus petit = rattrape la position reelle plus
    // vite. 0.12s glisse visiblement d'une case a l'autre sans jamais
    // accumuler de retard perceptible sur des deplacements a la frequence
    // normale du serveur (1 case par tick).
    constexpr float kMoveSmoothingTau = 0.12f;

    if (state.mapWidth <= 0 || state.mapHeight <= 0 || dt <= 0.f)
        return;

    float factor = 1.f - std::exp(-dt / kMoveSmoothingTau);

    for (auto &pair : state.players) {
        Player &player = pair.second;
        float dx = render_theme::shortestWrappedDelta(player.animX, player.x, state.mapWidth);
        float dy = render_theme::shortestWrappedDelta(player.animY, player.y, state.mapHeight);

        player.animX = render_theme::wrapTileCoord(player.animX + dx * factor, state.mapWidth);
        player.animY = render_theme::wrapTileCoord(player.animY + dy * factor, state.mapHeight);
    }
}

void Renderer::drawGameOverBanner(const GameState &state)
{
    if (!state.gameOver)
        return;

    sf::Vector2u size = _window.getSize();
    sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
    overlay.setFillColor(sf::Color(0, 0, 0, 180));
    _window.draw(overlay);

    render_theme::RgbColor winnerColor = teamColorFor(state, state.winningTeam);
    sf::Vector2f center(static_cast<float>(size.x) / 2.f, static_cast<float>(size.y) / 2.f);

    float pulse = 0.85f + 0.15f * std::sin(_animPhase * 3.f);
    sf::CircleShape banner(std::min(size.x, size.y) * 0.28f * pulse);
    banner.setFillColor(sf::Color::Transparent);
    banner.setOutlineThickness(4.f);
    banner.setOutlineColor(toSfColor(winnerColor));
    banner.setOrigin(banner.getRadius(), banner.getRadius());
    banner.setPosition(center);
    _window.draw(banner);

    if (!_hasFont)
        return;

    sf::Text title("VICTOIRE", _font, 28);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(sf::Color::White);
    sf::FloatRect titleBounds = title.getLocalBounds();
    title.setOrigin(titleBounds.left + titleBounds.width / 2.f, titleBounds.top + titleBounds.height / 2.f);
    title.setPosition(center.x, center.y - 22.f);
    _window.draw(title);

    sf::Text teamText("Equipe " + state.winningTeam, _font, 18);
    teamText.setFillColor(toSfColor(winnerColor));
    sf::FloatRect teamBounds = teamText.getLocalBounds();
    teamText.setOrigin(teamBounds.left + teamBounds.width / 2.f, teamBounds.top + teamBounds.height / 2.f);
    teamText.setPosition(center.x, center.y + 14.f);
    _window.draw(teamText);
}

void Renderer::processPendingClick(const GameState &state)
{
    if (!_hasPendingClick)
        return;
    _hasPendingClick = false;

    // Clic hors du rectangle ecran occupe par la vue carte (panneau HUD,
    // ou bande noire du letterbox) : rien a selectionner.
    if (!_mapViewportPx.contains(static_cast<float>(_pendingClickPx.x), static_cast<float>(_pendingClickPx.y)))
        return;

    // Convertit le pixel ecran en coordonnees "carte" (en tenant compte du
    // zoom/pan courants de _mapView), pour un hit-test correct meme
    // lorsque la camera n'est pas a son etat par defaut.
    sf::Vector2f mapPos = _window.mapPixelToCoords(_pendingClickPx, _mapView);
    float clickX = mapPos.x;
    float clickY = mapPos.y;

    // Joueur le plus proche du clic, dans un rayon raisonnable (permet de
    // cliquer un joueur meme sans viser exactement son centre).
    float pickRadius = render_theme::kCellSize * 0.5f;
    float bestDistSq = pickRadius * pickRadius;
    int bestId = -1;

    for (const auto &pair : state.players) {
        const Player &player = pair.second;
        if (!player.alive)
            continue;
        render_theme::PixelPos center = render_theme::tileCenterPxF(player.animX, player.animY);
        float dx = center.x - clickX;
        float dy = center.y - clickY;
        float distSq = dx * dx + dy * dy;
        if (distSq <= bestDistSq) {
            bestDistSq = distSq;
            bestId = player.id;
        }
    }

    if (bestId >= 0) {
        _selectedPlayerId = bestId;
        _hasSelectedTile = false;
        return;
    }

    int tileX = static_cast<int>(clickX / render_theme::kCellSize);
    int tileY = static_cast<int>(clickY / render_theme::kCellSize);
    if (tileX >= 0 && tileX < state.mapWidth && tileY >= 0 && tileY < state.mapHeight) {
        _selectedTileX = tileX;
        _selectedTileY = tileY;
        _hasSelectedTile = true;
        _selectedPlayerId = -1;
    }
}

void Renderer::drawText(const std::string &text, sf::Vector2f pos, unsigned int charSize,
    sf::Color color, bool bold)
{
    if (!_hasFont)
        return;

    sf::Text sfText(text, _font, charSize);
    if (bold)
        sfText.setStyle(sf::Text::Bold);
    sfText.setFillColor(color);
    sfText.setPosition(pos);
    _window.draw(sfText);
}

void Renderer::drawHud(const GameState &state)
{
    static const char *kResourceNames[RESOURCE_COUNT] = {
        "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"
    };

    sf::Vector2u winSize = _window.getSize();
    // La zone carte passe par une camera (zoom/pan/letterbox) depuis
    // updateMapView() : le panneau HUD occupe toujours exactement
    // kHudWidth a droite de la fenetre, quelle que soit la taille reelle
    // (en pixels carte) de la carte affichee.
    float hudPanelX = std::max(0.f, static_cast<float>(winSize.x) - kHudWidth);
    float panelWidth = static_cast<float>(winSize.x) - hudPanelX;

    sf::RectangleShape panel(sf::Vector2f(panelWidth, static_cast<float>(winSize.y)));
    panel.setPosition(hudPanelX, 0.f);
    panel.setFillColor(sf::Color(18, 18, 30, 255));
    _window.draw(panel);

    if (!_hasFont)
        return;

    const float padding = 12.f;
    float x = hudPanelX + padding;
    float y = padding;

    drawText("EQUIPES", sf::Vector2f(x, y), 14, sf::Color(180, 180, 200), true);
    y += 20.f;

    for (const auto &teamPair : state.teams) {
        const Team &team = teamPair.second;
        render_theme::RgbColor teamRgb = render_theme::teamColorByIndex(team.colorIndex);

        std::size_t aliveCount = 0;
        for (int pid : team.playerIds) {
            const Player *player = state.findPlayer(pid);
            if (player && player->alive)
                ++aliveCount;
        }

        sf::RectangleShape swatch(sf::Vector2f(10.f, 10.f));
        swatch.setFillColor(toSfColor(teamRgb));
        swatch.setPosition(x, y + 3.f);
        _window.draw(swatch);

        drawText(team.name + "  " + std::to_string(aliveCount) + "/"
            + std::to_string(team.spawnedCount) + " vivant(s)",
            sf::Vector2f(x + 16.f, y), 13, sf::Color::White);
        y += 18.f;

        // Liste id/niveau des joueurs vivants, plafonnee pour ne pas
        // deborder du panneau sur une grosse partie.
        constexpr std::size_t kMaxListedPlayers = 6;
        std::size_t listed = 0;
        for (int pid : team.playerIds) {
            const Player *player = state.findPlayer(pid);
            if (!player || !player->alive)
                continue;
            if (listed >= kMaxListedPlayers) {
                drawText("  ...", sf::Vector2f(x + 16.f, y), 12, sf::Color(150, 150, 160));
                y += 15.f;
                break;
            }
            drawText("  #" + std::to_string(player->id) + "  Lv" + std::to_string(player->level),
                sf::Vector2f(x + 16.f, y), 12, sf::Color(190, 190, 200));
            y += 15.f;
            ++listed;
        }
        y += 8.f;
    }

    y += 6.f;
    drawText("Frequence: " + std::to_string(state.timeUnit), sf::Vector2f(x, y), 13, sf::Color(200, 200, 210));
    y += 24.f;

    drawText("RESSOURCES", sf::Vector2f(x, y), 14, sf::Color(180, 180, 200), true);
    y += 20.f;
    for (int r = 0; r < RESOURCE_COUNT; ++r) {
        sf::CircleShape dot(5.f);
        dot.setFillColor(toSfColor(render_theme::resourceColor(r)));
        dot.setOrigin(dot.getRadius(), dot.getRadius());
        dot.setPosition(x + 5.f, y + 6.f);
        _window.draw(dot);
        drawText(kResourceNames[r], sf::Vector2f(x + 16.f, y), 12, sf::Color(190, 190, 200));
        y += 16.f;
    }
    y += 10.f;

    const Player *selectedPlayer = _selectedPlayerId >= 0 ? state.findPlayer(_selectedPlayerId) : nullptr;

    if (selectedPlayer && selectedPlayer->alive) {
        drawText("JOUEUR #" + std::to_string(selectedPlayer->id), sf::Vector2f(x, y), 14,
            sf::Color(255, 216, 10), true);
        y += 20.f;
        drawText("Equipe: " + selectedPlayer->team, sf::Vector2f(x, y), 12, sf::Color(200, 200, 210));
        y += 16.f;
        drawText("Niveau: " + std::to_string(selectedPlayer->level), sf::Vector2f(x, y), 12,
            sf::Color(200, 200, 210));
        y += 18.f;
        drawText("Inventaire:", sf::Vector2f(x, y), 12, sf::Color(180, 180, 200));
        y += 16.f;
        for (int r = 0; r < RESOURCE_COUNT; ++r) {
            sf::CircleShape dot(4.f);
            dot.setFillColor(toSfColor(render_theme::resourceColor(r)));
            dot.setOrigin(dot.getRadius(), dot.getRadius());
            dot.setPosition(x + 9.f, y + 6.f);
            _window.draw(dot);
            drawText(std::string(kResourceNames[r]) + ": " + std::to_string(selectedPlayer->inventory[r]),
                sf::Vector2f(x + 18.f, y), 12, sf::Color(190, 190, 200));
            y += 15.f;
        }
    } else if (_hasSelectedTile && _selectedTileX < state.mapWidth && _selectedTileY < state.mapHeight) {
        drawText("CASE (" + std::to_string(_selectedTileX) + "," + std::to_string(_selectedTileY) + ")",
            sf::Vector2f(x, y), 14, sf::Color(255, 216, 10), true);
        y += 20.f;
        const Tile &tile = state.tileAt(_selectedTileX, _selectedTileY);
        bool anyResource = false;
        for (int r = 0; r < RESOURCE_COUNT; ++r) {
            if (tile.resources[r] <= 0)
                continue;
            anyResource = true;
            sf::CircleShape dot(4.f);
            dot.setFillColor(toSfColor(render_theme::resourceColor(r)));
            dot.setOrigin(dot.getRadius(), dot.getRadius());
            dot.setPosition(x + 9.f, y + 6.f);
            _window.draw(dot);
            drawText(std::string(kResourceNames[r]) + ": " + std::to_string(tile.resources[r]),
                sf::Vector2f(x + 18.f, y), 12, sf::Color(190, 190, 200));
            y += 15.f;
        }
        if (!anyResource)
            drawText("(vide)", sf::Vector2f(x, y), 12, sf::Color(140, 140, 150));
    } else {
        drawText("Clique sur un joueur", sf::Vector2f(x, y), 12, sf::Color(140, 140, 150));
        y += 15.f;
        drawText("ou une case pour plus", sf::Vector2f(x, y), 12, sf::Color(140, 140, 150));
        y += 15.f;
        drawText("de details.", sf::Vector2f(x, y), 12, sf::Color(140, 140, 150));
    }
}

void Renderer::drawMessageLog(const GameState &state)
{
    if (!_hasFont || state.logMessages.empty())
        return;

    sf::Vector2u winSize = _window.getSize();
    float mapAreaWidth = std::max(1.f, static_cast<float>(winSize.x) - kHudWidth);
    const float padding = 8.f;
    const float lineHeight = 16.f;
    float panelHeight = static_cast<float>(state.logMessages.size()) * lineHeight + padding * 2.f;
    float panelY = static_cast<float>(winSize.y) - panelHeight - padding;

    sf::RectangleShape panel(sf::Vector2f(mapAreaWidth - padding * 2.f, panelHeight));
    panel.setPosition(padding, panelY);
    panel.setFillColor(sf::Color(10, 10, 18, 190));
    _window.draw(panel);

    float y = panelY + padding;
    for (const LogMessage &message : state.logMessages) {
        // S'estompe sur la derniere seconde avant expiration plutot que
        // de disparaitre d'un coup.
        float remaining = message.durationSeconds - message.elapsedSeconds;
        float fadeT = std::min(std::max(remaining, 0.f), 1.f);
        unsigned char alpha = static_cast<unsigned char>(255.f * fadeT);

        drawText(message.text, sf::Vector2f(padding * 2.f, y), 12, sf::Color(220, 220, 230, alpha));
        y += lineHeight;
    }
}

void Renderer::draw(GameState &state, float dt)
{
    _animPhase += dt;
    state.advanceIncantationAnimations(dt);
    state.advanceEggAnimations(dt);
    state.advanceEffects(dt);
    state.advanceLogMessages(dt);
    updatePlayerAnimations(state, dt);
    ensureWindowSize(state.mapWidth, state.mapHeight);
    updateMapView(state.mapWidth, state.mapHeight);
    processPendingClick(state);

    _window.clear(sf::Color::Black);

    // Zone carte : passe par la camera (zoom/pan/letterbox).
    _window.setView(_mapView);
    drawGrid();
    drawTiles(state);
    drawIncantations(state);
    drawEggs(state);
    drawPlayers(state);
    drawEffects(state);

    // HUD et bandeau de fin : coordonnees ecran fixes, jamais affectees
    // par le zoom/pan de la carte.
    // NB: on reconstruit la vue depuis la taille actuelle de la fenetre
    // plutot que d'utiliser _window.getDefaultView(), qui reste figee sur
    // la taille de la fenetre au moment de sa creation et ne suit pas les
    // redimensionnements (ex: passage en plein ecran) : avec l'ancienne
    // vue par defaut perimee, le HUD (dessine en coordonnees ecran issues
    // de la taille reelle et a jour de la fenetre) se retrouvait hors du
    // rectangle visible et disparaissait.
    sf::Vector2u screenSize = _window.getSize();
    _window.setView(sf::View(sf::FloatRect(0.f, 0.f,
        static_cast<float>(screenSize.x), static_cast<float>(screenSize.y))));
    drawHud(state);
    drawMessageLog(state);
    drawGameOverBanner(state);
    _window.display();
}
