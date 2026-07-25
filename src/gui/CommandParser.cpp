#include "CommandParser.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>

CommandParser::CommandParser(GameState &state) : _state(state)
{
    registerHandlers();
}

void CommandParser::registerHandlers()
{
    _handlers["msz"] = [this](std::istringstream &str) { handleMsz(str); };
    _handlers["bct"] = [this](std::istringstream &str) { handleBct(str); };
    _handlers["tna"] = [this](std::istringstream &str) { handleTna(str); };
    _handlers["pnw"] = [this](std::istringstream &str) { handlePnw(str); };
    _handlers["ppo"] = [this](std::istringstream &str) { handlePpo(str); };
    _handlers["plv"] = [this](std::istringstream &str) { handlePlv(str); };
    _handlers["pin"] = [this](std::istringstream &str) { handlePin(str); };
    _handlers["pex"] = [this](std::istringstream &str) { handlePex(str); };
    _handlers["pbc"] = [this](std::istringstream &str) { handlePbc(str); };
    _handlers["pic"] = [this](std::istringstream &str) { handlePic(str); };
    _handlers["pie"] = [this](std::istringstream &str) { handlePie(str); };
    _handlers["pfk"] = [this](std::istringstream &str) { handlePfk(str); };
    _handlers["pdr"] = [this](std::istringstream &str) { handlePdr(str); };
    _handlers["pgt"] = [this](std::istringstream &str) { handlePgt(str); };
    _handlers["pdi"] = [this](std::istringstream &str) { handlePdi(str); };
    _handlers["enw"] = [this](std::istringstream &str) { handleEnw(str); };
    _handlers["ebo"] = [this](std::istringstream &str) { handleEbo(str); };
    _handlers["edi"] = [this](std::istringstream &str) { handleEdi(str); };
    _handlers["sgt"] = [this](std::istringstream &str) { handleSgt(str); };
    _handlers["sst"] = [this](std::istringstream &str) { handleSst(str); };
    _handlers["seg"] = [this](std::istringstream &str) { handleSeg(str); };
    _handlers["smg"] = [this](std::istringstream &str) { handleSmg(str); };
    _handlers["suc"] = [this](std::istringstream &str) { handleSuc(str); };
    _handlers["sbp"] = [this](std::istringstream &str) { handleSbp(str); };
}

namespace {
constexpr int kMaxMapDimension = 10000;
}

int CommandParser::parseId(const std::string &token)
{
    if (token.size() < 2 || token[0] != '#')
        return -1;
    for (std::size_t i = 1; i < token.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(token[i])))
            return -1;
    try
    {
        return std::stoi(token.substr(1));
    }
    catch (const std::exception &)
    {
        return -1;
    }
}

bool CommandParser::requireCompleteLine(std::istringstream &args, const char *cmdName)
{
    if (args.fail())
    {
        std::cerr << "[parser] " << cmdName << ": ligne incomplete ou malformee, ignoree" << std::endl;
        return false;
    }
    return true;
}

bool CommandParser::isValidOrientation(int o)
{
    return o >= 1 && o <= 4;
}

bool CommandParser::isValidLevel(int level)
{
    return level >= 1 && level <= 8;
}

bool CommandParser::isInMapBounds(int x, int y) const
{
    return x >= 0 && y >= 0 && x < _state.mapWidth && y < _state.mapHeight;
}

void CommandParser::removeEggIdFromOwningTeam(const Egg &egg)
{
    Player *parent = egg.playerId >= 0 ? _state.findPlayer(egg.playerId) : nullptr;
    if (!parent)
        return;

    auto teamIt = _state.teams.find(parent->team);
    if (teamIt == _state.teams.end())
        return;

    auto &eggIds = teamIt->second.eggIds;
    eggIds.erase(std::remove(eggIds.begin(), eggIds.end(), egg.id), eggIds.end());
}

void CommandParser::removePlayerIdFromTeam(const std::string &teamName, int playerId)
{
    auto teamIt = _state.teams.find(teamName);
    if (teamIt == _state.teams.end())
        return;

    auto &ids = teamIt->second.playerIds;
    ids.erase(std::remove(ids.begin(), ids.end(), playerId), ids.end());
}

void CommandParser::dispatch(const std::string &line)
{
    if (line.empty())
        return;

    std::istringstream lineStream(line);
    std::string code;
    lineStream >> code;

    if (code.empty())
        return;

    auto it = _handlers.find(code);
    if (it == _handlers.end())
    {
        std::cerr << "[parser] commande inconnue ignoree: \"" << line << "\"" << std::endl;
        return;
    }

    try
    {
        it->second(lineStream);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[parser] erreur de parsing sur \"" << line << "\": " << e.what() << std::endl;
    }
}

