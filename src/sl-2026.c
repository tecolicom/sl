/*
 * sl-2026 - Steam Locomotive with conditional sweep
 *
 * Based on sl-2023. Reads SL_SWEEP_COL from the environment to determine
 * when to start sweeping (deleting characters at the left edge).
 * Without SL_SWEEP_COL, sweeps only at x=0 (last frame).
 */
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <term.h>

void mvprintw(int y, int x, const char* fmt, const char* str) {
    tputs(tparm(tgoto(cursor_address, x, y)), 1, putchar);
    printf(fmt, str);
}

/* Print at (y, x) but limit output to n display columns.
   All SL art characters are single-width, so 1 codepoint = 1 column. */
void mvputns(int y, int x, const char *s, int n) {
    if (n <= 0) return;
    tputs(tparm(tgoto(cursor_address, x, y)), 1, putchar);
    int col = 0;
    while (*s && col < n) {
        unsigned char c = (unsigned char)*s;
        int bytes = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
        for (int i = 0; i < bytes && s[i]; i++)
            putchar(s[i]);
        s += bytes;
        col++;
    }
}

/* Shimmer line on the bottom row – truecolor foreground on U+2594.
   Asymmetric comet shape: sharp leading edge, long trailing tail.
   Uses direct ANSI sequences because terminfo has no truecolor support. */
void render_scanner(int row, int cols, int center, int dir, int trail_div) {
    tputs(tparm(tgoto(cursor_address, 0, row)), 1, putchar);
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
                ? (int)(d * d * 255 / (hw * hw))               /* quadratic */
                : (int)(d * d * d * 255 / ((long)hw * hw * hw)); /* cubic   */
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
}

/* Trailing spaces on body lines erase remnants when not sweeping.
   Smoke line gets " o" appended each frame so it self-extends. */
char *sl[] = {
    "      o o o o o",
    "     o         ",
    "    o  ┯━━┯    ",
    "   ╒╕  │OO│    ",
    " ┌─┘└──┘  │    ",
    "┌┘        │    ",
    "┴─O=O O=O─┴ з  \n"
};

int main() {
    setupterm(NULL, STDOUT_FILENO, NULL);
    int COLS = tigetnum("cols"), LINES = tigetnum("lines");
    int len = strlen(sl[0]), height = sizeof(sl)/sizeof(sl[0]);
    int start_x = COLS, start_y = LINES - height - 1;
    char dch2[20] = "", *dch2p = tparm(tigetstr("dch"), 2);
    if (dch2p != NULL)
        strcpy(dch2, dch2p);
    char *env = getenv("SL_SWEEP_COL");
    int clear_col = (env && *env) ? atoi(env) : 0;
    int sweep_all = getenv("SL_SWEEP_ALL") != NULL;
    char smoke[1024]; strcpy(smoke, sl[0]); sl[0] = smoke;
    int shimmer = 1;
    char *senv = getenv("SL_SHIMMER");
    if (senv && *senv) shimmer = atoi(senv);
    if (shimmer == 0) shimmer = 1;
    int trail_div = abs(shimmer);
    int scan_pos = 0, scan_step = COLS / 12;
    if (scan_step < 2) scan_step = 2;
    if (shimmer > 0) scan_step = -scan_step;
    for (int x = start_x/2*2; x >= 0; x -= 2) {
        int maxcols = COLS - x;
        if (x <= clear_col) {
            int y0 = sweep_all ? 0 : start_y;
            int y1 = sweep_all ? LINES : start_y + height;
            for (int y = y0; y < y1; y++)
                mvprintw(y, 0, "%s", dch2);
        }
        for (int y = 0; y < height; y++)
            mvputns(start_y + y, x, sl[y], maxcols);
        render_scanner(LINES - 1, COLS, scan_pos, scan_step, trail_div);
        fflush(stdout);
        scan_pos = (scan_pos + scan_step + COLS) % COLS;
        strcat(smoke, " o");
        usleep(100000);
    }
    /* Clear scanner bar */
    printf("\033[0m");
    tputs(tparm(tgoto(cursor_address, 0, LINES - 1)), 1, putchar);
    tputs(tigetstr("el"), 1, putchar);
    fflush(stdout);
}
