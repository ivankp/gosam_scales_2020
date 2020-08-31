#!/bin/bash

find output/3d -type f -name '*.json' |
sed -rn '/H[123]j(B|NLO)_/p' |
while read a
do
  bin/draw "$a" |
  while read b
  do
    out="$(sed 's/\.json$//' <<< "$a")-$(sed 's/ /_/' <<< "$b").pdf"
    echo "$out"
    bin/draw "$a" "$b" "$out"
  done
done