void CommandParser::handleMsz(std::istringstream &args)
{
    int x, y;
    args >> x >> y;
    if (!requireCompleteLine(args, "msz"))
        return;
    if (x <= 0 || y <= 0 || x > kMaxMapDimension || y > kMaxMapDimension)
    {
        std::cerr << "[parser] msz: dimensions de map invalides (" << x << "x" << y << ")" << std::endl;
        return;
    }

    _state.resizeMap(x, y);
    std::cout << "[parser] map size: " << x << "x" << y << std::endl;
}

void CommandParser::handleBct(std::istringstream &args)
{
    int x, y;
    args >> x >> y;
    if (!requireCompleteLine(args, "bct") || !isInMapBounds(x, y))
    {
        std::cerr << "[parser] bct: coordonnees hors map ou ligne invalide" << std::endl;
        return;
    }

    int quantities[RESOURCE_COUNT];
    for (int i = 0; i < RESOURCE_COUNT; ++i)
        args >> quantities[i];
    if (!requireCompleteLine(args, "bct"))
        return;
    for (int i = 0; i < RESOURCE_COUNT; ++i)
        if (quantities[i] < 0)
        {
            std::cerr << "[parser] bct: quantite negative ignoree" << std::endl;
            return;
        }

    Tile &tile = _state.tileAt(x, y);
    for (int i = 0; i < RESOURCE_COUNT; ++i)
        tile.resources[i] = quantities[i];
}

void CommandParser::handleTna(std::istringstream &args)
{
    std::string name;
    args >> name;
    if (name.empty())
        return;

    _state.teams.emplace(name, Team{name, {}, {}});
    std::cout << "[parser] equipe: " << name << std::endl;
}

void CommandParser::handlePnw(std::istringstream &args)
{
    std::string idToken;
    int x, y, o, level;
    std::string team;

    args >> idToken >> x >> y >> o >> level >> team;
    int id = parseId(idToken);
    if (!requireCompleteLine(args, "pnw") || id < 0 || !isValidOrientation(o) || !isValidLevel(level))
    {
        std::cerr << "[parser] pnw: ligne invalide ignoree" << std::endl;
        return;
    }

    Player player;
    player.id = id;
    player.x = x;
    player.y = y;
    player.orientation = static_cast<Orientation>(o);
    player.level = level;
    player.team = team;
    player.alive = true;
    // Pas d'animation de deplacement a l'apparition : evite de le voir
    // glisser depuis (0, 0) au moment ou il rejoint la partie.
    player.animX = static_cast<float>(x);
    player.animY = static_cast<float>(y);

    _state.players[id] = player;

    auto teamIt = _state.teams.find(team);
    if (teamIt != _state.teams.end())
    {
        auto &ids = teamIt->second.playerIds;
        // pnw peut arriver deux fois pour le meme id (snapshot renvoye,
        // ligne dupliquee...) : ne pas dupliquer dans la liste de l'equipe,
        // ni recompter un spawn qui a deja ete compte.
        if (std::find(ids.begin(), ids.end(), id) == ids.end())
        {
            ids.push_back(id);
            teamIt->second.spawnedCount++;
        }
    }

    std::cout << "[parser] nouveau joueur #" << id << " (" << team << ") en (" << x << "," << y << ")" << std::endl;
}

void CommandParser::handlePpo(std::istringstream &args)
{
    std::string idToken;
    int x, y, o;

    args >> idToken >> x >> y >> o;
    int id = parseId(idToken);
    if (!requireCompleteLine(args, "ppo") || id < 0 || !isValidOrientation(o))
    {
        std::cerr << "[parser] ppo: ligne invalide ignoree" << std::endl;
        return;
    }

    Player *player = _state.findPlayer(id);
    if (!player)
    {
        std::cerr << "[parser] ppo: joueur #" << id << " inconnu" << std::endl;
        return;
    }

    player->x = x;
    player->y = y;
    player->orientation = static_cast<Orientation>(o);
}

void CommandParser::handlePlv(std::istringstream &args)
{
    std::string idToken;
    int level;

    args >> idToken >> level;
    int id = parseId(idToken);
    if (!requireCompleteLine(args, "plv") || id < 0 || !isValidLevel(level))
        return;

    Player *player = _state.findPlayer(id);
    if (!player)
        return;

    player->level = level;
    _state.addEffect(EffectKind::LevelUp, player->x, player->y, -1, level, 0.7f);
}

