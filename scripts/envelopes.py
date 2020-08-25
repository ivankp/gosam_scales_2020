#!/usr/bin/env python3

import sys
if len(sys.argv)<3:
    print("usage:",sys.argv[0],"in.cbor out.json [weights ...]")
    sys.exit(1)
print(sys.argv[1]+' -> '+sys.argv[2])

from cbor import cbor
import json, re, math
from collections import defaultdict
import lhapdf

with open(sys.argv[1],'rb') as f:
    hf = cbor.load(f)

class pdf_dict(dict):
    def __missing__(self, key):
        el = self[key] = lhapdf.getPDFSet(key)
        return el

pdf_sets = pdf_dict()

class variations:
    def __init__(self):
        self.scale = [ ]
        self.pdf = [ ]
    def add(self,i,pdf,ren,fac,pdf_name):
        if pdf==0:
            if (ren,fac)==(1,1):
                self.scale.insert(0,i)
                self.pdf.insert(0,i)
            else:
                self.scale.append(i)
            self.pdf_set = pdf_sets[pdf_name]
        else:
            self.pdf.append(i)
    def __call__(self,ws):
        w = ws[self.scale[0]]
        pdf = self.pdf_set.uncertainty( [ws[i][0] for i in self.pdf] )
        return (
            w[0],
            math.sqrt(w[1]),
            w[0] - min(ws[i][0] for i in self.scale),
            max(ws[i][0] for i in self.scale) - w[0],
            pdf.errminus,
            pdf.errplus,
            pdf.errsymm
        )

sets = defaultdict(variations)
indep = [ ] # independent weights
weight_re = re.compile(r'([^:]+):(\S+)\s+ren:(\S+)\s+fac:(\S+)')
for i, weight in enumerate(hf["bins"][0][0]):
    m = weight_re.match(weight)
    if m is None:
        indep.append((weight,i))
        continue
    g = m.groups()
    # print(g)
    sets[g[0]].add( i, int(g[1]), float(g[2]), float(g[3]), g[0].split()[1] )

# for name, s in sets.items():
#     print(name)
#     print(s.scale)
#     print(s.pdf)

hf['bins'] = [ w for w,i in indep ] + list(sets.keys())

# print()
# make envelopes
for hname, h in hf["hists"].items():
    # print(hname)
    for hbin in h['bins'][1]:
        ws1 = hbin[0]
        hbin[0] = [ ws1[i] for w,i in indep ] \
                + [ s(ws1) for s in sets.values() ]

def subaxes(dim,n):
    for axis in dim:
        yield axis
    for i in range(n-len(dim)):
        yield dim[-1]

def overflow(axes):
    o = [ False ]
    for dim in axes:
        for i,axis in enumerate(subaxes(dim,len(o))):
            n = len(axis)
            if n>0:
                o[i] = [True] + [o[i]] * (n-1) + [True]
            else:
                o[i] = [o[i]]
        o = [ item for sublist in o for item in sublist ]
    return o

def mask_list(xs,mask):
    for x,f in zip(xs,mask):
        if not f:
            yield x

# change structure & remove overflow bins
out = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))
for k,wname in enumerate(hf['bins']):
    if len(sys.argv)>3:
        if wname not in sys.argv[3:]:
            continue
    for hname, h in hf["hists"].items():
        hout = out[wname][hname]
        axes = hout['axes'] \
             = [ [ hf['axes'][i] for i in ii ] for ii in h['axes'] ]
        for y in mask_list( h['bins'][1], overflow(axes) ):
            y = y[0][k]
            # print(y)
            for y,yname in zip(y,(
                'xsec', 'mc_unc',
                'scale_down', 'scale_up',
                'pdf_errminus', 'pdf_errplus', 'pdf_errsymm'
            )):
                # print(yname,y)
                hout[yname].append(y)
        for a in axes[1:]:
            if not a[0]:
                a.pop(0)

with open(sys.argv[2],'w') as f:
    # json.dump(out,f,separators=(',',':'))
    jstr = json.dumps(out,indent=2)
    jstr = re.sub(r'(\d,|\[)\s+',r'\1',jstr)
    jstr = re.sub(r'\s+\]',r']',jstr)
    jstr = re.sub(r'\],\s+\[',r'],[',jstr)
    f.write(jstr)
    f.write('\n')

