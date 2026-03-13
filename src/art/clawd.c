/*
 * art/clawd.c - Claude Code mascot animation
 *
 * The mascot from Claude Code's startup screen, running across
 * the terminal with a spinner above its head cycling through
 * · ✢ ✳ ✶ ✻ ✽ (same as Claude Code's thinking indicator).
 *
 * Hat variants can be enabled via SL_HAT environment variable.
 */

#include <math.h>
#include "art.h"
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

/* Hat variants: 2-line arrays (stick + brim) */
#define HAT_LINES 2

typedef struct {
    const char *name;
    const char *art[HAT_LINES];
} hat_def;

static const hat_def hats[] = {
    { "none",  { "", "" } },
    { "party", { "    |",
                 "   ▟█▙" } },
};

/*
 * Block elements reference:
 *   Quarter blocks:  ▘ upper-left   ▝ upper-right
 *                    ▖ lower-left   ▗ lower-right
 *   Half blocks:     ▌ left   ▐ right
 *   3/4 blocks:      ▛ UL+UR+LL   ▜ UL+UR+LR   ▙ UL+LL+LR   ▟ UR+LL+LR
 *   Full block:      █
 */
#define POSE_LINES 3  /* head + body + legs */
static const char *pose[2][POSE_LINES] = {
    { " ▐▛███▜▌",     /* head */
      "▝▜█████▛▘",    /* body + arms */
      "  ▘▘ ▝▝" },    /* legs open */
    { "▗▟▛███▜▙▖",    /* head + raised arms */
      " ▐█████▌ ",    /* body + arm roots */
      " ▝▝   ▘▘" },   /* legs closed */
};

typedef struct {
    char spinner[SPINNER_BUF];
    const hat_def *hat;
    int sparkle_offset;
    int mod_offset;
    int pose_accum;
    int pose_phase;
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
    if (!hat_name && rand() % 20 == 0)
        hat_name = "party";
    c->hat = find_hat(hat_name);
    c->sparkle_offset = rand() % N_SPARKLE_CYCLE;
    c->mod_offset = rand() % MOD_PERIOD;
    c->pose_phase = rand() & 1;
}

static void clawd_draw(animation *a, int tick) {
    clawd_ctx *c = a->ctx;
    int row = 0;

    /* Row 0: single spinner character above hat center (col 4) */
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

    art_putline(row++, c->hat->art[0]);
    art_putline(row++, c->hat->art[1]);
    for (int i = 0; i < POSE_LINES; i++)
        art_putline(row++, pose[phase][i]);
}

static void clawd_cleanup(animation *a) {
    free(a->ctx);
    a->ctx = NULL;
}

animation clawd_animation = {
    .name    = "clawd",
    .height  = CLAWD_HEIGHT,
    .width   = 10,
    .step    = 100,
    .init    = clawd_init,
    .draw    = clawd_draw,
    .cleanup = clawd_cleanup,
};
