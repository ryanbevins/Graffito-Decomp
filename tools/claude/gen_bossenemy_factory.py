#!/usr/bin/env python3
"""Generate MarNameRefGen_BossEnemy.cpp from extracted asm entries."""
import io
import re
import sys
from pathlib import Path

# Re-run extraction inline to get clean structured data
asm = Path("build/GMSJ01/asm/System/MarNameRefGen_BossEnemy.s").read_text()
rodata_match = re.search(r"\.rodata\n(.*?)(?=\n\.section|\Z)", asm, re.S)
rodata_text = rodata_match.group(1) if rodata_match else ""

offset_to_bytes = {}
for m in re.finditer(
    r"# \.rodata:0x([0-9A-Fa-f]+) \| 0x[0-9A-Fa-f]+ \| size: 0x([0-9A-Fa-f]+)\n"
    r"\.obj \"@(\d+)\", local\n(.*?)\.endobj",
    rodata_text, re.S,
):
    off = int(m.group(1), 16)
    size = int(m.group(2), 16)
    body = m.group(4)
    raw = bytearray()
    for hm in re.finditer(r"\.4byte 0x([0-9A-Fa-f]+)", body):
        v = int(hm.group(1), 16)
        raw.extend(v.to_bytes(4, "big"))
    for bm in re.finditer(r"\.byte ([^\n]+)", body):
        for tok in bm.group(1).split(","):
            tok = tok.strip()
            if tok.startswith("0x"):
                raw.append(int(tok, 16))
    for sm in re.finditer(r'\.string "([^"]*)"', body):
        s = sm.group(1).encode("latin1").decode("unicode_escape").encode("latin1")
        raw.extend(s)
        raw.append(0)
    offset_to_bytes[off] = bytes(raw[:size])

# Build sda2 symbol map: name -> bytes
sda2_match = re.search(r'\.section \.sdata2.*?(?=\n\.section|\Z)', asm, re.S)
sda2_text = sda2_match.group(0) if sda2_match else ""
sda2_to_bytes = {}
for m in re.finditer(
    r"\.obj \"@(\d+)\", local\n(.*?)\.endobj",
    sda2_text, re.S,
):
    name = "@" + m.group(1)
    body = m.group(2)
    raw = bytearray()
    for hm in re.finditer(r"\.4byte 0x([0-9A-Fa-f]+)", body):
        v = int(hm.group(1), 16)
        raw.extend(v.to_bytes(4, "big"))
    for bm in re.finditer(r"\.byte ([^\n]+)", body):
        for tok in bm.group(1).split(","):
            tok = tok.strip()
            if tok.startswith("0x"):
                raw.append(int(tok, 16))
    for sm in re.finditer(r'\.string "([^"]*)"', body):
        s = sm.group(1).encode("latin1").decode("unicode_escape").encode("latin1")
        raw.extend(s)
        raw.append(0)
    sda2_to_bytes[name] = bytes(raw)

def decode(b):
    try:
        return b.split(b"\x00")[0].decode("shift_jis")
    except UnicodeDecodeError:
        return b.split(b"\x00")[0].decode("latin1", errors="replace")

fn_body = re.search(
    r"\.fn getNameRef_BossEnemy__14TMarNameRefGenCFPCc.*?\n(.*?)\n\.endfn", asm, re.S,
).group(1)
instr = []
for line in fn_body.splitlines():
    m = re.search(r"\*/\s*(\S.+?)$", line)
    if m:
        instr.append(m.group(1).strip())

