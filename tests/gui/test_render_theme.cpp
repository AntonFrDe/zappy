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
// orientationAngleDeg: North=0, East=1, South=2, West=3
// ---------------------------------------------------------------------------

Test(render_theme, orientation_angle_matches_each_direction)
{
    cr_assert_float_eq(render_theme::orientationAngleDeg(0), 270.f, 0.001);
    cr_assert_float_eq(render_theme::orientationAngleDeg(1), 0.f, 0.001);
    cr_assert_float_eq(render_theme::orientationAngleDeg(2), 90.f, 0.001);
    cr_assert_float_eq(render_theme::orientationAngleDeg(3), 180.f, 0.001);
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

Test(render_theme, tile_center_px_for_origin_tile)
{
    render_theme::PixelPos center = render_theme::tileCenterPx(0, 0);

    cr_assert_float_eq(center.x, render_theme::kCellSize * 0.5f, 0.001);
    cr_assert_float_eq(center.y, render_theme::kCellSize * 0.5f, 0.001);
}

Test(render_theme, tile_center_px_for_arbitrary_tile)
{
    render_theme::PixelPos center = render_theme::tileCenterPx(2, 3);

    cr_assert_float_eq(center.x, 2.5f * render_theme::kCellSize, 0.001);
    cr_assert_float_eq(center.y, 3.5f * render_theme::kCellSize, 0.001);
}
