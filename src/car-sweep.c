#include <stdlib.h>
#include <string.h>
#include "sl-coupler.h"

typedef struct {
    char dch2[20];
    int clear_col;
    int sweep_all;
    int start_y;
    int height;
} sweep_ctx;

static void sweep_origin(coupler *cpl, int COLS, int LINES) {
    sweep_ctx *ctx = malloc(sizeof(sweep_ctx));
    memset(ctx, 0, sizeof(*ctx));

    char *dch2p = tparm(tigetstr("dch"), 2);
    if (dch2p != NULL)
        strcpy(ctx->dch2, dch2p);

    char *env = getenv("SL_SWEEP_COL");
    ctx->clear_col = (env && *env) ? atoi(env) : 0;
    ctx->sweep_all = getenv("SL_SWEEP_ALL") != NULL;
    ctx->height = 7;  /* SL art height */
    ctx->start_y = LINES - ctx->height - 1;

    cpl->ctx = ctx;
}

static void sweep_arriving(coupler *cpl, int COLS, int LINES, int x) {
    sweep_ctx *ctx = cpl->ctx;
    if (x > ctx->clear_col) return;

    int y0 = ctx->sweep_all ? 0 : ctx->start_y;
    int y1 = ctx->sweep_all ? LINES : ctx->start_y + ctx->height;
    for (int y = y0; y < y1; y++)
        mvprintw(y, 0, "%s", ctx->dch2);
}

static void sweep_terminal(coupler *cpl, int COLS, int LINES) {
    free(cpl->ctx);
}

coupler sweep_coupler(void) {
    return (coupler){
        .origin = sweep_origin,
        .arriving = sweep_arriving,
        .terminal = sweep_terminal,
    };
}
