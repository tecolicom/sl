/*
 * art/mtoyoda/sl.c - D51, C51, Modern animations (from mtoyoda/sl)
 */

#include "../art.h"
#include "../../sl.h"
#include "sl.h"

/*---------------------------------------------------------------
 * Train definition: static frame data + per-train parameters
 *---------------------------------------------------------------*/

#define SMOKE_HEIGHT  6   /* rows reserved for smoke above body */
#define MAX_PATTERNS  6
#define MAX_LINES    12   /* max body lines (C51 has 11) */

typedef struct {
    int n_patterns;
    int n_lines;          /* body lines per frame */
    int frame_div;        /* frame divisor (1=every tick, 3=every 3 ticks) */
    int funnel;           /* smoke funnel column offset */
    int art_width;        /* total width including coal/cars */
    const char *frames[MAX_PATTERNS][MAX_LINES];
} train_def;

/*---------------------------------------------------------------
 * Smoke particle system (ported from mtoyoda/sl.c add_smoke)
 *---------------------------------------------------------------*/

#define SMOKE_PTNS   16
#define SMOKE_INTV    4   /* tick interval between smoke updates */
#define MAX_SMOKE  1000

static const char *SmokePtns[2][SMOKE_PTNS] = {
    {"(   )", "(    )", "(    )", "(   )", "(  )",
     "(  )" , "( )"   , "( )"   , "()"   , "()"  ,
     "O"    , "O"     , "O"     , "O"    , "O"   ,
     " "                                          },
    {"(@@@)", "(@@@@)", "(@@@@)", "(@@@)", "(@@)",
     "(@@)" , "(@)"   , "(@)"   , "@@"   , "@@"  ,
     "@"    , "@"     , "@"     , "@"    , "@"   ,
     " "                                          }
};

