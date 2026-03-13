/*
 * sl-coupler.h - Coupler framework for sl-2026
 *
 * HOW TO ADD A NEW CAR
 *
 * 1. Create cars/xxx.c with a constructor function:
 *
 *      #include "coupler.h"
 *      static void origin(coupler *cpl) { ... }
 *      static void arriving(coupler *cpl, int x) { ... }
 *      static void departed(coupler *cpl, int x) { ... }
 *      static void terminal(coupler *cpl) { ... }
 *      coupler xxx_coupler(void) {
 *          return (coupler){ .arriving = arriving,
 *                            .departed = departed };
 *      }
 *
 *    - ctx: per-instance data pointer (malloc in origin, free in terminal)
 *    - origin: called once before the animation loop (始発駅)
 *    - arriving: called each frame before SL is drawn (まもなく到着)
 *    - departed: called each frame after SL is drawn, before fflush (発車)
 *      - x: current SL position (decreasing from COLS toward 0)
 *    - terminal: called once after the animation loop (終着駅)
 *    - COLS, LINES: available as globals (extern in sl.h)
 *    - sl_step: negative column step (set to 0 to stop)
 *    - Any callback may be NULL if not needed.
 *
 * 2. Declare the constructor in this header (below).
 *
 * 3. Register in cars/couplers.c under #ifdef CAR_XXX:
 *
 *      #ifdef CAR_XXX
 *          if (car_enabled("XXX", CAR_XXX))
 *              couplers[n_couplers++] = xxx_coupler();
 *      #endif
 *
 * 4. Add to SL2026_CARS in Makefile (NAME=1 for default on, =0 for off):
 *
 *      SL2026_CARS := NULL=1 XXX=1
 *
 *    This auto-generates -DCAR_XXX=1 and cars/xxx.c from the entry.
 *
 * COMPILE-TIME DEFAULT
 *
 * The value of -DCAR_XXX sets the default: =1 enabled, =0 disabled.
 *
 * ENVIRONMENT
 *
 * SL_CAR_XXX overrides the compile-time default at runtime.
 * Set to "0" to disable, any other value to enable.
 */

#ifndef SL_COUPLER_H
#define SL_COUPLER_H

#include "../sl.h"

typedef struct coupler {
    void *ctx;
    void (*origin)(struct coupler *cpl);
    void (*arriving)(struct coupler *cpl, int x);
    void (*departed)(struct coupler *cpl, int x);
    void (*terminal)(struct coupler *cpl);
} coupler;

#define MAX_COUPLERS 8

extern coupler couplers[MAX_COUPLERS];
extern int n_couplers;

#define CALL_COUPLERS(hook, ...) \
    for (int i = 0; i < n_couplers; i++) \
        if (couplers[i].hook) couplers[i].hook(&couplers[i], ##__VA_ARGS__)

void couple(void);
int car_enabled(const char *name, int dflt);
coupler null_coupler(void);
coupler sweep_coupler(void);
coupler streak_coupler(void);

#endif
