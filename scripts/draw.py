#!/usr/bin/env python3

import sys
if len(sys.argv)<2:
    print('usage:',sys.argv[0],'in.json')
    sys.exit(1)

import json, math

with open(sys.argv[1],'r') as f:
    data = json.load(f)
    names = list(data.keys())
    names.sort()
    for i,name in enumerate(names):
        print('{}: {}'.format(i+1,name))
    i = int(input())-1
    print(names[i])
    for key, val in data[names[i]].items():
        locals()[key] = val if key=='xsec' else [ math.log2(x) for x in val ]

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import cm
# from matplotlib.ticker import LinearLocator, FormatStrFormatter

fig = plt.figure()
ax = fig.gca(projection='3d')

ax.view_init(30,60)
# ax.dist = 13
# ax.margins(0)
# ax.xaxis.labelpad=20
# ax.yaxis.labelpad=20
# ax.zaxis.labelpad=20

xsec_min, xsec_max = min(xsec), max(xsec)

cset = ax.scatter(ren, fac, xsec,
    c=[ math.log((x-xsec_min)/xsec_max+1) for x in xsec ], cmap=cm.viridis,
    s=40,
    linewidth=0, depthshade=0
)
# ax.clabel(cset, fontsize=8, inline=1)

ax.set_xlim(min(ren),max(ren))
ax.set_ylim(min(fac),max(fac))
ax.set_zlim(0, xsec_max*1.05)
# ax.zaxis.set_major_locator(LinearLocator(10))
# ax.zaxis.set_major_formatter(FormatStrFormatter('%.02f'))

# ax.zaxis.set_scale('log')
# ax.set_zticks(np.log10(zticks))
# ax.set_zticklabels(zticks)

plt.savefig('plot.pdf',bbox_inches='tight')
plt.close()

