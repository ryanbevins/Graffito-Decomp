# BMD Skeleton, Scene Graph, and Skinning Format Research

## Overview
This document details how BMD (Binary Model) files encode skeletal rigging, scene graphs, and skinning in INF1 (scene graph), JNT1 (joint data), EVP1 (envelope/skinning weights), and DRW1 (draw matrices) sections.

---

## 1. INF1 - Scene Graph (Hierarchy Structure)

### Header (24 bytes)
```
Offset  Size  Field
0x00    4     "INF1" tag
0x04    4     Section size (relative to start)
0x08    2     unknown1
0x0A    2     pad (0xffff)
0x0C    4     packetCount (unused in export)
0x10    4     vertexCount (metadata from VTX1)
0x14    4     offsetToEntries (relative to INF1 start)
```

### Entry Array (4 bytes per entry)
Starts at offset `offsetToEntries`. Entries are terminated with type=0x00.

```
struct Inf1Entry {
    u16 type;    // Node type
    u16 index;   // Index into Joint/Material/Shape table (0 for hierarchy ops)
};
```

### Node Types
- **0x10** = Joint (references JNT1 table by index)
- **0x11** = Material (references MAT3 table by index)
- **0x12** = Shape/Batch (references SHP1 table by index)
- **0x01** = Hierarchy Down (begin child scope, no index)
- **0x02** = Hierarchy Up (end child scope, no index)
- **0x00** = Terminator (end of scene graph)

### Scene Graph Construction
The entry array encodes a tree structure:
```
Joint(0)
├─ 0x01 (down)
│  ├─ Joint(1)
│  ├─ 0x01 (down)
│  │  └─ Material(0)
│  │     ├─ 0x01 (down)
│  │     │  └─ Shape(0)
│  │     │     └─ 0x02 (up)
│  │     └─ 0x02 (up)
│  └─ 0x02 (up)
└─ 0x02 (up)
```

**Export Pattern:**
1. Depth-first traversal of bone hierarchy
2. Emit Joint node (0x10, bone_index)
3. For each child bone: emit 0x01 (down), recurse, then 0x02 (up)
4. After all bones, insert Material and Shape nodes as children
5. Terminate with 0x00

---

## 2. JNT1 - Joint Data (Bone Poses)

### Header (24 bytes)
```
Offset  Size  Field
0x00    4     "JNT1" tag
0x04    4     Section size
0x08    2     count (number of joints)
0x0A    2     pad (0xffff)
0x0C    4     jntEntryOffset (relative to JNT1 start)
0x10    4     unknownOffset (maps joint index → string table index; always 0,1,2,...)
0x14    4     stringTableOffset (joint name strings)
```

### Joint Entry (0x40 = 64 bytes per entry)
```
struct JntEntry {
    u16 unknown;        // 0x00 (matrix type? always 0, 1, or 2)
    u16 pad;            // 0x00ff in Mario, 0x0000 in Zelda
    f32 sx, sy, sz;     // Scale (3D vector)
    s16 rx, ry, rz;     // Rotation as packed shorts (0-0xFFFF = 0-2π)
    u16 pad2;           // 0xffff
    f32 tx, ty, tz;     // Translation (3D vector)
    f32 unknown2;       // Purpose unclear
    f32 bbMin[3];       // Bounding box minimum
    f32 bbMax[3];       // Bounding box maximum
};
```

### Rotation Encoding
- Each short is in range [0x0000, 0xFFFF]
- Maps to radians: `radians = short * (π / 32768)`
- Conversion: `short = round(radians * 32768 / π)`

### Name Table
Starts at `stringTableOffset`. Standard Blender/BMD string table format (null-terminated strings with length prefix).

**Export Steps:**
1. Build array of JntFrame objects from Blender armature bones
2. For each bone, decompose local transform to (scale, rotation, translation)
3. Encode rotation shorts using the formula above
4. Write array in JntEntry format
5. Write unknownOffset array (0, 1, 2, ..., count-1)
6. Write stringTableOffset with bone names

---

## 3. EVP1 - Envelope/Skinning Weights (Vertex Bone Associations)

### Header (28 bytes)
```
Offset  Size  Field
0x00    4     "EVP1" tag
0x04    4     Section size
0x08    2     count (number of envelope entries = mesh vertex count in practice)
0x0A    2     pad (0xffff)
0x0C    4     offsets[0] (counts array)
0x10    4     offsets[1] (indices array)
0x14    4     offsets[2] (weights array)
0x18    4     offsets[3] (matrices array - inverse bind poses)
```

### Data Layout

**Table 0: Counts Array** (byte per entry)
- For each vertex, stores count of bones influencing it
- Length: `count` bytes
- Offset relative to EVP1 start: `offsets[0]`

