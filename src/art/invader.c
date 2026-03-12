/*
 * art/invader.c - Space Invaders animations
 *
 * Sprites are defined as pixel bitmaps and encoded at init time
 * into quarter-block characters with a half-column shift, giving
 * sub-character edge smoothing (same technique as clawd.c).
 */

#include "art.h"
#include "../sl.h"
#include <string.h>
#include <stdlib.h>

#define PIXEL_ROWS   8    /* pixel rows per alien sprite */
#define ALIEN_ROWS   4    /* text rows (PIXEL_ROWS / 2) */
#define N_COLS       3    /* aliens per formation row */
#define COL_GAP      6    /* columns between aliens */
#define ROW_GAP      1    /* text rows between alien type rows */
#define N_TYPES      3    /* squid, crab, octopus */
#define N_FRAMES     2
#define FRAME_DIV    8    /* ticks per frame change */

#define INVADER_HEIGHT (N_TYPES * ALIEN_ROWS + (N_TYPES - 1) * ROW_GAP)

/*
 * Quarter-block characters indexed by 4-bit pattern: UL UR LL LR
 *
 *   0=" " 1=▗ 2=▖ 3=▄ 4=▝ 5=▐ 6=▞ 7=▟
 *   8=▘  9=▚ A=▌ B=▙ C=▀ D=▜ E=▛ F=█
 */
static const char *qblock[16] = {
    " ", "▗", "▖", "▄", "▝", "▐", "▞", "▟",
    "▘", "▚", "▌", "▙", "▀", "▜", "▛", "█",
};

/*
 * Pixel data: [type][frame], each with width and 8 rows of '0'/'1' strings.
 */
typedef struct {
    int width;
    const char *px[PIXEL_ROWS];
} sprite_px;

static const sprite_px alien_px[N_TYPES][N_FRAMES] = {
    /* Squid (8 wide) */
    {
        { 8, { "00011000",    /*    ##    */
               "00111100",    /*   ####   */
               "01111110",    /*  ######  */
               "11011011",    /* ##.##.## */
               "11111111",    /* ######## */
               "00100100",    /* ..#..#.. */
               "01011010",    /* .#.##.#. */
               "10100101" } },/* #.#..#.# */
        { 8, { "00011000",
               "00111100",
               "01111110",
               "11011011",
               "11111111",
               "01011010",    /* .#.##.#. */
               "10000001",    /* #......# */
               "01000010" } },/* .#....#. */
    },
    /* Crab (11 wide) */
    {
        { 11, { "00100000100", /* ..#.....#.. */
                "00010001000", /* ...#...#... */
                "00111111100", /* ..#######.. */
                "01101110110", /* .##.###.##. */
                "11111111111", /* ########### */
                "10111111101", /* #.#######.# */
                "10100000101", /* #.#.....#.# */
                "00011011000" } },/* ...##.##... */
        { 11, { "00100000100",
                "10010001001", /* #..#...#..# */
                "10111111101", /* #.#######.# */
                "11101110111", /* ###.###.### */
                "11111111111",
                "01111111110", /* .#########. */
                "00100000100", /* ..#.....#.. */
                "01000000010" } },/* .#.......#. */
    },
    /* Octopus (12 wide) */
    {
        { 12, { "000011110000", /* ....####.... */
                "011111111110", /* .##########. */
                "111111111111", /* ############ */
                "111001100111", /* ###..##..### */
                "111111111111", /* ############ */
                "001110011100", /* ..###..###.. */
                "011001100110", /* .##..##..##. */
                "110000000011" } },/* ##........## */
        { 12, { "000011110000",
                "011111111110",
                "111111111111",
                "111001100111",
                "111111111111",
                "001110011100",
                "011001100110", /* .##..##..##. */
                "001100001100" } },/* ..##....##.. */
    },
};

/*
 * Encode a pair of pixel rows into a quarter-block string with
 * half-column shift.  Display column C maps to:
 *   UL = pixel[upper][C-1]  UR = pixel[upper][C]
 *   LL = pixel[lower][C-1]  LR = pixel[lower][C]
 * Result width = pixel width + 1.
 */
