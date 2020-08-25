#!/usr/bin/env python3

import sys
if len(sys.argv) < 3:
    print("usage:",sys.argv[0],"output.cbor in1.cbor ...")
    sys.exit(1)

def sum_lists(a,b):
    for i in range(len(a)):
        if isinstance(a[i],list):
            sum_lists(a[i],b[i])
        else:
            a[i] += b[i]

from cbor import cbor

out = None
for fname in sys.argv[2:]:
    print(fname)
    with open(fname,'rb') as f:
        hf = cbor.load(f)
    if out is None:
        out = hf
        continue

    for n in ('entries','events','count'):
        out['N'][n] += hf['N'][n]

    if hf["axes"]!=out["axes"]:
        raise ValueError("incompatible axes definitions in "+fname)
    if hf["bins"]!=out["bins"]:
        raise ValueError("incompatible bins definitions in "+fname)

    for hname, h in hf["hists"].items():
        # print(hname)
        hout = out["hists"][hname]
        if h["axes"]!=hout["axes"]:
            raise ValueError(
                "incompatible axes definitions for "+hname+" in "+fname)
        if h["bins"][0]!=hout["bins"][0]:
            raise ValueError(
                "incompatible bins definitions for "+hname+" in "+fname)

        sum_lists(hout["bins"][1],h["bins"][1])

print("\nwriting output")
with open(sys.argv[1],'wb') as f:
    cbor.dump(out,f)
print(sys.argv[1])

