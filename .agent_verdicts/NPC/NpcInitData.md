# NPC/NpcInitData audit

Verdict: needs_impl

Checked 2026-06-13 1:31am MNL during AUDIT sweep.

Unit: `mario/NPC/NpcInitData`

Blocking evidence:
- `SMSGetNpcInitData(unsigned long)` is a stub in source:
  `(void)idx; return (const TNpcInitInfo*)0;`.
- Target contains `__sinit_NpcInitData_cpp` and a large set of NPC init,
  model, color-change, string, rodata, data, and sdata2 symbols. Objdiff
  reports them all missing.

This TU needs real table reconstruction before AUDIT can review equivalence.