void CommandParser::handlePin(std::istringstream &args)
{
    std::string idToken;
    int x, y;

    args >> idToken >> x >> y;
    int id = parseId(idToken);
    if (id < 0)
        return;

    int quantities[RESOURCE_COUNT];
    for (int i = 0; i < RESOURCE_COUNT; ++i)
        args >> quantities[i];
    if (!requireCompleteLine(args, "pin"))
        return;
    for (int i = 0; i < RESOURCE_COUNT; ++i)
        if (quantities[i] < 0)
        {
            std::cerr << "[parser] pin: quantite negative ignoree" << std::endl;
            return;
        }

    Player *player = _state.findPlayer(id);
    if (!player)
        return;

    player->x = x;
    player->y = y;
    for (int i = 0; i < RESOURCE_COUNT; ++i)
        player->inventory[i] = quantities[i];
}

void CommandParser::handlePex(std::istringstream &args)
{
    std::string idToken;
    args >> idToken;
    int id = parseId(idToken);
    Player *player = id >= 0 ? _state.findPlayer(id) : nullptr;
    if (player)
        _state.addEffect(EffectKind::Expulsion, player->x, player->y);

    std::cout << "[parser] joueur #" << id << " expulse" << std::endl;
}

void CommandParser::handlePbc(std::istringstream &args)
{
    std::string idToken;
    args >> idToken;
    int id = parseId(idToken);

    std::string message;
    std::getline(args, message);
    if (!message.empty() && message[0] == ' ')
        message.erase(0, 1);

    Player *player = id >= 0 ? _state.findPlayer(id) : nullptr;
    if (player)
        _state.addEffect(EffectKind::Broadcast, player->x, player->y, -1, -1, 1.f);

    std::cout << "[parser] broadcast de #" << id << ": " << message << std::endl;
}

void CommandParser::handlePic(std::istringstream &args)
{
    int x, y, level;
    args >> x >> y >> level;
    if (!requireCompleteLine(args, "pic") || !isValidLevel(level))
    {
        std::cerr << "[parser] pic: ligne invalide ignoree" << std::endl;
        return;
    }

    Incantation incantation;
    incantation.x = x;
    incantation.y = y;
    incantation.level = level;
    incantation.active = true;

    std::string idToken;
    while (args >> idToken)
    {
        int pid = parseId(idToken);
        if (pid >= 0)
            incantation.playerIds.push_back(pid);
    }

    _state.incantations.push_back(incantation);
    std::cout << "[parser] incantation demarree en (" << x << "," << y
               << ") vers niveau " << level << " avec " << incantation.playerIds.size()
               << " joueur(s)" << std::endl;
}

void CommandParser::handlePie(std::istringstream &args)
{
    int x, y, result;
    args >> x >> y >> result;
    if (!requireCompleteLine(args, "pie"))
        return;

    // Ferme la plus recente incantation encore active sur cette case
    // (recherche depuis la fin) : sans ca, une ancienne incantation deja
    // terminee mais pas encore nettoyee restait trouvable en premier et
    // "absorbait" le pie destine a la nouvelle, qui restait active a vie.
    // Elle n'est pas retiree tout de suite : Renderer joue une animation
    // de succes/echec pendant environ une seconde avant de la nettoyer
    // (GameState::advanceIncantationAnimations).
    auto rit = std::find_if(_state.incantations.rbegin(), _state.incantations.rend(),
        [x, y](const Incantation &inc) { return inc.x == x && inc.y == y && inc.active; });
    if (rit != _state.incantations.rend())
    {
        rit->active = false;
        rit->result = result ? 1 : 0;
    }

    std::cout << "[parser] fin d'incantation en (" << x << "," << y << ") -> "
               << (result ? "succes" : "echec") << std::endl;
}

void CommandParser::handlePfk(std::istringstream &args)
{
    std::string id;
    args >> id;
    int playerId = parseId(id);
    Player *player = playerId >= 0 ? _state.findPlayer(playerId) : nullptr;
    if (player)
        _state.addEffect(EffectKind::EggLaying, player->x, player->y);

    std::cout << "[parser] joueur #" << playerId << " pond un oeuf" << std::endl;
}

void CommandParser::handlePdr(std::istringstream &args)
{
    std::string idToken;
    int resourceIndex;
    args >> idToken >> resourceIndex;
    if (!requireCompleteLine(args, "pdr"))
        return;

    int id = parseId(idToken);
    Player *player = id >= 0 ? _state.findPlayer(id) : nullptr;
    if (player && resourceIndex >= 0 && resourceIndex < RESOURCE_COUNT)
    {
        if (player->inventory[resourceIndex] > 0)
            player->inventory[resourceIndex]--;
        if (isInMapBounds(player->x, player->y))
            _state.tileAt(player->x, player->y).resources[resourceIndex]++;
        _state.addEffect(EffectKind::ResourceDrop, player->x, player->y, resourceIndex);
    }
}

