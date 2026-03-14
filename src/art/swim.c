/*
 * art/swim.c - Swimmer (front crawl) animation
 *
 * A swimmer doing the crawl stroke.  Only the above-water
 * portion is drawn; the streak car provides the water surface.
 * Part of the triathlon series (swim -> bike -> run).
 *
 * Crawl stroke side view (swimmer moving left <-):
 *
 *   ⌢       = head / swim cap (fixed horizontal position)
 *   ⎽⎽⎽    = body trailing behind head
 *   ______  = arm pulling through water
 *   ⎽⎼⎻⎺   = arm rising/falling (scan lines at different heights)
 *   ⎛⎞⎝⎠   = arm curve at top of arc (parenthesis hooks)
 *   ⌟⌞     = kick splash
 *
 * The head ⌢ stays at a fixed column; the arm rotates around it.
 * All frames use row 2 for the head+body line.
 * One full stroke cycle: pull -> exit -> arc over -> enter water.
 */

#include <stdlib.h>
#include "art.h"
#include "../sl.h"

#define SWIM_HEIGHT 3
#define SWIM_WIDTH  18
#define N_FRAMES    5
#define FRAME_DIV   3   /* ticks per frame */

/*
 * 5-frame crawl stroke cycle:
 *
 *   Frame 0: arm pulling through water (extended left), kick behind
 *   Frame 1: arm exits water behind, rising (scan lines ascend right)
 *   Frame 2: arm at top of arc, right side (⎛_ overhead, ⌢⎠ at body)
 *   Frame 3: arm coming down, left side (_⎞ overhead, ⎝⌢ at body)
 *   Frame 4: arm entering water ahead (scan lines descend left), kick
 *
 * Head ⌢ is at column 6 in every frame, always on row 2.
 * Rows 0-1 are used only when the arm arcs overhead (frames 2-3).
 */
static const char *frames[N_FRAMES][SWIM_HEIGHT] = {
    /* Frame 0: arm pulling through water, kick */
    {
        "                  ",
        "                  ",
        "______⌢ ⎽⎽⎽     __",
    },
    /* Frame 1: arm rising out of water behind */
    {
        "                  ",
        "                  ",
        "      ⌢⎽⎽⎼⎼⎻⎺    ",
    },
    /* Frame 2: arm at top of arc (right side) */
    {
        "         _        ",
        "        ⎛         ",
        "      ⌢⎠ ⎽⎽⎽     ",
    },
    /* Frame 3: arm coming down (left side) */
    {
        "   _              ",
        "    ⎞             ",
        "     ⎝⌢ ⎽⎽⎽      ",
    },
    /* Frame 4: arm entering water ahead, kick */
    {
        "                  ",
        "                  ",
        "⎺⎻⎼⎼⎽⎽⌢ ⎽⎽⎽   ⌟⌞",
    },
};

static void swim_init(animation *a) {
    a->ctx = calloc(1, 1);
}

static void swim_draw(animation *a, int tick) {
    int frame = (tick / FRAME_DIV) % N_FRAMES;
    for (int y = 0; y < SWIM_HEIGHT; y++)
        art_putline(y, frames[frame][y]);
}

static void swim_cleanup(animation *a) {
    free(a->ctx);
    a->ctx = NULL;
}

animation swim_animation = {
    .name    = "swim",
    .height  = SWIM_HEIGHT,
    .width   = SWIM_WIDTH,
    .step    = 100,
    .init    = swim_init,
    .draw    = swim_draw,
    .cleanup = swim_cleanup,
};
