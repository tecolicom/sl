# sl-screen.sh - Terminal screen capture and analysis library
#
# Provides functions for capturing terminal screen content,
# analyzing text positions, and calculating display widths.
# Requires $bindir to be set for Perl library path.

##############################################################################
# Terminal utilities
##############################################################################

NBSP=$'\xc2\xa0'  # U+00A0 Non-Breaking Space

# Set TERM_LINES and TERM_COLS from the terminal
get_terminal_size() {
    if size=$(stty size </dev/tty 2>/dev/null); then
        TERM_LINES=${size%% *}
        TERM_COLS=${size##* }
    else
        TERM_LINES=$(tput lines 2>/dev/null || echo "${LINES:-24}")
        TERM_COLS=$(tput cols 2>/dev/null || echo "${COLUMNS:-80}")
    fi
}


# Move cursor to (row, col) and print.
# Usage: mvprintf ROW COL FORMAT [ARGS...]
mvprintf() {
    local row=$1 col=$2
    shift 2
    printf "\033[%d;%dH" "$row" "$col"
    # shellcheck disable=SC2059
    printf "$@"
}

##############################################################################
# Screen capture backends
#
# Each backend function outputs the screen text to stdout.
# To add a new backend:
#   1. Define capture_screen__<name>()
#   2. Add detection logic to capture_screen_text()
##############################################################################

capture_screen__iterm() {
    osascript <<'APPLESCRIPT' 2>/dev/null
tell application "iTerm2"
  if not (exists current window) then return ""
  set s to current session of current tab of current window
  try
    return contents of s
  on error
    return text of s
  end try
end tell
APPLESCRIPT
}

capture_screen__apple_terminal() {
    osascript <<'APPLESCRIPT' 2>/dev/null
tell application "Terminal"
  if not (exists front window) then return ""
  return contents of selected tab of front window
end tell
APPLESCRIPT
}

# Dispatch to the appropriate backend based on TERM_PROGRAM
capture_screen_text() {
    case "${TERM_PROGRAM:-}" in
        iTerm.app)      capture_screen__iterm ;;
        Apple_Terminal) capture_screen__apple_terminal ;;
        *)              return 1 ;;
    esac
}

##############################################################################
# Screen text retrieval and analysis
##############################################################################

# Normalize captured screen text: convert \r to \n and strip trailing spaces
normalize_screen_text() {
    local s=$1
    s=${s//$'\r'/$'\n'}
    printf '%s' "$s" | sed 's/  *$//'
}

# Get the visible portion of the captured screen text.
# Output is normalized (trailing spaces stripped).
get_visible_screen() {
    get_terminal_size
    # Write NBSP (U+00A0) at the screen bottom to ensure the terminal
    # buffer extends to TERM_LINES.  Without a visible character,
    # AppleScript "contents" may omit trailing empty lines, causing
    # tail to pick up scrollback data after clear.
    # NBSP is stripped by normalize_screen_text and does not affect
    # sweep calculations.  The last line is outside the SL area.
    mvprintf "$TERM_LINES" 1 '%s\b' "$NBSP" > /dev/tty
    local screen
    screen=$(capture_screen_text) || return 1
    screen=$(normalize_screen_text "$screen")
    printf '%s\n' "$screen" | tail -n "$TERM_LINES"
}

# Calculate the maximum display width across input lines.
# Handles multibyte and wide characters correctly via Text::VisualWidth::PP.
# Reads from stdin, prints the max display width.
calc_max_display_col() {
    PERL_BADLANG=0 perl -Mlib="$bindir/perl5/lib/perl5" -CSD -MText::VisualWidth::PP=vwidth -e '
        my $m = -1;
        while (<STDIN>) {
            chomp;
            s/ +$//;
            my $w = vwidth($_);
            $m = $w if $w > $m;
        }
        print $m;
    '
}
