Graffito
========

A private, byte-identical decompilation of Super Mario Sunshine (`GMSJ01`, JPN Rev 0), descended from [doldecomp/sms](https://github.com/doldecomp/sms) and developed as a standalone project.

The goal is the same as any matching decomp: produce C/C++ source that compiles to the exact original object code under the original Metrowerks CodeWarrior compiler. The name comes from `graffito` (Italian, sing. of *graffiti*) — what Mario spends the game cleaning, and what this project spends its time scrubbing off.

This repository does **not** contain any game assets or assembly whatsoever. An existing copy of the game is required.

Supported versions:

- `GMSJ01`: Rev 0 (JPN)
- ~~`GMSP01`: Rev 0 (PAL)~~ partially broken — fixes welcome

Dependencies
============

Windows
-------

Native tooling is **highly recommended**. WSL and msys2 are **not** required, and under WSL [objdiff](#diffing) cannot get filesystem notifications for automatic rebuilds.

- Install [Python](https://www.python.org/downloads/) and add it to `%PATH%`.
  - Also available from the [Windows Store](https://apps.microsoft.com/store/detail/python-311/9NRWMJP3717K).
- Download [ninja](https://github.com/ninja-build/ninja/releases) and add it to `%PATH%`.
  - Quick install via pip: `pip install ninja`

macOS
-----

- Install [ninja](https://github.com/ninja-build/ninja/wiki/Pre-built-Ninja-packages):

  ```sh
  brew install ninja
  ```

- Install [wine-crossover](https://github.com/Gcenx/homebrew-wine):

  ```sh
  brew install --cask --no-quarantine gcenx/wine/wine-crossover
  ```

After OS upgrades, if macOS complains about `Wine Crossover.app` being unverified, unquarantine it:

```sh
sudo xattr -rd com.apple.quarantine '/Applications/Wine Crossover.app'
```

Linux
-----

- Install [ninja](https://github.com/ninja-build/ninja/wiki/Pre-built-Ninja-packages).
- For non-x86(_64) platforms: install wine from your package manager.
  - For x86(_64), [wibo](https://github.com/decompals/wibo) (a minimal 32-bit Windows binary wrapper) is downloaded automatically.

Building
========

- Clone the repository:

  ```sh
  git clone https://github.com/ryanbevins/graffito.git
  ```

- Copy your game's disc image to `orig/GMSJ01` (or the appropriate version folder).
  - Supported formats: ISO (GCM), RVZ, WIA, WBFS, CISO, NFS, GCZ, TGC
  - After the initial build the disc image can be deleted to save space.

- Configure:

  ```sh
  python configure.py
  ```

  Use `--version` to build a non-`GMSJ01` revision.

- Build:

  ```sh
  ninja
  ```

Diffing
=======

After the first successful build an `objdiff.json` will exist at the project root.

Grab the latest release of [encounter/objdiff](https://github.com/encounter/objdiff), point its **Project directory** at this repo, and the configuration loads automatically. Select an object from the sidebar to start diffing — source/header changes, `configure.py`, `splits.txt`, and `symbols.txt` all trigger automatic rebuilds.

![](assets/objdiff.png)

Workflow tools
==============

Project-specific helpers live under `tools/claude/`:

| Tool | Purpose |
|------|---------|
| `check_match.py <path>` | Per-function match percentages for a TU |
| `compare_asm.py <path> [symbol]` | Diff original vs compiled assembly |
| `find_easy_targets.py` | Surface small non-matching files |
| `get_symbols.py <path>` | List symbols for a source file |

See [`CLAUDE.md`](./CLAUDE.md) for the full matching playbook — MWCC quirks, common near-match patterns, the TParams framework, and the list of known-unsolvable patterns to skip.

Lineage
=======

Graffito branched off from [doldecomp/sms](https://github.com/doldecomp/sms) and is no longer part of its fork network. Upstream commits can still be cherry-picked via the `upstream` remote:

```sh
git remote add upstream https://github.com/doldecomp/sms.git
git fetch upstream
```

Thanks to the doldecomp community for the foundations.
