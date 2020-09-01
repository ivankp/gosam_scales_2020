#!/bin/bash

find output/3d -type f -name '*.json' |
sed -rn '/H[123]j(B|NLO)_/p' |
while read a
do
  bin/draw "$a" |
  while read b
  do
    out="$(sed 's/\.json$//' <<< "$a")-$(sed 's/ /_/' <<< "$b")"
    echo "$out"
    bin/draw "$a" "$b" "${out}.eps"
    inkscape --export-pdf="${out}.pdf" "${out}.eps"
  done
done
