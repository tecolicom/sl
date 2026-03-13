#include "coupler.h"

typedef struct {
    char dch_seq[20];
    int clear_col;
    int sweep_all;
    int start_y;
    int height;
    int last_col;
} sweep_ctx;

static void origin(coupler *cpl) {
    sweep_ctx *ctx = malloc(sizeof(sweep_ctx));
    memset(ctx, 0, sizeof(*ctx));

    int step = -sl_step;
    char *dchp = tparm(tigetstr("dch"), step);
    if (dchp != NULL)
        strcpy(ctx->dch_seq, dchp);

    ctx->clear_col = sl_option_int("SWEEP_COL", 0);
    ctx->sweep_all = sl_option_bool("SWEEP_ALL");
    ctx->height = sl_art_height;
    ctx->start_y = LINES - ctx->height - 1;

    ctx->last_col = COLS + 1;
    cpl->ctx = ctx;
}

static void arriving(coupler *cpl, int x) {
    sweep_ctx *ctx = cpl->ctx;
    if (x >= ctx->clear_col) return;
    if (x == ctx->last_col) return;
    ctx->last_col = x;

    int y0 = ctx->sweep_all ? 0 : ctx->start_y;
    int y1 = ctx->sweep_all ? LINES : ctx->start_y + ctx->height;
    for (int y = y0; y < y1; y++)
        mvprintw(y, 0, "%s", ctx->dch_seq);
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
