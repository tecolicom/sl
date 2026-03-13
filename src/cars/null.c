#include "coupler.h"

static void origin(coupler *cpl) {
}

static void arriving(coupler *cpl, int x) {
}

static void departed(coupler *cpl, int x) {
}

static void terminal(coupler *cpl) {
}

coupler null_coupler(void) {
    return (coupler){
        .origin = origin,
        .arriving = arriving,
        .departed = departed,
        .terminal = terminal,
    };
}
