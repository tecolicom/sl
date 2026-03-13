# Toyoda Art Migration to sl-2026

## Goal

Port D51, C51, and LOGO animations from `mtoyoda/sl.c` into sl-2026's animation framework (`art/toyoda.c`), preserving the original movement and smoke behavior.

## File Structure

- `art/toyoda.c` — Single file containing D51, C51, LOGO animations + smoke particle system
- `mtoyoda/sl.h` — Art data macros (included as-is)

## Architecture

Base on `mtoyoda/sl.c`, adapting to sl-2026's callback model:

### Adaptation from mtoyoda/sl.c

| Original | Adapted |
|----------|---------|
| ncurses `mvaddch`/`mvaddstr` | terminfo `cursor_address` + `putchar` via local `my_mvaddstr()` |
| Main loop with `add_D51(x)` etc. | `init()` / `draw(tick)` / `cleanup()` callbacks |
| Static variables (smoke state) | Per-instance context struct |
| Global `ACCIDENT`, `FLY`, `LOGO` | Not used (deferred) |

### Animation Parameters

| Parameter | Value | Matches toyoda |
|-----------|-------|----------------|
| step | 1 | x-- per frame |
| delay | 40000us | usleep(40000) |
| D51/C51 frame select | `tick % 6` | `(LENGTH+x) % 6` |
| LOGO frame select | `tick/3 % 6` | `(LENGTH+x)/3 % 6` |
| Smoke interval | `tick % 4 == 0` | `x % 4 == 0` |

### Art Height (including smoke area)

Smoke rises max 5 rows above locomotive (dy sum = 2+1+1+1 = 5).
Smoke area needs 6 rows so that particles at max rise (row 0) remain visible
through their full 16-stage lifecycle (stages 4-15 have dy=0, staying at row 0).

| Art | Body height | Smoke rows | Total height |
|-----|-------------|------------|--------------|
| D51 | 10 | 6 | 16 |
| C51 | 11 | 6 | 17 |
| LOGO | 6 | 6 | 12 |

Locomotive body drawn starting at row 6 (smoke rows 0-5 above).

### Frame Data

In `init()`, concatenate locomotive body + coal car (+ passenger cars for LOGO) into full-width frame lines using C string literal concatenation from `mtoyoda/sl.h` macros.

- D51: 7 body + 3 wheel + 1 delimiter lines, each = loco (53 chars) + coal (30 chars)
- C51: 7 body + 4 wheel + 1 delimiter lines, each = loco (55 chars) + coal (30 chars)
- LOGO: 4 body + 2 wheel + 1 delimiter lines, each = loco (21) + coal (21) + car (21) + car (21)

### Smoke Particle System

Port `add_smoke()` from `mtoyoda/sl.c`:

- 16 evolution stages with position deltas (dx[], dy[])
- Two alternating smoke kinds
- Particles tracked in context struct (not static variables)
- Rendering: convert absolute particle positions to art_x-relative offsets, place into row buffers, draw via `art_putline`
- Horizontal extent stays within art width (max offset ~78 < body width 83)

### Registration

In `art/arts.c`:
```c
extern animation d51_animation, c51_animation, logo_animation;
```

In `src/Makefile`:
```
SL2026_ANIM_SRCS += art/toyoda.c
```

## Deferred

- Accident mode (-a: people calling for help)
- Fly mode (-F: locomotive flies diagonally)
- Non-destructive smoke (save/restore underlying characters)

## Wrapper Script Integration

Add to `src/sl`:
- `sl d51` / `sl c51` / `sl logo` dispatch to `sl-2026 -a d51` etc.
