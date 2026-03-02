#!/bin/bash

version=2023
bindir=$(dirname "$0")/../libexec/sl-classic

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

exec "$bindir/sl-$version" "$@"
