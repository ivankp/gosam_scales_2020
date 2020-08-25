#!/bin/bash

mkdir -p output/merged

echo "merging parts"
find output/jobs -type f -name '*.cbor' |
sed 's/_[0-9]\+\.cbor$//' |
sort -u |
while read x
do
  out="$(sed 's:/jobs/:/merged/:' <<< "$x").cbor"
  echo $out
  need=false
  if [ -f "$out" ]; then
    for f in "$x"*; do
      if [ "$f" -nt "$out" ]; then
        need=true
        break
      fi
    done
  else
    need=true
  fi
  if [ "$need" == true ]; then
    scripts/merge.py "$out" "$x"*
    scripts/xsec.py "$out"
    echo
  fi
done

echo
echo "merging NLO"
find output/merged -maxdepth 1 -type f -name '*.cbor' |
sed -r 's/\.cbor$//;s/^([^_]*)(B|RS|I|V)_/\1NLO_/' |
sort -u |
while read x
do
  out="${x}.cbor"
  echo $out
  parts=()
  need=false
  for f in B RS I V; do
    f=$(sed -r "s/^([^_]*)NLO_/\\1${f}_/" <<< "$out")
    if [ ! -f "$f" ]; then
      break
    fi
    parts+=("$f")
    if [ ! -f "$out" ] || [ "$f" -nt "$out" ]; then
      need=true
    fi
  done
  if [ "$need" == true ] && [ "${#parts[@]}" == "4" ]; then
    scripts/merge.py "$out" "${parts[@]}"
    echo
  fi
done

mkdir -p output/envelopes

echo
echo "envelopes"
find output/merged -maxdepth 1 -type f -name '*.cbor' |
sort -u |
while read x
do
  out="$(sed 's:/merged/:/envelopes/:;s:\.cbor$:.json:' <<< "$x")"
  if [ ! -f "$out" ] || [ "$x" -nt "$out" ]; then
    scripts/envelopes.py "$x" "$out" # 'HT1 CT14nlo'
  fi
done

