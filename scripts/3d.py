#!/usr/bin/env python3

import sys
if len(sys.argv) not in (2,3):
    print('usage:',sys.argv[0],'in.cbor [out.json]')
    sys.exit(1)
if len(sys.argv)==2:
    sys.argv.append(sys.argv[1].rsplit('.',1)[0]+'.json')
print(sys.argv[1]+' -> '+sys.argv[2])

from cbor import cbor
import re, json
from collections import defaultdict

with open(sys.argv[1],'rb') as f:
    hf = cbor.load(f)

xsec = [ x[0] for x in hf['hists']['total']['bins'][1][0][0] ]

class scale_pdf:
    def __init__(self):
        self.fac = [ ]
        self.ren = [ ]
        self.xsec = [ ]

rews = defaultdict(scale_pdf)

weight_re = re.compile(r'([^:]+):0\s+ren:(\S+)\s+fac:(\S+)')
for i,name in enumerate(hf['bins'][0][0]):
    m = weight_re.match(name)
    if m is None: continue
    g = m.groups()
    rew = rews[g[0]]
    rew.ren.append(float(g[1]))
    rew.fac.append(float(g[2]))
    rew.xsec.append(xsec[i])


with open(sys.argv[2],'w') as f:
    json.dump({
        name: { 'fac': r.fac, 'ren': r.ren, 'xsec': r.xsec }
        for name,r in rews.items()
    }, f, separators=(',',':'))

