/*
 * art/clawd1.c - Claude Code mascot animation
 *
 * The mascot from Claude Code's startup screen, running across
 * the terminal with a spinner above its head cycling through
 * · ✢ ✳ ✶ ✻ ✽ (same as Claude Code's thinking indicator).
 *
 * Hat variants can be enabled via SL_HAT environment variable.
 */

#include <math.h>
#include "art.h"
#include "qblock.h"
#include "../sl.h"

/* Spinner cycle: blank → grow → peak → shrink → blank → ... */
static const char *sparkle_cycle[] = {
    " ", "·", "✳", "✶", "✢", "✻", "✽", "✻", "✢", "✶", "✳", "·"
};
#define N_SPARKLE_CYCLE (sizeof(sparkle_cycle)/sizeof(sparkle_cycle[0]))

#define CLAWD_HEIGHT  6  /* spinner + hat(2) + head + body + legs */
#define SPINNER_BUF  32
#define LEGS_TICKS   10  /* base ticks per leg state */
#define MOD_PERIOD  120  /* ticks per speed modulation cycle */
#define MOD_RANGE    60  /* speed variation ±60% of base */
#define SPEED_SCALE 100
#define POSE_PX_WIDTH 18

/* Hat variants: stick is plain text, brim is pixel art */
#define BRIM_PX_ROWS 2

typedef struct {
    const char *name;
    const char *stick;            /* text */
    const char *brim_px[BRIM_PX_ROWS]; /* pixel art for brim */
    int brim_width;               /* pixel width (0 = no hat) */
} hat_def;

/*
 * Quarter-block encoded hat (for reference):
 *   party:  "    |"       stick (text)
 *           "   ▟█▙"     brim (pixel art encoded)
 */
static const hat_def hats[] = {
    { "none",  "", { "", "" }, 0 },
    { "party", "    |",
               { "       @@@@       ",   /* brim upper */
                 "      @@@@@@      " }, /* brim lower */
               POSE_PX_WIDTH },
};

#define POSE_LINES    3   /* head + body + legs */
#define POSE_PX_ROWS  6   /* POSE_LINES * 2 */

static const char *clawd_px[2][POSE_PX_ROWS] = {
    { "   @@@@@@@@@@@@   ",    /* head upper */
      "   @@ @@@@@@ @@   ",    /* head lower */
      " @@@@@@@@@@@@@@@@ ",    /* body upper */
      "   @@@@@@@@@@@@   ",    /* body lower */
      "    @ @    @ @    ",    /* legs upper */
      "                  " },  /* legs lower */
    { "   @@@@@@@@@@@@   ",    /* head upper (raised arms) */
      " @@@@ @@@@@@ @@@@ ",    /* head lower */
      "   @@@@@@@@@@@@   ",    /* body upper */
      "   @@@@@@@@@@@@   ",    /* body lower */
      "   @ @      @ @   ",    /* legs upper */
      "                  " },  /* legs lower */
};

/*
 * Quarter-block encoded forms (for reference):
 *
 *   Pose 0:  " ▐▛███▜▌"      head
 *            "▝▜█████▛▘"     body + arms
 *            "  ▘▘ ▝▝"       legs open
 *
 *   Pose 1:  "▗▟▛███▜▙▖"     head + raised arms
 *            " ▐█████▌ "     body + arm roots
 *            " ▝▝   ▘▘"      legs closed
 */

typedef struct {
    char spinner[SPINNER_BUF];
    const hat_def *hat;
    int sparkle_offset;
    int mod_offset;
    int pose_accum;
    int pose_phase;
    char *pose[2][POSE_LINES];     /* [phase][line] */
    char *brim;                     /* encoded brim */
} clawd_ctx;

static const hat_def *find_hat(const char *name) {
    if (name && *name) {
        for (int i = 0; i < (int)(sizeof(hats)/sizeof(hats[0])); i++)
            if (strcmp(hats[i].name, name) == 0)
                return &hats[i];
    }
    return &hats[0];  /* none */
}

static void clawd_init(animation *a) {
    clawd_ctx *c = calloc(1, sizeof(clawd_ctx));
    a->ctx = c;
    const char *hat_name = sl_option("HAT");
    if (!hat_name) {
        int pct = sl_option_int("HAT_PCT", 5);
        if (pct > 0 && rand() % 100 < pct)
            hat_name = "party";
    }
    c->hat = find_hat(hat_name);
    c->sparkle_offset = rand() % N_SPARKLE_CYCLE;
    c->mod_offset = rand() % MOD_PERIOD;
    c->pose_phase = rand() & 1;

    /* Encode pixel art poses into quarter-block strings */
    for (int p = 0; p < 2; p++)
        for (int i = 0; i < POSE_LINES; i++)
            c->pose[p][i] = qb_encode_row(
                clawd_px[p][i * 2], clawd_px[p][i * 2 + 1],
                POSE_PX_WIDTH, 0);

    /* Encode brim pixel art into quarter-block string */
    if (c->hat->brim_width > 0)
        c->brim = qb_encode_row(
            c->hat->brim_px[0], c->hat->brim_px[1],
            c->hat->brim_width, 0);
    else
        c->brim = strdup("");
}

static void clawd_draw(animation *a, int tick) {
    clawd_ctx *c = a->ctx;
    int row = 0;

    /* Row 0: single spinner character above hat center */
    const char *ch = sparkle_cycle[(tick + c->sparkle_offset) % N_SPARKLE_CYCLE];
    snprintf(c->spinner, SPINNER_BUF, "    %s", ch);

    art_putline(row++, c->spinner);

    /* FM-like pose modulation: sine wave varies speed smoothly */
    double phase_rad = 2.0 * M_PI * (tick + c->mod_offset) / MOD_PERIOD;
    int speed = SPEED_SCALE + (int)(MOD_RANGE * sin(phase_rad));
    c->pose_accum += speed;
    if (c->pose_accum >= LEGS_TICKS * SPEED_SCALE) {
        c->pose_accum -= LEGS_TICKS * SPEED_SCALE;
        c->pose_phase ^= 1;
    }
    int phase = c->pose_phase;
    art_putline(row++, c->hat->stick);
    art_putline(row++, c->brim);
    for (int i = 0; i < POSE_LINES; i++)
        art_putline(row++, c->pose[phase][i]);
}

static void clawd_cleanup(animation *a) {
    clawd_ctx *c = a->ctx;
    for (int p = 0; p < 2; p++)
        for (int i = 0; i < POSE_LINES; i++)
            free(c->pose[p][i]);
    free(c->brim);
    free(c);
    a->ctx = NULL;
}

animation clawd1_animation = {
    .name    = "clawd1",
    .height  = CLAWD_HEIGHT,
    .width   = 10,
    .step    = 100,
    .init    = clawd_init,
    .draw    = clawd_draw,
    .cleanup = clawd_cleanup,
};
