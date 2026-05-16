#!/usr/bin/env python3
"""Check if any position index is shared between batches with different transforms."""

import sys
sys.path.insert(0, r"C:\Users\ryana\Documents\sms\tools")
from bmd_deep_diff import BMDParser

def main():
    for label, path in [("ORIG", r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"),
                         ("EXP", r"C:\Users\ryana\Downloads\reconstruct_test.bmd")]:
        print(f"\n{'='*60}")
        print(f"[{label}] Position index sharing analysis")
        print(f"{'='*60}")

        bmd = BMDParser(path)
        batches = bmd.parse_shp1()
        drw_w, drw_d = bmd.parse_drw1()

        # For each position index, track which (batch, DRW) combinations use it
        pos_usage = {}  # pos_idx -> set of (batch_idx, drw_idx, drw_type)

        for bi, batch in enumerate(batches):
            is_weighted = batch['has_mtx_idx']

            # Need to handle 0xFFFF carry-forward for original
            prev_mtx = [None] * 20
            for pi, pkt in enumerate(batch['packets']):
                mtx_table = list(pkt['matrixTable'])
                # Resolve 0xFFFF
                for k in range(len(mtx_table)):
                    if mtx_table[k] == 0xFFFF:
                        mtx_table[k] = prev_mtx[k] if k < len(prev_mtx) and prev_mtx[k] is not None else 0
                    if k < len(prev_mtx):
                        prev_mtx[k] = mtx_table[k]

                for vert in pkt['vertices']:
                    pos_idx = vert.get('posIndex', 0)
                    if is_weighted:
                        mtx_idx = vert.get('matrixIndex', 0)
                        local_idx = mtx_idx // 3
                        if local_idx < len(mtx_table):
                            drw_idx = mtx_table[local_idx]
                        else:
                            drw_idx = -1
                    else:
                        drw_idx = mtx_table[0] if mtx_table else 0

                    if drw_idx >= 0 and drw_idx < len(drw_w):
                        drw_type = 'W' if drw_w[drw_idx] else 'R'
                        data_val = drw_d[drw_idx]
                    else:
                        drw_type = '?'
                        data_val = -1

                    if pos_idx not in pos_usage:
                        pos_usage[pos_idx] = set()
                    pos_usage[pos_idx].add((bi, drw_idx, drw_type, data_val))

        # Find positions used by multiple DRW entries
        multi_drw = {pi: usages for pi, usages in pos_usage.items()
                     if len(set((drw_idx, drw_type) for _, drw_idx, drw_type, _ in usages)) > 1}

        print(f"  Total unique pos indices: {len(pos_usage)}")
        print(f"  Positions with multiple DRW types: {len(multi_drw)}")

        if multi_drw:
            print(f"\n  Shared positions (different DRW entries):")
            for pi in sorted(multi_drw.keys())[:20]:
                usages = sorted(multi_drw[pi])
                print(f"    pos[{pi}]: {usages}")


if __name__ == '__main__':
    main()