static const int smoke_dy[SMOKE_PTNS] = {
    2,  1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static const int smoke_dx[SMOKE_PTNS] = {
    -2, -1, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3
};

typedef struct {
    int rel_y;    /* row relative to art top (0 = topmost smoke row) */
    int rel_x;    /* column relative to art_x */
    int ptrn;     /* evolution stage 0-15 */
    int kind;     /* alternating kind 0 or 1 */
} smoke_particle;

/*---------------------------------------------------------------
 * D51 frame data (locomotive + coal car concatenated)
 *---------------------------------------------------------------*/

static const train_def d51_def = {
    .n_patterns = D51PATTERNS,
    .n_lines    = D51HEIGHT,   /* 10 */
    .frame_div  = 1,
    .funnel     = D51FUNNEL,
    .art_width  = D51LENGTH,
    .frames = {
        { D51STR1 COAL01, D51STR2 COAL02, D51STR3 COAL03,
          D51STR4 COAL04, D51STR5 COAL05, D51STR6 COAL06,
          D51STR7 COAL07, D51WHL11 COAL08, D51WHL12 COAL09,
          D51WHL13 COAL10 },
        { D51STR1 COAL01, D51STR2 COAL02, D51STR3 COAL03,
          D51STR4 COAL04, D51STR5 COAL05, D51STR6 COAL06,
          D51STR7 COAL07, D51WHL21 COAL08, D51WHL22 COAL09,
          D51WHL23 COAL10 },
        { D51STR1 COAL01, D51STR2 COAL02, D51STR3 COAL03,
          D51STR4 COAL04, D51STR5 COAL05, D51STR6 COAL06,
          D51STR7 COAL07, D51WHL31 COAL08, D51WHL32 COAL09,
          D51WHL33 COAL10 },
        { D51STR1 COAL01, D51STR2 COAL02, D51STR3 COAL03,
          D51STR4 COAL04, D51STR5 COAL05, D51STR6 COAL06,
          D51STR7 COAL07, D51WHL41 COAL08, D51WHL42 COAL09,
          D51WHL43 COAL10 },
        { D51STR1 COAL01, D51STR2 COAL02, D51STR3 COAL03,
          D51STR4 COAL04, D51STR5 COAL05, D51STR6 COAL06,
          D51STR7 COAL07, D51WHL51 COAL08, D51WHL52 COAL09,
          D51WHL53 COAL10 },
        { D51STR1 COAL01, D51STR2 COAL02, D51STR3 COAL03,
          D51STR4 COAL04, D51STR5 COAL05, D51STR6 COAL06,
          D51STR7 COAL07, D51WHL61 COAL08, D51WHL62 COAL09,
          D51WHL63 COAL10 },
    }
};

/*---------------------------------------------------------------
 * Context and callbacks
 *---------------------------------------------------------------*/

typedef struct {
    const train_def *def;
    smoke_particle smoke[MAX_SMOKE];
    int n_smoke;
    int smoke_count;    /* total created, for alternating kind */
} train_ctx;

static void init_with(animation *a, const train_def *def) {
    train_ctx *c = calloc(1, sizeof(train_ctx));
    c->def = def;
    a->ctx = c;
}

static void render_smoke(train_ctx *ctx, const train_def *d) {
    char buf[256];
    for (int row = 0; row < SMOKE_HEIGHT; row++) {
        int w = d->art_width;
        if (w > (int)sizeof(buf) - 1) w = (int)sizeof(buf) - 1;
        memset(buf, ' ', w);
        buf[w] = '\0';
        for (int i = 0; i < ctx->n_smoke; i++) {
            smoke_particle *p = &ctx->smoke[i];
            if (p->rel_y != row) continue;
            if (p->rel_x < 0 || p->rel_x >= w) continue;
            const char *pat = SmokePtns[p->kind][p->ptrn];
            for (int j = 0; pat[j] && p->rel_x + j < w; j++)
                buf[p->rel_x + j] = pat[j];
        }
        art_putline(row, buf);
    }
}

static void update_smoke(train_ctx *ctx, const train_def *d, int tick) {
    /* Every frame: particles drift right relative to art (loco moves left) */
    for (int i = 0; i < ctx->n_smoke; i++)
        ctx->smoke[i].rel_x += 1;

    if (tick % SMOKE_INTV != 0)
        return;

    /* On smoke steps: update existing particles */
    for (int i = 0; i < ctx->n_smoke; i++) {
        smoke_particle *p = &ctx->smoke[i];
        p->rel_x += smoke_dx[p->ptrn];
        p->rel_y -= smoke_dy[p->ptrn];
        if (p->ptrn < SMOKE_PTNS - 1)
            p->ptrn++;
    }

    /* Create new particle at funnel position */
    if (ctx->n_smoke < MAX_SMOKE) {
        ctx->smoke[ctx->n_smoke++] = (smoke_particle){
            .rel_y = SMOKE_HEIGHT - 1,
            .rel_x = d->funnel,
            .ptrn  = 0,
            .kind  = ctx->smoke_count++ % 2,
        };
    }
}

static void draw(animation *a, int tick) {
    train_ctx *c = a->ctx;
    const train_def *d = c->def;
    int frame = (d->n_patterns - tick / d->frame_div % d->n_patterns) % d->n_patterns;

    /* Draw smoke area */
    render_smoke(c, d);

    /* Draw body */
    for (int i = 0; i < d->n_lines; i++)
        art_putline(SMOKE_HEIGHT + i, d->frames[frame][i]);

    /* Update smoke for next frame */
    update_smoke(c, d, tick);
}

static void cleanup(animation *a) {
    free(a->ctx);
    a->ctx = NULL;
}

static void d51_init(animation *a) { init_with(a, &d51_def); }

animation d51_animation = {
    .name    = "d51",
    .height  = SMOKE_HEIGHT + D51HEIGHT,   /* 6 + 10 = 16 */
    .width   = D51LENGTH,
    .step    = 100,
    .delay   = 40000,
    .init    = d51_init,
    .draw    = draw,
    .cleanup = cleanup,
};

/*---------------------------------------------------------------
 * C51 frame data (locomotive + coal car concatenated)
 *---------------------------------------------------------------*/

static const train_def c51_def = {
    .n_patterns = C51PATTERNS,
    .n_lines    = C51HEIGHT,   /* 11 */
    .frame_div  = 1,
    .funnel     = C51FUNNEL,
    .art_width  = C51LENGTH,
    .frames = {
        { C51STR1 COALDEL, C51STR2 COAL01, C51STR3 COAL02,
          C51STR4 COAL03,  C51STR5 COAL04, C51STR6 COAL05,
          C51STR7 COAL06,
          C51WH11 COAL07, C51WH12 COAL08, C51WH13 COAL09,
          C51WH14 COAL10 },
        { C51STR1 COALDEL, C51STR2 COAL01, C51STR3 COAL02,
          C51STR4 COAL03,  C51STR5 COAL04, C51STR6 COAL05,
          C51STR7 COAL06,
          C51WH21 COAL07, C51WH22 COAL08, C51WH23 COAL09,
          C51WH24 COAL10 },
        { C51STR1 COALDEL, C51STR2 COAL01, C51STR3 COAL02,
          C51STR4 COAL03,  C51STR5 COAL04, C51STR6 COAL05,
          C51STR7 COAL06,
          C51WH31 COAL07, C51WH32 COAL08, C51WH33 COAL09,
          C51WH34 COAL10 },
        { C51STR1 COALDEL, C51STR2 COAL01, C51STR3 COAL02,
          C51STR4 COAL03,  C51STR5 COAL04, C51STR6 COAL05,
          C51STR7 COAL06,
          C51WH41 COAL07, C51WH42 COAL08, C51WH43 COAL09,
          C51WH44 COAL10 },
        { C51STR1 COALDEL, C51STR2 COAL01, C51STR3 COAL02,
          C51STR4 COAL03,  C51STR5 COAL04, C51STR6 COAL05,
          C51STR7 COAL06,
          C51WH51 COAL07, C51WH52 COAL08, C51WH53 COAL09,
          C51WH54 COAL10 },
        { C51STR1 COALDEL, C51STR2 COAL01, C51STR3 COAL02,
          C51STR4 COAL03,  C51STR5 COAL04, C51STR6 COAL05,
          C51STR7 COAL06,
          C51WH61 COAL07, C51WH62 COAL08, C51WH63 COAL09,
          C51WH64 COAL10 },
    }
};

static void c51_init(animation *a) { init_with(a, &c51_def); }

animation c51_animation = {
    .name    = "c51",
    .height  = SMOKE_HEIGHT + C51HEIGHT,   /* 6 + 11 = 17 */
    .width   = C51LENGTH,
    .step    = 100,
    .delay   = 40000,
    .init    = c51_init,
    .draw    = draw,
    .cleanup = cleanup,
};

/*---------------------------------------------------------------
 * Modern (LOGO) frame data (locomotive + coal car + 2 cars concatenated)
 *---------------------------------------------------------------*/

static const train_def logo_def = {
    .n_patterns = LOGOPATTERNS,
    .n_lines    = LOGOHEIGHT,   /* 6 */
    .frame_div  = 3,
    .funnel     = LOGOFUNNEL,
    .art_width  = LOGOLENGTH,
    .frames = {
        { LOGO1 LCOAL1 LCAR1 LCAR1, LOGO2 LCOAL2 LCAR2 LCAR2,
          LOGO3 LCOAL3 LCAR3 LCAR3, LOGO4 LCOAL4 LCAR4 LCAR4,
          LWHL11 LCOAL5 LCAR5 LCAR5, LWHL12 LCOAL6 LCAR6 LCAR6 },
        { LOGO1 LCOAL1 LCAR1 LCAR1, LOGO2 LCOAL2 LCAR2 LCAR2,
          LOGO3 LCOAL3 LCAR3 LCAR3, LOGO4 LCOAL4 LCAR4 LCAR4,
          LWHL21 LCOAL5 LCAR5 LCAR5, LWHL22 LCOAL6 LCAR6 LCAR6 },
        { LOGO1 LCOAL1 LCAR1 LCAR1, LOGO2 LCOAL2 LCAR2 LCAR2,
          LOGO3 LCOAL3 LCAR3 LCAR3, LOGO4 LCOAL4 LCAR4 LCAR4,
          LWHL31 LCOAL5 LCAR5 LCAR5, LWHL32 LCOAL6 LCAR6 LCAR6 },
        { LOGO1 LCOAL1 LCAR1 LCAR1, LOGO2 LCOAL2 LCAR2 LCAR2,
          LOGO3 LCOAL3 LCAR3 LCAR3, LOGO4 LCOAL4 LCAR4 LCAR4,
          LWHL41 LCOAL5 LCAR5 LCAR5, LWHL42 LCOAL6 LCAR6 LCAR6 },
        { LOGO1 LCOAL1 LCAR1 LCAR1, LOGO2 LCOAL2 LCAR2 LCAR2,
          LOGO3 LCOAL3 LCAR3 LCAR3, LOGO4 LCOAL4 LCAR4 LCAR4,
          LWHL51 LCOAL5 LCAR5 LCAR5, LWHL52 LCOAL6 LCAR6 LCAR6 },
        { LOGO1 LCOAL1 LCAR1 LCAR1, LOGO2 LCOAL2 LCAR2 LCAR2,
          LOGO3 LCOAL3 LCAR3 LCAR3, LOGO4 LCOAL4 LCAR4 LCAR4,
          LWHL61 LCOAL5 LCAR5 LCAR5, LWHL62 LCOAL6 LCAR6 LCAR6 },
    }
};

static void logo_init(animation *a) { init_with(a, &logo_def); }

animation modern_animation = {
    .name    = "modern",
    .height  = SMOKE_HEIGHT + LOGOHEIGHT,   /* 6 + 6 = 12 */
    .width   = LOGOLENGTH,
    .step    = 100,
    .delay   = 40000,
    .init    = logo_init,
    .draw    = draw,
    .cleanup = cleanup,
};
