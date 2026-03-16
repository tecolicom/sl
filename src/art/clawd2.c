/*
 * art/clawd2.c - Claude Code mascot animation
 *
 * Pixel art rendered dynamically via quarter-block encoding.
 * Supports half-column horizontal and half-row vertical movement
 * for smooth animation including walking and jumping.
 */

#include <math.h>
#include "art.h"
#include "qblock.h"
#include "../sl.h"

#define PX_WIDTH  18
#define PX_ROWS   6
#define N_POSES   4

enum { POSE_WALK_A, POSE_WALK_B, POSE_SQUAT, POSE_AIR };

static const char *poses[N_POSES][PX_ROWS] = {
    [POSE_WALK_A] = {
        "   @@@@@@@@@@@@   ",    /* head upper */
        "   @@ @@@@@@ @@   ",    /* head lower */
        " @@@@@@@@@@@@@@@@ ",    /* body upper */
        "   @@@@@@@@@@@@   ",    /* body lower */
        "    @ @    @ @    ",    /* legs upper */
        "                  ",    /* legs lower */
    },
    [POSE_WALK_B] = {
        "   @@@@@@@@@@@@   ",    /* head upper (raised arms) */
        " @@@@ @@@@@@ @@@@ ",    /* head lower */
        "   @@@@@@@@@@@@   ",    /* body upper */
        "   @@@@@@@@@@@@   ",    /* body lower */
        "   @ @      @ @   ",    /* legs upper */
        "                  ",    /* legs lower */
    },
    [POSE_SQUAT] = {
        "                  ",    /* legs upper (wide) */
        "   @@@@@@@@@@@@   ",    /* head upper */
        " @@@@ @@@@@@ @@@@ ",    /* head lower */
        "   @@@@@@@@@@@@   ",    /* body upper */
        " @@@@@@@@@@@@@@@@ ",    /* body lower */
        "                  ",    /* legs lower */
    },
    [POSE_AIR] = {
        "   @@@@@@@@@@@@   ",    /* head upper */
        "   @@ @@@@@@ @@   ",    /* head lower */
        "   @@@@@@@@@@@@   ",    /* body upper */
        "   @@@@@@@@@@@@   ",    /* body lower */
        "     @ @  @ @     ",    /* legs upper (tucked) */
        "                  ",    /* legs lower */
    },
};

/*
 * Quarter-block encoded poses (for reference):
 *
 *   walk A:   " ▐▛███▜▌"      head
 *             "▝▜█████▛▘"     body + arms
 *             "  ▘▘ ▝▝"       legs open
 *
 *   walk B:   "▗▟▛███▜▙▖"     head + raised arms
 *             " ▐█████▌ "     body
 *             " ▝▝   ▘▘"      legs closed
 *
 *   squat:    " ▐▛███▜▌"      head
 *             "▝▜█████▛▘"     body + arms
 *             " ▘▝     ▘▝"    legs wide
 *
 *   air:      " ▐▛███▜▌"      head
 *             "▝▜█████▛▘"     body + arms
 *             "  ▝▘ ▝▘"       legs tucked
 */

#define CLAWD_HEIGHT  7    /* max rows: 3 char + 4 jump height */
#define GROUND_ROW    4    /* first text row at ground level (sy=0) */

/* Walk parameters */
#define LEGS_TICKS   20    /* base ticks per leg state */
#define MOD_PERIOD  240    /* ticks per speed modulation cycle */
#define MOD_RANGE    60    /* speed variation ±60% of base */
#define SPEED_SCALE 100

/* Jump parameters */
#define JUMP_CHANCE  1000  /* 1/N chance per walk tick */
#define SQUAT_FRAMES  5
#define LAND_FRAMES   0

/* Jump curves: parabolic trajectories in half-row units.
   Higher jumps have proportionally longer airtime, with natural
   dwelling near the apex (physically correct parabolic behavior). */
static const int jump_mid[] = {
    0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 5, 5, 4, 4, 3, 3, 2, 1, 1, 0
};
static const int jump_big[] = {
    0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 6, 6, 6, 5, 5, 5, 4, 4, 3,
    3, 2, 2, 1, 1, 0
};

typedef struct {
    const int *curve;
    int frames;
} jump_def;

#define JUMP_DEF(arr) { arr, (int)(sizeof(arr) / sizeof(arr[0])) }

static const jump_def jumps[] = {
    JUMP_DEF(jump_mid),
    JUMP_DEF(jump_big),
};
#define N_JUMPS ((int)(sizeof(jumps) / sizeof(jumps[0])))