# Walk and find entries
# Each entry: addi r4,r31,OFFSET ; bl strcmp ; cmpwi r3,0 ; bne SKIP ; li r3,SIZE ; bl __nw__ ; mr. r30,r3 ; beq ... ; (load args) ; bl __ct__CLASS...
def class_demangle(ctor):
    # ctor like "11TMapObjBaseFPCc" -> ("TMapObjBase", "FPCc")
    # or "Q26JDrama8TNameRefFPCc" -> ("JDrama::TNameRef", "FPCc")
    if ctor.startswith("Q"):
        # qualified: Q<count><len><name>...<len><name>
        # Q26 means qualified with 2 components, then 6=len of JDrama, then 8=len of TNameRef
        m = re.match(r"Q(\d)(\d+)([^F]+)F(.*)", ctor)
        if m:
            n_parts = int(m.group(1))
            rest = ctor[len(m.group(1)) + 1:]
            parts = []
            i = 0
            for _ in range(n_parts):
                ln_str = ""
                while i < len(rest) and rest[i].isdigit():
                    ln_str += rest[i]
                    i += 1
                ln = int(ln_str)
                parts.append(rest[i:i + ln])
                i += ln
            args = rest[i + 1:]  # skip 'F'
            return "::".join(parts), args
    m = re.match(r"(\d+)(.+)", ctor)
    if not m:
        return ctor, ""
    ln = int(m.group(1))
    rest = m.group(2)
    cls = rest[:ln]
    args = rest[ln + 1:]  # skip 'F'
    return cls, args

# Parse weak destructors to discover inheritance: each __dt__T...Fv contains
# a call to its parent's __dt__, which tells us the inheritance chain.
weak_dt_to_parent = {}
for m in re.finditer(
    r"\.fn (__dt__(\d+)(\w+?)Fv|__dt__(Q\d\d+\w+)Fv), weak\n(.*?)\n\.endfn",
    asm, re.S,
):
    sym = m.group(1)
    body = m.group(5)
    # Determine the class name from the symbol
    if m.group(2):
        n = int(m.group(2))
        nm = m.group(3)
        cls = nm[:n] if len(nm) >= n else nm
    else:
        # qualified - skip
        continue
    # Find first bl __dt__ in the body
    inner = re.search(r"bl __dt__(\d+)(\w+?)Fv", body)
    if inner:
        pn = int(inner.group(1))
        pnm = inner.group(2)
        parent = pnm[:pn] if len(pnm) >= pn else pnm
        weak_dt_to_parent[cls] = parent

