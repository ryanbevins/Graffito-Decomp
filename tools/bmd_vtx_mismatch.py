#!/usr/bin/env python3
"""Investigate the 151 unmatched export vertices in batch 9."""

import struct
import math
from collections import defaultdict

# Reuse the parser from deep diff
import sys
sys.path.insert(0, r"C:\Users\ryana\Documents\sms\tools")
from bmd_deep_diff import BMDParser, build_bone_world_matrices, compute_drw_matrix, mat4_transform_point, dist3

def main():
    orig_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
    export_path = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

    orig = BMDParser(orig_path)
    exp = BMDParser(export_path)

    orig_positions = orig.parse_vtx1_positions()
    exp_positions = exp.parse_vtx1_positions()

    orig_joints = orig.parse_jnt1()
    exp_joints = exp.parse_jnt1()
    orig_parent = orig.parse_inf1()
    exp_parent = exp.parse_inf1()
    orig_evp, orig_inv = orig.parse_evp1()
    exp_evp, exp_inv = exp.parse_evp1()
    orig_dw, orig_dd = orig.parse_drw1()
    exp_dw, exp_dd = exp.parse_drw1()
    orig_world = build_bone_world_matrices(orig_joints, orig_parent)
    exp_world = build_bone_world_matrices(exp_joints, exp_parent)

    orig_batches = orig.parse_shp1()
    exp_batches = exp.parse_shp1()

    # Get ALL unique pos indices used in batch 9 for each file
    batch_idx = 9

    def get_batch_pos_indices(batches, bi):
        batch = batches[bi]
        pos_indices = set()
        for pkt in batch['packets']:
            for vert in pkt['vertices']:
                pos_indices.add(vert.get('posIndex', 0))
        return pos_indices

    orig_pos_indices = get_batch_pos_indices(orig_batches, batch_idx)
    exp_pos_indices = get_batch_pos_indices(exp_batches, batch_idx)

    print(f"Batch 9 unique pos indices: orig={len(orig_pos_indices)}, exp={len(exp_pos_indices)}")
    print(f"Total VTX1 positions: orig={len(orig_positions)}, exp={len(exp_positions)}")

    # Build position lookup tables
    orig_pos_lookup = {}
    for pi in orig_pos_indices:
        if pi < len(orig_positions):
            p = orig_positions[pi]
            key = (round(p[0], 2), round(p[1], 2), round(p[2], 2))
            orig_pos_lookup[key] = (pi, p)

    exp_pos_lookup = {}
    for pi in exp_pos_indices:
        if pi < len(exp_positions):
            p = exp_positions[pi]
            key = (round(p[0], 2), round(p[1], 2), round(p[2], 2))
            exp_pos_lookup[key] = (pi, p)

    # Find positions in export that don't match any original
    unmatched_exp_keys = set(exp_pos_lookup.keys()) - set(orig_pos_lookup.keys())
    unmatched_orig_keys = set(orig_pos_lookup.keys()) - set(exp_pos_lookup.keys())

    print(f"\nPositions in export but not in original: {len(unmatched_exp_keys)}")
    print(f"Positions in original but not in export: {len(unmatched_orig_keys)}")

    # For each unmatched export position, find the closest original position
    print(f"\n--- Unmatched export positions (closest original) ---")
    orig_all_pos = [(orig_positions[pi], pi) for pi in orig_pos_indices if pi < len(orig_positions)]

    unmatched_details = []
    for key in sorted(unmatched_exp_keys):
        exp_pi, exp_p = exp_pos_lookup[key]
        # Find closest original position
        min_dist = float('inf')
        closest_orig = None
        for orig_p, orig_pi in orig_all_pos:
            d = dist3(exp_p, orig_p)
            if d < min_dist:
                min_dist = d
                closest_orig = (orig_pi, orig_p)
        unmatched_details.append((exp_pi, exp_p, min_dist, closest_orig))

    unmatched_details.sort(key=lambda x: -x[2])
    for i, (exp_pi, exp_p, min_dist, closest) in enumerate(unmatched_details[:30]):
        print(f"  exp pos[{exp_pi}] = ({exp_p[0]:.4f}, {exp_p[1]:.4f}, {exp_p[2]:.4f}) "
              f"closest_dist={min_dist:.4f}" +
              (f" orig pos[{closest[0]}] = ({closest[1][0]:.4f}, {closest[1][1]:.4f}, {closest[1][2]:.4f})"
               if closest else ""))

    # Now check: do these unmatched positions produce correct runtime results?
    # For each unmatched exp vertex, compute its runtime position and check
    # if it matches ANY original runtime position.
    print(f"\n--- Runtime position check for unmatched export vertices ---")

    # Gather ALL original runtime positions (for ALL batches 9+10)
    orig_runtime_all = []
    for bi in (9, 10):
        if bi >= len(orig_batches):
            continue
        batch = orig_batches[bi]
        # Need to handle 0xFFFF carry-forward
        prev_mtx = [None] * 20
        for pkt in batch['packets']:
            mtx_table = list(pkt['matrixTable'])
            # Resolve 0xFFFF
            for k in range(len(mtx_table)):
                if mtx_table[k] == 0xFFFF:
                    mtx_table[k] = prev_mtx[k] if k < len(prev_mtx) and prev_mtx[k] is not None else 0
                if k < len(prev_mtx):
                    prev_mtx[k] = mtx_table[k]

            for vert in pkt['vertices']:
                pos_idx = vert.get('posIndex', 0)
                if pos_idx >= len(orig_positions):
                    continue
                vtx_pos = orig_positions[pos_idx]

                if batch['has_mtx_idx']:
                    mtx_idx = vert.get('matrixIndex', 0)
                    local_idx = mtx_idx // 3
                    if local_idx < len(mtx_table):
                        drw_idx = mtx_table[local_idx]
                    else:
                        continue
                else:
                    drw_idx = mtx_table[0] if mtx_table else 0

                if drw_idx >= len(orig_dw):
                    continue

                drw_mat, _, _ = compute_drw_matrix(drw_idx, orig_dw, orig_dd,
                                                    orig_world, orig_evp, orig_inv)
                rp = mat4_transform_point(drw_mat, vtx_pos)
                orig_runtime_all.append(rp)

    print(f"Total original runtime positions gathered: {len(orig_runtime_all)}")

    # Now compute export runtime positions for the unmatched vertices
    exp_batch = exp_batches[batch_idx]
    unmatched_runtime_check = []

    for pkt in exp_batch['packets']:
        for vert in pkt['vertices']:
            pos_idx = vert.get('posIndex', 0)
            if pos_idx >= len(exp_positions):
                continue
            vtx_pos = exp_positions[pos_idx]
            key = (round(vtx_pos[0], 2), round(vtx_pos[1], 2), round(vtx_pos[2], 2))
            if key not in unmatched_exp_keys:
                continue

            mtx_idx = vert.get('matrixIndex', 0)
            local_idx = mtx_idx // 3
            if local_idx < len(pkt['matrixTable']):
                drw_idx = pkt['matrixTable'][local_idx]
            else:
                continue

            if drw_idx >= len(exp_dw):
                continue

            drw_mat, drw_type, drw_data = compute_drw_matrix(drw_idx, exp_dw, exp_dd,
                                                               exp_world, exp_evp, exp_inv)
            rp = mat4_transform_point(drw_mat, vtx_pos)

            # Find closest original runtime position
            min_d = float('inf')
            for orp in orig_runtime_all:
                d = dist3(rp, orp)
                if d < min_d:
                    min_d = d

            unmatched_runtime_check.append((pos_idx, vtx_pos, rp, drw_idx, drw_type, drw_data, min_d))

    # Sort by worst runtime match
    unmatched_runtime_check.sort(key=lambda x: -x[6])

    good_runtime = sum(1 for x in unmatched_runtime_check if x[6] < 1.0)
    bad_runtime = sum(1 for x in unmatched_runtime_check if 1.0 <= x[6] < 10.0)
    explosion_runtime = sum(1 for x in unmatched_runtime_check if x[6] >= 10.0)

    print(f"\nUnmatched export vertices runtime check:")
    print(f"  Total: {len(unmatched_runtime_check)}")
    print(f"  GOOD runtime (dist < 1):     {good_runtime}")
    print(f"  BAD runtime (1-10):          {bad_runtime}")
    print(f"  EXPLOSION runtime (>= 10):   {explosion_runtime}")

    print(f"\n--- Top 20 worst unmatched runtime positions ---")
    for i, (pi, vp, rp, drw_idx, drw_type, drw_data, min_d) in enumerate(unmatched_runtime_check[:20]):
        print(f"  #{i+1}: min_runtime_dist={min_d:.2f}")
        print(f"    vtx1_pos=({vp[0]:.4f}, {vp[1]:.4f}, {vp[2]:.4f})")
        print(f"    runtime=({rp[0]:.2f}, {rp[1]:.2f}, {rp[2]:.2f})")
        print(f"    DRW[{drw_idx}] {drw_type} data={drw_data}")


if __name__ == '__main__':
    main()
