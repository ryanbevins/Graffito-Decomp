# Cape Powerup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a cape powerup to Super Mario Sunshine via a BetterSunshineEngine Kuribo module — cape replaces FLUDD, gives SM64-style dive-and-swoop flight for 60 seconds.

**Architecture:** Standalone BSE `.kxe` module. Registers a custom map object (CapeBox pickup), a custom player state (glide), and per-player data (CapeData). Hooks into TMario's update loop and state machine via BSE callbacks. Cape model rendered via patching Mario's draw call.

**Tech Stack:** C++20, BetterSunshineEngine, Kuribo mod loader, Clang PowerPC cross-compiler, CMake + Ninja

**Spec:** `docs/superpowers/specs/2026-03-18-cape-powerup-design.md`

**Reference repo for patterns:** `JoshuaMKW/Super-Mario-Eclipse` (custom objects, custom player states)

---

## Decomp Field Reference

These are the verified field names from the SMS decomp (`include/Player/MarioMain.hpp`). BSE's `SMS/Player/Mario.hxx` may alias some of these differently — always verify against BSE headers when compiling.

| Decomp field | Offset | Type | BSE alias (verify) |
|-------------|--------|------|---------------------|
| `mFaceAngle` | 0x94 | `TVec3<s16>` | `mAngle` |
| `mVel` | 0xA4 | `TVec3<f32>` | `mSpeed` |
| `mForwardVel` | 0xB0 | `f32` | `mForwardSpeed` |
| `unk108` | 0x108 | cast to `TMarioControllerWork*` | `mController` |
| `mState` | 0x118 | `u32` | `mState` |
| `mWaterGun` | 0x3E4 | `TWaterGun*` | `mFludd` |

Controller buttons (from `include/System/MarioGamePad.hpp`):
- R = `0x20`
- A = `0x100`
- B = `0x200`
- L = `0x4000`

Controller access pattern in decomp: `((TMarioControllerWork*)unk108)->mInput & TMarioControllerWork::R`

Fall state code: `0x88C` (verified from `MarioMove.cpp` line 1500)

---

## File Structure

The module lives in a new directory `mods/cape-powerup/` at the root of this repo (sibling to `src/`, keeps mod code separate from decomp).

```
mods/cape-powerup/
├── CMakeLists.txt              -- Build config (based on BetterSunshineModule template)
├── CMakeSettings.json          -- VS2022 settings
├── .gitmodules                 -- BSE submodule
├── lib/
│   └── BetterSunshineEngine/   -- Git submodule (main branch)
├── include/
│   ├── cape_data.hxx           -- CapeData struct, constants, physics params
│   ├── cape_timer.hxx          -- giveCapeTo/removeCape/tickCapeTimer declarations
│   ├── cape_box.hxx            -- TCapeBox class definition
│   └── cape_state.hxx          -- Glide state function declarations
├── src/
│   ├── main.cpp                -- Module entry, all BSE registrations
│   ├── cape_data.cpp           -- CapeData init/cleanup helpers
│   ├── cape_box.cpp            -- TCapeBox object (pickup logic)
│   ├── cape_state.cpp          -- STATE_CAPE_GLIDE per-frame flight physics + collision
│   ├── cape_timer.cpp          -- Timer tick, FLUDD store/restore
│   └── cape_render.cpp         -- Cape model draw hook, fade effect
└── targets/
    └── (copied from BetterSunshineModule template)
```

---

## Prerequisites

