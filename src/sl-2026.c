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

#include "sl-coupler.h"

coupler couplers[MAX_COUPLERS];
int n_couplers = 0;

int main() {
    couple();

    setupterm(NULL, STDOUT_FILENO, NULL);
    int COLS = tigetnum("cols"), LINES = tigetnum("lines");
    int len = strlen(sl[0]), height = sizeof(sl)/sizeof(sl[0]);
    int start_x = COLS, start_y = LINES - height - 1;
    char smoke[1024]; strcpy(smoke, sl[0]); sl[0] = smoke;
    for (int i = 0; i < n_couplers; i++)
        if (couplers[i].origin) couplers[i].origin(&couplers[i], COLS, LINES);
    for (int x = start_x/2*2; x >= 0; x -= 2) {
        int maxcols = COLS - x;
        for (int i = 0; i < n_couplers; i++)
            if (couplers[i].arriving) couplers[i].arriving(&couplers[i], COLS, LINES, x);
        for (int y = 0; y < height; y++)
            mvputns(start_y + y, x, sl[y], maxcols);
        for (int i = 0; i < n_couplers; i++)
            if (couplers[i].departed) couplers[i].departed(&couplers[i], COLS, LINES, x);
        fflush(stdout);
        strcat(smoke, " o");
        usleep(100000);
    }
    for (int i = 0; i < n_couplers; i++)
        if (couplers[i].terminal) couplers[i].terminal(&couplers[i], COLS, LINES);
}
