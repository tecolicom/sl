#!/bin/bash

version=2023
self=$0
if [ -L "$self" ]; then
    link=$(readlink "$self")
    case "$link" in
        /*) self=$link ;;
        *)  self=$(dirname "$self")/$link ;;
    esac
fi
selfdir=$(dirname "$self")
if [ -d "$selfdir/../libexec/sl-classic" ]; then
    bindir=$selfdir/../libexec/sl-classic
else
    bindir=$selfdir
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --2023|--2010|--1985)
            version=${1#--}
            shift
            ;;
        *)
            break
            ;;
    esac
done

slowout() {
    PERL_BADLANG=0 perl -e '
        $|=1;
        my $delay = 0.0005;
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
                print $seq;
            } else {
                print $c;
                select(undef, undef, undef, $delay);
            }
        }
    '
}

case "$version" in
    1985)
        if script --help 2>&1 | grep -q -- '--command'; then
            script -qc "$bindir/sl-$version" /dev/null
        else
            script -q /dev/null "$bindir/sl-$version" "$@"
        fi | slowout
        ;;
    *)
        exec "$bindir/sl-$version" "$@"
        ;;
esac