**Table 1: Indices Array** (u16 per weight)
- Concatenated list of bone indices for all vertices
- Total entries: sum of all counts
- Each u16 references a matrix in the matrices table
- Offset: `offsets[1]`

**Table 2: Weights Array** (f32 per weight)
- Bone weights corresponding to indices
- Same order and count as Table 1
- Sum of weights per vertex should equal 1.0
- Offset: `offsets[2]`

**Table 3: Matrices Array** (3×4 matrix per entry)
- Inverse bind pose matrices (3×4 floats = 12 floats = 48 bytes each)
- Maps bone indices to world-space inverse transforms
- Offset: `offsets[3]`

### Multi-Bone Structure
```cpp
class MultiMatrix {
    vector<u16> indices;      // Bone indices (length from counts[i])
    vector<f32> weights;      // Weights (same length)
};
// For vertex i: MultiMatrix mm = weightedIndices[i];
// Vertex influenced by mm.indices[j] with weight mm.weights[j]
```

### Export Algorithm

1. **Build vertex→bone mapping from Blender weights:**
   ```python
   for each vertex:
       mm = MultiMatrix()
       for each vertex group with weight > 0.0001:
           mm.indices.append(bone_group_index)
           mm.weights.append(weight)
   ```

2. **Deduplicate MultiMatrix entries** (BModel_out.py: lines 115-123)
   - Use `is_near()` to find duplicate weight patterns
   - Store unique entries in `unique_MMs`
   - Map vertices to unique MM indices