enum { ACT_WALK, ACT_JUMP };

typedef struct {
    int action;
    int action_tick;
    int pose;
    int pose_accum;
    int mod_offset;
    int dy;          /* vertical offset in half-rows (0 = ground) */
    int jump_chance;  /* 1/N chance per walk tick (0 = no jump) */
    int jump_type;    /* index into jumps[] (randomized per jump) */
} clawd_ctx;

static const char blank_row[] = "                  ";  /* PX_WIDTH spaces */

static void clawd_init(animation *a) {
    clawd_ctx *c = calloc(1, sizeof(clawd_ctx));
    a->ctx = c;
    c->mod_offset = rand() % MOD_PERIOD;
    c->pose = rand() & 1;
    c->jump_chance = sl_option_int("CLAWD_JUMP", JUMP_CHANCE);
}

static void update_walk(clawd_ctx *c, int tick) {
    /* FM-like pose modulation: sine wave varies leg speed smoothly */
    double phase_rad = 2.0 * M_PI * (tick + c->mod_offset) / MOD_PERIOD;
    int speed = SPEED_SCALE + (int)(MOD_RANGE * sin(phase_rad));
    c->pose_accum += speed;
    if (c->pose_accum >= LEGS_TICKS * SPEED_SCALE) {
        c->pose_accum -= LEGS_TICKS * SPEED_SCALE;
        c->pose ^= 1;  /* toggle WALK_A / WALK_B */
    }
    c->dy = 0;

    /* Random jump trigger */
    if (c->jump_chance > 0 && rand() % c->jump_chance == 0) {
        c->action = ACT_JUMP;
        c->action_tick = 0;
        c->jump_type = rand() % N_JUMPS;
    }
}

static void update_jump(clawd_ctx *c) {
    int t = c->action_tick;
    const jump_def *j = &jumps[c->jump_type];

    if (t < SQUAT_FRAMES) {
        /* Squat phase */
        c->pose = POSE_SQUAT;
        c->dy = 0;
    } else if (t < SQUAT_FRAMES + j->frames) {
        /* Airborne phase */
        c->pose = POSE_AIR;
        c->dy = j->curve[t - SQUAT_FRAMES];
    } else if (t < SQUAT_FRAMES + j->frames + LAND_FRAMES) {
        /* Landing phase */
        c->pose = POSE_SQUAT;
        c->dy = 0;
    } else {
        /* Return to walk */
        c->action = ACT_WALK;
        c->action_tick = 0;
        c->pose = POSE_WALK_A;
        c->dy = 0;
        return;
    }
    c->action_tick++;
}

static void clawd_draw(animation *a, int tick) {
    clawd_ctx *c = a->ctx;
    int sx = art_subx ? 1 : 0;

    /* Update animation state */
    switch (c->action) {
    case ACT_WALK: update_walk(c, tick); break;
    case ACT_JUMP: update_jump(c);      break;
    }

    int dy = c->dy;
    int sy = dy % 2;
    int text_rows = sy ? 4 : 3;
    int start_row = GROUND_ROW - (dy + 1) / 2;

    /* Clear all rows */
    for (int r = 0; r < CLAWD_HEIGHT; r++)
        art_putline(r, "");

    /* Encode and draw each text row */
    char buf[256];
    for (int r = 0; r < text_rows; r++) {
        int py = r * 2 - sy;  /* first pixel row of this pair */
        const char *upper = (py >= 0 && py < PX_ROWS)
            ? poses[c->pose][py] : blank_row;
        const char *lower = (py + 1 >= 0 && py + 1 < PX_ROWS)
            ? poses[c->pose][py + 1] : blank_row;
        qb_encode_row_buf(upper, lower, PX_WIDTH, sx, buf, sizeof(buf));
        art_putline(start_row + r, buf);
    }
}

static void clawd_cleanup(animation *a) {
    free(a->ctx);
    a->ctx = NULL;
}

animation clawd2_animation = {
    .name    = "clawd2",
    .height  = CLAWD_HEIGHT,
    .width   = 10,
    .step    = 50,
    .init    = clawd_init,
    .draw    = clawd_draw,
    .cleanup = clawd_cleanup,
};

/* "clawd" is an alias for "clawd2" */
animation clawd_animation = {
    .name    = "clawd",
    .height  = CLAWD_HEIGHT,
    .width   = 10,
    .step    = 50,
    .init    = clawd_init,
    .draw    = clawd_draw,
    .cleanup = clawd_cleanup,
};
