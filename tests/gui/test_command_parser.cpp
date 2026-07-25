/*
** Unit tests for src/gui/CommandParser.cpp (protocole serveur -> GUI)
** Framework: Criterion
*/

#include <criterion/criterion.h>
#include <criterion/redirect.h>

#include "CommandParser.hpp"
#include "GameState.hpp"

static void redirect_all_std(void)
{
    cr_redirect_stdout();
    cr_redirect_stderr();
}

// ---------------------------------------------------------------------------
// msz: map size
// ---------------------------------------------------------------------------

Test(command_parser, msz_resizes_map)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("msz 5 3");

    cr_assert_eq(state.mapWidth, 5);
    cr_assert_eq(state.mapHeight, 3);
    cr_assert_eq(state.tiles.size(), (size_t)(5 * 3));
}

// ---------------------------------------------------------------------------
// bct: tile content
// ---------------------------------------------------------------------------

Test(command_parser, bct_sets_tile_resources)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("msz 3 3");
    parser.dispatch("bct 1 1 1 2 3 4 5 6 7");

    Tile &tile = state.tileAt(1, 1);
    cr_assert_eq(tile.resources[0], 1);
    cr_assert_eq(tile.resources[6], 7);
}

Test(command_parser, bct_out_of_bounds_ignored, .init = redirect_all_std)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("msz 2 2");
    parser.dispatch("bct 5 5 1 1 1 1 1 1 1");

    for (const Tile &tile : state.tiles)
        for (int r : tile.resources)
            cr_assert_eq(r, 0);
}

// ---------------------------------------------------------------------------
// tna: team names
// ---------------------------------------------------------------------------

Test(command_parser, tna_registers_team)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("tna rouge");

    cr_assert_eq(state.teams.count("rouge"), (size_t)1);
    cr_assert_str_eq(state.teams.at("rouge").name.c_str(), "rouge");
}

// ---------------------------------------------------------------------------
// pnw: new player
// ---------------------------------------------------------------------------

Test(command_parser, pnw_adds_player_and_links_team)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("tna rouge");
    parser.dispatch("pnw #1 2 3 1 1 rouge");

    Player *player = state.findPlayer(1);
    cr_assert_not_null(player);
    cr_assert_eq(player->x, 2);
    cr_assert_eq(player->y, 3);
    cr_assert_eq(player->level, 1);
    cr_assert_str_eq(player->team.c_str(), "rouge");
    cr_assert(player->alive);

    Team &team = state.teams.at("rouge");
    cr_assert_eq(team.playerIds.size(), (size_t)1);
    cr_assert_eq(team.playerIds[0], 1);
}

Test(command_parser, pnw_invalid_id_ignored)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("pnw 1 2 3 1 1 rouge");

    cr_assert_null(state.findPlayer(1));
}

// ---------------------------------------------------------------------------
// ppo: player position
// ---------------------------------------------------------------------------

Test(command_parser, ppo_updates_known_player)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("pnw #1 0 0 1 1 rouge");
    parser.dispatch("ppo #1 4 5 2");

    Player *player = state.findPlayer(1);
    cr_assert_not_null(player);
    cr_assert_eq(player->x, 4);
    cr_assert_eq(player->y, 5);
    cr_assert_eq((int)player->orientation, 2);
}

Test(command_parser, ppo_unknown_player_ignored, .init = redirect_all_std)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("ppo #99 4 5 2");

    cr_assert_null(state.findPlayer(99));
}

// ---------------------------------------------------------------------------
// plv: player level
// ---------------------------------------------------------------------------

Test(command_parser, plv_updates_level)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("pnw #1 0 0 1 1 rouge");
    parser.dispatch("plv #1 4");

    cr_assert_eq(state.findPlayer(1)->level, 4);
}

// ---------------------------------------------------------------------------
// pin: player inventory
// ---------------------------------------------------------------------------

Test(command_parser, pin_updates_position_and_inventory)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("pnw #1 0 0 1 1 rouge");
    parser.dispatch("pin #1 2 2 1 2 3 4 5 6 7");

    Player *player = state.findPlayer(1);
    cr_assert_eq(player->x, 2);
    cr_assert_eq(player->y, 2);
    cr_assert_eq(player->inventory[0], 1);
    cr_assert_eq(player->inventory[6], 7);
}

// ---------------------------------------------------------------------------
// pdi: player death
// ---------------------------------------------------------------------------

Test(command_parser, pdi_marks_player_dead)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("pnw #1 0 0 1 1 rouge");
    parser.dispatch("pdi #1");

    cr_assert_not(state.findPlayer(1)->alive);
}

// ---------------------------------------------------------------------------
// pgt / pdr: resource pickup/drop transfers between player and tile
// ---------------------------------------------------------------------------

Test(command_parser, pgt_takes_resource_from_tile)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("msz 3 3");
    parser.dispatch("bct 0 0 5 0 0 0 0 0 0");
    parser.dispatch("pnw #1 0 0 1 1 rouge");

    parser.dispatch("pgt #1 0");

    Player *player = state.findPlayer(1);
    cr_assert_eq(player->inventory[0], 1);
    cr_assert_eq(state.tileAt(0, 0).resources[0], 4);
}

