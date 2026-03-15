/*
 * art/invader.c - Space Invaders animations
 *
 * Sprites are defined as pixel bitmaps and encoded at init time
 * into quarter-block characters.  Individual aliens pre-compute
 * both normal and half-column-shifted images for smooth sub-column
 * movement (step=50).
 */

#include "art.h"
#include "qblock.h"
#include "../sl.h"

#define PIXEL_ROWS   8    /* pixel rows per alien sprite */
#define ALIEN_ROWS   5    /* text rows for formation ((PIXEL_ROWS + 2) / 2) */
#define SOLO_ROWS    7    /* text rows for individual aliens (matches SL) */
#define N_COLS       3    /* aliens per formation row */
#define COL_GAP      6    /* columns between aliens */
#define ROW_GAP      1    /* text rows between alien type rows */
#define N_TYPES      3    /* formation types: squid, crab, octopus */
#define MAX_FRAMES   2    /* max frames per sprite (for struct sizing) */
#define FRAME_DIV    8    /* ticks per frame change */

#define INVADER_HEIGHT (N_TYPES * ALIEN_ROWS + (N_TYPES - 1) * ROW_GAP)

/*
 * Pixel data: each sprite_px has a width and 8 rows of '@'/' ' strings.
 * Per-type arrays; frame count derived from array size.
 */
typedef struct {
    int width;
    const char *px[PIXEL_ROWS];
} sprite_px;

static const sprite_px squid_px[] = {
    { 18, { "       @@@@       ",
            "     @@@@@@@@     ",
            "   @@@@@@@@@@@@   ",
            " @@@@  @@@@  @@@@ ",
            " @@@@@@@@@@@@@@@@ ",
            "     @@    @@     ",
            "   @@  @@@@  @@   ",
            " @@  @@    @@  @@ " } },
    { 18, { "       @@@@       ",
            "     @@@@@@@@     ",
            "   @@@@@@@@@@@@   ",
            " @@@@  @@@@  @@@@ ",
            " @@@@@@@@@@@@@@@@ ",
            "   @@  @@@@  @@   ",
            " @@            @@ ",
            "   @@        @@   " } },
};

static const sprite_px crab_px[] = {
    { 24, { "     @@          @@     ",
            "       @@      @@       ",
            "     @@@@@@@@@@@@@@     ",
            "   @@@@  @@@@@@  @@@@   ",
            " @@@@@@@@@@@@@@@@@@@@@@ ",
            " @@  @@@@@@@@@@@@@@  @@ ",
            " @@  @@          @@  @@ ",
            "       @@@@  @@@@       " } },
    { 24, { "     @@          @@     ",
            " @@    @@      @@    @@ ",
            " @@  @@@@@@@@@@@@@@  @@ ",
            " @@@@@@  @@@@@@  @@@@@@ ",
            " @@@@@@@@@@@@@@@@@@@@@@ ",
            "   @@@@@@@@@@@@@@@@@@   ",
            "     @@          @@     ",
            "   @@              @@   " } },
};

static const sprite_px octopus_px[] = {
    { 26, { "         @@@@@@@@         ",
            "   @@@@@@@@@@@@@@@@@@@@   ",
            " @@@@@@@@@@@@@@@@@@@@@@@@ ",
            " @@@@@@    @@@@    @@@@@@ ",
            " @@@@@@@@@@@@@@@@@@@@@@@@ ",
            "     @@@@@@    @@@@@@     ",
            "   @@@@    @@@@    @@@@   ",
            " @@@@                @@@@ " } },
    { 26, { "         @@@@@@@@         ",
            "   @@@@@@@@@@@@@@@@@@@@   ",
            " @@@@@@@@@@@@@@@@@@@@@@@@ ",
            " @@@@@@    @@@@    @@@@@@ ",
            " @@@@@@@@@@@@@@@@@@@@@@@@ ",
            "     @@@@@@    @@@@@@     ",
            "   @@@@    @@@@    @@@@   ",
            "     @@@@        @@@@     " } },
};

