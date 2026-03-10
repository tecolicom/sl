# NAME

sl - sl runs across your terminal

# SYNOPSIS

**sl** \[*VERSION*] \[**-wWsStTgGdnl**] \[**-a** *name*] \[**-C**] \[**-c** *mode*] \[**-p** *K=V*] \[*SUBCOMMAND*]

**Options:**\
**-w**/**-W**, **--sweep**/**--no-sweep** — Enable/disable sweep (2026, default: on)\
**-C**, **--clear** — Sweep the entire screen (2026)\
**-s**/**-S**, **--stop**/**--no-stop** — Stop/don't stop at text (2026)\
**-t**/**-T**, **--streak**/**--no-streak** — Enable/disable streak (2026, default: on)\
**-g**/**-G**, **--gone**/**--no-gone** — Run off screen to the left (2026)\
**-a**, **--art**=*NAME* — Animation art (2026, default: sl)\
**-l**, **--list** — List available animations\
**-c**, **--color**=*MODE* — Color mode: truecolor/24bit/256\
**-p**, **--param**=*K=V* — Set coupler parameter (e.g., STREAK=rumble)\
**-d**, **--debug** — Show debug info on the last line before running\
**-n**, **--dryrun** — Dry run (show debug info and exit)\
**-h**, **--help** — Show help\
**--version** — Show version

**Subcommands:**\
**capture** — Dump captured screen text\
**sweep-area** — Show the area where SL runs\
**clear-col** — Print the column where sweeping should start\
**debug** — Show detailed sweep area analysis\
**mark** — Draw a visual marker at the sweep start column

**Versions** (must be the first argument)**:**\
**modern** — Toyoda Masashi's SL (mtoyoda/sl)\
**2023** — Unicode art version, always sweeps\
**2010** — Direct terminal control version\
**1985** — Original curses version with retro delay

# DESCRIPTION

**sl** is a wrapper script that dispatches to versioned sl
binaries.  When you mistype **ls**, an SL runs across your
terminal as a gentle punishment.

