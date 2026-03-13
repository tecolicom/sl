# sl-1985.sh - Run sl-1985 with retro delay output
#
# sl-1985 uses curses and needs a pty; use script(1) to provide one.
# Output is filtered through slowout for per-character delay.

# Output filter: add per-character delay for retro feel.
# Strips smcup/rmcup sequences to prevent alternate screen switching.
slowout() {
    PERL_BADLANG=0 perl -e '
        $|=1;
        my $delay = 0.0005;
        my %skip;
        for my $cap (qw(smcup rmcup)) {
            my $s = `tput $cap 2>/dev/null`;
            $skip{$s} = 1 if length $s;
        }
        while (sysread(STDIN, my $c, 1)) {
            if ($c eq "\e") {
                my $seq = $c;
                sysread(STDIN, $c, 1) or last;
                $seq .= $c;
                if ($c eq "[" || $c eq "(") {
                    while (sysread(STDIN, $c, 1)) {
                        $seq .= $c;
                        last if $c =~ /[A-Za-z]/;
                    }
                }
                print $seq unless $skip{$seq};
            } else {
                print $c;
                select(undef, undef, undef, $delay);
            }
        }
    '
}

if script --help 2>&1 | grep -q -- '--command'; then
    script -qc "sl-1985 $*" /dev/null
else
    script -q /dev/null sl-1985 "$@"
fi | slowout
