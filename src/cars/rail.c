#include "coupler.h"

/* Upper block elements: thin / thick */
static const char *blocks[] = {
    "\xe2\x96\x94",  /* ▔ U+2594 UPPER ONE EIGHTH BLOCK */
    "\xe2\x96\x80",  /* ▀ U+2580 UPPER HALF BLOCK       */
};

typedef struct {
    int scan_pos;
    int scan_step;
    int trail_div;
    int row;
    int use_truecolor;
    int use_bg;     /* Apple Terminal: background-colored spaces */
    int rumble;     /* ▔/▀ thickness modulation */
} rail_ctx;

static void origin(coupler *cpl) {
    rail_ctx *ctx = malloc(sizeof(rail_ctx));
    memset(ctx, 0, sizeof(*ctx));
    ctx->row = LINES - 1;
    ctx->scan_pos = 0;
    ctx->scan_step = COLS / 12;
    if (ctx->scan_step < 2) ctx->scan_step = 2;
    ctx->scan_step = -ctx->scan_step;  /* default: right-to-left */
    ctx->trail_div = 1;

    const char *ct = getenv("COLORTERM");
    ctx->use_truecolor = ct &&
        (strcmp(ct, "truecolor") == 0 || strcmp(ct, "24bit") == 0);

    const char *mode = sl_option("CAR_RAIL");
    ctx->rumble = mode && strcmp(mode, "rumble") == 0;

    const char *tp = getenv("TERM_PROGRAM");
    int apple_terminal = tp && strcmp(tp, "Apple_Terminal") == 0;
    ctx->use_bg = apple_terminal;

    cpl->ctx = ctx;
}

/* Rail effect on the bottom row.
   Asymmetric gradient: sharp leading edge, long trailing tail.
   Two modes: "shimmer" (thin ▔ only, default) and
   "rumble" (▔/▀ thickness modulation via SL_CAR_RAIL=rumble).
   Apple Terminal: background-colored spaces (▔/▀ are ambiguous-width).
   Falls back to 256-color when COLORTERM is not truecolor/24bit. */
static void departed(coupler *cpl, int x) {
    rail_ctx *ctx = cpl->ctx;
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
        int r, g, b, bi;
        if (dist >= hw) {
            r = 15; g = 12; b = 10;
            bi = 0;
        } else {
            long d = hw - dist;
            int v = lead
                ? (int)(d * d * 255 / (hw * hw))
                : (int)(d * d * d * 255 / ((long)hw * hw * hw));
            r = 15 + 240 * v / 255;
            g = 12 + 208 * v / 255;
            b = 10 + 190 * v / 255;
            bi = (ctx->rumble && !ctx->use_bg) ? v % 2 : 0;
        }
        if (r != pr || g != pg || b != pb) {
            if (ctx->use_truecolor)
                printf(ctx->use_bg ? "\033[48;2;%d;%d;%dm"
                                   : "\033[38;2;%d;%d;%dm", r, g, b);
            else {
                int ci = 16
                    + 36 * (r < 48 ? 0 : r < 115 ? 1 : (r - 35) / 40)
                    +  6 * (g < 48 ? 0 : g < 115 ? 1 : (g - 35) / 40)
                    +      (b < 48 ? 0 : b < 115 ? 1 : (b - 35) / 40);
                printf(ctx->use_bg ? "\033[48;5;%dm"
                                   : "\033[38;5;%dm", ci);
            }
            pr = r; pg = g; pb = b;
        }
        if (ctx->use_bg)
            putchar(' ');
        else
            fputs(blocks[bi], stdout);
    }
    printf("\033[0m");

    ctx->scan_pos = (ctx->scan_pos + ctx->scan_step + cols) % cols;
}

static void terminal(coupler *cpl) {
    rail_ctx *ctx = cpl->ctx;
    printf("\033[0m");
    tputs(tparm(tgoto(cursor_address, 0, ctx->row)), 1, putchar);
    tputs(tigetstr("el"), 1, putchar);
    fflush(stdout);
    free(ctx);
}

coupler rail_coupler(void) {
    return (coupler){
        .origin   = origin,
        .departed = departed,
        .terminal = terminal,
    };
}
