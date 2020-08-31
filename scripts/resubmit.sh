#!/bin/bash

for f in condor/*.sh
do
  f="$(sed 's/\.sh$//' <<< "$f")"
  if [ -f "${f}.out" ] && grep -q '100\.00%' "${f}.out"
  then # OK
    :
  else # NEED
    find . -path "./${f}.*" | sed -n '/\.sh$/!p' | xargs -n1 rm -fv

    condor_submit << JOB
Universe   = vanilla
Executable = ${f}.sh
Output     = ${f}.out
Error      = ${f}.err
Log        = ${f}.log
getenv = True
+IsMediumJob = True
Queue
JOB
  fi
done


