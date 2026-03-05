/*
 * sl.h - Common definitions for SL binaries
 */

#ifndef SL_H
#define SL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <term.h>

extern int COLS, LINES;
extern int sl_step;

/* Look up option SL_<name>.  Currently from environment;
   may later support command-line options too.
   Returns NULL if unset. */
static inline const char *sl_option(const char *name) {
    char var[64];
    snprintf(var, sizeof(var), "SL_%s", name);
    return getenv(var);
}

/* Return integer value of SL_<name>, or dflt if unset/empty. */
static inline int sl_option_int(const char *name, int dflt) {
    const char *v = sl_option(name);
    return (v && *v) ? atoi(v) : dflt;
}

/* Return 1 if SL_<name> is set (to anything other than "0"). */
static inline int sl_option_bool(const char *name) {
    const char *v = sl_option(name);
    return v != NULL && strcmp(v, "0") != 0;
}

static inline void mvprintw(int y, int x, const char *fmt, const char *str) {
    tputs(tparm(tgoto(cursor_address, x, y)), 1, putchar);
    printf(fmt, str);
}

#endif