entries = []
i = 0
while i < len(instr):
    line = instr[i]
    # rodata cmp: addi r4, r31, OFFSET
    m = re.match(r"addi r4, r31, (0x[0-9A-Fa-f]+)", line)
    # sda2 cmp: li r4, "@NNNN"@sda21
    m2 = re.match(r'li r4, "(@\d+)"@sda21', line)
    if (m or m2) and i + 1 < len(instr) and "bl strcmp" in instr[i + 1]:
        cmp_off = int(m.group(1), 16) if m else None
        cmp_sda2 = m2.group(1) if m2 else None
        size = None
        ctor = None
        ctor_str_off = None
        e_sda2 = None
        e_vtable_class = None
        int_arg = None  # for ctors with int args
        for j in range(i + 2, min(i + 25, len(instr))):
            sl = instr[j]
            sm = re.match(r"li r3, (0x[0-9A-Fa-f]+|\d+)", sl)
            if sm and size is None and "li r3, 0x" in sl:
                size = int(sm.group(1), 0)
            cm = re.match(r"bl __ct__(\S+)", sl)
            if cm:
                ctor = cm.group(1)
                ctor_str_sda2 = None
                # After ctor call, look forward for a vtable swap pattern:
                #   lis r?, __vt__NNTXXX@ha   (NN is the name length prefix)
                #   addi r?, r?, __vt__NNTXXX@l
                #   stw r?, 0(r30)
                # If present, the *actual* class is TXXX, not the base ctor's class.
                vtable_class = None
                for kk in range(j + 1, min(j + 12, len(instr))):
                    vt = re.search(r"__vt__(\d+)(\w+)@(ha|l)", instr[kk])
                    if vt:
                        n = int(vt.group(1))
                        nm = vt.group(2)
                        vtable_class = nm[:n] if len(nm) >= n else nm
                        break
                    # qualified vtables: __vt__Q26JDrama8TViewObj etc.
                    vtq = re.search(r"__vt__Q\d(\w+)@(ha|l)", instr[kk])
                    if vtq:
                        # Skip qualified vtables for now - usually base class
                        break
                    if "bl __ct__" in instr[kk] or "bl strcmp" in instr[kk]:
                        break
                e_vtable_class = vtable_class
                # find ctor's args. Look back from j to find r4/r5 setup.
                for k in range(j - 1, max(j - 8, i + 1), -1):
                    al = instr[k]
                    # rodata via r31: addi r4/r5, r31, OFFSET
                    am1 = re.match(r"addi r5, r31, (0x[0-9A-Fa-f]+)", al)
                    if am1 and ctor_str_off is None and ctor_str_sda2 is None:
                        ctor_str_off = int(am1.group(1), 16)
                    am2 = re.match(r"addi r4, r31, (0x[0-9A-Fa-f]+)", al)
                    if am2 and ctor_str_off is None and ctor_str_sda2 is None:
                        ctor_str_off = int(am2.group(1), 16)
                    # sda2: li r4, "@NNNN"@sda21  (actual encoding addi r4,r2,off)
                    sd1 = re.match(r'li r4, "(@\d+)"@sda21', al)
                    if sd1 and ctor_str_sda2 is None and ctor_str_off is None:
                        ctor_str_sda2 = sd1.group(1)
                    sd2 = re.match(r'li r5, "(@\d+)"@sda21', al)
                    if sd2 and ctor_str_sda2 is None and ctor_str_off is None:
                        ctor_str_sda2 = sd2.group(1)
                    # int args: li r4, N (number, no @)
                    im = re.match(r"li r4, (0x[0-9A-Fa-f]+|\d+)\s*$", al)
                    if im:
                        int_arg = int(im.group(1), 0)
                # Save sda2 reference into a separate hop
                e_sda2 = ctor_str_sda2
                break
            if "bl strcmp" in sl or sl.startswith("b ") and ".L_" in sl:
                break
        if cmp_off is not None:
            cmp_str = decode(offset_to_bytes.get(cmp_off, b""))
        elif cmp_sda2:
            cmp_str = decode(sda2_to_bytes.get(cmp_sda2, b""))
        else:
            cmp_str = ""
        if ctor_str_off is not None:
            ctor_str = decode(offset_to_bytes.get(ctor_str_off, b""))
        elif e_sda2:
            ctor_str = decode(sda2_to_bytes.get(e_sda2, b""))
        else:
            ctor_str = ""
        entries.append({
            "cmp": cmp_str,
            "cmp_off": cmp_off,
            "size": size,
            "ctor": ctor,
            "ctor_str": ctor_str,
            "int_arg": int_arg,
            "vtable_class": e_vtable_class,
        })
    i += 1

