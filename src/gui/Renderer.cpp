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
        render_theme::RgbColor color = player ? render_theme::teamColor(player->team)
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

Renderer::Renderer(int initialMapWidth, int initialMapHeight)
    : _window(sf::VideoMode(
          static_cast<unsigned int>(std::max(initialMapWidth, 1) * render_theme::kCellSize),
          static_cast<unsigned int>(std::max(initialMapHeight, 1) * render_theme::kCellSize)),
          "Zappy GUI", sf::Style::Default, makeAntialiasedSettings()),
      _lastWidth(initialMapWidth),
      _lastHeight(initialMapHeight),
      _animPhase(0.f)
{
    _window.setVerticalSyncEnabled(true);
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
    }
}

void Renderer::ensureWindowSize(int mapWidth, int mapHeight)
{
    if (mapWidth <= 0 || mapHeight <= 0)
        return;
    if (mapWidth == _lastWidth && mapHeight == _lastHeight)
        return;

    _window.setSize(sf::Vector2u(
        static_cast<unsigned int>(mapWidth * render_theme::kCellSize),
        static_cast<unsigned int>(mapHeight * render_theme::kCellSize)));
    _lastWidth = mapWidth;
    _lastHeight = mapHeight;
}

void Renderer::drawGrid(int mapWidth, int mapHeight)
{
    sf::RectangleShape line;
    line.setFillColor(sf::Color(30, 34, 84));

    float width = static_cast<float>(mapWidth) * render_theme::kCellSize;
    float height = static_cast<float>(mapHeight) * render_theme::kCellSize;

    for (int x = 0; x <= mapWidth; ++x) {
        line.setSize(sf::Vector2f(1.f, height));
        line.setPosition(x * render_theme::kCellSize, 0.f);
        _window.draw(line);
    }
    for (int y = 0; y <= mapHeight; ++y) {
        line.setSize(sf::Vector2f(width, 1.f));
        line.setPosition(0.f, y * render_theme::kCellSize);
        _window.draw(line);
    }
}

void Renderer::drawTiles(const GameState &state)
{
    for (int y = 0; y < state.mapHeight; ++y) {
        for (int x = 0; x < state.mapWidth; ++x) {
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

                if (render_theme::isRareResource(r)) {
                    // pulsation resserree (0.85-1.15) pour ne jamais devenir
                    // trop petite/pale a l'ecran.
                    float pulse = 1.f + 0.15f * std::sin(_animPhase * 4.f);
                    sf::CircleShape power(render_theme::kCellSize * 0.13f * pulse);
                    power.setFillColor(resourceColor);
                    power.setOutlineThickness(1.2f);
                    power.setOutlineColor(sf::Color(255, 255, 255, 180));
                    power.setOrigin(power.getRadius(), power.getRadius());
                    power.setPosition(slotPos);
                    _window.draw(power);
                } else {
                    sf::CircleShape pellet(render_theme::kCellSize * 0.05f);
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
    for (const auto &pair : state.eggs) {
        const Egg &egg = pair.second;
        if (egg.hatched)
            continue;

        render_theme::PixelPos center = render_theme::tileCenterPx(egg.x, egg.y);
        float bob = std::sin(_animPhase * 3.f) * render_theme::kCellSize * 0.05f;
        float boxSize = render_theme::kCellSize * 0.62f;
        float scale = boxSize / 40.f;
        sf::Vector2f bodyPos(center.x, center.y + bob);

        drawGlow(_window, bodyPos, boxSize * 0.85f, sf::Color(255, 92, 179), 130);

        sf::ConvexShape ghost = makeGhostShape(boxSize);
        ghost.setFillColor(sf::Color(255, 92, 179));
        ghost.setPosition(bodyPos);
        _window.draw(ghost);

        for (float side : {-1.f, 1.f}) {
            sf::Vector2f eyePos(bodyPos.x + side * 6.f * scale, bodyPos.y - 1.f * scale);

            sf::CircleShape eye(4.2f * scale);
            eye.setFillColor(sf::Color::White);
            eye.setOrigin(eye.getRadius(), eye.getRadius());
            eye.setPosition(eyePos);
            _window.draw(eye);

            sf::CircleShape pupil(2.f * scale);
            pupil.setFillColor(sf::Color(26, 26, 46));
            pupil.setOrigin(pupil.getRadius(), pupil.getRadius());
            pupil.setPosition(eyePos.x + side * 1.3f * scale, eyePos.y);
            _window.draw(pupil);
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

            render_theme::RgbColor team = render_theme::teamColor(player.team);

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

void Renderer::draw(GameState &state, float dt)
{
    _animPhase += dt;
    state.advanceIncantationAnimations(dt);
    updatePlayerAnimations(state, dt);
    ensureWindowSize(state.mapWidth, state.mapHeight);

    _window.clear(sf::Color::Black);
    drawGrid(state.mapWidth, state.mapHeight);
    drawTiles(state);
    drawIncantations(state);
    drawEggs(state);
    drawPlayers(state);
    _window.display();
}
