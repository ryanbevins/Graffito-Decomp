#!/usr/bin/env python3
# usage: python3 tools/agent/find_ctr_diffs.py [max_fuzzy]  -- non-matching functions whose mtctr count (ctr loops + jump tables) differs retail vs ours
"""Hints: ours>retail often = unrolled/extra loop (array-vs-fields ctor, empty loop); retail>ours = missing jump table or memset/loop form."""
import sys
MAXF=float(sys.argv[1]) if len(sys.argv)>1 else 99.5
import json,re,subprocess,glob,os,collections
r=json.load(open('build/GMSJ01/report.json'))
nm={}
for u in r['units']:
    for f in u.get('functions',[]):
        if f.get('fuzzy_match_percent',100)<MAXF: nm[(u['name'].replace('mario/',''),f['name'])]=f.get('fuzzy_match_percent')
units=set(k[0] for k in nm)
for rel in sorted(units):
    asm=f"build/GMSJ01/asm/{rel}.s"; obj=f"build/GMSJ01/src/{rel}.o"
    if not (os.path.exists(asm) and os.path.exists(obj)): continue
    ret=collections.Counter();cur=None
    for l in open(asm,errors='ignore'):
        m=re.match(r'\.fn (?:"([^"]+)"|([^,\s]+)),',l)
        if m: cur=m.group(1) or m.group(2)
        elif '\tmtctr ' in l and cur: ret[cur]+=1
    ours=collections.Counter();cur=None
    out=subprocess.run(['build/binutils/powerpc-eabi-objdump','-d',obj],capture_output=True,text=True).stdout
    for l in out.splitlines():
        m=re.match(r'[0-9a-f]+ <(.+)>:',l)
        if m: cur=m.group(1)
        elif '\tmtctr' in l and cur: ours[cur]+=1
    for fn in set(ret)|set(ours):
        if (rel,fn) in nm and ret.get(fn,0)!=ours.get(fn,0):
            print(f"{rel} {fn} retail={ret.get(fn,0)} ours={ours.get(fn,0)} fuzzy={nm[(rel,fn)]:.1f}")
