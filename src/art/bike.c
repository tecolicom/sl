/*
 * art/bike.c - TT bike (time trial) animation
 *
 * A triathlete on a TT bike in aero position.
 * Part of the triathlon series (swim -> bike -> run).
 *
 * Side view (rider moving left <-):
 * Simple stick-figure style for clear recognition.
 *
 *   o       = head
 *   ___     = flat back (aero position)
 *   >/     = aero bars + arms
 *   /|\    = leg pedaling (rotates)
 *   (_)    = wheels
 */

#include <stdlib.h>
#include "art.h"
#include "../sl.h"

#define BIKE_HEIGHT 5
#define BIKE_WIDTH  16
#define N_FRAMES    4
#define FRAME_DIV   2   /* ticks per frame */

/*
 * 4-frame pedaling cycle.
 * The foot (o) moves visibly around the crank:
 *
 *   Frame 0: foot forward     (3 o'clock)
 *   Frame 1: foot down        (6 o'clock)
 *   Frame 2: foot back        (9 o'clock)
 *   Frame 3: foot up          (12 o'clock)
 */
static const char *frames[N_FRAMES][BIKE_HEIGHT] = {
    /* Frame 0: foot forward */
    {
        "      o⎼⎽_           ",
        "    =⌿   \\          ",
        "    | _⎡  ⑊          ",
        "   ( )   ( )         ",
        "    ⌣     ⌣          ",
    },
    /* Frame 1: foot down */
    {
        "      o⎼⎽_           ",
        "    =⌿   \\          ",
        "    |  ⎢  ⑊          ",
        "   ( ) ⏌ ( )         ",
        "    ⌣     ⌣          ",
    },
    /* Frame 2: foot back */
    {
        "      o⎼⎽_           ",
        "    =⌿   \\          ",
        "    | ⟨_  ⑊          ",
        "   ( )   ( )         ",
        "    ⌣     ⌣          ",
    },
    /* Frame 3: foot up */
    {
        "      o⎼⎽_           ",
        "    =⌿<  \\          ",
        "    |  ⎺  ⑊          ",
        "   ( )   ( )         ",
        "    ⌣     ⌣          ",
    },
};

static void bike_init(animation *a) {
    a->ctx = calloc(1, 1);
}

static void bike_draw(animation *a, int tick) {
    int frame = (tick / FRAME_DIV) % N_FRAMES;
    for (int y = 0; y < BIKE_HEIGHT; y++)
        art_putline(y, frames[frame][y]);
}

static void bike_cleanup(animation *a) {
    free(a->ctx);
    a->ctx = NULL;
}

animation bike_animation = {
    .name    = "bike",
    .height  = BIKE_HEIGHT,
    .width   = BIKE_WIDTH,
    .step    = 100,
    .init    = bike_init,
    .draw    = bike_draw,
    .cleanup = bike_cleanup,
};