# Class -> (header path, expects_int_first)
# Headers are best-effort; missing ones produce stub forward decls
class_to_header = {
    "TBossGesso": "Enemy/BossGesso.hpp",
    "TBossGessoManager": "Enemy/BossGesso.hpp",
    "THinokuri2": "Enemy/Hinokuri2.hpp",
    "THinokuri2Manager": "Enemy/Hinokuri2.hpp",
    "TSpineEnemy": "Enemy/Enemy.hpp",
    "TEnemyManager": "Enemy/EnemyManager.hpp",
    "TBathtubKiller": "Enemy/BathtubKiller.hpp",
    "TBathtubKillerManager": "Enemy/BathtubKiller.hpp",
    "TCoasterKiller": "Enemy/CoasterKiller.hpp",
    "TCoasterKillerManager": "Enemy/CoasterKiller.hpp",
    "TMapObjBase": "MoveBG/MapObjBase.hpp",
    "TMapObjGeneral": "MoveBG/MapObjGeneral.hpp",
    "TItem": "MoveBG/Item.hpp",
    "TResetFruit": "MoveBG/MapObjBall.hpp",
    "TRandomFruit": "MoveBG/MapObjBall.hpp",
    "TMapStaticObj": "Map/MapStaticObject.hpp",
    "TMapObjSoundGroup": "Map/MapStaticObject.hpp",
    "TMapObjManager": "MoveBG/MapObjManager.hpp",
    "TMapObjBaseManager": "MoveBG/MapObjManager.hpp",
    "TItemManager": "MoveBG/ItemManager.hpp",
    "TPoolManager": "MoveBG/Pool.hpp",
    "TMapWireManager": "Map/MapWireManager.hpp",
    "TMapObjPoleManager": "MoveBG/MapObjPole.hpp",
    "TMapObjWave": "MoveBG/MapObjWave.hpp",
    "TMapObjPlane": "MoveBG/MapObjPlane.hpp",
    "TMapObjGrassManager": "MoveBG/MapObjGrass.hpp",
    "TMapObjGrassGroup": "MoveBG/MapObjGrass.hpp",
    "TMammaMirrorMapOperator": "MoveBG/MapObjMamma.hpp",
    "THitActor": "Strategic/HitActor.hpp",
    "THideObjBase": "MoveBG/MapObjHide.hpp",
    "TMapObjWaterSpray": "MoveBG/MapObjTown.hpp",
    "TMapObjFloatOnSea": "MoveBG/MapObjFloat.hpp",
    "TWoodBarrel": "MoveBG/WoodBarrel.hpp",
    "TFence": "MoveBG/MapObjFence.hpp",
    "TRailFence": "MoveBG/MapObjFence.hpp",
    "TMapObjBall": "MoveBG/MapObjBall.hpp",
    "TPool": "MoveBG/Pool.hpp",
    "TWaterHitPictureHideObj": "MoveBG/MapObjSirena.hpp",
    "TMapObjSwitch": "MoveBG/MapObjTown.hpp",
    "TRedCoinSwitch": "MoveBG/MapObjTown.hpp",
    "THideObjInfo": "MoveBG/MapObjTown.hpp",
    "TJumpBase": "MoveBG/MapObjItem2.hpp",
    "TLampTrapIron": "MoveBG/MapObjTrap.hpp",
    "TLampTrapSpike": "MoveBG/MapObjTrap.hpp",
    "TMapObjTree": "MoveBG/MapObjTree.hpp",
    "TMapObjTreeScale": "MoveBG/MapObjTree.hpp",
    "TRideCloud": "MoveBG/MapObjCloud.hpp",
    "TAirportSwitch": "MoveBG/MapObjAirport.hpp",
    "TAirportEventSink": "MoveBG/MapObjAirport.hpp",
    "TMonumentShine": "MoveBG/MapObjDolpic.hpp",
    "TBellDolpic": "MoveBG/MapObjDolpic.hpp",
    "TMapObjTurn": "MoveBG/MapObjTurn.hpp",
    "TRiccoWatermill": "MoveBG/MapObjRicco.hpp",
    "TSandBird": "MoveBG/MapObjMamma.hpp",
    "TSandBase": "MoveBG/MapObjMamma.hpp",
    "TSandBombBase": "MoveBG/MapObjMamma.hpp",
    "TLeanMirror": "MoveBG/MapObjMamma.hpp",
    "TBigWatermelon": "MoveBG/MapObjBall.hpp",
    "TShiningStone": "MoveBG/MapObjMamma.hpp",
    "TSandCastle": "MoveBG/MapObjMamma.hpp",
    "TMammaBlockRotate": "MoveBG/MapObjMamma.hpp",
    "TGoalWatermelon": "MoveBG/MapObjMamma.hpp",
    "TMerrygoround": "MoveBG/MapObjPinna.hpp",
    "TFerrisWheel": "MoveBG/MapObjPinna.hpp",
    "TShellCup": "MoveBG/MapObjPinna.hpp",
    "TPinnaCoaster": "MoveBG/MapObjPinna.hpp",
    "TCogwheel": "MoveBG/MapObjMare.hpp",
    "TMapObjGrowTree": "MoveBG/MapObjMare.hpp",
    "TWireBell": "MoveBG/MapObjMare.hpp",
    "TMuddyBoat": "MoveBG/MapObjMare.hpp",
    "TJointCoin": "MoveBG/MapObjEx.hpp",
    "TRoulette": "MoveBG/MapObjSirena.hpp",
    "TSlotDrum": "MoveBG/MapObjSirena.hpp",
    "TItemSlotDrum": "MoveBG/MapObjSirena.hpp",
    "TCasinoPanelGate": "MoveBG/MapObjSirena.hpp",
    "TWarpAreaActor": "MoveBG/MapObjSirena.hpp",
    "TSakuCasino": "MoveBG/MapObjSirena.hpp",
    "THangingBridge": "MoveBG/MapObjMonte.hpp",
    "TSwingBoard": "MoveBG/MapObjMonte.hpp",
    "TFluffManager": "MoveBG/MapObjMonte.hpp",
    "TFileLoadBlock": "MoveBG/MapObjOption.hpp",
    "TNormalLift": "MoveBG/MapObjRailBlock.hpp",
    "TRailBlock": "MoveBG/MapObjRailBlock.hpp",
    "TRollBlock": "MoveBG/MapObjRailBlock.hpp",
    "TWoodBlock": "MoveBG/MapObjRailBlock.hpp",
    "TMapObjNail": "MoveBG/MapObjEx.hpp",
    "TMapObjRevivalPollution": "MoveBG/MapObjPollution.hpp",
    "TPolluterBase": "MoveBG/MapObjPollution.hpp",
    "TCoinBlue": "MoveBG/Item.hpp",
    "TCoin": "MoveBG/Item.hpp",
    "TNozzleBox": "MoveBG/Item.hpp",
    "TMushroom1up": "MoveBG/MapObjItem2.hpp",
    "TEggYoshi": "MoveBG/Item.hpp",
    "JDrama::TNameRef": "JSystem/JDrama/JDRNameRef.hpp",
    "TCloset": "MoveBG/MapObjSirena.hpp",
    "TCoinRed": "MoveBG/Item.hpp",
    "TDonchou": "MoveBG/MapObjSirena.hpp",
    "TDoor": "MoveBG/MapObjTown.hpp",
    "TManhole": "MoveBG/MapObjTown.hpp",
    "TShine": "MoveBG/Item.hpp",
    "TViking": "MoveBG/MapObjPinna.hpp",
    "TBathtub": "MoveBG/MapObjCorona.hpp",
    "TGateShadow": "MoveBG/MapObjSample.hpp",
    "TAmiKing": "MoveBG/MapObjPinna.hpp",
    "TBalloonKoopaJr": "MoveBG/MapObjPinna.hpp",
    "TBasketReverse": "MoveBG/MapObjTown.hpp",
    "TBreakHideObj": "MoveBG/MapObjHide.hpp",
    "TBreakableBlock": "MoveBG/MapObjBlock.hpp",
    "TBrickBlock": "MoveBG/MapObjBlock.hpp",
    "TChestRevolve": "MoveBG/MapObjSirena.hpp",
    "TCoverFruit": "MoveBG/MapObjBall.hpp",
    "TCraneRotY": "MoveBG/MapObjRicco.hpp",
    "TCraneUpDown": "MoveBG/MapObjRicco.hpp",
    "TDamageObj": "MoveBG/MapObjTown.hpp",
    "TDemoCannon": "MoveBG/MapObjDolpic.hpp",
    "TDptMonteFence": "MoveBG/MapObjDolpic.hpp",
    "TFenceWater": "MoveBG/MapObjFence.hpp",
    "TFlowerCoin": "MoveBG/Item.hpp",
    "TFruitLauncher": "MoveBG/MapObjRicco.hpp",
    "TFruitSwitch": "MoveBG/MapObjRicco.hpp",
    "TGoalFlag": "MoveBG/MapObjMonte.hpp",
    "TIceBlock": "MoveBG/MapObjBlock.hpp",
    "TItemNozzle": "MoveBG/Item.hpp",
    "TJuiceBlock": "MoveBG/MapObjBlock.hpp",
    "TJumpMushroom": "MoveBG/MapObjMonte.hpp",
    "TMammaYacht": "MoveBG/MapObjMamma.hpp",
    "TMapObjBillboard": "MoveBG/MapObjTown.hpp",
    "TMapObjChangeStage": "MoveBG/MapObjTown.hpp",
    "TMapObjElasticCode": "MoveBG/MapObjMare.hpp",
    "TMapObjMonteRoot": "MoveBG/MapObjMonte.hpp",
    "TMapObjPuncher": "MoveBG/MapObjMare.hpp",
    "TMapObjSmoke": "MoveBG/MapObjDolpic.hpp",
    "TMapObjStartDemo": "MoveBG/MapObjTown.hpp",
    "TMapObjSteam": "MoveBG/MapObjEx.hpp",
    "TMareCork": "MoveBG/MapObjMare.hpp",
    "TMareEventPoint": "MoveBG/MapObjMare.hpp",
    "TMareFall": "MoveBG/MapObjMare.hpp",
    "TMareGate": "MoveBG/MapObjDolpic.hpp",
    "TPanelRevolve": "MoveBG/MapObjSirena.hpp",
    "TPictureTelesa": "MoveBG/MapObjSirena.hpp",
    "TPinnaEntrance": "MoveBG/MapObjPinna.hpp",
    "TRockPlane": "MoveBG/MapObjPlane.hpp",
    "TSandBlock": "MoveBG/MapObjBlock.hpp",
    "TSandEgg": "MoveBG/MapObjMamma.hpp",
    "TSandLeaf": "MoveBG/MapObjMamma.hpp",
    "TSandLeafBase": "MoveBG/MapObjMamma.hpp",
    "TSandPlane": "MoveBG/MapObjPlane.hpp",
    "TSirenaCasinoRoof": "MoveBG/MapObjSirena.hpp",
    "TSirenabossWall": "MoveBG/MapObjSirena.hpp",
    "TSurfGesoObj": "MoveBG/MapObjRicco.hpp",
    "TTakeActor": "Strategic/TakeActor.hpp",
    "TTurboNozzleDoor": "MoveBG/MapObjDolpic.hpp",
    "TWaterRecoverObj": "MoveBG/MapObjPinna.hpp",
}