Five versions are available: **1985** (the original), **2010**, **2023**,
**2026**, and **modern** (Toyoda Masashi's SL).  The version must be
specified as the first argument; without it, **2026** is used.

The **2026** version detects existing text on the terminal screen and
begins sweeping (pushing text off via the **dch** terminal capability)
only when the train reaches it, preserving text that the train has not
yet passed.  The train enters gradually from beyond the right edge of
the screen, like the original 1985 version.

# REQUIREMENTS

The 2026 version captures the terminal screen content to detect
existing text.  This feature currently supports the following
terminal emulators on macOS:

- **iTerm2** (iTerm.app)
- **Apple Terminal** (Terminal.app)

On unsupported terminals or platforms, the train always sweeps
(equivalent to the 2023 behavior).  Other versions (1985, 2010,
2023) work on any terminal.

# OPTIONS

- **-a**, **--art**=*NAME*

  Select animation art.  Use **-l** to list available animations.
  Default: **sl**.
  Can also be set via the **SL\_ART** environment variable.
  Use **random** to pick randomly from all available animations.
  A comma-separated list (e.g., **sl,sl,sl,clawd**) picks one
  entry at random, allowing weighted selection.

- **-l**, **--list**

  List available animations with their properties (name, height, step).

- **-c**, **--color**=*MODE*

  Set color mode: **truecolor**, **24bit**, or **256**.

- **-p**, **--param**=*KEY*=*VALUE*

  Set coupler parameter (see COUPLERS for available keys).

## Sweep

- **-w**/**-W**, **--sweep**/**--no-sweep**

  Enable or disable the sweep effect (default: on).

- **-C**, **--clear**

  Sweep the entire screen instead of just the train area.

- **-s**/**-S**, **--stop**/**--no-stop**

  Stop or don't stop the train at existing text (default: off).

- **-g**/**-G**, **--gone**/**--no-gone**

  Let the train run off screen to the left until it fully
  disappears (default: off).  Without this option, the train
  stops at the left edge.

## Streak

- **-t**/**-T**, **--streak**/**--no-streak**

  Enable or disable the streak effect on the bottom row (default: on).

- **-p STREAK=**{**rumble**,**reverse**}

  Set streak mode.  **rumble** enables thickness modulation,
  **reverse** reverses the direction.  Combine with commas
  (e.g., **-p STREAK=rumble,reverse**).

## Debug

- **-d**, **--debug**

  Show debug information on the last line before running.

- **-n**, **--dryrun**

  Dry run.  Show debug information and exit without running the train.

# VERSIONS

- **1985**

  The original SL, written in K&R C with curses.
  Output is filtered through a per-character delay for a retro feel.

- **2010**

  Updated version using direct terminal control instead of curses.

- **2023**

  Unicode art version with box-drawing characters.  Always sweeps text
  from the left edge.

- **2026**

  Screen-aware version.  Detects text on the terminal and sweeps only
  when the train reaches it.  Enters from beyond the right edge of the
  screen to preserve right-side text.

- **modern**

  Toyoda Masashi's SL ([mtoyoda/sl](https://github.com/mtoyoda/sl)).
  Supports **-a** (accident), **-F** (fly), **-l** (logo), and **-c**
  (C51) options (e.g., **sl modern -F -a**).

# ANIMATIONS

The 2026 version supports pluggable animation modules selected with
the **-a** option or **SL\_ART** environment variable.
Use **-l** to list available animations.

Each animation defines its own movement step (columns per frame) and
frame delay, allowing different animations to move at different speeds.

## sl

The default SL animation with growing smoke trail.
Moves 2 columns per frame.

## clawd

The Claude Code mascot running across the terminal with a
spinning symbol above its head cycling through the Claude Code
thinking indicator characters.
Moves 1 column per frame for smoother motion.

- **SL\_HAT** — Hat variant.  Set to **party** for a party hat.
  If not set, a party hat appears randomly with a 1-in-20 chance.

# SUBCOMMANDS

These subcommands are primarily for debugging the screen capture and
sweep detection mechanism.

- **capture**

  Dump the captured visible screen text to stdout.

- **sweep-area**

  Show the area where the train runs.

- **clear-col**

  Print the column number where sweeping should start.

- **debug**

  Show detailed sweep area analysis with visual output.

- **mark**

  Draw a visual marker at the sweep start column and print
  the column number.

# COUPLERS

Couplers are pluggable effect modules for the 2026 version.
Each coupler can be disabled by setting its SL\_CAR variable to 0.
Coupler-specific parameters can be set with the **-p** option
(e.g., **-p STREAK=rumble,reverse**).

## Sweep

The sweep coupler deletes characters at the left edge as the
train passes.  Set **SL\_CAR\_SWEEP=0** to disable.

- **SL\_SWEEP\_COL** — Column at which sweeping begins.
- **SL\_STOP\_COL** — Column at which the train stops (default: 0 = left edge,
  -1 = run off screen).
- **SL\_SWEEP\_ALL** — Sweep all screen lines instead of just the train area.

## Streak

The streak coupler draws a glowing light bar on the bottom row.
Set **SL\_CAR\_STREAK=0** to disable.

- **SL\_STREAK\_RUMBLE** — Enable ▔/▀ thickness modulation.
- **SL\_STREAK\_REVERSE** — Reverse direction (left-to-right).

# TERMINAL TIPS

The 2026 version uses Unicode box-drawing and block characters
whose East Asian Width property is "Ambiguous".  Terminals that
treat ambiguous-width characters as double-width will cause visual
glitches (line wrapping / "derailment").  Make sure to uncheck
this setting:

- **iTerm2**: Profiles → Text → Unicode → **Ambiguous characters
  are double-width**

- **Apple Terminal**: Profiles → Advanced → **Unicode East Asian
  Ambiguous characters are wide**

# INSTALL

    brew tap tecolicom/tap
    brew install sl-classic

# SEE ALSO

ls(1)

- Kazumasa Utashiro, "JUNET and Hacker Culture", IPSJ Magazine, Vol.66, No.8, 2025.
  Special issue: "How the Internet Began in Japan -- 40 Years of JUNET"
- [40 Years of the sl Command -- A Genealogy of Useless Things](https://tecolicom.github.io/sl/junet40th/junet40th.html)
  (JUNET 40th Anniversary Symposium, 2024)
- [SL (Wikipedia)](https://en.wikipedia.org/wiki/SL)
- [Sl (UNIX) (Wikipedia, Japanese)](https://ja.wikipedia.org/wiki/Sl_(UNIX))
- [sl by Toyoda Masashi](https://github.com/mtoyoda/sl) (Homebrew core: `brew install sl`)
- <https://github.com/tecolicom/sl> (Homebrew tap: `brew install tecolicom/tap/sl-classic`)

# LICENSE

BSD 2-Clause License

# AUTHOR

Kazumasa Utashiro
<https://github.com/tecolicom/sl>

AIKAWA, Shigechika (streak effect)
