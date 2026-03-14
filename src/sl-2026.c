/*
 * sl-2026 - sl with conditional sweep
 *
 * Based on sl-2023. Reads SL_SWEEP_COL from the environment to determine
 * when to start sweeping (deleting characters at the left edge).
 * Without SL_SWEEP_COL, sweeps only at x=0 (last frame).
 */
#include <signal.h>
#include <time.h>
#include "sl.h"
#include "art/art.h"

/* Drawing API state — set by art_set_pos before each frame */
static int art_start_y, art_x;
int art_maxcols;
int art_skip;
int art_subx;

void art_set_pos(int start_y, int x) {
    art_start_y = start_y;
    art_x = x < 0 ? 0 : x;
    art_skip = x < 0 ? -x : 0;
    art_maxcols = COLS - art_x;
}

void art_goto(int row) {
    tputs(tparm(tgoto(cursor_address, art_x, art_start_y + row)), 1, putchar);
}

/* Output string clipped to art_maxcols display columns,
   skipping the first art_skip columns (for left-side clipping).
   All SL art characters are single-width, so 1 codepoint = 1 column. */
void art_puts(const char *s) {
    int n = art_maxcols;
    if (n <= 0) return;
    /* Skip leading columns when art extends past left edge */
    int skip = art_skip;
    while (*s && skip > 0) {
        unsigned char c = (unsigned char)*s;
        int bytes = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
        s += bytes;
        skip--;
    }
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
            printf("[%s.width]=%d\n", a->name, a->width);
            printf("[%s.step]=%d\n", a->name, a->step > 0 ? a->step : DEFAULT_STEP);
        }
        return 0;
    }
    animation *anim = get_animation(sl_option("ART"));

    couple();

    setupterm(NULL, STDOUT_FILENO, NULL);
    COLS = tigetnum("cols"); LINES = tigetnum("lines");

    unsigned int seed;
    FILE *f = fopen("/dev/urandom", "r");
    if (f) { fread(&seed, sizeof(seed), 1, f); fclose(f); }
    else   { seed = time(NULL) ^ getpid(); }
    srand(seed);

    anim->init(anim);

    sl_art_height = anim->height;
    int step = anim->step > 0 ? anim->step : DEFAULT_STEP;
    int delay = anim->delay > 0 ? anim->delay : DEFAULT_DELAY * step / 100;
    int speed = sl_option_int("SPEED", 1);
    if (speed > 1) delay /= speed;
    int stop_col = sl_option_int("STOP_COL", 0);
    /* Round up stop_col to match step alignment (in column units) */
    int step_cols = (step + 99) / 100;  /* ceiling: 50→1, 100→1, 200→2 */
    if (stop_col >= 0 && step_cols > 1 && (stop_col % step_cols))
        stop_col += step_cols - (stop_col % step_cols);
    sl_step = -step_cols;
    int start_y = LINES - sl_art_height - 1;
    /* Start just beyond right edge, aligned to step so x reaches 0 */
    int start_x = ((COLS * 100 + step - 1) / step) * step;
#ifdef SL_NOECHO
    sl_noecho();
#endif
    tputs(cursor_invisible, 1, putchar);
    signal(SIGINT, on_sigint);
    CALL_COUPLERS(origin);
    int tick = 0;
    for (int x = start_x; x >= -(anim->width > 0 ? anim->width * 100 : COLS * 100) && sl_step && !interrupted; x -= step) {
        int col = x / 100;
        int maxcols = COLS - col;
        CALL_COUPLERS(arriving, col);
        if (!sl_step && tick == 0) {
            anim->cleanup(anim);
            tputs(cursor_normal, 1, putchar);
#ifdef SL_NOECHO
            sl_echo();
#endif
            return 1;
        }
        art_subx = x % 100;
        art_set_pos(start_y, col);
        anim->draw(anim, tick * step / DEFAULT_STEP);
        CALL_COUPLERS(departed, col);
        tputs(tparm(tgoto(cursor_address, 0, LINES - 1)), 1, putchar);
        fflush(stdout);
        usleep(delay);
        if (maxcols > 0) tick++;
        if (stop_col >= 0 && x <= stop_col * 100) break;
    }
    CALL_COUPLERS(terminal);
    anim->cleanup(anim);
    tputs(cursor_normal, 1, putchar);
#ifdef SL_NOECHO
    sl_echo();
#endif
    if (interrupted) return 130;
}
