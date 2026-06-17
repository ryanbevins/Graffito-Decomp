# Enemy/koopajr Audit

Verdict: needs_impl  
Date: 2026-06-13 4:00am MNL

Not promoted.

Reason:

- `python tools/decomp-diff.py -u mario/Enemy/koopajr` shows missing target
  text/data, including
  `TSpineBase<TLiveActor>::pushNerve(const TNerveBase<TLiveActor>*)`,
  `.ctors` `@2111`, `@2194`-`@2197`, `@2659`,
  `koopajrsubmarine_bastable`, `onetimeFilenames$3171`, and
  `@6026`-`@6029`.
- The rebuilt object emits target-absent nerve accessors, base/helper
  destructors, `TWaterGun::isEmitting()`, `TYoshi::onYoshi()`, and infectious
  data symbols.
- Missing target symbols make this a `needs_impl` verdict; no
  `configure.py` promotion was attempted.
