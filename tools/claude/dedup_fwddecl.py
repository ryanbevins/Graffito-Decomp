#!/usr/bin/env python3
"""Delete forward declarations of classes that now resolve via included headers."""
import re
from pathlib import Path

src_path = Path("src/System/MarNameRefGen_Enemy.cpp")
src = src_path.read_text(encoding="utf-8")

# Classes whose headers we now include — remove their forward decls.
classes_with_headers = {
    "TTypicalEnemy", "TTypicalManager",
    "TRiccoHook", "TRiccoHookManager",
    "TEnemyManager",
    "TAnimalBase", "TAnimalManagerBase",
    "TEggGenerator", "TEggGenManager",
    "TEffectColumWaterManager", "TEffectBombColumWaterManager",
    "TEffectColumSandManager", "TEffectExplosionManager",
    "TSmallEnemyManager", "TSimpleEffect", "TEffectEnemy",
    "TSpineEnemy",
    "THamuKuriManager", "THamuKuri",
    "THaneHamuKuriManager", "THaneHamuKuri", "THaneHamuKuri2",
    "TDoroHaneKuriManager", "TDoroHaneKuri",
    "TDangoHamuKuriManager",
    "TBossDangoHamuKuriManager", "TBossDangoHamuKuri",
    "TFireHamuKuriManager", "TFireHamuKuri",
    "TDoroHamuKuriManager", "TDoroHamuKuri",
    "TNameKuriManager", "TNameKuri",
    "TPoiHanaManager", "TPoiHana", "TSleepPoiHana",
    "TBoxTelesa", "TLoopTelesa", "TMarioModokiTelesa",
    "TSeeTelesa", "TTelesa", "TTelesaManager",
    "TGesso", "TGessoManager",
    "TMameGesso", "TMameGessoManager",
    "TWalkerEnemy",
    "TLauncher", "TLauncherManager",
    "TCommonLauncher", "TCommonLauncherManager",
    "TKumokun", "TKumokunManager",
    "TTamaNoko", "TTamaNokoManager",
    "TAmenbo", "TAmenboManager",
    "TRocket",
    "TBeamManager",
    "TFireWanwan", "TFireWanwanManager",
}

# Pattern: class NAME ... { ... };  (single block)
pattern = re.compile(
    r"^class (\w+) : public [\w:]+ \{[^}]*?\};\n+",
    re.M,
)

def maybe_drop(match):
    name = match.group(1)
    if name in classes_with_headers:
        return ""
    return match.group(0)

new_src = pattern.sub(maybe_drop, src)

# Also drop the JDrama::TActor redeclaration (now from JDRActor.hpp)
new_src = re.sub(
    r"namespace JDrama \{\nclass TActor : public TNameRef \{[^}]*?\};\n\} // namespace JDrama\n+",
    "",
    new_src,
)

src_path.write_text(new_src, encoding="utf-8")
print(f"New line count: {len(new_src.splitlines())}")
