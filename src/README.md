# NAME

sl - sl runs across your terminal

# SYNOPSIS

**sl** [**-dn**] [*version*|*subcommand*] [**-as**]

# DESCRIPTION

**sl** is a wrapper script that dispatches to versioned sl
binaries.  When you mistype **ls**, a steam locomotive runs across your
terminal as a gentle punishment.

Four versions are available: **1985** (the original), **2010**, **2023**,
and **2026**.  Without a version argument, the latest available version
is selected automatically.

The **2026** version detects existing text on the terminal screen and
begins sweeping (pushing text off via the **dch2** terminal capability)
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

- **-d**

  Show debug information on the last line before running.

- **-n**

  Dry run.  Show debug information and exit without running the train.

# OPTIONS (2026 only)

- **-a**

  Sweep the entire screen.  Without this option, only the 7-line area
  where the train runs is swept.  With **-a**, all screen lines are
  swept and the sweep start position is determined from the full screen
  content rather than just the train area.

- **-s**

  Stop the train when it reaches existing text on the screen instead
  of sweeping through it.

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

# SUBCOMMANDS

These subcommands are primarily for debugging the screen capture and
sweep detection mechanism.

- **capture**

  Dump the captured visible screen text to stdout.

- **sweep-area**

  Show the 7-line area where the train runs.

- **clear-col**

  Print the column number where sweeping should start.

- **debug**

  Show detailed sweep area analysis with visual output.

- **mark**

  Draw a visual marker at the sweep start column and print
  the column number.

# ENVIRONMENT

- **SL\_SWEEP\_COL**

  Passed internally to the sl-2026 binary.  Specifies the column at
  which sweeping begins.

- **SL\_STOP\_COL**

  Passed internally to the sl-2026 binary.  Specifies the column at
  which the train stops.

- **SL\_SWEEP\_ALL**

  Passed internally to the sl-2026 binary.  When set, sweeps all
  screen lines instead of just the train area.

- **SL\_CAR\_STREAK**

  Control the streak effect on the bottom row (2026 only).
  Set to **0** to disable, **rumble** for ▔/▀ thickness modulation,
  or any other value to enable the default shimmer mode.
  On Apple Terminal, background-colored spaces are used instead of
  block characters to avoid ambiguous-width rendering issues.

- **SL\_STREAK**

  Set the streak mode.  **rumble** for ▔/▀ thickness modulation.

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