void CommandParser::handlePgt(std::istringstream &args)
{
    std::string idToken;
    int resourceIndex;
    args >> idToken >> resourceIndex;
    if (!requireCompleteLine(args, "pgt"))
        return;

    int id = parseId(idToken);
    Player *player = id >= 0 ? _state.findPlayer(id) : nullptr;
    if (player && resourceIndex >= 0 && resourceIndex < RESOURCE_COUNT)
    {
        player->inventory[resourceIndex]++;
        if (isInMapBounds(player->x, player->y))
        {
            Tile &tile = _state.tileAt(player->x, player->y);
            if (tile.resources[resourceIndex] > 0)
                tile.resources[resourceIndex]--;
        }
        _state.addEffect(EffectKind::ResourcePickup, player->x, player->y, resourceIndex);
    }
}

void CommandParser::handlePdi(std::istringstream &args)
{
    std::string idToken;
    args >> idToken;
    int id = parseId(idToken);
    Player *player = id >= 0 ? _state.findPlayer(id) : nullptr;
    if (player)
    {
        _state.addEffect(EffectKind::Death, player->x, player->y, -1, -1, 0.8f);
        // Retire immediatement le joueur de l'etat plutot que de le garder
        // indefiniment avec alive=false : l'animation de mort n'en depend
        // pas (TransientEffect vit independamment dans state.effects),
        // donc rien n'est perdu visuellement, et l'etat ne grossit plus
        // sans fin sur une longue partie. Team::spawnedCount (jamais
        // decremente) reste le total affiche par le HUD.
        removePlayerIdFromTeam(player->team, id);
        _state.players.erase(id);
    }

    std::cout << "[parser] mort du joueur #" << id << std::endl;
}

void CommandParser::handleEnw(std::istringstream &args)
{
    std::string eggIdToken, playerIdToken;
    int x, y;

    args >> eggIdToken >> playerIdToken >> x >> y;
    if (!requireCompleteLine(args, "enw"))
        return;
    int eggId = parseId(eggIdToken);
    int playerId = parseId(playerIdToken);
    if (eggId < 0)
        return;

    Egg egg;
    egg.id = eggId;
    egg.playerId = playerId;
    egg.x = x;
    egg.y = y;
    egg.hatched = false;

    _state.eggs[eggId] = egg;

    Player *parent = playerId >= 0 ? _state.findPlayer(playerId) : nullptr;
    if (parent)
    {
        auto teamIt = _state.teams.find(parent->team);
        if (teamIt != _state.teams.end())
            teamIt->second.eggIds.push_back(eggId);
    }
}

void CommandParser::handleEbo(std::istringstream &args)
{
    std::string id;
    args >> id;
    int eggId = parseId(id);

    Egg *egg = eggId >= 0 ? _state.findEgg(eggId) : nullptr;
    if (egg)
    {
        egg->hatched = true;
        egg->hatchedElapsedSeconds = 0.f;
        removeEggIdFromOwningTeam(*egg);
    }

    // id contient deja le "#" (ex: "#10") : le concatener a un second "#"
    // affichait "##10" au lieu de "#10".
    std::cout << "[parser] oeuf " << id << " a eclos" << std::endl;
}

void CommandParser::handleEdi(std::istringstream &args)
{
    std::string id;
    args >> id;
    int eggId = parseId(id);
    if (eggId < 0)
        return;

    Egg *egg = _state.findEgg(eggId);
    if (egg)
        removeEggIdFromOwningTeam(*egg);
    _state.eggs.erase(eggId);
}

void CommandParser::handleSgt(std::istringstream &args)
{
    int t;
    args >> t;
    if (!requireCompleteLine(args, "sgt") || t <= 0)
        return;
    _state.timeUnit = t;
    std::cout << "[parser] time unit: " << t << std::endl;
}

void CommandParser::handleSst(std::istringstream &args)
{
    int t;
    args >> t;
    if (!requireCompleteLine(args, "sst") || t <= 0)
        return;
    _state.timeUnit = t;
    std::cout << "[parser] time unit modifie: " << t << std::endl;
}

void CommandParser::handleSeg(std::istringstream &args)
{
    std::string team;
    args >> team;
    _state.gameOver = true;
    _state.winningTeam = team;
    std::cout << "[parser] fin de partie, equipe gagnante: " << team << std::endl;
}

void CommandParser::handleSmg(std::istringstream &args)
{
    std::string message;
    std::getline(args, message);
    if (!message.empty() && message[0] == ' ')
        message.erase(0, 1);
    std::cout << "[parser] message serveur: " << message << std::endl;
}

void CommandParser::handleSuc(std::istringstream &)
{
    std::cerr << "[parser] le serveur signale une commande inconnue (suc)" << std::endl;
}

void CommandParser::handleSbp(std::istringstream &)
{
    std::cerr << "[parser] le serveur signale un mauvais nombre de parametres (sbp)" << std::endl;
}