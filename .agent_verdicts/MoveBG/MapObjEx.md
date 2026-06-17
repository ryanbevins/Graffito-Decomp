# MoveBG/MapObjEx

Verdict: equivalent
Date: 2026-06-12 2:41pm MNL

Reason: all target text functions are present. The remaining text diffs are
codegen-class:

- `TJointCoin::makeObj`: same coin-vs-map-object creation, object array store,
  actor-type flag edits, appear/hit/live flag updates, joint-index store,
  count increment, and returned object pointer. Residue is callee-saved register
  coloring and source temporary order.
- `TJointCoin::control`: same rail actor update/calc, base matrix copy,
  attached-object loop, optional matrix copy, position update with Y offset, and
  base control call. Residue is frame size/slot placement only.
- `TMapObjNail::receiveMessage`: same hip-drop/life/count guards, down-height
  move, collision reset, sound call, life timer, nail count, optional item
  spawn, and throw parameters. Residue is frame size/slot placement and label
  ownership for static constants.

Data note: `.data` / `.sdata` extras are source-owned weak/destructor and
infectious-string helper labels; no runtime static used by these functions is
missing.

Proof: `python configure.py --non-matching && ninja` linked from source, then
plain `python configure.py && ninja` passed and verified `mario.dol: OK`.

2026-06-13 8:13am MNL recheck:
- Overview still has no missing target functions. Nonmatching text remains
  limited to `TJointCoin::makeObj`, `TJointCoin::control`, and
  `TMapObjNail::receiveMessage`.
- `TJointCoin::makeObj`: same `strcmp("coin")` dispatch, same
  `newAndRegisterCoinReal` / `newAndRegisterObj` calls, same array stores,
  actor-type flag edits, appear/hit/live flag updates, joint-index store,
  count increment, and returned object. Diff is saved-register coloring and
  local-label ownership only.
- `TJointCoin::control`: same MActor update/calc calls, matrix copy,
  attached-object loop, optional child-matrix copy, position update with
  `mYOffset`, and base `TMapObjBase::control()` call. Diff is stack frame/slot
  size only.
- `TMapObjNail::receiveMessage`: same hip-drop/life/count gates, down-height
  move, collision reset, sound gate/start call, life timer, nail count, optional
  item spawn, and `throwObjToFront` parameters. Diff is stack frame/slot size
  and anonymous constant-label ownership only.
- Proof refreshed: `python configure.py --non-matching && ninja` linked from
  source; normal `python configure.py && ninja` restored matching config and
  verified `build/GMSJ01/mario.dol: OK`.

2026-06-13 11:43am MNL recheck: verdict remains `equivalent`. Current overview
is unchanged: no missing target rows, all target ctor/rodata rows present, and
the same `TJointCoin::makeObj`, `TJointCoin::control`, and
`TMapObjNail::receiveMessage` text residue. The earlier full-diff review still
applies; remaining drift is register/stack/local-label and source-owned weak or
infectious-string owner placement. Shared proof from this tick passed:
`python configure.py --non-matching && ninja`, then normal `python configure.py
&& ninja` verified `build/GMSJ01/mario.dol: OK`.
