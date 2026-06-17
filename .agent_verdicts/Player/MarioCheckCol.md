verdict: needs_impl
date: 2026-06-13 2:29am MNL
unit: mario/Player/MarioCheckCol

Reason:
- Fails the strict audit symbol gate. Objdiff reports missing target data
  symbol `@3352`.
- `TMario::checkCollision()` remains a large 76.5% routine, so it needs further
  implementation/investigation before certification.
