# System/RenderModeObj audit

Verdict: equivalent  
Date: 2026-06-13 6:45am MNL
Status: reverified current source-link proof

Proof: `python configure.py --non-matching && ninja` links cleanly with
`System/RenderModeObj.cpp` built from source. All target `.text` functions are
present. A follow-up plain `python configure.py && ninja` restored the matching
configuration and verified `build/GMSJ01/mario.dol: OK`.

Reason: Non-100% diffs in `SMSSetupMovieRenderingInfo`,
`SMSSetupGameRenderingInfo`, `SMSSetupTitleRenderingInfo`, and
`SMSSetupGCLogoRenderingInfo` are codegen-class stack-frame/slot offsets plus
anonymous data-label naming/ownership. Calls, stores, constants, and branch
structure match. The extra `SMSSetupGCLogoRenderMode` standalone helper is
target-absent dead source text while its logic appears in the target
`SMSSetupGCLogoRenderingInfo` body.

2026-06-13 10:43am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for all four setup functions. Each still calls `VIGetTvFormat`,
selects the same render-mode data, copies the same sample pattern/vfilter
tables, writes the same render-mode fields, and clears the same display flag
bits. Drift is only stack frame/slot size and anonymous data-label ownership
(`SMSAASamplePattern_non` versus `dummy1210`). Proof refreshed with
`python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