Test(command_parser, pdr_drops_resource_on_tile)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("msz 3 3");
    parser.dispatch("pnw #1 0 0 1 1 rouge");
    parser.dispatch("pgt #1 0");

    parser.dispatch("pdr #1 0");

    Player *player = state.findPlayer(1);
    cr_assert_eq(player->inventory[0], 0);
    cr_assert_eq(state.tileAt(0, 0).resources[0], 1);
}

Test(command_parser, pdr_never_goes_negative)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("msz 3 3");
    parser.dispatch("pnw #1 0 0 1 1 rouge");

    parser.dispatch("pdr #1 0");

    cr_assert_eq(state.findPlayer(1)->inventory[0], 0);
}

// ---------------------------------------------------------------------------
// pic / pie: incantations
// ---------------------------------------------------------------------------

Test(command_parser, pic_starts_incantation_with_players)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("pic 1 1 2 #1 #2");

    cr_assert_eq(state.incantations.size(), (size_t)1);
    Incantation &incantation = state.incantations[0];
    cr_assert_eq(incantation.x, 1);
    cr_assert_eq(incantation.y, 1);
    cr_assert_eq(incantation.level, 2);
    cr_assert(incantation.active);
    cr_assert_eq(incantation.playerIds.size(), (size_t)2);
}

Test(command_parser, pie_ends_incantation)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("pic 1 1 2 #1 #2");
    parser.dispatch("pie 1 1 1");

    // findIncantationAt ne renvoie plus que les incantations actives, mais
    // celle-ci reste dans la liste (inactive, resultat = succes) le temps
    // que Renderer joue son animation de resultat.
    cr_assert_null(state.findIncantationAt(1, 1));
    cr_assert_eq(state.incantations.size(), (size_t)1);
    cr_assert_not(state.incantations[0].active);
    cr_assert_eq(state.incantations[0].result, 1);
}

Test(command_parser, pie_failure_sets_result_to_zero)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("pic 1 1 2 #1");
    parser.dispatch("pie 1 1 0");

    cr_assert_eq(state.incantations.size(), (size_t)1);
    cr_assert_not(state.incantations[0].active);
    cr_assert_eq(state.incantations[0].result, 0);
}

// Regression: une incantation terminee et jamais nettoyee ne doit pas
// "absorber" le pie destine a une nouvelle incantation demarree ensuite
// sur la meme case (sinon la nouvelle reste active indefiniment).
Test(command_parser, pie_closes_the_new_incantation_not_a_stale_one)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("pic 1 1 2 #1");
    parser.dispatch("pie 1 1 0");
    parser.dispatch("pic 1 1 2 #1");
    parser.dispatch("pie 1 1 1");

    cr_assert_null(state.findIncantationAt(1, 1));
}

// ---------------------------------------------------------------------------
// advanceIncantationAnimations: tient le resultat ~1s avant nettoyage
// ---------------------------------------------------------------------------

Test(command_parser, finished_incantation_survives_until_hold_duration)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("pic 1 1 2 #1");
    parser.dispatch("pie 1 1 1");

    state.advanceIncantationAnimations(0.5f);
    cr_assert_eq(state.incantations.size(), (size_t)1);

    state.advanceIncantationAnimations(0.5f);
    cr_assert_eq(state.incantations.size(), (size_t)0);
}

Test(command_parser, active_incantation_is_not_pruned_by_animation_advance)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("pic 1 1 2 #1");

    state.advanceIncantationAnimations(10.f);

    cr_assert_eq(state.incantations.size(), (size_t)1);
    cr_assert(state.incantations[0].active);
}

// ---------------------------------------------------------------------------
// enw / ebo / edi: eggs
// ---------------------------------------------------------------------------

Test(command_parser, enw_adds_egg_and_links_team)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("tna rouge");
    parser.dispatch("pnw #1 0 0 1 1 rouge");
    parser.dispatch("enw #10 #1 2 2");

    Egg *egg = state.findEgg(10);
    cr_assert_not_null(egg);
    cr_assert_eq(egg->playerId, 1);
    cr_assert_not(egg->hatched);

    Team &team = state.teams.at("rouge");
    cr_assert_eq(team.eggIds.size(), (size_t)1);
    cr_assert_eq(team.eggIds[0], 10);
}

Test(command_parser, ebo_marks_egg_hatched)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("enw #10 #1 2 2");
    parser.dispatch("ebo #10");

    cr_assert(state.findEgg(10)->hatched);
}

Test(command_parser, edi_removes_egg)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("enw #10 #1 2 2");
    parser.dispatch("edi #10");

    cr_assert_null(state.findEgg(10));
}

Test(command_parser, ebo_removes_egg_id_from_owning_team)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("tna rouge");
    parser.dispatch("pnw #1 0 0 1 1 rouge");
    parser.dispatch("enw #10 #1 2 2");
    parser.dispatch("ebo #10");

    Team &team = state.teams.at("rouge");
    cr_assert_eq(team.eggIds.size(), (size_t)0);
}

