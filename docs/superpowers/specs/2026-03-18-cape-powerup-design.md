# Cape Powerup for Super Mario Sunshine

## Overview

A cape powerup for SMS implemented as a BetterSunshineEngine (BSE) Kuribo module. The cape replaces FLUDD temporarily and gives Mario SM64 Wing Cap-style dive-and-swoop flight.

## Platform

- **Framework**: BetterSunshineEngine + Kuribo mod loader
- **Compiler**: Bundled Clang (PowerPC cross-compiler)
- **Output**: `.kxe` module loaded at runtime from ISO
- **Reference**: SMS decomp (doldecomp/sms) for understanding game internals

The decomp's decompiled Player/ source (MarioMove.cpp, MarioJump.cpp, WaterGun.cpp, etc.) serves as readable reference for hooking into Mario's state machine and physics.

**NOTE**: BSE hook names below are based on research of the BSE GitHub repo. Exact API signatures must be verified against the actual BetterSunshineEngine SDK before implementation begins. If any hooks are missing or differ, the implementation plan must adapt (e.g., fall back to raw `SMS_PATCH_BL` hooks at specific addresses).

## Cape Box (Pickup)

- Custom object `TCapeBox` inheriting from `TMapObjGeneral`
- Registered via `Objects::registerObjectAsMapObj("CapeBox", ...)`
- Placed alongside existing nozzle boxes in levels using SMS Bin Editor
- Uses a custom BMD model (reskinned nozzle box or new asset)
- Player breaks it by ground pound or attack (same interaction as vanilla nozzle box)
- On break: removes FLUDD, gives cape, starts 60-second timer

## Cape State (Player Behavior)

### Activation
- Cape replaces FLUDD on pickup. FLUDD state is stored and restored when cape expires.
- Press R while airborne to engage glide.

### State Machine Integration
TMario's state machine uses a flat `u32` status code dispatched via switch/case in `changePlayerStatus()`. To add a custom glide state:
- Reserve an unused status code value (e.g., `STATE_CAPE_GLIDE = 0x0000088F`)
- Hook `TMario::changePlayerStatus()` via trampoline — intercept the custom code before the vanilla switch/case
- Alternatively, use BSE's `Player::registerStateMachine()` if available (this wraps the same mechanism)
- From the update callback, call `changePlayerStatus(STATE_CAPE_GLIDE)` when R is pressed while airborne and cape is active

### Flight Model (SM64 Wing Cap Style)
- **Dive (stick down)**: Build speed, lose altitude
- **Pull up (stick up)**: Trade speed for height, lose speed
- **Neutral**: Gentle descent at current speed
- **Stick left/right**: Bank and turn
- Skilled play can maintain or gain altitude through dive-and-swoop cycles

### Controls
| Input | Action |
|-------|--------|
| R (airborne, cape active) | Engage glide |
| Release R | Exit glide, enter normal fall |
| B (while gliding) | Exit glide, enter normal fall (B does NOT trigger dive attack on the exit frame) |
| Control stick | Pitch (up/down) and yaw (left/right) |

### No Combat
- No attacks while gliding. Gliding is purely movement.
- Exiting glide enters a one-frame fall transition, then normal move options resume (ground pound, wall jump, etc.)

### Input Handling
- R is normally the FLUDD trigger, read from `TMarioControllerWork`
- Since cape replaces FLUDD, R input is free — no conflict
- The update callback checks `mController.mButtons & R_BUTTON` and `mState == airborne` to transition into glide
- FLUDD's `emit()` path is skipped while cape is active (FLUDD pointer hidden/nulled or flag checked)

## Timer

- 60 seconds from pickup, always ticking (ground and air)
- Cape model visually fades as time runs out (gradual transparency in final 10 seconds)
- When timer hits zero: cape removed, FLUDD restored automatically
- No way to extend or refresh the timer
- If timer expires mid-glide: glide exits immediately, Mario enters normal fall, FLUDD restores

## FLUDD Store/Restore

Storing FLUDD state on cape pickup:
```
storedNozzle: u8         (mCurrentNozzle)
storedSecondNozzle: u8   (mSecondNozzle)
storedWater: s32         (mCurrentWater)
```

