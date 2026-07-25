/*
** Unit tests for src/gui/RenderTheme.cpp (calculs purs utilises par le
** rendu SFML : couleurs d'equipe, ressources rares, angles).
** Framework: Criterion
*/

#include <criterion/criterion.h>

#include "RenderTheme.hpp"

// ---------------------------------------------------------------------------
// teamColor: deterministe (meme nom -> meme couleur)
// ---------------------------------------------------------------------------

Test(render_theme, team_color_is_deterministic)
{
    render_theme::RgbColor first = render_theme::teamColor("rouge");
    render_theme::RgbColor second = render_theme::teamColor("rouge");

    cr_assert_eq(first.r, second.r);
    cr_assert_eq(first.g, second.g);
    cr_assert_eq(first.b, second.b);
}

// ---------------------------------------------------------------------------
// isRareResource: ordre protocole food=0, linemate=1, deraumere=2,
// sibur=3, mendiane=4, phiras=5, thystame=6
// ---------------------------------------------------------------------------

Test(render_theme, common_resources_are_not_rare)
{
    cr_assert_not(render_theme::isRareResource(0));
    cr_assert_not(render_theme::isRareResource(1));
    cr_assert_not(render_theme::isRareResource(2));
}

Test(render_theme, sibur_mendiane_phiras_thystame_are_rare)
{
    cr_assert(render_theme::isRareResource(3));
    cr_assert(render_theme::isRareResource(4));
    cr_assert(render_theme::isRareResource(5));
    cr_assert(render_theme::isRareResource(6));
}

// ---------------------------------------------------------------------------
// resourceColor: une teinte distincte par ressource (0..6)
// ---------------------------------------------------------------------------

static bool colorsDiffer(render_theme::RgbColor a, render_theme::RgbColor b)
{
    return a.r != b.r || a.g != b.g || a.b != b.b;
}

Test(render_theme, each_resource_has_a_distinct_color)
{
    render_theme::RgbColor colors[7];

    for (int i = 0; i < 7; ++i)
        colors[i] = render_theme::resourceColor(i);

    for (int i = 0; i < 7; ++i)
        for (int j = i + 1; j < 7; ++j)
            cr_assert(colorsDiffer(colors[i], colors[j]),
                "resourceColor(%d) et resourceColor(%d) sont identiques", i, j);
}

// ---------------------------------------------------------------------------
// resourceQuantityScale: agrandit la pastille selon la quantite
// ---------------------------------------------------------------------------

Test(render_theme, resource_quantity_scale_is_normal_for_a_single_unit)
{
    cr_assert_float_eq(render_theme::resourceQuantityScale(0), 1.f, 0.001);
    cr_assert_float_eq(render_theme::resourceQuantityScale(1), 1.f, 0.001);
}

Test(render_theme, resource_quantity_scale_grows_with_quantity)
{
    float two = render_theme::resourceQuantityScale(2);
    float five = render_theme::resourceQuantityScale(5);

    cr_assert_gt(two, 1.f);
    cr_assert_gt(five, two);
}

Test(render_theme, resource_quantity_scale_is_capped_for_huge_quantities)
{
    float capped = render_theme::resourceQuantityScale(6);
    float huge = render_theme::resourceQuantityScale(1000);

    cr_assert_float_eq(huge, capped, 0.001);
}

// ---------------------------------------------------------------------------
// orientationAngleDeg: protocole reseau North=1, East=2, South=3, West=4
// ---------------------------------------------------------------------------

Test(render_theme, orientation_angle_matches_each_direction)
{
    cr_assert_float_eq(render_theme::orientationAngleDeg(1), 270.f, 0.001);
    cr_assert_float_eq(render_theme::orientationAngleDeg(2), 0.f, 0.001);
    cr_assert_float_eq(render_theme::orientationAngleDeg(3), 90.f, 0.001);
    cr_assert_float_eq(render_theme::orientationAngleDeg(4), 180.f, 0.001);
}

// ---------------------------------------------------------------------------
// mouthHalfAngleDeg: reste dans les bornes, periodique
// ---------------------------------------------------------------------------

Test(render_theme, mouth_half_angle_stays_within_bounds)
{
    for (float t = 0.f; t < 2.f; t += 0.05f) {
        float angle = render_theme::mouthHalfAngleDeg(t);

        cr_assert_geq(angle, render_theme::kMouthMinDeg - 0.001f);
        cr_assert_leq(angle, render_theme::kMouthMaxDeg + 0.001f);
    }
}

Test(render_theme, mouth_half_angle_is_periodic)
{
    float first = render_theme::mouthHalfAngleDeg(0.1f);
    float afterOnePeriod = render_theme::mouthHalfAngleDeg(0.1f + render_theme::kMouthPeriodSeconds);

    cr_assert_float_eq(first, afterOnePeriod, 0.01);
}

// ---------------------------------------------------------------------------
// tileCenterPx
// ---------------------------------------------------------------------------

Test(render_theme, tile_center_px_for_arbitrary_tile)
{
    render_theme::PixelPos center = render_theme::tileCenterPx(2, 3);

    cr_assert_float_eq(center.x, 2.5f * render_theme::kCellSize, 0.001);
    cr_assert_float_eq(center.y, 3.5f * render_theme::kCellSize, 0.001);
}

// ---------------------------------------------------------------------------
// shortestWrappedDelta / wrapTileCoord: deplacement toroidal
// ---------------------------------------------------------------------------

Test(render_theme, shortest_wrapped_delta_direct_move_no_wrap)
{
    cr_assert_float_eq(render_theme::shortestWrappedDelta(2.f, 5, 10), 3.f, 0.001);
    cr_assert_float_eq(render_theme::shortestWrappedDelta(5.f, 2, 10), -3.f, 0.001);
}

Test(render_theme, shortest_wrapped_delta_goes_the_short_way_around_the_edge)
{
    // De la case 0 vers la derniere case (size-1) : le plus court chemin
    // est -1 (passer par le bord), pas +9 (traverser toute la carte).
    cr_assert_float_eq(render_theme::shortestWrappedDelta(0.f, 9, 10), -1.f, 0.001);
    // Symetriquement, de la derniere case vers la case 0 : +1.
    cr_assert_float_eq(render_theme::shortestWrappedDelta(9.f, 0, 10), 1.f, 0.001);
}

Test(render_theme, shortest_wrapped_delta_without_map_size_is_raw_delta)
{
    cr_assert_float_eq(render_theme::shortestWrappedDelta(0.f, 9, 0), 9.f, 0.001);
}

Test(render_theme, wrap_tile_coord_brings_negative_and_overflow_back_in_range)
{
    cr_assert_float_eq(render_theme::wrapTileCoord(-0.5f, 10), 9.5f, 0.001);
    cr_assert_float_eq(render_theme::wrapTileCoord(10.5f, 10), 0.5f, 0.001);
    cr_assert_float_eq(render_theme::wrapTileCoord(4.f, 10), 4.f, 0.001);
}
