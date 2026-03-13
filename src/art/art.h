/*
 * art.h - Animation abstraction for sl-2026
 *
 * Animations implement init/draw/cleanup callbacks.
 * draw() uses art_goto/art_puts to output without knowing
 * absolute coordinates or column clipping.
 */

#ifndef SL_ART_H
#define SL_ART_H

#define DEFAULT_STEP  100       /* percent: 200=2col, 100=1col, 50=half */
#define DEFAULT_DELAY 50000     /* microseconds per 1 column */

typedef struct animation {
    const char *name;
    int height;
    int width;       /* body width in columns (for run-off end condition) */
    int step;        /* percent per frame: 200=2col, 100=1col, 50=half (0 = DEFAULT_STEP) */
    int delay;       /* frame delay in microseconds (0 = auto from step) */
    void *ctx;
    void (*init)(struct animation *a);
    void (*draw)(struct animation *a, int tick);
    void (*cleanup)(struct animation *a);
} animation;

/* Drawing API — set up by art_set_pos before each frame */
extern int art_skip;     /* columns clipped on the left (0 when fully visible) */
extern int art_maxcols;  /* max columns to output (COLS - art_x) */
extern int art_subx;     /* sub-column offset (0 or 50) */
void art_set_pos(int start_y, int x);
void art_goto(int row);
void art_puts(const char *s);
#define art_putline(row, s) \
    (art_goto(row), art_puts(s), tputs(clr_eol, 1, putchar))

/* Animation registry */
extern animation *animations[];
animation *get_animation(const char *name);

#endif
