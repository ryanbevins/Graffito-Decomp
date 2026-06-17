# Audit verdict: equivalent

Verdict: equivalent
Time: 2026-06-13 10:28am MNL

Date: 2026-06-13 12:39am MNL

`mario/Map/MapEventDolpic` is functionally equivalent.

Reviewed functions:

- 2026-06-13 10:28am MNL recheck: current full/no-collapse diffs preserve the
  same operations. The visible residue is stack/temp construction, rodata/SDA
  labels, branch layout, and operand ordering for equivalent clamp/comparison
  expressions.

- `TDolpicEventRiccoMammaGate::load(JSUMemoryInputStream&)`: same stream field
  reads, name-based gate flag/level selection, collision object creation,
  completed-flag branch, joint scale/translation setup, model calc, particle
  resource loads, warp/demo positions, and `unk30` state.
- `TDolpicEventRiccoMammaGate::loadAfter()`: same timer thresholds, scale
  speed calculation, warp collision setup, event disable, and pollution
  counter-layer clear.
- `TDolpicEventRiccoMammaGate::watch()`: same flag gate, joint show/scale,
  collision setup, demo camera selection, particle emits, pollution layer
  marks, and Mario warp. The low score is unused matrix temporary stack shape
  and local `TFlagT<u16>`/string label ownership.
- `TDolpicEventRiccoMammaGate::control()`: same scale/translation update,
  rumble/sound windows, countdown, final joint/collision/demo/music state.
- `TDolpicEventBiancoGate::loadAfter()`: same dptKing lookup, kill call, and
  initial Y offset.
- `TDolpicEventBiancoGate::control()`: same Y raise, camera shake, rumble,
  pollution clean, initial-position clamp, collision setup, event completion,
  and return value. Residue is frame/operand-order shape.

Proof:

- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.
- 2026-06-13 10:28am MNL recheck reused the current proof batch:
  `python configure.py --non-matching && ninja`, then normal
  `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
