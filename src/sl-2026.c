/*
 * sl-2026 - sl with conditional sweep
 *
 * Based on sl-2023. Reads SL_SWEEP_COL from the environment to determine
 * when to start sweeping (deleting characters at the left edge).
 * Without SL_SWEEP_COL, sweeps only at x=0 (last frame).
 */
#include <signal.h>
#include "sl.h"
#include "art/art.h"

/* Drawing API state — set by art_set_pos before each frame */
static int art_start_y, art_x, art_maxcols;

void art_set_pos(int start_y, int x, int maxcols) {
    art_start_y = start_y;
    art_x = x;
    art_maxcols = maxcols;
}

void art_goto(int row) {
    tputs(tparm(tgoto(cursor_address, art_x, art_start_y + row)), 1, putchar);
}

/* Output string clipped to art_maxcols display columns.
   All SL art characters are single-width, so 1 codepoint = 1 column. */
void art_puts(const char *s) {
    int n = art_maxcols;
    if (n <= 0) return;
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

#include "cars/coupler.h"

int COLS, LINES;
int sl_step;
int sl_art_height;
coupler couplers[MAX_COUPLERS];
int n_couplers = 0;

static volatile sig_atomic_t interrupted = 0;

static void on_sigint(int sig) {
    interrupted = 1;
}

int main() {
    if (sl_option("QUERY")) {
        for (int i = 0; animations[i]; i++) {
            animation *a = animations[i];
            printf("[%s.height]=%d\n", a->name, a->height);
            printf("[%s.step]=%d\n", a->name, a->step > 0 ? a->step : DEFAULT_STEP);
        }
        return 0;
    }
    animation *anim = get_animation(sl_option("ART"));

    couple();

    setupterm(NULL, STDOUT_FILENO, NULL);
    COLS = tigetnum("cols"); LINES = tigetnum("lines");
    anim->init(anim);

    sl_art_height = anim->height;
    int step = anim->step > 0 ? anim->step : DEFAULT_STEP;
    int delay = anim->delay > 0 ? anim->delay : DEFAULT_DELAY;
    sl_step = -step;
    int start_y = LINES - sl_art_height - 1;
    /* Start just beyond right edge, aligned to step so x reaches 0 */
    int start_x = ((COLS + step - 1) / step) * step;
    sl_noecho();
    signal(SIGINT, on_sigint);
    CALL_COUPLERS(origin);
    int tick = 0;
    for (int x = start_x; x >= 0 && sl_step && !interrupted; x += sl_step) {
        int maxcols = COLS - x;
        CALL_COUPLERS(arriving, x);
        if (!sl_step && tick == 0) {
            anim->cleanup(anim);
            sl_echo();
            return 1;
        }
        art_set_pos(start_y, x, maxcols);
        anim->draw(anim, tick);
        CALL_COUPLERS(departed, x);
        fflush(stdout);
        usleep(delay);
        if (maxcols > 0) tick++;
    }
    CALL_COUPLERS(terminal);
    anim->cleanup(anim);
    sl_echo();
}
