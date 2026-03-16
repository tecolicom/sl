/*
 * art/clawd2.c - Claude Code mascot animation
 *
 * 1-bit pixel art is drawn onto a 4-bit canvas at pixel coordinates,
 * then rendered via quarter-block lookup.  Half-column and half-row
 * movement are handled as simple coordinate offsets.
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
#define GROUND_ROW    4    /* ground level in cell rows (pixel y = GROUND_ROW * 2) */

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

/* 4-bit canvas: each cell holds a quarter-block pattern (2×2 pixels).
   1-bit bitmaps are drawn onto the canvas at pixel coordinates,
   then rendered to screen via qblock[] lookup. */
#define CANVAS_ROWS  CLAWD_HEIGHT
#define CANVAS_COLS  (PX_WIDTH / 2 + 1)  /* +1 for half-column shift */

static const int pixel_bit[2][2] = {
    { 8, 4 },  /* upper: UL, UR */
    { 2, 1 },  /* lower: LL, LR */
};

/* Draw a 1-bit bitmap onto the 4-bit canvas at pixel offset (ox, oy) */
static void canvas_draw(uint8_t canvas[][CANVAS_COLS],
                         int ox, int oy,
                         const char *bitmap[], int rows, int width) {
    for (int r = 0; r < rows; r++) {
        int py = oy + r;
        if (py < 0 || py >= CANVAS_ROWS * 2) continue;
        for (int x = 0; x < width; x++) {
            if (bitmap[r][x] == ' ') continue;
            int px = ox + x;
            if (px < 0 || px >= CANVAS_COLS * 2) continue;
            canvas[py / 2][px / 2] |= pixel_bit[py & 1][px & 1];
        }
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

    /* Draw pose onto canvas */
    uint8_t canvas[CANVAS_ROWS][CANVAS_COLS];
    memset(canvas, 0, sizeof(canvas));
    int py = GROUND_ROW * 2 - c->dy;
    canvas_draw(canvas, sx, py, poses[c->pose], PX_ROWS, PX_WIDTH);

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
