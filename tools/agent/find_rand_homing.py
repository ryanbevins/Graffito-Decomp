# usage: python3 tools/agent/find_rand_homing.py
# Scans build/GMSJ01/asm for the int/float "rand-homing" signature:
# a `subf rN`/`fsubs fN` into a callee-saved reg within ~14 instrs before `bl rand`,
# preceded by literal `li r0, imm; stw` homing. Maps each hit to its enclosing .fn
# and the function's fuzzy match % from report.json. Prints candidates <100%.
import json, os, re, glob
ASM='build/GMSJ01/asm'
rep=json.load(open('build/GMSJ01/report.json'))
units=rep['units'] if 'units' in rep else rep
pct={}
for u in units:
    for f in u.get('functions',[]):
        pct[f.get('name','')]=f.get('fuzzy_match_percent',0)
def callee_saved(reg):
    m=re.match(r'r(\d+)$',reg)
    if m: return 14<=int(m.group(1))<=31
    m=re.match(r'f(\d+)$',reg)
    if m: return 14<=int(m.group(1))<=31
    return False
hits=[]
for s in glob.glob(ASM+'/**/*.s',recursive=True):
    lines=open(s,errors='ignore').read().splitlines()
    cur=None
    for i,l in enumerate(lines):
        m=re.match(r'\.fn\s+(\S+),',l)
        if m: cur=m.group(1).rstrip(',')
        if 'bl rand' in l and cur:
            window=lines[max(0,i-14):i]
            sig=None
            for w in window:
                ws=w.split('*/',1)[-1].strip()
                m2=re.match(r'(subf|fsubs)\s+([rf]\d+),',ws)
                if m2 and callee_saved(m2.group(2)):
                    sig=ws
            if sig:
                p=pct.get(cur,None)
                if p is not None and p<100.0:
                    hits.append((round(p,1),os.path.relpath(s,ASM),cur,sig))
hits.sort()
seen=set()
for p,f,fn,sig in hits:
    if fn in seen: continue
    seen.add(fn)
    print(f"{p:5.1f}%  {f:28s} {fn}")
    print(f"        {sig}")
