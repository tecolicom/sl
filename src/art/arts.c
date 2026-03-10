/*
 * art/arts.c - Animation registry
 */

#include "art.h"
#include <string.h>

extern animation sl_animation;
extern animation clawd_animation;
extern animation d51_animation;
extern animation c51_animation;
extern animation modern_animation;

animation *animations[] = {
    &sl_animation,
    &clawd_animation,
    &d51_animation,
    &c51_animation,
    &modern_animation,
    NULL
};

animation *get_animation(const char *name) {
    if (!name)
        return animations[0];
    for (int i = 0; animations[i]; i++)
        if (strcmp(animations[i]->name, name) == 0)
            return animations[i];
    return animations[0];
}