Before starting, you need:
- **Visual Studio 2022** (or CMake 3.8+ and Ninja on PATH)
- **Git** (for submodule clone)
- **Dolphin Emulator** (for testing)
- **Extracted SMS ISO** (use `build/tools/dtk.exe disc extract` on the original ISO)
- **BetterSunshineEngine.kxe** and **KuriboKernel.bin** — download from [BSE GitHub Releases](https://github.com/DotKuribo/BetterSunshineEngine/releases) or build BSE from source using its own CMake setup

Testing is manual: build module → copy `.kxe` to extracted ISO → launch in Dolphin. No automated test framework exists for GC mods.

**BSE Header Convention:** BSE re-exports game headers via `<SMS/...>` (e.g., `<SMS/Player/Mario.hxx>` for TMario). BSE's own API is under `<BetterSMS/...>` (e.g., `<BetterSMS/player.hxx>`). These are NOT the same as the decomp's `include/Player/MarioMain.hpp`. BSE field names may differ from decomp names — see the reference table above.

---

## Task 1: Project Scaffold

**Files:**
- Create: `mods/cape-powerup/CMakeLists.txt`
- Create: `mods/cape-powerup/.gitmodules`
- Create: `mods/cape-powerup/src/main.cpp`
- Create: `mods/cape-powerup/include/cape_data.hxx`

- [ ] **Step 1: Create directory structure**

```bash
mkdir -p mods/cape-powerup/{src,include,lib,targets}
```

- [ ] **Step 2: Clone BetterSunshineModule template for reference and add BSE submodule**

```bash
cd mods/cape-powerup
git submodule add https://github.com/DotKuribo/BetterSunshineEngine.git lib/BetterSunshineEngine
```

Copy the `targets/` directory and `CMakeSettings.json` from the [BetterSunshineModule](https://github.com/DotKuribo/BetterSunshineModule) template repo. These contain the Clang cross-compiler toolchain files that BSE requires.

- [ ] **Step 3: Create CMakeLists.txt**

Reference the BetterSunshineModule template's `CMakeLists.txt` for the canonical pattern. Key elements:

```cmake
cmake_minimum_required(VERSION 3.8)
project(CapePowerup)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(lib/BetterSunshineEngine)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.hxx")

add_executable(CapePowerup ${SOURCES} ${HEADERS})
target_include_directories(CapePowerup PRIVATE include)

# SMS_COMPILE_FLAGS and SMS_LINK_FLAGS are defined by BSE's CMake
target_compile_options(CapePowerup PRIVATE ${SMS_COMPILE_FLAGS})
target_link_options(CapePowerup PRIVATE ${SMS_LINK_FLAGS})
```

Note: This is a starting point. The exact CMake may need adjustments — compare against the template's CMakeLists.txt which handles the KuriboConverter post-build step to produce the `.kxe` file.

- [ ] **Step 4: Create cape_data.hxx**

```cpp
// include/cape_data.hxx
#pragma once

#include <Dolphin/types.h>

// Forward declare — BSE header provides full definition
class TMario;

// Custom state ID — must satisfy (state & 0x1C0) == 0x1C0
// This is a BSE requirement: registerStateMachine validates this mask.
// Format matches Eclipse pattern: 0xF000__C0
constexpr u32 STATE_CAPE_GLIDE = 0xF00031C0;

// Fall state (verified from decomp MarioMove.cpp:1500)
constexpr u32 STATE_FALL = 0x88C;

// Per-player data key for BSE's registerData system
constexpr const char *CAPE_DATA_KEY = "__cape_powerup";

// Physics constants — all tunable, start conservative
constexpr f32 CAPE_TIMER_DURATION   = 60.0f;   // seconds
constexpr f32 CAPE_FADE_START       = 50.0f;   // seconds elapsed when fade begins
constexpr f32 CAPE_BASE_GLIDE_SPEED = 20.0f;
constexpr f32 CAPE_MAX_DIVE_SPEED   = 60.0f;
constexpr f32 CAPE_STALL_SPEED      = 8.0f;
constexpr f32 CAPE_DRAG_DECEL       = 0.2f;
constexpr f32 CAPE_DIVE_ACCEL       = 1.5f;
constexpr f32 CAPE_CLIMB_DECEL      = 1.2f;
constexpr f32 CAPE_PITCH_RATE       = 2.0f;    // degrees per frame
constexpr f32 CAPE_TURN_RATE        = 4.0f;    // degrees per frame
constexpr f32 CAPE_PITCH_MIN        = -60.0f;  // max dive angle (degrees)
constexpr f32 CAPE_PITCH_MAX        = 60.0f;   // max climb angle (degrees)

// Button masks (verified from decomp MarioGamePad.hpp)
constexpr u32 BTN_R = 0x20;
constexpr u32 BTN_A = 0x100;
constexpr u32 BTN_B = 0x200;
constexpr u32 BTN_L = 0x4000;

struct CapeData {
    bool hasCape;
    f32 timer;
    bool isGliding;
    f32 glideSpeed;
    f32 glidePitch;         // degrees, negative = diving
    f32 glideYaw;           // degrees, facing direction
    u8 storedNozzle;        // saved TWaterGun::mCurrentNozzle
    u8 storedSecondNozzle;  // saved TWaterGun::mSecondNozzle
    s32 storedWater;        // saved TWaterGun::mCurrentWater
    bool persistAcrossLoad; // true = preserve cape across loading zone
};

CapeData *getCapeData(TMario *player);
void initCapeData(CapeData *data);
```

- [ ] **Step 5: Create minimal main.cpp**

```cpp
// src/main.cpp
#include <BetterSMS/module.hxx>

static BetterSMS::ModuleInfo sModuleInfo("Cape Powerup", 1, 0, nullptr);

static void initModule() {
    BetterSMS::registerModule(sModuleInfo);
}

KURIBO_MODULE_BEGIN("Cape Powerup", "SMS Decomp", "1.0") {
    KURIBO_EXECUTE_ON_LOAD { initModule(); }
    KURIBO_EXECUTE_ON_UNLOAD { }
}
KURIBO_MODULE_END()
```

- [ ] **Step 6: Build the module**

```bash
cd mods/cape-powerup
cmake -G Ninja -B build
ninja -C build
```

Verify: `.kxe` file is produced. If the KuriboConverter step is missing, check BSE's CMake for the post-build command that converts ELF to KXE.

- [ ] **Step 7: Test loading in Dolphin**

```bash
mkdir -p <extracted_iso>/files/Kuribo!/System
mkdir -p <extracted_iso>/files/Kuribo!/Mods
cp <path_to>/KuriboKernel.bin <extracted_iso>/files/Kuribo!/System/
cp <path_to>/BetterSunshineEngine.kxe <extracted_iso>/files/Kuribo!/Mods/
cp build/CapePowerup.kxe <extracted_iso>/files/Kuribo!/Mods/_CapePowerup.kxe
```

The `_` prefix ensures CapePowerup loads after BetterSunshineEngine. Launch in Dolphin — game should boot normally with no crash.

- [ ] **Step 8: Commit**

```bash
git add mods/cape-powerup/
git commit -m "feat(cape): scaffold BSE module project with empty module"
```

---

## Task 2: CapeData + Timer + FLUDD Store/Restore

**Files:**
- Create: `mods/cape-powerup/src/cape_data.cpp`
- Create: `mods/cape-powerup/include/cape_timer.hxx`
- Create: `mods/cape-powerup/src/cape_timer.cpp`
- Modify: `mods/cape-powerup/src/main.cpp`

- [ ] **Step 1: Implement cape_data.cpp**

```cpp
// src/cape_data.cpp
#include "cape_data.hxx"
#include <BetterSMS/player.hxx>
#include <SMS/Player/Mario.hxx>

CapeData *getCapeData(TMario *player) {
    return static_cast<CapeData *>(
        Player::getRegisteredData(player, CAPE_DATA_KEY)
    );
}

void initCapeData(CapeData *data) {
    data->hasCape = false;
    data->timer = 0.0f;
    data->isGliding = false;
    data->glideSpeed = 0.0f;
    data->glidePitch = 0.0f;
    data->glideYaw = 0.0f;
    data->storedNozzle = 0;
    data->storedSecondNozzle = 0;
    data->storedWater = 0;
    data->persistAcrossLoad = false;
}
```

- [ ] **Step 2: Create cape_timer.hxx**

```cpp
// include/cape_timer.hxx
#pragma once

class TMario;

void giveCapeTo(TMario *player);
void removeCape(TMario *player);
void tickCapeTimer(TMario *player);
```

- [ ] **Step 3: Implement cape_timer.cpp**

```cpp
// src/cape_timer.cpp
#include "cape_timer.hxx"
#include "cape_data.hxx"
#include <SMS/Player/Mario.hxx>
#include <SMS/Player/Watergun.hxx>

void giveCapeTo(TMario *player) {
    CapeData *cape = getCapeData(player);
    if (!cape)
        return;

    // If already have cape, just reset timer
    if (cape->hasCape) {
        cape->timer = CAPE_TIMER_DURATION;
        return;
    }

    // Store FLUDD state
    // BSE header may name this mFludd; decomp names it mWaterGun.
    // Adjust field name based on what compiles against BSE headers.
    TWaterGun *fludd = player->mFludd;  // BSE alias; decomp: mWaterGun
    if (fludd) {
        cape->storedNozzle = fludd->mCurrentNozzle;
        cape->storedSecondNozzle = fludd->mSecondNozzle;
        cape->storedWater = fludd->mCurrentWater;
    }

    cape->hasCape = true;
    cape->timer = CAPE_TIMER_DURATION;
    cape->isGliding = false;
    cape->glideSpeed = 0.0f;
    cape->glidePitch = 0.0f;
    cape->persistAcrossLoad = true;

    // Disable FLUDD emit
    if (fludd) {
        fludd->mIsEmitWater = false;
    }
}

void removeCape(TMario *player) {
    CapeData *cape = getCapeData(player);
    if (!cape || !cape->hasCape)
        return;

    // Restore FLUDD state
    TWaterGun *fludd = player->mFludd;
    if (fludd) {
        fludd->mCurrentNozzle = cape->storedNozzle;
        fludd->mSecondNozzle = cape->storedSecondNozzle;
        fludd->mCurrentWater = cape->storedWater;
    }

    cape->hasCape = false;
    cape->isGliding = false;
    cape->timer = 0.0f;
    cape->persistAcrossLoad = false;
}

void tickCapeTimer(TMario *player) {
    CapeData *cape = getCapeData(player);
    if (!cape || !cape->hasCape)
        return;

    // SMS runs at 30fps game logic
    cape->timer -= (1.0f / 30.0f);

    // Keep FLUDD emit disabled while cape is active
    TWaterGun *fludd = player->mFludd;
    if (fludd) {
        fludd->mIsEmitWater = false;
    }

    if (cape->timer <= 0.0f) {
        cape->timer = 0.0f;

        // If gliding, exit glide
        if (cape->isGliding) {
            cape->isGliding = false;
            player->changePlayerStatus(STATE_FALL, 0, false);
        }

        removeCape(player);
    }
}
```

- [ ] **Step 4: Wire up callbacks in main.cpp**

```cpp
// src/main.cpp
#include <BetterSMS/module.hxx>
#include <BetterSMS/player.hxx>
#include <SMS/Player/Mario.hxx>
#include "cape_data.hxx"
#include "cape_timer.hxx"

static BetterSMS::ModuleInfo sModuleInfo("Cape Powerup", 1, 0, nullptr);

// Per-player cape storage (single-player game, one global instance)
static CapeData sPlayerCapeData;

BETTER_SMS_FOR_CALLBACK void onPlayerInit(TMario *player, bool isMario) {
    if (!isMario)
        return;

    // Check if cape should persist (loading zone, not death)
    // On death, the game resets Mario fully — persistAcrossLoad would
    // have been cleared by the exit callback. On loading zone transition,
    // it stays true.
    if (sPlayerCapeData.persistAcrossLoad && sPlayerCapeData.hasCape) {
        // Preserve cape state, just re-register the data pointer
        Player::registerData(player, CAPE_DATA_KEY, &sPlayerCapeData);
        return;
    }

    initCapeData(&sPlayerCapeData);
    Player::registerData(player, CAPE_DATA_KEY, &sPlayerCapeData);
}

BETTER_SMS_FOR_CALLBACK void onPlayerUpdate(TMario *player, bool isMario) {
    if (!isMario)
        return;
    tickCapeTimer(player);
}

static void initModule() {
    BetterSMS::registerModule(sModuleInfo);
    Player::addInitCallback(onPlayerInit);
    Player::addUpdateCallback(onPlayerUpdate);
}

KURIBO_MODULE_BEGIN("Cape Powerup", "SMS Decomp", "1.0") {
    KURIBO_EXECUTE_ON_LOAD { initModule(); }
    KURIBO_EXECUTE_ON_UNLOAD { }
}
KURIBO_MODULE_END()
```

- [ ] **Step 5: Handle death via stage exit callback**

Add to main.cpp:

```cpp
#include <BetterSMS/stage.hxx>

BETTER_SMS_FOR_CALLBACK void onStageExit(TApplication *app) {
    // Clear persistence flag on death/stage exit
    // Loading zone transitions also trigger this, but the game
    // re-inits Mario immediately after, so we use a heuristic:
    // TODO: Distinguish death from loading zone if needed.
    // For now, always clear — simplest correct behavior.
    sPlayerCapeData.persistAcrossLoad = false;
}

// Add to initModule():
Stage::addExitCallback(onStageExit);
```

Note: Distinguishing death from loading zone may require checking `TMarDirector` state or Mario's health. Start with always-clearing (cape lost on any transition), then refine if needed.

- [ ] **Step 6: Build and test**

```bash
ninja -C build
cp build/CapePowerup.kxe <extracted_iso>/files/Kuribo!/Mods/_CapePowerup.kxe
```

Game should boot and play normally. No way to activate cape yet — this just validates the data system doesn't crash.

- [ ] **Step 7: Commit**

```bash
git add mods/cape-powerup/src/ mods/cape-powerup/include/
git commit -m "feat(cape): add CapeData, timer, FLUDD store/restore"
```

---

## Task 3: Cape Glide State

**Files:**
- Create: `mods/cape-powerup/include/cape_state.hxx`
- Create: `mods/cape-powerup/src/cape_state.cpp`
- Modify: `mods/cape-powerup/src/main.cpp`

- [ ] **Step 1: Create cape_state.hxx**

```cpp
// include/cape_state.hxx
#pragma once

#include <SMS/Player/Mario.hxx>

bool capeGlideState(TMario *player);
```

- [ ] **Step 2: Implement cape_state.cpp with flight physics and collision**

```cpp
// src/cape_state.cpp
#include "cape_state.hxx"
#include "cape_data.hxx"
#include <SMS/Player/Mario.hxx>
#include <math.h>

static constexpr f32 DEG_TO_RAD = 3.14159265f / 180.0f;
static constexpr f32 RAD_TO_DEG = 180.0f / 3.14159265f;

// Update pitch/yaw from stick input
static void updateFlightAngles(TMario *player, CapeData *cape) {
    // BSE exposes controller via player->mController (verify exact accessor).
    // Decomp pattern: ((TMarioControllerWork*)player->unk108)->mStickV
    // BSE likely provides: player->mController->mStickV or similar.
    // Adjust based on what compiles. Using BSE-style access below.

    // Stick Y: push forward = dive (negative pitch), pull back = climb (positive pitch)
    f32 stickY = player->mController->mStickV;  // -1.0 to 1.0, verify BSE name
    f32 stickX = player->mController->mStickH;

    // Invert Y: pushing stick forward (positive Y) should dive (negative pitch)
    cape->glidePitch -= stickY * CAPE_PITCH_RATE;
    if (cape->glidePitch < CAPE_PITCH_MIN) cape->glidePitch = CAPE_PITCH_MIN;
    if (cape->glidePitch > CAPE_PITCH_MAX) cape->glidePitch = CAPE_PITCH_MAX;

    cape->glideYaw += stickX * CAPE_TURN_RATE;
}

static void updateFlightSpeed(CapeData *cape) {
    if (cape->glidePitch < -5.0f) {
        // Diving — gain speed
        cape->glideSpeed += CAPE_DIVE_ACCEL;
        if (cape->glideSpeed > CAPE_MAX_DIVE_SPEED)
            cape->glideSpeed = CAPE_MAX_DIVE_SPEED;
    } else if (cape->glidePitch > 5.0f) {
        // Climbing — lose speed (but only climb if you have speed)
        cape->glideSpeed -= CAPE_CLIMB_DECEL;
    } else {
        // Neutral — gentle drag
        cape->glideSpeed -= CAPE_DRAG_DECEL;
    }

    // Floor at zero
    if (cape->glideSpeed < 0.0f)
        cape->glideSpeed = 0.0f;
}

static void applyFlightVelocity(TMario *player, CapeData *cape) {
    f32 pitchRad = cape->glidePitch * DEG_TO_RAD;
    f32 yawRad = cape->glideYaw * DEG_TO_RAD;

    f32 horizSpeed = cape->glideSpeed * cosf(pitchRad);
    f32 vertSpeed = cape->glideSpeed * sinf(pitchRad);

    // BSE alias: mSpeed; Decomp: mVel (TVec3<f32> at 0xA4)
    // Adjust field name based on BSE headers
    player->mSpeed.x = horizSpeed * sinf(yawRad);
    player->mSpeed.y = vertSpeed;
    player->mSpeed.z = horizSpeed * cosf(yawRad);

    // Set facing angle to match glide yaw
    // BSE alias: mAngle.y; Decomp: mFaceAngle.y (s16)
    // 360 degrees = 65536 s16 units, so 1 degree = 182.04
    player->mAngle.y = (s16)(cape->glideYaw * 182.04f);
}

// Exit the glide — transition to fall state
static void exitGlide(TMario *player, CapeData *cape) {
    cape->isGliding = false;
    // Transition to fall state (0x88C verified from decomp MarioMove.cpp:1500)
    player->changePlayerStatus(STATE_FALL, 0, false);
}

bool capeGlideState(TMario *player) {
    CapeData *cape = getCapeData(player);
    if (!cape) {
        // No data — force exit to fall
        player->changePlayerStatus(STATE_FALL, 0, false);
        return true;
    }

    // --- Exit conditions ---

    // Cape expired
    if (!cape->hasCape) {
        exitGlide(player, cape);
        return true;
    }

    // R released (check held input, not frame input)
    // BSE: player->mController->mInput; Decomp: ((TMarioControllerWork*)unk108)->mInput
    bool rHeld = (player->mController->mInput & BTN_R);
    if (!rHeld) {
        exitGlide(player, cape);
        return true;
    }

    // B pressed (frame input = just pressed this frame)
    bool bPressed = (player->mController->mFrameInput & BTN_B);
    if (bPressed) {
        exitGlide(player, cape);
        return true;
    }

    // Stall — too slow to maintain flight
    if (cape->glideSpeed < CAPE_STALL_SPEED) {
        exitGlide(player, cape);
        return true;
    }

    // --- Collision checks ---

    // Ground collision: if Mario's Y position is at or below ground height
    // TMario has mFloorBelow (f32) — the Y height of the floor beneath Mario
    // BSE: player->mFloorBelow; Decomp: check TMario fields around 0x80-0x90
    if (player->mPosition.y <= player->mFloorBelow + 10.0f) {
        exitGlide(player, cape);
        return true;
    }

    // Wall collision: check if Mario hit a wall this frame
    // TMario::mWallPlane is non-null when touching a wall
    if (player->mWallPlane != nullptr) {
        exitGlide(player, cape);
        return true;
    }

    // Water surface: if Mario enters water, exit glide
    // TMario has water height tracking — check mWaterHeight vs position
    // Verify exact field name from BSE headers
    if (player->mPosition.y <= player->mWaterHeight) {
        exitGlide(player, cape);
        return true;
    }

    // --- Update flight ---
    updateFlightAngles(player, cape);
    updateFlightSpeed(cape);
    applyFlightVelocity(player, cape);

    // Ceiling bounce: if roof is close, push down and reduce speed
    if (player->mRoofPlane != nullptr) {
        player->mSpeed.y = -2.0f;
        cape->glideSpeed *= 0.8f;
        cape->glidePitch = -10.0f;  // force slight dive
    }

    // Set animation — use Mario's spread-arms falling animation for prototype
    // TODO: Replace with custom cape glide animation
    // player->setAnimation(SOME_FALL_ANIM, 1.0f);

    return false;  // state continues
}
```

Note: Field names like `mFloorBelow`, `mWallPlane`, `mRoofPlane`, `mWaterHeight`, `mPosition`, `mSpeed`, `mAngle` are BSE-style names. If they don't compile, check BSE's `SMS/Player/Mario.hxx` for the exact names. The decomp equivalents are: `mGroundY`/`mFloorY` for floor height, `mWallPlane`/`mRoofPlane` for collision planes. These field names WILL need adjustment when you first try to compile — use compiler errors to find the right BSE names.

- [ ] **Step 3: Add glide entry to update callback in main.cpp**

Expand `onPlayerUpdate`:

```cpp
#include "cape_state.hxx"

BETTER_SMS_FOR_CALLBACK void onPlayerUpdate(TMario *player, bool isMario) {
    if (!isMario)
        return;

    tickCapeTimer(player);

    CapeData *cape = getCapeData(player);
    if (!cape || !cape->hasCape)
        return;

    // Don't enter glide if already gliding
    if (cape->isGliding)
        return;

    // Check: R just pressed AND airborne
    // BSE controller access — adjust if needed
    bool rPressed = (player->mController->mFrameInput & BTN_R);

    // Airborne check: mState contains status flags
    // In SMS, airborne states include jumping, falling, etc.
    // Check if Mario is not on ground — use mState or position check
    bool airborne = !(player->mState & 0x4);  // bit 2 = on ground? Verify from decomp

    if (rPressed && airborne) {
        cape->isGliding = true;
        cape->glideSpeed = CAPE_BASE_GLIDE_SPEED;
        cape->glidePitch = 0.0f;
        // Convert Mario's s16 facing angle to degrees
        cape->glideYaw = (f32)(player->mAngle.y) / 182.04f;
        player->changePlayerStatus(STATE_CAPE_GLIDE, 0, false);
    }
}
```

Note: The airborne check `!(player->mState & 0x4)` is a guess. The correct approach is checking the current player status code — airborne states in SMS include `0x02000880` (basic jump), `0x88C` (fall), and others. A simpler check: `player->mPosition.y > player->mFloorBelow + 50.0f`. Try both and see what feels right.

- [ ] **Step 4: Register state machine in initModule()**

Add to `initModule()`:

```cpp
#include "cape_state.hxx"

// In initModule():
Player::registerStateMachine(STATE_CAPE_GLIDE, capeGlideState);
```

- [ ] **Step 5: Build and test with temporary auto-cape**

For testing, temporarily add `giveCapeTo(player)` in `onPlayerInit` so the cape is always active:

```cpp
// TEMPORARY — remove after testing
#include "cape_timer.hxx"
// In onPlayerInit, after registerData:
giveCapeTo(player);
```

Build, deploy, test in Dolphin:
- Jump → press R → should enter glide
- Stick down → should dive and gain speed
- Stick up → should climb and lose speed
- Release R → should exit to normal fall
- Press B → should exit to normal fall
- Hit ground while gliding → should land
- Hit wall → should exit glide
- After 60 seconds → FLUDD returns
- Fly into water → should exit glide

Remove the temporary `giveCapeTo` after testing.

- [ ] **Step 6: Commit**

```bash
git add mods/cape-powerup/src/cape_state.cpp mods/cape-powerup/include/cape_state.hxx mods/cape-powerup/src/main.cpp
git commit -m "feat(cape): implement glide state with flight physics and collision"
```

---

## Task 4: Cape Box Object

**Files:**
- Create: `mods/cape-powerup/include/cape_box.hxx`
- Create: `mods/cape-powerup/src/cape_box.cpp`
- Modify: `mods/cape-powerup/src/main.cpp`

- [ ] **Step 1: Create cape_box.hxx**

```cpp
// include/cape_box.hxx
#pragma once

#include <SMS/MapObj/MapObjGeneral.hxx>
#include <SMS/MapObj/MapObjInit.hxx>
#include <JSystem/JDrama/JDRNameRef.hxx>

class TCapeBox : public TMapObjGeneral {
public:
    TCapeBox(const char *name);
    ~TCapeBox() override = default;

    void load(JSUMemoryInputStream &stream) override;
    void control() override;
    BOOL receiveMessage(THitActor *sender, u32 message) override;

    static JDrama::TNameRef *instantiate() {
        return new TCapeBox("CapeBox");
    }

private:
    bool mBroken;
};

extern ObjData capeBoxData;
```

- [ ] **Step 2: Implement cape_box.cpp**

```cpp
// src/cape_box.cpp
#include "cape_box.hxx"
#include "cape_data.hxx"
#include "cape_timer.hxx"
#include <SMS/Player/Mario.hxx>
#include <SMS/Strategic/HitActor.hxx>

static hit_data capeBoxHitData = {
    200.0f,  // mAttackRadius
    200.0f,  // mAttackHeight
    100.0f,  // mReceiveRadius
    200.0f,  // mReceiveHeight
};

static obj_hit_info capeBoxHitInfo = {
    1,
    0x80000000,
    0.0f,
    &capeBoxHitData,
};

ObjData capeBoxData = {
    .mMdlName          = "nozzleBox",       // reuse NozzleBox model for prototype
    .mObjectID         = 0x80000500,        // unique — must not collide with other mods
    .mLiveManagerName  = "アイテムマネージャー",  // Item Manager (ShiftJIS)
    .mObjKey           = "アイテムグループ",      // Item Group (ShiftJIS)
    .mAnimInfo         = nullptr,
    .mObjCollisionData = &capeBoxHitInfo,
    .mMapCollisionInfo = nullptr,
    .mSoundInfo        = nullptr,
    .mPhysicalInfo     = nullptr,
    .mSinkData         = nullptr,
    ._28               = nullptr,
    .mBckMoveData      = nullptr,
    ._30               = 50.0f,
    .mUnkFlags         = 0x10004000,
    .mKeyCode          = 0,
};

TCapeBox::TCapeBox(const char *name)
    : TMapObjGeneral(name)
    , mBroken(false)
{
}

void TCapeBox::load(JSUMemoryInputStream &stream) {
    TMapObjGeneral::load(stream);
}

void TCapeBox::control() {
    if (mBroken)
        return;
    TMapObjGeneral::control();
}

BOOL TCapeBox::receiveMessage(THitActor *sender, u32 message) {
    if (mBroken)
        return FALSE;

    // Accept attack messages
    // Verify message IDs from decomp's THitActor/TMapObjGeneral message handling
    // Common: 1 = body contact, 7 = ground pound, 0xE = generic attack
    bool isAttack = (message == 1 || message == 7 || message == 0xE);
    if (!isAttack)
        return TMapObjGeneral::receiveMessage(sender, message);

    // Verify sender is Mario before casting
    // THitActor has mObjectType or similar — check BSE headers.
    // Safest: check if sender pointer matches the global Mario pointer.
    extern TMario *gpMarioAddress;
    if (sender != (THitActor *)gpMarioAddress)
        return FALSE;

    TMario *player = static_cast<TMario *>(sender);

    mBroken = true;
    giveCapeTo(player);

    // Hide this object — mark as dead
    // TMapObjGeneral state flags vary. Try:
    makeObjDead();  // if this virtual exists in BSE headers
    // Fallback: mStateFlags |= some_dead_flag, or just move off-screen

    return TRUE;
}
```

Note: `makeObjDead()`, the exact message IDs, and the ShiftJIS string encoding all need verification against BSE headers. The Japanese strings may need to be raw UTF-8 or ShiftJIS depending on how BSE handles them. Check Eclipse's ObjData examples for the exact encoding.

- [ ] **Step 3: Register CapeBox in initModule()**

Add to main.cpp:

```cpp
#include "cape_box.hxx"

// In initModule():
Objects::registerObjectAsMapObj("CapeBox", &capeBoxData, TCapeBox::instantiate);
```

- [ ] **Step 4: Test with programmatic spawn or SMS Bin Editor**

**Option A (SMS Bin Editor):** Open a stage's `scene.bin`, add a "CapeBox" object, set position near a known location, save, repack the SZS archive.

**Option B (programmatic spawn):** Add a stage init callback that creates a CapeBox. Note: this requires knowing how to properly add the object to the scene graph. Reference Eclipse's `src/object/` files for the pattern. If this proves too complex, use the Bin Editor.

Build, deploy, test:
- CapeBox appears in level (looks like a nozzle box for now)
- Attack/ground pound it → cape activates, FLUDD disabled
- Jump + R → glide works
- After 60s → FLUDD returns

- [ ] **Step 5: Commit**

```bash
git add mods/cape-powerup/src/cape_box.cpp mods/cape-powerup/include/cape_box.hxx mods/cape-powerup/src/main.cpp
git commit -m "feat(cape): add CapeBox pickup object"
```

---

## Task 5: Cape Render Stub + FLUDD Model Hiding

**Files:**
- Create: `mods/cape-powerup/src/cape_render.cpp`
- Modify: `mods/cape-powerup/src/main.cpp`

- [ ] **Step 1: Implement cape_render.cpp**

```cpp
// src/cape_render.cpp
#include "cape_data.hxx"
#include <SMS/Player/Mario.hxx>
#include <SMS/Player/Watergun.hxx>

void updateCapeVisual(TMario *player) {
    CapeData *cape = getCapeData(player);
    if (!cape)
        return;

    if (cape->hasCape) {
        // Hide FLUDD model while cape is active
        // TWaterGun has a model pointer (mFluddModel or similar)
        // Try setting visibility flag or moving model off-screen
        // TODO: Find exact FLUDD model hide mechanism from BSE headers
        // For prototype: FLUDD emit is already disabled, visual may still show.
        // This is acceptable for initial testing.

        // Fade effect: cape transparency in last 10 seconds
        f32 elapsed = CAPE_TIMER_DURATION - cape->timer;
        if (elapsed > CAPE_FADE_START) {
            f32 fadeRatio = cape->timer / (CAPE_TIMER_DURATION - CAPE_FADE_START);
            // fadeRatio goes from 1.0 (solid) to 0.0 (invisible)
            // TODO: Apply to cape model material alpha when model exists
            (void)fadeRatio;
        }
    }
}
```

- [ ] **Step 2: Hook into player update**

Add `updateCapeVisual(player)` call at the end of `onPlayerUpdate` in main.cpp.

- [ ] **Step 3: Build and test**

Verify no crash. FLUDD may still visually appear — that's fine for the prototype. Focus is on gameplay feel.

- [ ] **Step 4: Commit**

```bash
git add mods/cape-powerup/src/cape_render.cpp mods/cape-powerup/src/main.cpp
git commit -m "feat(cape): add render stub with fade timer and FLUDD hide placeholder"
```

---

## Task 6: Integration Testing and Tuning

**Files:**
- Modify: `mods/cape-powerup/include/cape_data.hxx` (tuning constants)
- Modify: `mods/cape-powerup/src/cape_state.cpp` (physics adjustments)

- [ ] **Step 1: Full integration test in Dolphin**

Test checklist:
- [ ] Game boots without crash
- [ ] CapeBox appears in level
- [ ] Breaking CapeBox gives cape, FLUDD stops emitting
- [ ] Jump + R enters glide
- [ ] Stick down → dive, gain speed
- [ ] Stick up → climb, lose speed
- [ ] Release R → exit glide, normal fall
- [ ] Press B → exit glide, normal fall
- [ ] Stall (too slow) → exit glide
- [ ] Hit ground while gliding → land
- [ ] Hit wall while gliding → exit glide
- [ ] Hit ceiling → bounce down, reduced speed
- [ ] Enter water → exit glide
- [ ] After 60 seconds → FLUDD returns
- [ ] Can spray water normally after cape expires
- [ ] Die with cape → respawn with FLUDD, no cape
- [ ] Break second CapeBox → timer resets to 60

- [ ] **Step 2: Tune physics constants**

Adjust values in `cape_data.hxx` based on feel. Iterate: edit → rebuild → copy kxe → test.

Key tuning questions:
- `CAPE_BASE_GLIDE_SPEED 20.0` — too fast/slow on entry?
- `CAPE_DIVE_ACCEL 1.5` / `CAPE_CLIMB_DECEL 1.2` — is the swoop satisfying?
- `CAPE_STALL_SPEED 8.0` — too punishing? Too forgiving?
- `CAPE_PITCH_RATE 2.0` / `CAPE_TURN_RATE 4.0` — responsive enough?
- `CAPE_TIMER_DURATION 60.0` — right amount of time?

- [ ] **Step 3: Commit tuning**

```bash
git add mods/cape-powerup/
git commit -m "feat(cape): tune flight physics from playtesting"
```

---

## Deferred Work (Post-Prototype)

Not part of initial implementation — tracked for future tasks:

1. **Custom cape BMD model** — model in Blender, export with SuperBMD
2. **Cape animation** — BCK for idle/glide/fade
3. **Cape rendering on Mario's back** — J3DModel attachment to skeleton joint
4. **FLUDD model hiding** — properly hide the FLUDD backpack model
5. **Sound effects** — wing flap, pickup jingle, expiration warning
6. **Camera** — pull back and raise angle during glide
7. **HUD timer** — on-screen countdown via `Stage::addDraw2DCallback`
8. **Particle effects** — poof on expiration, speed trail during dive
9. **Level design** — place CapeBoxes in all levels via SMS Bin Editor
10. **Cutscene timer pause** — check `TMarDirector` state to pause timer during demos
11. **Custom CapeBox model** — replace NozzleBox placeholder with cape-themed box