Test(command_parser, edi_removes_egg_id_from_owning_team)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("tna rouge");
    parser.dispatch("pnw #1 0 0 1 1 rouge");
    parser.dispatch("enw #10 #1 2 2");
    parser.dispatch("edi #10");

    Team &team = state.teams.at("rouge");
    cr_assert_eq(team.eggIds.size(), (size_t)0);
}

// Un oeuf initial (present au lancement de la partie, avant qu'aucun
// joueur n'ait pondu) n'a pas de parent connu : le protocole peut envoyer
// un id de parent invalide/negatif pour ce cas. La GUI doit quand meme
// creer et retrouver l'oeuf, sans planter ni tenter de le lier a une
// equipe.
Test(command_parser, enw_with_unknown_parent_still_creates_egg)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("tna rouge");
    parser.dispatch("enw #10 #-1 2 2");

    Egg *egg = state.findEgg(10);
    cr_assert_not_null(egg);
    cr_assert_eq(egg->playerId, -1);

    Team &team = state.teams.at("rouge");
    cr_assert_eq(team.eggIds.size(), (size_t)0);
}

Test(command_parser, hatched_egg_survives_until_hold_duration)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("enw #10 #1 2 2");
    parser.dispatch("ebo #10");

    state.advanceEggAnimations(GameState::kEggHatchHoldSeconds / 2.f);
    cr_assert_not_null(state.findEgg(10));

    state.advanceEggAnimations(GameState::kEggHatchHoldSeconds);
    cr_assert_null(state.findEgg(10));
}

Test(command_parser, unhatched_egg_is_not_pruned_by_animation_advance)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("enw #10 #1 2 2");

    state.advanceEggAnimations(10.f);

    cr_assert_not_null(state.findEgg(10));
}

// ---------------------------------------------------------------------------
// sgt / sst / seg: server/game state
// ---------------------------------------------------------------------------

Test(command_parser, sgt_sets_time_unit)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("sgt 100");

    cr_assert_eq(state.timeUnit, 100);
}

Test(command_parser, sst_updates_time_unit)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("sgt 100");
    parser.dispatch("sst 50");

    cr_assert_eq(state.timeUnit, 50);
}

Test(command_parser, seg_ends_game_with_winner)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("seg rouge");

    cr_assert(state.gameOver);
    cr_assert_str_eq(state.winningTeam.c_str(), "rouge");
}

// ---------------------------------------------------------------------------
// Robustness: unknown / empty / malformed lines
// ---------------------------------------------------------------------------

Test(command_parser, unknown_command_is_ignored_without_crash, .init = redirect_all_std)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("zzz totally unknown command");

    cr_assert_eq(state.mapWidth, 0);
}

Test(command_parser, empty_line_is_ignored)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("");

    cr_assert_eq(state.mapWidth, 0);
    cr_assert_eq(state.players.size(), (size_t)0);
}

Test(command_parser, missing_arguments_do_not_crash, .init = redirect_all_std)
{
    GameState state;
    CommandParser parser(state);

    // requireCompleteLine() rejette toute ligne dont l'extraction des
    // entiers a echoue avant qu'ils soient utilises (resize de map,
    // indexation de tableau, etc.).
    parser.dispatch("msz");
    parser.dispatch("bct");
    parser.dispatch("pnw");
    parser.dispatch("ppo");
    parser.dispatch("plv");
    parser.dispatch("pin");
    parser.dispatch("pic");
    parser.dispatch("pie");
    parser.dispatch("enw");
    parser.dispatch("sgt");
    parser.dispatch("sst");

    cr_assert_eq(state.mapWidth, 0);
    cr_assert_eq(state.mapHeight, 0);
}

Test(command_parser, msz_rejects_non_positive_or_absurd_dimensions, .init = redirect_all_std)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("msz 0 5");
    cr_assert_eq(state.mapWidth, 0);

    parser.dispatch("msz -3 5");
    cr_assert_eq(state.mapWidth, 0);

    parser.dispatch("msz 999999999 5");
    cr_assert_eq(state.mapWidth, 0);

    parser.dispatch("msz 10 8");
    cr_assert_eq(state.mapWidth, 10);
    cr_assert_eq(state.mapHeight, 8);
}

Test(command_parser, pnw_rejects_invalid_orientation_or_level, .init = redirect_all_std)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("pnw #1 0 0 0 1 rouge");
    cr_assert_null(state.findPlayer(1));

    parser.dispatch("pnw #1 0 0 5 1 rouge");
    cr_assert_null(state.findPlayer(1));

    parser.dispatch("pnw #1 0 0 1 9 rouge");
    cr_assert_null(state.findPlayer(1));

    parser.dispatch("pnw #1 0 0 1 8 rouge");
    cr_assert_not_null(state.findPlayer(1));
}

Test(command_parser, bct_rejects_negative_quantities, .init = redirect_all_std)
{
    GameState state;
    CommandParser parser(state);

    parser.dispatch("msz 3 3");
    parser.dispatch("bct 1 1 -1 0 0 0 0 0 0");

    Tile &tile = state.tileAt(1, 1);
    for (int i = 0; i < RESOURCE_COUNT; ++i)
        cr_assert_eq(tile.resources[i], 0);
}