static const sprite_px ufo_px[] = {
    { 34, { "                                  ",
            "           @@@@@@@@@@@@           ",
            "       @@@@@@@@@@@@@@@@@@@@       ",
            "     @@@@@@@@@@@@@@@@@@@@@@@@     ",
            "   @@@@  @@@@  @@@@  @@@@  @@@@   ",
            " @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ",
            "     @@@@@@@@        @@@@@@@@     ",
            "       @@@@            @@@@       " } },
};

typedef struct {
    const sprite_px *frames;
    int n_frames;
} sprite_type;

#define SPRITE_TYPE(arr) { (arr), sizeof(arr) / sizeof((arr)[0]) }

static const sprite_type alien_types[] = {
    SPRITE_TYPE(squid_px),
    SPRITE_TYPE(crab_px),
    SPRITE_TYPE(octopus_px),
    SPRITE_TYPE(ufo_px),
};


#define N_SUBX 2   /* 0=normal, 1=half-column shifted */

static int build_sprite(const sprite_px *sp, int n_rows, char *out[], int shifted) {
    int w = sp->width;
    char blank[w + 1];
    memset(blank, ' ', w);
    blank[w] = '\0';

    /* pad with blank rows: top gets extra rows, bottom gets 1 */
    int total_px = n_rows * 2;
    int top_pad = total_px - PIXEL_ROWS - 1;
    const char *rows[total_px];
    for (int i = 0; i < top_pad; i++)
        rows[i] = blank;
    for (int i = 0; i < PIXEL_ROWS; i++)
        rows[top_pad + i] = sp->px[i];
    rows[total_px - 1] = blank;

    for (int r = 0; r < n_rows; r++)
        out[r] = qb_encode_row(rows[r * 2], rows[r * 2 + 1], w, shifted);
    return shifted ? (w / 2 + 1) : (w / 2);
}

/*
 * Previous half-block encoded sprites (without half-column shift):
 *

 * Squid F1: "  ▄██▄  "
             "▄█▀██▀█▄"
             "▀▀█▀▀█▀▀"
             "▄▀▄▀▀▄▀▄"

 * Squid F2: "  ▄██▄  "
             "▄█▀██▀█▄"
             "▀█▀██▀█▀"
             "▀▄    ▄▀"

 * Crab  F1: "  ▀▄   ▄▀  "
             " ▄█▀███▀█▄ "
             "█▀███████▀█"
             "▀ ▀▄▄ ▄▄▀ ▀"

 * Crab  F2: "▄ ▀▄   ▄▀ ▄"
             "█▄█▀███▀█▄█"
             "▀█████████▀"
             " ▄▀     ▀▄ "

 * Octo  F1: " ▄▄▄████▄▄▄ "
             "███▀▀██▀▀███"
             "▀▀███▀▀███▀▀"
             "▄█▀  ▀▀  ▀█▄"

 * Octo  F2: " ▄▄▄████▄▄▄ "
             "███▀▀██▀▀███"
             "▀▀███▀▀███▀▀"
             " ▀█▄ ▀▀ ▄█▀ "
 */

/*---------------------------------------------------------------
 * Formation animation
 *---------------------------------------------------------------*/

typedef struct {
    int n_frames;
    int width;
    char *rows[MAX_FRAMES][INVADER_HEIGHT];
} invader_ctx;

static int append_spaces(char *buf, int pos, int n) {
    memset(buf + pos, ' ', n);
    return pos + n;
}

static int append_str(char *buf, int pos, const char *s) {
    int len = strlen(s);
    memcpy(buf + pos, s, len);
    return pos + len;
}

