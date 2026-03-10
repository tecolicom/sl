# sl-sweep.sh - Sweep area detection for sl-2026
#
# Detects existing text on the terminal screen to determine where
# sl-2026 should begin sweeping (pushing text off via dch).

. sl-screen.sh

: "${SL_HEIGHT:=7}"

# Get the sweep area: the screen region where SL runs
get_sweep_area() {
    get_visible_screen | tail -n $((SL_HEIGHT + 1)) | head -n "$SL_HEIGHT"
}

# Calculate the column where SL should start sweeping
calc_clear_col() {
    get_sweep_area | calc_max_display_col
}

# Draw a vertical marker at the sweep start column
draw_column_marker() {
    local col=$1
    get_terminal_size
    local start_y=$((TERM_LINES - SL_HEIGHT - 1))
    ((start_y < 0)) && return 1
    ((col < 0)) && return 0

    local c=$((col + 1))
    local row
    for ((row = 0; row < SL_HEIGHT; row++)); do
        mvprintf "$((start_y + row + 1))" "$c" "<"
    done
    mvprintf "$((start_y + SL_HEIGHT))" "$((c + 1))" " $col"
    mvprintf "$TERM_LINES" 1 ""
}

# Show detailed debug info for the sweep area.
# Note: hit values are character counts, not display widths.
debug_capture_and_col() {
    get_terminal_size
    echo "TERM=${TERM:-} TERM_PROGRAM=${TERM_PROGRAM:-} LANG=${LANG:-}"
    echo "TERM_LINES=$TERM_LINES TERM_COLS=$TERM_COLS"

    local -a target
    while IFS= read -r line; do
        target+=("$line")
    done < <(get_sweep_area)

    local max_col=-1 i hit
    echo "--- target rows (${#target[@]} lines) ---"
    for ((i = 0; i < ${#target[@]}; i++)); do
        line=${target[i]}
        hit=-1
        for ((j = ${#line} - 1; j >= 0; j--)); do
            if [[ "${line:j:1}" != " " ]]; then
                hit=$j
                break
            fi
        done
        ((hit > max_col)) && max_col=$hit
        printf 'row=%d hit=%d line=[%s]\n' "$i" "$hit" "$line"
    done

    echo "clear_col=$max_col"
}