static char *encode_shifted_row(const char *upper, const char *lower, int w) {
    char buf[256];
    int pos = 0;
    for (int c = 0; c <= w; c++) {
        int ul = (c > 0 && upper[c-1] == '1') ? 8 : 0;
        int ur = (c < w  && upper[c]   == '1') ? 4 : 0;
        int ll = (c > 0 && lower[c-1] == '1') ? 2 : 0;
        int lr = (c < w  && lower[c]   == '1') ? 1 : 0;
        const char *ch = qblock[ul | ur | ll | lr];
        int len = strlen(ch);
        memcpy(buf + pos, ch, len);
        pos += len;
    }
    buf[pos] = '\0';
    return strdup(buf);
}

static int build_sprite(const sprite_px *sp, char *out[ALIEN_ROWS]) {
    for (int r = 0; r < ALIEN_ROWS; r++)
        out[r] = encode_shifted_row(sp->px[r*2], sp->px[r*2+1], sp->width);
    return sp->width + 1;
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
    char *rows[N_FRAMES][INVADER_HEIGHT];
    int width;
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
    int dw[N_TYPES];
    char *sprites[N_TYPES][N_FRAMES][ALIEN_ROWS];

    for (int t = 0; t < N_TYPES; t++)
        for (int f = 0; f < N_FRAMES; f++)
            dw[t] = build_sprite(&alien_px[t][f], sprites[t][f]);

    int max_w = N_COLS * dw[N_TYPES - 1] + (N_COLS - 1) * COL_GAP;
    c->width = max_w;

    for (int frame = 0; frame < N_FRAMES; frame++) {
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
        for (int f = 0; f < N_FRAMES; f++)
            for (int r = 0; r < ALIEN_ROWS; r++)
                free(sprites[t][f][r]);
}

static void invader_init(animation *a) {
    invader_ctx *c = calloc(1, sizeof(invader_ctx));
    a->ctx = c;
    build_formation(c);
    a->width = c->width;
}

static void invader_draw(animation *a, int tick) {
    invader_ctx *c = a->ctx;
    int frame = (tick / FRAME_DIV) % N_FRAMES;
    for (int y = 0; y < INVADER_HEIGHT; y++)
        art_putline(y, c->rows[frame][y]);
}

static void invader_cleanup(animation *a) {
    invader_ctx *c = a->ctx;
    for (int f = 0; f < N_FRAMES; f++)
        for (int r = 0; r < INVADER_HEIGHT; r++)
            free(c->rows[f][r]);
    free(c);
    a->ctx = NULL;
}

animation invader_animation = {
    .name    = "invader",
    .height  = INVADER_HEIGHT,
    .step    = 1,
    .delay   = 60000,
    .init    = invader_init,
    .draw    = invader_draw,
    .cleanup = invader_cleanup,
};

/*---------------------------------------------------------------
 * Individual alien animations
 *---------------------------------------------------------------*/

typedef struct {
    char *rows[N_FRAMES][ALIEN_ROWS];
} alien_ctx;

static void alien_init(animation *a, int type) {
    alien_ctx *c = calloc(1, sizeof(alien_ctx));
    a->ctx = c;
    for (int f = 0; f < N_FRAMES; f++)
        a->width = build_sprite(&alien_px[type][f], c->rows[f]);
}

static void alien_cleanup(animation *a) {
    alien_ctx *c = a->ctx;
    for (int f = 0; f < N_FRAMES; f++)
        for (int r = 0; r < ALIEN_ROWS; r++)
            free(c->rows[f][r]);
    free(c);
    a->ctx = NULL;
}

#define DEFINE_ALIEN(cname, idx) \
    static void cname##_init(animation *a) { alien_init(a, idx); } \
    static void cname##_draw(animation *a, int tick) { \
        alien_ctx *c = a->ctx; \
        int frame = (tick / FRAME_DIV) % N_FRAMES; \
        for (int y = 0; y < ALIEN_ROWS; y++) \
            art_putline(y, c->rows[frame][y]); \
        putchar('\n'); \
    } \
    animation cname##_animation = { \
        .name    = #cname, \
        .height  = ALIEN_ROWS, \
        .init    = cname##_init, \
        .draw    = cname##_draw, \
        .cleanup = alien_cleanup, \
    }

DEFINE_ALIEN(squid,   0);
DEFINE_ALIEN(crab,    1);
DEFINE_ALIEN(octopus, 2);