static void build_formation(invader_ctx *c) {
    int n_frames = c->n_frames;
    int dw[N_TYPES];
    char *sprites[N_TYPES][n_frames][ALIEN_ROWS];

    for (int t = 0; t < N_TYPES; t++)
        for (int f = 0; f < n_frames; f++)
            dw[t] = build_sprite(&alien_types[t].frames[f], ALIEN_ROWS, sprites[t][f], /*shifted=*/0);

    int max_w = N_COLS * dw[N_TYPES - 1] + (N_COLS - 1) * COL_GAP;
    c->width = max_w;

    for (int frame = 0; frame < n_frames; frame++) {
        int row = 0;
        for (int type = 0; type < N_TYPES; type++) {
            if (type > 0) {
                c->rows[frame][row] = strdup("");
                row++;
            }
            int type_w = N_COLS * dw[type] + (N_COLS - 1) * COL_GAP;
            int pad = (max_w - type_w) / 2;

            for (int ar = 0; ar < ALIEN_ROWS; ar++) {
                char buf[512];
                int pos = 0;
                pos = append_spaces(buf, pos, pad);
                for (int col = 0; col < N_COLS; col++) {
                    if (col > 0)
                        pos = append_spaces(buf, pos, COL_GAP);
                    pos = append_str(buf, pos, sprites[type][frame][ar]);
                }
                buf[pos] = '\0';
                c->rows[frame][row] = strdup(buf);
                row++;
            }
        }
    }

    for (int t = 0; t < N_TYPES; t++)
        for (int f = 0; f < n_frames; f++)
            for (int r = 0; r < ALIEN_ROWS; r++)
                free(sprites[t][f][r]);
}

static void invader_init(animation *a) {
    invader_ctx *c = calloc(1, sizeof(invader_ctx));
    c->n_frames = alien_types[0].n_frames;
    a->ctx = c;
    build_formation(c);
    a->width = c->width;
}

static void invader_draw(animation *a, int tick) {
    invader_ctx *c = a->ctx;
    int frame = (tick / FRAME_DIV) % c->n_frames;
    for (int y = 0; y < INVADER_HEIGHT; y++)
        art_putline(y, c->rows[frame][y]);
}

static void invader_cleanup(animation *a) {
    invader_ctx *c = a->ctx;
    for (int f = 0; f < c->n_frames; f++)
        for (int r = 0; r < INVADER_HEIGHT; r++)
            free(c->rows[f][r]);
    free(c);
    a->ctx = NULL;
}

animation invader_animation = {
    .name    = "invader",
    .height  = INVADER_HEIGHT,
    .step    = 100,
    .delay   = 60000,
    .init    = invader_init,
    .draw    = invader_draw,
    .cleanup = invader_cleanup,
};

/*---------------------------------------------------------------
 * Individual alien animations
 *---------------------------------------------------------------*/

typedef struct {
    int n_frames;
    char *rows[N_SUBX][MAX_FRAMES][SOLO_ROWS];
} alien_ctx;

static void alien_init(animation *a, int type) {
    const sprite_type *st = &alien_types[type];
    alien_ctx *c = calloc(1, sizeof(alien_ctx));
    c->n_frames = st->n_frames;
    a->ctx = c;
    for (int s = 0; s < N_SUBX; s++)
        for (int f = 0; f < c->n_frames; f++)
            a->width = build_sprite(&st->frames[f], SOLO_ROWS, c->rows[s][f], s);
}

static void alien_cleanup(animation *a) {
    alien_ctx *c = a->ctx;
    for (int s = 0; s < N_SUBX; s++)
        for (int f = 0; f < c->n_frames; f++)
            for (int r = 0; r < SOLO_ROWS; r++)
                free(c->rows[s][f][r]);
    free(c);
    a->ctx = NULL;
}

#define DEFINE_ALIEN(cname, idx) \
    static void cname##_init(animation *a) { alien_init(a, idx); } \
    static void cname##_draw(animation *a, int tick) { \
        alien_ctx *c = a->ctx; \
        int frame = (tick / FRAME_DIV) % c->n_frames; \
        int sx = art_subx ? 1 : 0; \
        for (int y = 0; y < SOLO_ROWS; y++) \
            art_putline(y, c->rows[sx][frame][y]); \
        putchar('\n'); \
    } \
    animation cname##_animation = { \
        .name    = #cname, \
        .height  = SOLO_ROWS, \
        .step    = 50, \
        .init    = cname##_init, \
        .draw    = cname##_draw, \
        .cleanup = alien_cleanup, \
    }

DEFINE_ALIEN(squid,   0);
DEFINE_ALIEN(crab,    1);
DEFINE_ALIEN(octopus, 2);
DEFINE_ALIEN(ufo,     3);