On pickup:
1. Save the three fields above into CapeData
2. Set a flag that disables FLUDD rendering and input processing
3. Do NOT null `mWaterGun` — other systems may dereference it

On restore (timer expiry or loading zone):
1. Write saved values back to TWaterGun fields
2. Clear the FLUDD-disabled flag
3. Remove cape model

This preserves water level and nozzle selection. Pressure/emit state reset naturally since FLUDD was inactive.

## Visual Feedback

- Cape model attached to Mario's back bone joint (need to identify exact joint index from Mario's skeleton — likely the spine/backpack joint that FLUDD uses)
- Rendering: hook into Mario's draw pass (BSE's `Player::addDraw2DCallback()` or a `SMS_PATCH_BL` on `TMario::addCallBack()`) to render cape model using the joint's world matrix
- Cape animates during glide (billowing, flapping speed based on glideSpeed)
- Cape gradually fades via material alpha as timer approaches zero (final 10 seconds)
- On expiration: brief poof particle effect, FLUDD model reappears

## Collision During Glide

| Collision | Behavior |
|-----------|----------|
| Ground | Exit glide, land normally |
| Wall | Exit glide, enter normal fall (no wall slide from glide) |
| Ceiling | Bounce down slightly, continue glide at reduced speed |
| Enemy | Mario takes damage, exits glide, enters damage knockback state. Cape timer continues. |
| Coins/collectibles | Collected normally (pass-through pickup) |
| Water surface | Exit glide, enter water/swim state. Cape timer continues on ground. |
| Goop/hazards | Normal hazard response (damage, slip, etc.) |
| Loading zone | Cape state preserved across loading zone. Timer keeps ticking. |

## Data Architecture

### CapeData (stored in module-managed global, keyed by TMario pointer)
```cpp
struct CapeData {
    bool hasCape;              // cape is active
    f32 timer;                 // counts down from 60.0 (seconds)
    bool isGliding;            // currently in glide state
    f32 glideSpeed;            // scalar speed along flight vector
    f32 glidePitch;            // pitch angle in degrees (-90 to +90)
    f32 glideYaw;              // facing direction
    TVec3f glideVelocity;      // decomposed velocity for physics (derived from speed+pitch)
    u8 storedNozzle;           // saved mCurrentNozzle
    u8 storedSecondNozzle;     // saved mSecondNozzle
    s32 storedWater;           // saved mCurrentWater
    J3DModel* capeModel;       // cape BMD model instance
};
```

`glideSpeed` is the scalar magnitude along the flight vector. Each frame, it decomposes into:
- Horizontal speed: `glideSpeed * cos(glidePitch)`
- Vertical speed: `glideSpeed * sin(glidePitch)`

### BSE Hooks Used
| Hook | Purpose | Fallback if unavailable |
|------|---------|------------------------|
| `Player::registerStateMachine()` | Custom glide state | `SMS_PATCH_BL` on `changePlayerStatus` |
| `Player::addUpdateCallback()` | Tick timer, check glide entry | `SMS_PATCH_BL` on `TMario::perform` |
| `Player::registerData()` | Attach CapeData to TMario | Module-managed global `CapeData` |
| `Player::addInitCallback()` | Init cape data on level load | `Stage::addInitCallback()` |
| `Objects::registerObjectAsMapObj()` | Register CapeBox | `SMS_PATCH_BL` on factory function |
| `Stage::addInitCallback()` | Optional programmatic placement | Direct scene graph insertion |
| Draw hook (TBD) | Render cape model on Mario | `SMS_PATCH_BL` on `TMario::addCallBack` |

### Custom State: STATE_CAPE_GLIDE
Per-frame logic:
1. Read stick Y for pitch input, stick X for yaw input
2. Update pitch: `glidePitch += stickY * pitchRate` (clamped to [-60, +60] degrees)
3. Update yaw: `glideYaw += stickX * turnRate`
4. Update speed based on pitch:
   - Pitch down (negative): `glideSpeed += diveAccel` (capped at maxDiveSpeed)
   - Pitch up (positive): `glideSpeed -= climbDecel` (if speed < stallSpeed, auto-exit glide)
   - Neutral: `glideSpeed -= dragDecel` (gentle slowdown)
