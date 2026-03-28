/*
 * art/clawd2.c - Claude Code mascot animation
 *
 * 1-bit pixel art is drawn onto a 4-bit canvas at pixel coordinates,
 * then rendered via quarter-block lookup.  Half-column and half-row
 * movement are handled as simple coordinate offsets.
 *
 * Timing note: sl-2026.c passes tick = frame * step / DEFAULT_STEP,
 * so step=50 delivers ticks at half the frame rate.  Walk updates
 * run per-tick (not per-frame), allowing LEGS_TICKS and MOD_PERIOD
 * to use the same values as clawd1.  Jump updates run per-frame
 * for smooth parabolic curves.
 */

#include <math.h>
#include <stdint.h>
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
        "                  ",    /* (blank: body shifted down) */
        "   @@@@@@@@@@@@   ",    /* head upper */
        "   @@ @@@@@@ @@   ",    /* head lower */
        "   @@@@@@@@@@@@   ",    /* body upper */
        "   @@@@@@@@@@@@   ",    /* body lower */
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
 *   squat:    " ▗▄▄▄▄▄▖"      head (lower half)
 *             " ▐▙███▟▌"      head lower + body
 *             " ▝▀▀▀▀▀▘"      body (upper half)
 *
 *   air:      " ▐▛███▜▌"      head
 *             " ▐█████▌"      body
 *             "  ▝▝ ▘▘"       legs tucked
 */

#define CLAWD_HEIGHT  7    /* max rows: 3 char + 4 jump height */
#define GROUND_ROW    4    /* ground level in cell rows (pixel y = GROUND_ROW * 2) */

/* Walk parameters (normalized tick units — same values as clawd1) */
#define LEGS_TICKS   10    /* base ticks per leg state */
#define MOD_PERIOD  120    /* ticks per speed modulation cycle */
#define MOD_RANGE    60    /* speed variation ±60% of base */
#define SPEED_SCALE 100

/* Jump parameters */
#define JUMP_CHANCE   500  /* 1/N chance per walk tick */
#define SQUAT_FRAMES  5

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
    int last_tick;    /* for tick-based walk update */
    int dy;          /* vertical offset in half-rows (0 = ground) */
    int jump_chance;  /* 1/N chance per walk tick (0 = no jump) */
    int jump_type;    /* index into jumps[] (randomized per jump) */
} clawd_ctx;

static uint32_t bitmaps[N_POSES][PX_ROWS];  /* shared across instances */

/* 4-bit canvas: each cell holds a quarter-block pattern (2×2 pixels).
   1-bit bitmaps are drawn onto the canvas at pixel coordinates,
   then rendered to screen via qblock[] lookup. */
#define CANVAS_ROWS  CLAWD_HEIGHT
#define CANVAS_COLS  (PX_WIDTH / 2 + 1)  /* +1 for half-column shift */

/* Map 2-bit pixel pair to upper/lower half of 4-bit cell.
   Index: bit0 = left pixel, bit1 = right pixel. */
static const uint8_t cell_map[2][4] = {
    { 0, 8, 4, 12 },  /* upper: 0, UL, UR, UL+UR */
    { 0, 2, 1,  3 },  /* lower: 0, LL, LR, LL+LR */
};

/* Draw a 1-bit bitmap onto the 4-bit canvas at pixel offset (ox, oy).
   Bitmap is an array of uint32_t bitmasks (bit 0 = leftmost pixel).
   Half-column shift is achieved by shifting the bitmap left by ox bits. */
static void canvas_draw(uint8_t canvas[][CANVAS_COLS],
                         int ox, int oy,
                         const uint32_t *bitmap, int rows) {
    for (int r = 0; r < rows; r++) {
        int py = oy + r;
        if (py < 0 || py >= CANVAS_ROWS * 2) continue;
        uint32_t bm = bitmap[r] << ox;
        const uint8_t *map = cell_map[py & 1];
        uint8_t *crow = canvas[py / 2];
        for (int c = 0; c < CANVAS_COLS; c++)
            crow[c] |= map[(bm >> (2 * c)) & 3];
    }
}

/* Render a canvas row to a quarter-block string */
static int canvas_render(const uint8_t *row, int cols,
                          char *buf, int bufsize) {
    int last = cols - 1;
    while (last >= 0 && row[last] == 0) last--;
    int pos = 0;
    for (int c = 0; c <= last; c++) {
        const char *ch = qblock[row[c] & 0xF];
        int len = strlen(ch);
        if (pos + len >= bufsize) break;
        memcpy(buf + pos, ch, len);
        pos += len;
    }
    buf[pos] = '\0';
    return pos;
}

static void clawd_init(animation *a) {
    clawd_ctx *c = calloc(1, sizeof(clawd_ctx));
    a->ctx = c;
    c->mod_offset = rand() % MOD_PERIOD;
    c->pose = rand() & 1;
    c->last_tick = -1;
    c->jump_chance = sl_option_int("CLAWD_JUMP", JUMP_CHANCE);

    /* Convert pixel art strings to bitmasks (once, shared) */
    static int bitmaps_ready;
    if (!bitmaps_ready) {
        for (int p = 0; p < N_POSES; p++)
            for (int r = 0; r < PX_ROWS; r++) {
                uint32_t bm = 0;
                for (int x = 0; x < PX_WIDTH; x++)
                    if (poses[p][r][x] != ' ')
                        bm |= 1U << x;
                bitmaps[p][r] = bm;
            }
        bitmaps_ready = 1;
    }
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

    /* Walk updates run per-tick (not per-frame) so that
       LEGS_TICKS and MOD_PERIOD match clawd1 directly.
       Jump updates run per-frame for smooth curves. */
    if (c->action == ACT_WALK) {
        if (tick != c->last_tick) {
            c->last_tick = tick;
            update_walk(c, tick);
        }
    } else {
        update_jump(c);
    }

    /* Draw pose onto canvas */
    uint8_t canvas[CANVAS_ROWS][CANVAS_COLS];
    memset(canvas, 0, sizeof(canvas));
    int py = GROUND_ROW * 2 - c->dy;
    canvas_draw(canvas, sx, py, bitmaps[c->pose], PX_ROWS);

    /* Render canvas to screen */
    char buf[256];
    for (int r = 0; r < CANVAS_ROWS; r++) {
        canvas_render(canvas[r], CANVAS_COLS, buf, sizeof(buf));
        art_putline(r, buf);
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