# Some entries get inlined in a way that the ctor symbol points to a base class.
# Map cmp_str -> override class name.
cmp_to_class_override = {
    "GateShadow": "TGateShadow",
}

header_classes_with_name_ctor = {
    "TBathtubKiller", "TBathtubKillerManager", "TBossGesso", "TBossGessoManager",
    "TCoasterKiller", "TCoasterKillerManager", "THinokuri2", "THinokuri2Manager",
    "TSpineEnemy", "TEnemyManager",
    # Enemy/* derived classes with const-char ctors
    "TDangoHamuKuri", "TDangoHamuKuriManager",
    "TFireHamuKuri", "TFireHamuKuriManager",
    "TDoroHamuKuri", "TDoroHamuKuriManager",
    "TDoroHaneKuri", "TDoroHaneKuriManager",
    "THaneHamuKuri", "THaneHamuKuri2", "THaneHamuKuriManager",
    "THamuKuri", "THamuKuriManager",
    "TBossDangoHamuKuri", "TBossDangoHamuKuriManager",
}

# Classes we need stub declarations for (no header).
# Sizes are derived from the entries themselves at generation time.
stub_classes_with_size = {}

# Header classes we know exist
known_header_classes = set()

# Build the output
def cpp_class_ref(ctor):
    cls, args = class_demangle(ctor)
    return cls, args

