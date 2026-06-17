# mario/MarioUtil/PacketUtil

Verdict: equivalent
Status: equivalent
Time: 2026-06-15 7:39pm MNL

## Reason

Certified under the AUDIT behavioral-equivalence rule.

- `python tools/decomp-diff.py -u mario/MarioUtil/PacketUtil -s missing`
  reports no missing target symbols.
- The only extra symbol is the weak/local `J3DPEBlock::getFog()` helper emitted
  from the header path; source-linking does not require a target reference.
- The nonmatching text rows are codegen-class:
  - `SMS_InitPacket_Fog` differs by helper ownership around `getFog()`, but it
    stores the same fog pointer into the same packet user-area fields and
    installs the same callback.
  - `FifoSetFog` differs in stack/register layout and bitfield construction
    order, but writes the same GX fog BP register values.
  - `ShapePacketCallBackFunc` uses the same dispatch values, user-area offsets,
    FIFO writes, fog calls, and timing-1 fog-off behavior. Target leaves
    `r3 = 1` on return, but `J3DCallBackPacket::draw()` is byte-matched and
    ignores callback return values.
- Source-link proof passed after temporary promotion:
  `python configure.py --non-matching && ninja`.
- Normal matching build passed afterward:
  `python configure.py && ninja` (`build/GMSJ01/mario.dol: OK`).

Keep the TU in `Equivalent`; remaining work is byte-polish / source-shape only.
