#include "coupler.h"

typedef struct {
    char dch2[20];
    int clear_col;
    int stop_col;
    int sweep_all;
    int start_y;
    int height;
} sweep_ctx;

static void origin(coupler *cpl) {
    sweep_ctx *ctx = malloc(sizeof(sweep_ctx));
    memset(ctx, 0, sizeof(*ctx));

    char *dch2p = tparm(tigetstr("dch"), 2);
    if (dch2p != NULL)
        strcpy(ctx->dch2, dch2p);

    ctx->clear_col = sl_option_int("SWEEP_COL", 0);
    ctx->stop_col = sl_option_int("STOP_COL", -1);
    ctx->sweep_all = sl_option_bool("SWEEP_ALL");
    ctx->height = 7;  /* SL art height */
    ctx->start_y = LINES - ctx->height - 1;

    cpl->ctx = ctx;
}

static void arriving(coupler *cpl, int x) {
    sweep_ctx *ctx = cpl->ctx;
    if (ctx->stop_col >= 0 && x <= ctx->stop_col) {
        sl_step = 0;
        return;
    }
    if (x > ctx->clear_col) return;

    int y0 = ctx->sweep_all ? 0 : ctx->start_y;
    int y1 = ctx->sweep_all ? LINES : ctx->start_y + ctx->height;
    for (int y = y0; y < y1; y++)
        mvprintw(y, 0, "%s", ctx->dch2);
}

static void terminal(coupler *cpl) {
    free(cpl->ctx);
}

coupler sweep_coupler(void) {
    return (coupler){
        .origin = origin,
        .arriving = arriving,
        .terminal = terminal,
    };
}