# Determine includes needed
includes = set()
needed_stubs = set()
for e in entries:
    if not e["ctor"]:
        continue
    base_cls, _ = cpp_class_ref(e["ctor"])
    cls = base_cls
    if e["cmp"] in cmp_to_class_override:
        cls = cmp_to_class_override[e["cmp"]]
    # Only use vtable-detected derived class when (a) it differs from base
    # and (b) it doesn't already have a header (avoids signature mismatch with
    # existing ctors that take no args).
    elif (e["vtable_class"] and e["vtable_class"] != base_cls
          and (e["vtable_class"] not in class_to_header
               or e["vtable_class"] in header_classes_with_name_ctor)):
        cls = e["vtable_class"]
    if cls in class_to_header:
        includes.add(class_to_header[cls])
    else:
        needed_stubs.add(cls)
        # Pin size from the entry's size (will be overwritten if multiple entries use same class with different sizes - last wins)
        if e["size"] is not None:
            stub_classes_with_size[cls] = e["size"]
        # Also include base class header so we can inherit from it
        if base_cls in class_to_header:
            includes.add(class_to_header[base_cls])

out = io.StringIO()
print("#include <System/MarNameRefGen.hpp>", file=out)
print("#include <string.h>", file=out)
for inc in sorted(includes):
    print(f"#include <{inc}>", file=out)
