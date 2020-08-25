#!/usr/bin/env python3

import sys
if len(sys.argv) not in (2,3):
    print('usage:',sys.argv[0],'in.json [out.pdf]')
    sys.exit(1)
if len(sys.argv)==2:
    sys.argv.append(sys.argv[1].rsplit('.',1)[0]+'.pdf')
print(sys.argv[1]+' -> '+sys.argv[2])

import json
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages

with open(sys.argv[1],'r') as f:
    hf = json.load(f)

rew = 'HT1 PDF4LHC15_nlo_30'

def mult(xs,f):
    for x in xs:
        yield from f(x)

with PdfPages(sys.argv[2]) as out:
    for name, hist in hf[rew].items():
        print(name)

        for key, val in hist.items():
            locals()[key] = val

        n = len(xsec)
        xv = [ i+0.5 for i in range(n) ]
        xu = [ 0.5 for x in xv ]

        plt.figure(figsize=(6,4))
        plt.margins(x=0)
        ax = plt.gca()

        umax = [ max(u) for u in zip(mc_unc,scale_up,pdf_errplus) ]
        ymax = max( y+u for y,u in zip(xsec,umax) )
        plt.ylim(0,ymax*1.15)

        plt.fill_between(
            list(mult( xv, lambda x: (x-0.5,x+0.5) )),
            list(mult( (y-u for y,u in zip(xsec,scale_down)),
                       lambda x: (x,x) )),
            list(mult( (y+u for y,u in zip(xsec,scale_up)),
                      lambda x: (x,x) )),
            color='red', alpha=0.6, linewidth=0,
            label="Scale"
        )
        plt.fill_between(
            list(mult( xv, lambda x: (x-0.5,x+0.5) )),
            list(mult( (y-u for y,u in zip(xsec,pdf_errminus)),
                       lambda x: (x,x) )),
            list(mult( (y+u for y,u in zip(xsec,pdf_errplus)),
                      lambda x: (x,x) )),
            color='blue', alpha=0.6, linewidth=0,
            label="PDF"
        )

        plt.errorbar(xv, xsec, mc_unc, xu, ' ',
            linewidth=1.5, color='black',
            # label=rew
        )

        ax.set_title(name, fontname='monospace', fontsize=10)
        ax.set_xlabel(r'bin', ha='right')
        ax.xaxis.set_label_coords(1,-0.08)
        ax.set_ylabel(r'$\sigma$, fb', ha='right')
        ax.yaxis.set_label_coords(-0.08,1)
        ax.set_xticks(list(range(n+1)))
        ax.set_xticklabels([])

        for x,y,i in zip(xv,xsec,range(n)):
            if y >= 0:
                plt.text(x, y+0.03*ymax, '{:.3g}'.format(y),
                    ha='center', fontsize=8,
                    rotation=(45 if (y>0) and (n>=12 or (n>=10 and y<1e-2)) else 0))
            plt.text(x, -0.07*ymax, '{}'.format(i), ha='center')

        plt.legend(ncol=2)

        out.savefig(bbox_inches='tight')
        plt.close()

