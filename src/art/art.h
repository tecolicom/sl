/*
 * art.h - Animation abstraction for sl-2026
 *
 * Animations implement init/draw/cleanup callbacks.
 * draw() uses art_goto/art_puts to output without knowing
 * absolute coordinates or column clipping.
 */

#ifndef SL_ART_H
#define SL_ART_H

#define DEFAULT_STEP  2
#define DEFAULT_DELAY 100000

typedef struct animation {
    const char *name;
    int height;
    int step;        /* columns per frame (0 = DEFAULT_STEP) */
    int delay;       /* frame delay in microseconds (0 = DEFAULT_DELAY) */
    void *ctx;
    void (*init)(struct animation *a);
    void (*draw)(struct animation *a, int tick);
    void (*cleanup)(struct animation *a);
} animation;

/* Drawing API — set up by art_set_pos before each frame */
void art_set_pos(int start_y, int x, int maxcols);
void art_goto(int row);
void art_puts(const char *s);

/* Animation registry */
extern animation *animations[];
animation *get_animation(const char *name);

#endif
