#pragma once

#include <string>
#include <sstream>
#include <functional>
#include <unordered_map>

#include "GameState.hpp"


class CommandParser
{
    public:
        explicit CommandParser(GameState &state);
        void dispatch(const std::string &line);

    private:
        GameState &_state;
        std::unordered_map<std::string, std::function<void(std::istringstream &)>> _handlers;

        void registerHandlers();

        void handleMsz(std::istringstream &args);   // msz X Y
        void handleBct(std::istringstream &args);   // bct X Y q0..q6
        void handleTna(std::istringstream &args);   // tna N
        void handlePnw(std::istringstream &args);   // pnw #n X Y O L N
        void handlePpo(std::istringstream &args);   // ppo #n X Y O
        void handlePlv(std::istringstream &args);   // plv #n L
        void handlePin(std::istringstream &args);   // pin #n X Y q0..q6
        void handlePex(std::istringstream &args);   // pex #n
        void handlePbc(std::istringstream &args);   // pbc #n M
        void handlePic(std::istringstream &args);   // pic X Y L #n #n ...
        void handlePie(std::istringstream &args);   // pie X Y R
        void handlePfk(std::istringstream &args);   // pfk #n
        void handlePdr(std::istringstream &args);   // pdr #n i
        void handlePgt(std::istringstream &args);   // pgt #n i
        void handlePdi(std::istringstream &args);   // pdi #n
        void handleEnw(std::istringstream &args);   // enw #e #n X Y
        void handleEbo(std::istringstream &args);   // ebo #e
        void handleEdi(std::istringstream &args);   // edi #e
        void handleSgt(std::istringstream &args);   // sgt T
        void handleSst(std::istringstream &args);   // sst T
        void handleSeg(std::istringstream &args);   // seg N
        void handleSmg(std::istringstream &args);   // smg M
        void handleSuc(std::istringstream &args);   // suc
        void handleSbp(std::istringstream &args);   // sbp
        static int parseId(const std::string &token);

        // Rejette une ligne dont il manque des parametres (stream en echec
        // apres extraction) ou qui contient des donnees en trop : sans ca,
        // les entiers lus restent non initialises et sont quand meme
        // utilises (resize de map, indexation de tableau...).
        bool requireCompleteLine(std::istringstream &args, const char *cmdName);
        static bool isValidOrientation(int o);
        static bool isValidLevel(int level);
        bool isInMapBounds(int x, int y) const;

        // Retire l'id de cet oeuf de Team::eggIds (equipe du joueur qui l'a
        // pondu), pour que la liste ne contienne plus que des oeufs non
        // eclos/non detruits. A appeler avant qu'un oeuf n'eclose (ebo) ou
        // ne disparaisse sans eclore (edi).
        void removeEggIdFromOwningTeam(const Egg &egg);
};
