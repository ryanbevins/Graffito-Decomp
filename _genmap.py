import subprocess, collections, sys
OBJDUMP = r"C:/Users/ryana/Documents/sms/build/binutils/powerpc-eabi-objdump.exe"
ELF = r"C:/Users/ryana/Documents/sms/build/GMSJ01/mario.elf"
out = subprocess.run([OBJDUMP, "-t", ELF], capture_output=True, text=True).stdout
secs = collections.defaultdict(list)
for line in out.splitlines():
    # format: ADDR flags... SECTION SIZE NAME   (function F or object O)
    p = line.split()
    if len(p) < 5: continue
    try: addr = int(p[0], 16)
    except: continue
    if 'F' not in line[:30] and 'O' not in line[:30]:  # keep F/O symbols
        continue
    # find section (starts with .) and size (hex) and name (last)
    sec = None; size = 0; name = p[-1]
    for i,tok in enumerate(p):
        if tok.startswith('.') and i>0:
            sec = tok
            if i+1 < len(p):
                try: size = int(p[i+1],16)
                except: size = 0
            break
    if sec is None or name=='' : continue
    if addr < 0x80000000: continue
    secs[sec].append((addr,size,name))
order = ['.init','.text','.ctors','.dtors','.rodata','.data','.bss','.sdata','.sdata2','.sbss','.sbss2','extab','extabindex']
lines=[]
done=set()
def emit(sec):
    if sec not in secs: return
    lines.append(f"{sec} section layout")
    for a,sz,nm in sorted(secs[sec]):
        lines.append(f"  {a:08x} {sz:08x} {a:08x}  0 {nm}")
    lines.append("")
    done.add(sec)
for s in order: emit(s)
for s in sorted(secs):
    if s not in done: emit(s)
open("_GMSJ01.map","w",encoding="utf-8").write("\n".join(lines))
print("symbols:", sum(len(v) for v in secs.values()), "sections:", len(secs))
import re
m=[l for l in lines if l.endswith(" OSReport")]
print("OSReport line:", m[0] if m else "NOT FOUND")
