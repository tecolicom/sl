#include "coupler.h"

/* Check if car "NAME" is enabled.
   If SL_CAR_NAME is set, "0" means disabled, anything else means enabled.
   If unset, falls back to the compile-time default. */
int car_enabled(const char *name, int dflt) {
    char var[64];
    snprintf(var, sizeof(var), "CAR_%s", name);
    const char *val = sl_option(var);
    if (val == NULL) return dflt;
    return strcmp(val, "0") != 0;
}

void couple(void) {
#ifdef CAR_SWEEP
    if (car_enabled("SWEEP", CAR_SWEEP))
        couplers[n_couplers++] = sweep_coupler();
#endif
#ifdef CAR_NULL
    if (car_enabled("NULL", CAR_NULL))
        couplers[n_couplers++] = null_coupler();
#endif
#ifdef CAR_RAIL
    if (car_enabled("RAIL", CAR_RAIL))
        couplers[n_couplers++] = rail_coupler();
#endif
}
