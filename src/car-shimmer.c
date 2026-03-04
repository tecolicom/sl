#include <stdlib.h>
#include "sl-coupler.h"

typedef struct {
    int scan_pos;
    int scan_step;
    int trail_div;
    int row;
} shimmer_ctx;

static void shimmer_origin(coupler *cpl) {
    shimmer_ctx *ctx = malloc(sizeof(shimmer_ctx));
    ctx->row = LINES - 1;
    ctx->scan_pos = 0;
    ctx->scan_step = COLS / 12;
    if (ctx->scan_step < 2) ctx->scan_step = 2;
    ctx->scan_step = -ctx->scan_step;  /* default: right-to-left */
    ctx->trail_div = 1;
    cpl->ctx = ctx;
}

/* Shimmer line – truecolor foreground on U+2594 (▔).
   Asymmetric comet shape: sharp leading edge, long trailing tail.
   Uses direct ANSI sequences because terminfo has no truecolor support. */
static void shimmer_departed(coupler *cpl, int x) {
    shimmer_ctx *ctx = cpl->ctx;
    int cols = COLS;
    int center = ctx->scan_pos;
    int dir = ctx->scan_step;
    int trail_div = ctx->trail_div;

    tputs(tparm(tgoto(cursor_address, 0, ctx->row)), 1, putchar);
    int hw_lead = cols / 16;
    int hw_trail = cols / trail_div;
    if (hw_lead < 2) hw_lead = 2;
    if (hw_trail < 8) hw_trail = 8;
    int pr = -1, pg = -1, pb = -1;
    for (int col = 0; col < cols; col++) {
        int delta = col - center;
        if (delta >  cols / 2) delta -= cols;
        if (delta < -cols / 2) delta += cols;
        int dist = abs(delta);
        int lead = (dir > 0) ? (delta > 0) : (delta < 0);
        int hw = lead ? hw_lead : hw_trail;
        int r, g, b;
        if (dist >= hw) {
            r = 15; g = 12; b = 10;
        } else {
            long d = hw - dist;
            int v = lead
                ? (int)(d * d * 255 / (hw * hw))
                : (int)(d * d * d * 255 / ((long)hw * hw * hw));
            r = 15 + 240 * v / 255;
            g = 12 + 208 * v / 255;
            b = 10 + 190 * v / 255;
        }
        if (r != pr || g != pg || b != pb) {
            printf("\033[38;2;%d;%d;%dm", r, g, b);
            pr = r; pg = g; pb = b;
        }
        fputs("\xe2\x96\x94", stdout);              /* ▔ */
    }
    printf("\033[0m");

    ctx->scan_pos = (ctx->scan_pos + ctx->scan_step + cols) % cols;
}

static void shimmer_terminal(coupler *cpl) {
    shimmer_ctx *ctx = cpl->ctx;
    printf("\033[0m");
    tputs(tparm(tgoto(cursor_address, 0, ctx->row)), 1, putchar);
    tputs(tigetstr("el"), 1, putchar);
    fflush(stdout);
    free(ctx);
}

coupler shimmer_coupler(void) {
    return (coupler){
        .origin   = shimmer_origin,
        .departed = shimmer_departed,
        .terminal = shimmer_terminal,
    };
}
