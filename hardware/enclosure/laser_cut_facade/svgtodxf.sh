#!/bin/bash

# This script converts an SVG file to DXF format. It uses Inkscape to convert the
# SVG to EPS and Pstoedit to convert the EPS to DXF.

function assert {
    local rc
    local message="$1"
    shift
    "$@" 2>&1 > /dev/null
    rc=$?
    [ $rc -eq 0 ] && return 0
    set $(caller)
    date=$(date "+%Y-%m-%d %T%z")
    echo "$date $2 [$$]: $message (line=$1, rc=$rc)" >&2
    exit $rc
}

infile="$1"
outfile="$2" 

assert "Inkscape not found. Inkscape and Pstoedit are both required." which inkscape
assert "Pstoedit not found. Inkscape and Pstoedit are both required." which pstoedit
assert "svg2dxf takes 2 arguments: svg2dxf INFILE OUTFILE" test $# -eq 2
assert "Input file not found" test -f "$infile"

printf "Converting %s to %s..." "$infile" "$outfile" >&2
inkscape "$infile" --export-eps=/dev/stderr 2>&1 >/dev/null \
    | pstoedit -dt -f 'dxf_14:-polyaslines -mm -ctl' /dev/stdin /dev/stdout \
    > "$outfile" 2> /dev/null
printf " OK\n" >&2

