/*
 * art/sl.c - SL animation with grayscale smoke fading
 */

#include "art.h"
#include "../sl.h"
#include <string.h>
#include <stdlib.h>

#define SL_HEIGHT    7
#define MAX_PUFFS    512
#define SMOKE_PREFIX 6     /* leading spaces before first 'o' */
#define INITIAL_PUFFS 5    /* number of 'o' in initial art */
#define SMOKE_MAX_AGE 24   /* 24 grayscale levels (232-255) */

static const char *sl_art[SL_HEIGHT] = {
    "      o o o o o",
    "     o         ",
    "    o  ┯━━┯    ",
    "   ╒╕  │OO│    ",
    " ┌─┘└──┘  │    ",
    "┌┘        │    ",
    "┴─O=O O=O─┴ з  "
};

typedef struct {
    int n_puffs;
    int dark_mode;
    int fade_offset;       /* extra fade for post-stop fadeout */
    char *art_rows[SL_HEIGHT];
} sl_ctx;

/* Render smoke row with distance-based grayscale coloring.
   Puffs near the chimney (low index) are bright, far puffs are faded.
   Handles left clipping (art_skip) and right clipping (art_maxcols). */
static void render_smoke(sl_ctx *c) {
    int total_width = SMOKE_PREFIX + c->n_puffs * 2;
    int skip = art_skip;
    int maxcols = art_maxcols;
    int col_out = 0;

    for (int col = 0; col < total_width && col_out < maxcols; col++) {
        /* Left clipping */
        if (col < skip)
            continue;

        if (col < SMOKE_PREFIX) {
            putchar(' ');
            col_out++;
        } else {
            int puff_idx = (col - SMOKE_PREFIX) / 2;
            int is_puff = ((col - SMOKE_PREFIX) % 2 == 0);

            if (is_puff && puff_idx < c->n_puffs) {
                int fade = puff_idx + c->fade_offset;
                if (fade >= SMOKE_MAX_AGE) {
                    putchar(' ');
                } else {
                    int color_code;
                    if (c->dark_mode)
                        color_code = 255 - fade;  /* bright → dark */
                    else
                        color_code = 232 + fade;  /* dark → bright */
                    printf("\033[38;5;%dm", color_code);
                    putchar('o');
                    printf("\033[0m");
                }
            } else {
                putchar(' ');
            }
            col_out++;
        }
    }

    tputs(clr_eol, 1, putchar);
}

static void sl_init(animation *a) {
    sl_ctx *c = calloc(1, sizeof(sl_ctx));
    a->ctx = c;
    c->dark_mode = sl_option_bool("DARK");
    c->n_puffs = INITIAL_PUFFS;
    c->fade_offset = 0;

    c->art_rows[0] = NULL;  /* row 0 rendered manually */
    for (int i = 1; i < SL_HEIGHT; i++)
        c->art_rows[i] = (char *)sl_art[i];
}

static void sl_draw(animation *a, int tick) {
    sl_ctx *c = a->ctx;

    /* Row 0: smoke with grayscale fading */
    art_goto(0);
    render_smoke(c);

    /* Rows 1-6: unchanged art */
    for (int y = 1; y < a->height; y++) {
        art_goto(y);
        art_puts(c->art_rows[y]);
    }

    putchar('\n');

    /* Add new puff when fully visible on screen */
    if (art_skip == 0 && c->n_puffs < MAX_PUFFS)
        c->n_puffs++;

    /* Update width to include visible smoke trail for -g exit */
    int visible = c->n_puffs < SMOKE_MAX_AGE ? c->n_puffs : SMOKE_MAX_AGE;
    int smoke_width = SMOKE_PREFIX + visible * 2;
    if (smoke_width > a->width)
        a->width = smoke_width;
}

static void sl_cleanup(animation *a) {
    sl_ctx *c = a->ctx;
    int delay = a->delay > 0 ? a->delay : DEFAULT_DELAY;
    int speed = sl_option_int("SPEED", 1);
    if (speed > 1) delay /= speed;

    /* Skip fadeout if train has left the screen */
    if (art_skip == 0) {
        /* Post-stop fadeout: increment fade_offset until all puffs gone */
        while (c->fade_offset < c->n_puffs + SMOKE_MAX_AGE) {
            c->fade_offset += 2;
            if (c->fade_offset >= SMOKE_MAX_AGE)
                break;
            art_goto(0);
            render_smoke(c);
            /* Fade chimney smoke on rows 1-2 */
            static const int chimney_col[] = { 5, 4 };
            for (int i = 0; i < 2; i++) {
                if (chimney_col[i] + 1 > art_maxcols) continue;
                int color_code;
                if (c->dark_mode)
                    color_code = 255 - c->fade_offset;
                else
                    color_code = 232 + c->fade_offset;
                art_goto(i + 1);
                for (int j = 0; j < chimney_col[i]; j++)
                    putchar(' ');
                printf("\033[38;5;%dmo\033[0m", color_code);
            }
            art_goto(a->height - 1);
            putchar('\n');
            fflush(stdout);
            usleep(delay);
        }

    }

    /* Clear all smoke rows */
    art_goto(0);
    tputs(clr_eol, 1, putchar);
    art_goto(1);
    art_puts("      ");   /* clear smoke 'o' at column 5 */
    art_goto(2);
    art_puts("     ");    /* clear smoke 'o' at column 4 */

    /* Move cursor below art area */
    art_goto(a->height - 1);
    putchar('\n');
    fflush(stdout);

    free(a->ctx);
    a->ctx = NULL;
}

animation sl_animation = {
    .name    = "sl",
    .height  = SL_HEIGHT,
    .width   = 16,
    .step    = 200,
    .init    = sl_init,
    .draw    = sl_draw,
    .cleanup = sl_cleanup,
};