3. **Build EVP1 tables:**
   - For each unique MM:
     - Append count byte (# bones)
     - Append bone indices (u16 each)
     - Append weights (f32 each)
   - For each matrix: append 3×4 inverse bind matrix

4. **Inverse Bind Matrices:**
   - Computed as: `inv_bind = (parent_matrix * bone_matrix).inverse()`
   - Or directly: `inv_bind = bone_world_matrix.inverse()`
   - Matrix is 3×4 (dropped last row of 4×4 homogeneous matrix)

---

## 4. DRW1 - Draw Matrices (Batch Skinning References)

### Header (20 bytes)
```
Offset  Size  Field
0x00    4     "DRW1" tag
0x04    4     Section size
0x08    2     count (number of draw matrices)
0x0A    2     pad (0xff)
0x0C    4     offsetToIsWeighted (array of u8 flags)
0x10    4     offsetToData (array of u16 indices)
```

### Data Layout

**isWeighted Array** (u8 per entry)
- For each draw matrix, 0 = rigid (single bone), 1 = skinned (multi-bone)
- Offset: `offsetToIsWeighted`

**Data Array** (u16 per entry)
- For rigid (isWeighted[i] == 0): index into joint matrix table (JNT1)
- For skinned (isWeighted[i] == 1): index into EVP1 envelope table
- Offset: `offsetToData`

### Export Algorithm
1. **Determine draw matrix type per batch:**
   - Check all vertices in batch
   - If all have 1 bone weight: rigid (0), data = joint index
   - If any have >1 bone: skinned (1), data = EVP1 envelope index
2. **Write isWeighted and data arrays** (paired 1-to-1)

---

## 5. Blender → BMD Mapping

### Blender Structures → BMD Structures

| Blender | BMD | Purpose |
|---------|-----|---------|
| Armature (bones) | JNT1 | Joint hierarchy and transforms |
| Bone parent-child | INF1 0x10/0x01/0x02 | Scene graph hierarchy |
| Vertex Groups | EVP1 indices/weights | Skinning association |
| Vertex Group Weights | EVP1 weights | Bone influence per vertex |
| Bone Pose (edit mode) | JNT1 JntEntry | Scale, rotation, translation |
| Inverse Bind Pose | EVP1 matrices[i] | World-space inverse transforms |

### Coordinate Space Transforms
Blender and BMD use different coordinate conventions:
- Blender: +Y up, +X right, -Z forward (RHS)
- BMD: +Y up, +X right, +Z forward (LHS)

**Conversion (from BModel_out.py line 34-37):**
```python
# Swap Y↔Z and negate Z
position.xyz = position.xzy
position.z *= -1
```

### Bone Hierarchy Export (BModel_out.py lines 49-53)
```python
# Build scene graph from armature
rootSG = Inf1.SceneGraph()
rootSG.type = 0x10
BuildScenegraph(rootSG, rootBone)
# Result: self._bones array reordered by tree traversal
```

---

## 6. Current BModel_out.py Status

### What's Implemented
✅ **Armature parsing** (lines 8-47)
- Extract bone hierarchy from Blender armature
- Convert coordinate spaces (Blender → BMD)
- Handle multiple root bones (insert synthetic `__root__`)

✅ **Vertex weight extraction** (lines 100-123)
- Build MultiMatrix per vertex from vertex groups
- Deduplicate weight patterns
- Map loops to unique MultiMatrix indices

✅ **Mesh data assembly** (lines 65-97)
- Vertex positions (coordinate conversion)
- Loop data (normals, UVs, vertex colors)
- Face data with material indices

✅ **Batch splitting logic** (lines 138-204)
- Split single-boned faces per bone+material
- Group multi-boned faces
- Build `singleboneBatches` and `multiboneBatches` dicts

✅ **Batch insertion** (lines 206-293)
- Insert Material (0x11) and Shape (0x12) nodes into scene graph
- Optimize material node tree

✅ **Bone analysis** (lines 356-381)
- Decompose bone matrices to local transform
- Populate JNT1 frames with scale, rotation, translation

### What's Missing
❌ **INF1 export** — Scene graph serialization (entries array)
❌ **JNT1 export** — Joint entry encoding, string table, rotation shorts
❌ **EVP1 export** — Envelope data structure, inverse bind matrices, table layout
❌ **DRW1 export** — Draw matrix type and index tables
❌ **VTX1 export** — Vertex buffer (positions, normals, UVs, colors)
❌ **SHP1 export** — Shape/batch vertex list indexing
❌ **MAT3 export** — Material definitions
❌ **File header** — BMD header, section ordering, padding
❌ **DumpModel integration** — Orchestrate all exports to file

---

## 7. Export Order and Dependencies

Typical BMD section order:
1. **INF1** — Scene graph (depends on nothing; generates byte sequence)
2. **VTX1** — Vertex data (depends on geometry)
3. **SHP1** — Batch vertex indices (depends on VTX1 vertex count + batch split logic)
4. **JNT1** — Joint data (depends on armature + AnalyseBones output)
5. **EVP1** — Envelope weights (depends on unique_MMs + inverse matrices)
6. **DRW1** — Draw matrices (depends on batch type + EVP1/JNT1 indices)
7. **MAT3** — Materials (depends on material list)
8. **TEX1** — Textures (depends on material references)
9. **MDL3** — Model metadata (generated last)

---

## 8. Key Implementation Notes

### Scene Graph Deduplication (INF1)
The `extractEntries()` function (Inf1.py line 101) does the inverse of `buildSceneGraph()`:
```python
def extractEntries(self, sg, dest):
    # Emit node type and index
    e = Inf1Entry()
    e.type = sg.type
    e.index = sg.index
    dest.append(e)

    # Emit hierarchy markers for children
    for s2 in sg.children:
        dest.append(Inf1Entry(0x01, 0))  # down
        extractEntries(s2, dest)
        dest.append(Inf1Entry(0x02, 0))  # up

    # Terminator at root level (type == 0)
```

### Matrix Deduplication (EVP1 / DRW1)
- `unique_MMs` array stores deduplicated weight patterns
- Each DRW1 entry references a unique EVP1 envelope
- Critical for memory efficiency (many vertices share same weight pattern)

### Coordinate Space Handling
All coordinates must be transformed before writing:
- Vertex positions: x, z, -y
- Normals: x, z, -y
- Bone positions/endpoints: same transform
- Z-axis negation critical for left-handed → right-handed conversion

### Rotation Encoding Precision
- Input: Euler angles in radians (from Blender)
- Output: s16 in range [-32768, 32767]
- Precision: ~0.000095 radians (0.0055°)
- Always round: `s16 = round(radian * 32768 / π)`

---

## 9. Next Steps for Export Implementation

1. **INF1 export:**
   - Call `extractEntries(rootSG, entries)` from `DumpModel()`
   - Serialize header + entries array + padding

2. **JNT1 export:**
   - Convert JntFrame rotation to s16 shorts
   - Write JntEntry structs
   - Serialize string table with bone names

3. **EVP1 export:**
   - From `unique_MMs`, flatten to 4 tables
   - Compute inverse bind matrices (bone world matrix inverse)
   - Serialize all 4 table offsets and data

4. **DRW1 export:**
   - From batch split results, determine rigid vs skinned
   - Build isWeighted and data arrays
   - Serialize paired arrays

5. **Integration:**
   - Populate VTX1, SHP1, MAT3, TEX1, MDL3 stubs
   - Write BMD file header (0x20 bytes)
   - Write all sections with correct offsets and padding
   - Verify against reference BMD file

---

## 10. Reference Files

- **blemd Inf1.py**: 174 lines, defines header + entry format + tree building
- **blemd Jnt1.py**: 259 lines, joint entry format + name table
- **blemd Evp1.py**: 153 lines, envelope structure (4 tables)
- **blemd Drw1.py**: 92 lines, draw matrix type + index
- **blemd BModel_out.py**: 382 lines, partial export (bones + batches + weights)
- **SMS decomp J3DNode.hpp**: Scene graph node base class
- **SMS decomp J3DJoint.hpp**: Matrix calculation for skinning