print(file=out)

if needed_stubs:
    NAMEREF_BASE_SIZE = 0x8
    # For each stub, figure out base class - prefer the asm-detected base ctor.
    stub_to_base = {}
    for e in entries:
        if not e["ctor"]:
            continue
        base_cls, _ = cpp_class_ref(e["ctor"])
        cls = base_cls
        if e["cmp"] in cmp_to_class_override:
            cls = cmp_to_class_override[e["cmp"]]
        elif e["vtable_class"] and e["vtable_class"] != base_cls:
            cls = e["vtable_class"]
        if cls in needed_stubs and cls != base_cls:
            stub_to_base[cls] = base_cls
            # Also need the base class as a stub if it's not in a header
            if base_cls not in class_to_header:
                needed_stubs.add(base_cls)
                if base_cls not in stub_classes_with_size and e["size"] is not None:
                    stub_classes_with_size[base_cls] = e["size"]

    # Use weak destructor chain info to find parents of stubs.
    # Repeat until stable so chains like A -> B -> C resolve.
    for _ in range(5):
        changed = False
        for cls in list(needed_stubs):
            if cls in stub_to_base:
                continue
            parent = weak_dt_to_parent.get(cls)
            if parent and parent != cls:
                stub_to_base[cls] = parent
                changed = True
                # If parent is also unknown and not in a header, add as stub
                if parent not in class_to_header and parent not in needed_stubs:
                    needed_stubs.add(parent)
                    if parent not in stub_classes_with_size:
                        stub_classes_with_size[parent] = stub_classes_with_size.get(cls, 0x8)
        if not changed:
            break
    # Determine ctor signature per stub class from entries
    stub_ctor_sig = {}
    for e in entries:
        if not e["ctor"]:
            continue
        bcls, args = cpp_class_ref(e["ctor"])
        c = bcls
        if e["cmp"] in cmp_to_class_override:
            c = cmp_to_class_override[e["cmp"]]
        elif e["vtable_class"] and e["vtable_class"] != bcls and e["vtable_class"] not in class_to_header:
            c = e["vtable_class"]
        if c in needed_stubs:
            stub_ctor_sig[c] = args

    def ctor_decl_for(cls, args):
        if args in ("Fv", "v"):
            return f"{cls}();"
        if args in ("FiPCc", "iPCc"):
            return f"{cls}(int, const char*);"
        if args in ("FUlPCc", "UlPCc"):
            return f"{cls}(u32, const char*);"
        if args in ("FPCci", "PCci"):
            return f"{cls}(const char*, int);"
        return f"{cls}(const char*);"

    print("// Forward declarations for classes whose headers don't exist yet.", file=out)
    print("// Sizes are pinned to match what the original asm passed to operator new.", file=out)
    for cls in sorted(needed_stubs):
        sz = stub_classes_with_size[cls]
        base = stub_to_base.get(cls, "JDrama::TNameRef")
        has_weak_dt = cls in weak_dt_to_parent
        args = stub_ctor_sig.get(cls, "FPCc")
        # If base is in headers, use it as the parent.
        if base != "JDrama::TNameRef" and (base in class_to_header or base in {"JDrama::TNameRef", "THitActor", "TLiveActor"}):
            pad = sz - NAMEREF_BASE_SIZE
            print(f"class {cls} : public {base} {{", file=out)
            print("public:", file=out)
            if args in ("FiPCc", "iPCc"):
                print(f"\t{cls}(int kind, const char* name) : {base}(name) {{ (void)kind; }}", file=out)
            elif args in ("FUlPCc", "UlPCc"):
                print(f"\t{cls}(u32 kind, const char* name) : {base}(name) {{ (void)kind; }}", file=out)
            elif args in ("FPCci", "PCci"):
                print(f"\t{cls}(const char* name, int kind) : {base}(name) {{ (void)kind; }}", file=out)
            else:
                print(f"\t{cls}(const char* name) : {base}(name) {{}}", file=out)
            if has_weak_dt:
                print(f"\tvirtual ~{cls}() {{}}", file=out)
            print("};", file=out)
        else:
            pad = sz - NAMEREF_BASE_SIZE
            print(f"class {cls} : public JDrama::TNameRef {{", file=out)
            print("public:", file=out)
            print(f"\t{ctor_decl_for(cls, args)}", file=out)
            if pad > 0:
                print(f"\tchar _stub[0x{pad:x}];", file=out)
            print("};", file=out)
        print(file=out)

