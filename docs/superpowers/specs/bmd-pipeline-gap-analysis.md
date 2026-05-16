# BMD Export Pipeline: Technical Gap Analysis

**Date:** 2026-03-20
**Context:** Cape powerup mod for Super Mario Sunshine requires custom BMD model assets (cape on Mario's back, CapeBox pickup). This document analyzes what exists, what's missing, and the work needed to build a Blender-to-BMD pipeline.

---

## 1. Current State

### What Exists

**Research & Documentation:**
- `docs/superpowers/INF1_JNT1_EVP1_DRW1_RESEARCH.md` — Binary format specs for skeleton/skinning sections, export algorithms from blemd, coordinate space mapping
- SuperBMD source code analyzed — full understanding of how a working DAE→BMD converter builds all sections
- blemd (Python BMD library) source analyzed — partial Blender export code with batch splitting, weight deduplication, scene graph construction

**Working Tools (External):**
- **SuperBMD** (C#/.NET) — Full DAE/FBX→BMD converter. Works but requires Assimp-compatible input (Collada DAE preferred). Can roundtrip BMD→DAE→BMD.
- **j3dview** / **j3d-model-viewer** — BMD viewers for visual verification
- **blemd** (Python) — Partial Blender→BMD exporter. Has armature parsing, weight extraction, batch splitting. Missing: all binary section serializers.

**Cape Mod Scaffold:**
- `mods/cape-powerup/` — BSE module project with CMakeLists.txt, cape_data.hxx, minimal main.cpp
- Design spec and implementation plan complete
- No model assets created yet

### What's Needed

Two BMD models:
1. **Cape model** — Skinned mesh attached to Mario's back bone. Needs to deform with a simple bone chain (2-4 bones for cloth sim feel). Requires: JNT1, EVP1, DRW1, INF1, SHP1, VTX1, MAT3, TEX1.
2. **CapeBox model** — Static object (prototype reuses NozzleBox). Eventually a custom box with cape icon. Single bone, no skinning. Simpler: rigid mesh, one material.

---

## 2. Per-Section Gap Analysis

### INF1 — Scene Graph

**Current knowledge:** Complete binary format spec. SuperBMD builds it with two-pass algorithm (root-level multi-bone meshes, then bone-nested single-bone meshes). blemd has `extractEntries()` for tree→flat array conversion.

**Gap:** No Blender-native export code. blemd has the tree construction (`BuildScenegraph`) but not the binary serializer.

**Proposed approach:** Use SuperBMD as-is. Export from Blender as DAE (Blender's built-in Collada exporter), feed to SuperBMD. INF1 is generated automatically from the DAE scene graph.

**Effort:** None if using SuperBMD pipeline. If writing custom: ~2 days (tree traversal + binary writer).

---

### JNT1 — Joint/Bone Data

**Current knowledge:** Complete. 64-byte entry format documented. Rotation encoding (s16, radians * 32768/pi). SuperBMD does depth-first flatten, stores local transforms.

**Gap:** No direct Blender→JNT1 writer. Blender's Collada exporter handles bone hierarchy export, but SuperBMD expects a `skeleton_root` parent node.

**Proposed approach:** In Blender, ensure armature root is named or parented under a node named `skeleton_root`. Export as DAE. SuperBMD picks it up. For the cape (2-4 bones), this is trivial.

**Risk:** Blender Collada exporter may not name the skeleton root correctly. Workaround: post-process the DAE XML to insert `skeleton_root` wrapper, or use SuperBMD's fallback (single "root" bone if no skeleton_root found — works for the CapeBox static model).

**Effort:** None if using SuperBMD. Manual fixup of DAE if skeleton naming is wrong: ~1 hour.

---

### EVP1 — Envelope/Skinning Weights

**Current knowledge:** Complete. Weight count per entry, bone indices (u16), weight values (f32), inverse bind matrices (3x4 float). SuperBMD reads IBMs from Assimp's bone.OffsetMatrix (transposed). blemd documents deduplication algorithm.

**Gap:** SuperBMD's EVP1 export path has a quirk — when constructing from a scene (not reading from BMD), the `Weights` list stays empty. The weight data flows through DRW1→SHP1 instead. This works for the roundtrip but means EVP1.Write() emits an empty section (32 bytes) when there are no multi-bone weights.

**Impact on cape model:** The cape model will have skinned vertices (multiple bones influencing cloth vertices). Blender vertex groups → Collada skin weights → Assimp bones → SuperBMD EVP1/DRW1. This should work as long as Blender's Collada exporter correctly writes `<skin>` controllers with vertex weights.

**Risk:** Blender's Collada exporter has known issues with vertex weight export (sometimes drops weights, normalizes incorrectly). Mitigation: verify exported DAE manually, use "Better Collada Exporter" Blender addon if needed.

**Effort:** None if weights export correctly. Debugging weight issues: ~2-4 hours.

---

### DRW1 — Draw Matrix Table

**Current knowledge:** Complete. Bool array (rigid vs weighted) + index array (bone index for rigid, EVP1 index for weighted). SuperBMD classifies by vertex weight count, deduplicates, orders rigid-first.

**Gap:** Same as EVP1 — relies on SuperBMD's processing. The weighted indices start as 0 and get filled during SHP1 generation. This internal dependency is handled by SuperBMD's pipeline.

**Proposed approach:** No custom work needed. SuperBMD handles this end-to-end.

**Effort:** None.

---

### SHP1 — Shape/Batch Data

**Current knowledge:** Good. SuperBMD groups vertices into packets by weight set, generates triangle strips. blemd has batch splitting (single-bone batches per bone+material, multi-bone batches).

**Gap:** No custom work needed for basic models. SuperBMD handles vertex descriptor setup, packet matrix indices, primitive encoding, and tri-strip generation.

**Risk:** Large meshes may generate suboptimal tri-strips. For the cape model (low poly, ~100-500 triangles), this is not a concern.

**Effort:** None.

---

### VTX1 — Vertex Data

**Current knowledge:** Good. Positions, normals, UVs, vertex colors. SuperBMD reads from Assimp mesh data. Coordinate space conversion handled by Assimp's import post-processing.

**Gap:** Coordinate space. Blender uses Z-up, BMD uses Y-up. Blender's Collada exporter applies the axis conversion. SuperBMD + Assimp handle the rest.

**Risk:** Double axis conversion (Blender applies one, Assimp applies another) can produce inverted models. Mitigation: test with a simple cube first, verify orientation.

**Effort:** None if axis conversion works. Debugging orientation: ~1-2 hours.

---

### MAT3 — Materials

**Current knowledge:** Complete. SuperBMD has two paths: JSON preset (preferred) or auto-generate from scene. The JSON path loads a `materials.json` with full TEV pipeline configuration. The auto-generate path creates minimal single-stage TEV (texture + optional vertex color).

**Gap:** This is the **biggest gap**. BMD materials are GameCube TEV (Texture Environment) pipeline configurations — not standard PBR/Blinn-Phong materials. A material needs:
- TEV stage configuration (color/alpha combine modes)
- Texture coordinate generation
- Channel controls (lighting)
- Alpha compare, blend mode, z-mode
- Color constants

**Proposed approach (two options):**

**Option A — JSON material presets (recommended for cape):**
1. Find an existing SMS BMD with similar material properties (semi-transparent, possibly double-sided for cloth)
2. Export that BMD with SuperBMD to get its `materials.json`
3. Edit the JSON to match cape requirements (transparency for fade effect, backface culling off)
4. Feed the JSON to SuperBMD when building the cape BMD

**Option B — Auto-generated materials (for CapeBox prototype):**
- SuperBMD's `LoadFromScene` creates a basic textured material automatically
- Sufficient for the CapeBox (opaque, single texture, standard lighting)

**Effort:** Option A: ~2-4 hours (find reference material, extract, modify). Option B: ~0 (automatic).

---

### TEX1 — Textures

**Current knowledge:** Complete. SuperBMD supports JSON texture headers (format, wrap modes) + image files, or auto-detect from scene. Supports all GC formats (CMPR, RGBA32, etc.).

**Gap:** Need actual texture images for the cape and CapeBox. Can use PNG/TGA source images.

**Proposed approach:**
- Create textures in any image editor (Photoshop, GIMP, Krita)
- For cape: simple cloth texture, 128x128 or 256x256, CMPR format (compressed, good for opaque textures) or RGB5A3 (if alpha needed for fade)
- For CapeBox: reuse NozzleBox texture initially, custom later
- Use SuperBMD's `tex_headers.json` to specify GC format, wrap mode, filter mode

**Risk:** Wrong texture format can cause rendering artifacts on GC hardware (but looks fine in Dolphin). CMPR has known block compression artifacts on gradients.

**Effort:** Texture creation: ~2-4 hours. Format setup: ~30 minutes with tex_headers.json.

---

### MDL3 — Material Display Lists (BDL only)

**Not needed.** The cape mod uses BMD format, not BDL. MDL3 is only for BDL files. SuperBMD skips it for BMD output.

---

## 3. End-to-End Pipeline

### Recommended Workflow

```
Blender (model + rig + UVs + weights)
    │
    ▼
Blender Collada Export (.dae)
    │  - Ensure skeleton_root naming
    │  - Verify vertex weights in DAE
    ▼
SuperBMD (CLI)
    │  superbmd input.dae --mat materials.json --texheader tex_headers.json
    │  - Generates all BMD sections automatically
    │  - Uses materials.json for TEV configuration
    │  - Uses tex_headers.json for texture format control
    ▼
Output .bmd file
    │
    ▼
Place in extracted ISO → Test in Dolphin
```

### Pipeline Dependencies

```
Texture images (PNG) ──┐
                       ▼
tex_headers.json ──► SuperBMD ──► output.bmd
materials.json ────►    │
Blender .dae ──────►    │
                        ▼
                   Verify in Dolphin
```

### What Must Be Set Up Once

1. **SuperBMD binary** — Build from source (.NET) or use prebuilt release
2. **Reference materials.json** — Extract from an existing SMS BMD with similar properties
3. **Blender export settings** — Document the correct Collada export options (axis, scale, armature settings)
4. **tex_headers.json template** — One-time setup for texture format preferences

---

## 4. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Blender Collada exporter drops vertex weights | Medium | High | Use "Better Collada Exporter" addon; verify DAE manually |
| Skeleton naming mismatch (`skeleton_root`) | Medium | Low | Post-process DAE or rename in Blender |
| Double axis conversion (inverted model) | Medium | Medium | Test with simple cube first |
| TEV material setup wrong (visual artifacts) | High | Medium | Use extracted JSON from reference BMD |
| SuperBMD crashes on edge cases | Low | Medium | Test incrementally; SuperBMD is mature |
| Texture format wrong for alpha fade | Low | Low | Use RGB5A3 instead of CMPR for alpha-needed textures |

---

## 5. Effort Estimates

| Task | Effort | Prerequisites |
|------|--------|---------------|
| Set up SuperBMD (build/install) | 1-2 hours | .NET SDK |
| Create cape model in Blender (low-poly cloth, 2-4 bone rig) | 4-8 hours | Blender skills |
| Create cape texture | 2-4 hours | Image editor |
| Export DAE from Blender, fix any naming issues | 1-2 hours | Blender Collada addon |
| Extract reference materials.json from existing SMS BMD | 1-2 hours | SuperBMD working |
| Run SuperBMD to produce cape.bmd | 30 min | All above |
| Create CapeBox model (simple static mesh) | 2-3 hours | Blender |
| Run SuperBMD to produce capebox.bmd | 30 min | Model + texture |
| Integration: load BMD in mod, attach to Mario joint | 4-8 hours | BSE J3DModel API |
| Debug and iterate (visual issues, orientation, scale) | 4-8 hours | Dolphin testing |
| **Total** | **~20-40 hours** | |

---

## 6. Implementation Order

### Phase 1: Pipeline Validation (Day 1-2)
1. **Set up SuperBMD** — build from source or download release
2. **Roundtrip test** — take an existing SMS BMD (e.g., NozzleBox), export to DAE with SuperBMD, re-import to BMD, verify binary similarity
3. **Blender test** — create a simple textured cube in Blender, export as DAE, convert to BMD with SuperBMD, load in Dolphin
4. **Document working export settings** — Blender Collada options that produce correct results

### Phase 2: CapeBox Model (Day 3)
5. **Model the CapeBox** in Blender — simple box shape, single material, no skeleton
6. **Create texture** — cape icon on box face
7. **Export and convert** — DAE → SuperBMD → BMD
8. **Test in-game** — replace NozzleBox reference with custom BMD

### Phase 3: Cape Model (Day 4-6)
9. **Model the cape mesh** in Blender — low-poly cloth shape attached to back
10. **Rig with bone chain** — 2-4 bones for basic cloth deformation
11. **Paint vertex weights** — smooth falloff along cape length
12. **Extract reference material** — find SMS model with similar semi-transparent/cloth material, extract materials.json
13. **Export and convert** — DAE → SuperBMD → BMD
14. **Test standalone** — verify model loads, weights work, material renders

### Phase 4: Integration (Day 7-8)
15. **Load cape BMD in mod** — use J3DModelLoader or BSE model loading API
16. **Attach to Mario skeleton** — find back/spine joint index, compute attachment transform
17. **Test in-game** — cape visible on Mario's back during glide
18. **Debug** — fix orientation, scale, clipping, deformation issues

### Phase 5: Polish (Day 9-10)
19. **Cape animation** — BCK for idle flap, glide billow (separate pipeline, BCK export)
20. **Alpha fade** — modify material at runtime for timer fade effect
21. **Final tuning** — visual quality pass

---

## 7. Key Decisions

### Use SuperBMD (not custom pipeline)
SuperBMD is a mature, tested BMD converter that handles all 8+ sections. Writing a custom Blender→BMD exporter would require implementing: vertex buffer encoding, triangle strip generation, TEV material serialization, texture format conversion, scene graph construction, skeleton encoding, weight deduplication, packet splitting — each non-trivial. SuperBMD already does all of this.

The tradeoff: an extra export step (Blender→DAE→BMD instead of Blender→BMD directly). This is acceptable for a mod with 2 models.

### Materials via JSON presets
BMD materials are GameCube TEV configurations, not mappable from standard Blender materials. Extracting a `materials.json` from a reference BMD and modifying it is far more reliable than trying to auto-generate TEV settings. SMS uses specific TEV configurations for visual consistency.

### Start with static CapeBox
The CapeBox is simpler (no skinning, one material) and validates the entire pipeline before tackling the skinned cape model. If the pipeline breaks, debug with the simpler model first.

---

## 8. Open Questions

1. **Which SMS BMD has the closest material to the cape?** — Need semi-transparent, possibly double-sided. Candidates: Mario's shirt, any cloth/flag objects, shine sprite glow.
2. **BCK animation export** — SuperBMD does not export BCK. Need a separate tool (j3d-animation-editor, or bck-tools). This is deferred but needed for polish.
3. **Joint attachment at runtime** — BSE/J3D API for attaching external model to a specific joint of another model. Need to research `J3DModel::getJointMtx()` or similar.
4. **Maximum vertex weight count** — GC hardware supports up to 10 matrix entries per packet. For a simple cape, 2-3 weights per vertex is fine.
