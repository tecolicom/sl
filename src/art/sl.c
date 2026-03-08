/*
 * art/sl.c - SL animation
 */

#include "art.h"
#include <string.h>
#include <stdlib.h>

#define SL_HEIGHT 7
#define SMOKE_BUFSZ 1024

static const char *sl_art[SL_HEIGHT] = {
    "      o o o o o",
    "     o         ",
    "    o  ┯━━┯    ",
    "   ╒╕  │OO│    ",
    " ┌─┘└──┘  │    ",
    "┌┘        │    ",
    "┴─O=O O=O─┴ з  \n"
};

typedef struct {
    char smoke[SMOKE_BUFSZ];
    char *lines[SL_HEIGHT];
} sl_ctx;

static void sl_init(animation *a) {
    sl_ctx *c = calloc(1, sizeof(sl_ctx));
    a->ctx = c;
    strcpy(c->smoke, sl_art[0]);
    c->lines[0] = c->smoke;
    for (int i = 1; i < SL_HEIGHT; i++)
        c->lines[i] = (char *)sl_art[i];
}

static void sl_draw(animation *a, int tick) {
    sl_ctx *c = a->ctx;
    for (int y = 0; y < a->height; y++) {
        art_goto(y);
        art_puts(c->lines[y]);
    }
    if (strlen(c->smoke) + 3 < SMOKE_BUFSZ)
        strcat(c->smoke, " o");
}

static void sl_cleanup(animation *a) {
    free(a->ctx);
    a->ctx = NULL;
}

animation sl_animation = {
    .name    = "sl",
    .height  = SL_HEIGHT,
    .init    = sl_init,
    .draw    = sl_draw,
    .cleanup = sl_cleanup,
};
