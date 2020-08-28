#!/usr/bin/env python3

import sys, os, sqlite3, json, re
from subprocess import Popen, PIPE
from collections import defaultdict
from itertools import product

loc = os.path.abspath(os.path.join(
     os.path.dirname(os.path.realpath(__file__)),os.pardir))

chunk_size = 50e6

with open(loc+'/config.json') as f:
    config = json.load(f)
config['higgs_decay_seed'] = 0 # use time as seed

db = sqlite3.connect('/home/ivanp/work/ntuple_analysis/sql/ntuples.db')

subcount = defaultdict(lambda:0)

def mkdirs(*ds):
    for d in ds:
        if not os.path.isdir(d):
            os.makedirs(d)

mkdirs(loc+'/condor',loc+'/output/jobs')

def get(names,vals):
    fs = [ ( x[-1], x[0]+'/'+x[1], x[3],
        '{}{}j{}_{:g}TeV_antikt{:g}'.format(
            x[2], x[3], x[4],
            x[5],
            config['jet_R']*10
        )
    ) for x in db.execute('''
SELECT dir,file,particle,njets,part,energy,info,nentries
FROM ntuples
WHERE
'''+' and '.join(a+'=?' for a in names),vals).fetchall() ]

    pref = set([f[-1] for f in fs])
    if len(pref) > 1:
        raise Exception('multiple types in single selection: '+' '.join(pref))
    elif len(pref)==0:
        return [ ]
    pref = pref.pop()

    chunks = [ ]
    n = 0
    for x in fs:
        if n == 0:
            subcount[pref] += 1
            chunks.append(('{}_{:0>3d}'.format(pref,subcount[pref]),[],x[2]))
        chunks[-1][1].append(x[1])
        n += x[0]
        if n >= chunk_size:
            n = 0

    return chunks

LD_LIBRARY_PATH = os.environ['LD_LIBRARY_PATH']

def condor(chunk):
    script = loc+'/condor/'+chunk[0]+'.sh'
    config['njets_min'] = chunk[2]
    with open(script,'w') as f:
        f.write('''\
#!/bin/bash
export LD_LIBRARY_PATH={}\n
cd {}
bin/hist \\\n  <(echo '{}') \\\n  {} \\\n  {}
'''.format(
            LD_LIBRARY_PATH,
            loc,
            json.dumps(config),
            'output/jobs/'+chunk[0]+'.cbor',
            ' \\\n  '.join(chunk[1])
        ))
    os.chmod(script,0o775)
    return '''\
Universe   = vanilla
Executable = {0}.sh
Output     = {0}.out
Error      = {0}.err
Log        = {0}.log
getenv = True
+IsMediumJob = True
Queue
'''.format(chunk[0])

params = tuple(zip(
    ('njets',(1,2,3)),
    ('part',('B','RS','I','V')),
    ('particle',('H',)),
    ('energy',(13,))
))

infos = [
    ({ 'njets': 1 },
     [ 'GGFHT pt25.0 eta4.5' ]),
    ({ 'njets': 2 },
     [ 'ED GGFHT pt25.0 eta4.5' ]),
    ({ 'njets': 3 },
     [ 'ED GGFHT pt25.0 eta4.5' ]),
]

os.chdir(loc+'/condor')
for vals in product(*params[1]):
    for info_key, info in infos:
        matched = True
        for key, val in zip(params[0],vals):
            val2 = info_key.get(key)
            if val2 is not None and val2 != val:
                matched = False
                break
        if matched:
            for x in info:
                for chunk in get(params[0]+('info',),vals+(x,)):
                    outf = loc + '/condor/' + chunk[0] + '.out'
                    if os.path.exists(outf):
                        continue

                    print(chunk[0])

                    p = Popen(('condor_submit','-'), stdin=PIPE, stdout=PIPE)
                    p.stdin.write( condor(chunk).encode() )
                    p.communicate()
                    p.stdin.close()