print("JDrama::TNameRef* TMarNameRefGen::getNameRef_BossEnemy(const char* name) const", file=out)
print("{", file=out)
for e in entries:
    cmp_s = e["cmp"]
    if not cmp_s or not e["ctor"]:
        continue
    base_cls, args = cpp_class_ref(e["ctor"])
    cls = base_cls
    # Escape special chars (only " for now)
    cmp_escaped = cmp_s.replace("\\", "\\\\").replace('"', '\\"')
    ctor_arg = e["ctor_str"]
    ctor_arg_escaped = ctor_arg.replace("\\", "\\\\").replace('"', '\\"')

    # Override class if needed
    if cmp_s in cmp_to_class_override:
        cls = cmp_to_class_override[cmp_s]
    elif (e["vtable_class"] and e["vtable_class"] != base_cls
          and (e["vtable_class"] not in class_to_header
               or e["vtable_class"] in header_classes_with_name_ctor)):
        cls = e["vtable_class"]

    if args == "Fv" or args == "v":
        ctor_call = f"new {cls}()"
    elif args in ("FiPCc", "iPCc", "FUlPCc", "UlPCc"):
        intv = e["int_arg"] if e["int_arg"] is not None else 0
        ctor_call = f'new {cls}({intv}, "{ctor_arg_escaped}")'
    elif args in ("FPCci", "PCci"):
        intv = e["int_arg"] if e["int_arg"] is not None else 0
        ctor_call = f'new {cls}("{ctor_arg_escaped}", {intv})'
    else:
        ctor_call = f'new {cls}("{ctor_arg_escaped}")'

    print(f'\tif (strcmp(name, "{cmp_escaped}") == 0)', file=out)
    print(f'\t\treturn {ctor_call};', file=out)

print("\treturn nullptr;", file=out)
print("}", file=out)

sys.stdout.buffer.write(out.getvalue().encode("utf-8"))
