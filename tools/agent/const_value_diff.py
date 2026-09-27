# usage: python tools/agent/const_value_diff.py <unit e.g. mario/Enemy/amiNoko> [function substrings...]
"""Per-function sdata2 literal value mismatches (retail vs ours) from decomp-diff relocation rows."""
import json,re,subprocess,struct,sys
unit=sys.argv[1]; rel=unit.split('/',1)[1]
src=f'build/GMSJ01/src/{rel}.o'; asm=f'build/GMSJ01/asm/{rel}.s'
fns=sys.argv[2:]
if not fns:
    r=json.load(open('build/GMSJ01/report.json'))
    for u in r['units']:
        if u['name']==unit:
            fns=[f['metadata'].get('demangled_name',f['name']).split('(')[0] for f in u.get('functions',[]) if f.get('fuzzy_match_percent',0)<100]
B='build/binutils/powerpc-eabi-'
# ours
syms={}
for l in subprocess.run([B+'objdump','-t',src],capture_output=True,text=True).stdout.splitlines():
    m=re.match(r'([0-9a-f]+)\s+l\s+O\s+\.sdata2\s+([0-9a-f]+)\s+(\S+)',l)
    if m: syms[m.group(3)]=(int(m.group(1),16),int(m.group(2),16))
data=subprocess.run([B+'objcopy','-O','binary','-j','.sdata2',src,'/dev/stdout'],capture_output=True).stdout
def ours(n):
    o,s=syms[n]; b=data[o:o+s]
    return struct.unpack('>f',b)[0] if s==4 else struct.unpack('>d',b)[0]
ret={}
txt=open(asm).read()
for m in re.finditer(r'\.obj "?(@\d+)"?, local\s*\n\s*\.(float|double) (\S+)',txt): ret[m.group(1)]=m.group(3)
for f in fns:
    out=subprocess.run(['python','tools/decomp-diff.py','-u',unit,'-d',f],capture_output=True,text=True).stdout
    for l in out.splitlines():
        m=re.search(r'\{lf[sd] f\d+, (@\d+)@sda21\}\s*\|\s*\{lf[sd] f\d+, (@\d+)@sda21\}',l)
        if m:
            a,b=m.groups(); ov=ours(b); rv=ret.get(a)
            flag='' if rv is not None and abs(float(rv)-ov)<1e-6*max(1,abs(ov)) else '  <<< DIFF'
            if flag: print(f, l.split('|')[0].strip()[:8], a, rv, b, ov, flag)