5. Decompose velocity: `velX = speed * cos(pitch) * sin(yaw)`, `velY = speed * sin(pitch)`, `velZ = speed * cos(pitch) * cos(yaw)`
6. Apply to Mario position
7. Check ground/wall/ceiling collision → exit glide if hit
8. Check R release or B press → exit glide
9. Update cape animation playback speed based on glideSpeed

## Physics Parameters (Tuning)

| Parameter | Initial Value | Notes |
|-----------|--------------|-------|
| Base glide speed | 20.0 | Speed magnitude when entering glide |
| Max dive speed | 60.0 | Cap on speed during dive |
| Min speed (stall) | 8.0 | Below this, glide auto-exits |
| Drag deceleration | 0.2 | Speed loss per frame at neutral pitch |
| Dive acceleration | 1.5 | Speed gain per frame while pitching down |
| Climb deceleration | 1.2 | Speed loss per frame while pitching up |
| Pitch rate | 2.0 degrees/frame | How fast pitch changes from stick input |
| Turn rate | 4.0 degrees/frame | How fast Mario banks left/right |
| Pitch clamp | [-60, +60] degrees | Prevents full vertical dive/climb |
| Timer duration | 60.0 seconds | Total cape lifetime |
| Fade start | 50.0 seconds elapsed | When cape starts becoming transparent |

All values need in-game tuning. Start conservative and adjust.

## Camera

- During glide: camera pulls back slightly (increase follow distance) and raises angle to show more ground ahead
- Implementation: adjust `TCamera` follow parameters while `isGliding == true`, restore on exit
- If BSE provides camera hooks, use those; otherwise patch camera update function directly
- Camera should smoothly interpolate to glide position and back

## Module Structure

```
src/
  main.cpp          -- Module registration, BSE callbacks, hook setup
  cape_data.hpp     -- CapeData struct, constants, physics params
  cape_state.cpp    -- STATE_CAPE_GLIDE per-frame logic (flight physics)
  cape_box.cpp      -- TCapeBox object (pickup, breaking, FLUDD swap)
  cape_box.hpp      -- TCapeBox class definition
  cape_timer.cpp    -- Timer tick, expiration handling, FLUDD restore
  cape_render.cpp   -- Cape model loading, draw hook, fade effect
```

## Asset Requirements

| Asset | Format | Description |
|-------|--------|-------------|
| Cape box model | BMD | Box with cape icon, placed in /scene/MapObj/ |
| Cape box break animation | BCK | Box breaking open |
| Cape model (on Mario) | BMD | Cape attached to Mario's back joint |
| Cape glide animation | BCK | Cape billowing during flight |
| Cape idle animation | BCK | Cape resting while grounded |

## Edge Cases

| Scenario | Behavior |
|----------|----------|
| Timer expires mid-glide | Exit glide immediately, normal fall, FLUDD restores |
| Pick up second cape box while cape active | Reset timer to 60 seconds |
| Enter loading zone with cape | Cape persists, timer keeps ticking |
| Enter water while gliding | Exit glide, enter swim state, cape timer continues |
| Take damage while gliding | Exit glide, normal damage knockback, cape timer continues |
| Die with cape active | Cape lost on death, FLUDD restores on respawn |
| Enter cutscene/demo with cape | Timer pauses during cutscene, resumes after |

## Implementation Order

1. Set up BetterSunshineModule project, verify build and loading in Dolphin
2. Verify BSE hooks — confirm which APIs exist, identify fallbacks needed
3. CapeData + timer system (no visuals, just data ticking down)
4. FLUDD store/restore (pickup disables FLUDD, timer expiry re-enables)
5. Cape glide state (flight physics, input handling)
6. Cape box object (pickup in world)
7. Cape model rendering (visual on Mario's back)
8. Camera adjustments
9. Polish: fade effect, particles, sound, edge cases
