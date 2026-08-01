# Project Helga (working title)

A small SSTO assembly-and-flight sim built around the Commonwealth's
"Helga" SSTO -- the same ship the player builds in
[Aevoria Simulator](../aevoria-simulator)'s Assembly Bay. This is a
standalone game, not a Kerbal Space Program clone: one specific
vehicle, one real flight computer, real aerodynamics, and a real
mission -- get to orbit, then carry Aevoria Simulator's asteroid miners
out to the belt.

This is one of two small titles meant to help establish the studio
(the other is [Asteroid Miner](../asteroid-miner)) -- scoped
intentionally small so it can actually ship, rather than growing into
another Aevoria-sized project.

## Concept

- Assemble/configure the Helga SSTO (reusing or adapting Aevoria
  Simulator's `PartDefinition`/`PartAssembler`/`AssemblyBlueprint` C++
  classes and socket-based part system rather than inventing a second
  one).
- Fly it with **real flight aerodynamics** through atmosphere to
  orbit -- lift, drag, angle of attack, stall, all modeled, not an
  arcade flight model.
- The SSTO's onboard flight computer is a **C++ finite-state machine**,
  the same architectural pattern Aevoria Simulator already uses for
  its CUR compliance/governance systems (see that project's
  `CURComplianceMonitor`/`cur_fsm_display.gd`) -- proven, testable,
  and consistent across both codebases, just driving flight-phase
  logic (ascent/orbit-insertion/descent) instead of governance state.
- Once in orbit, Helga can carry out its actual in-universe job:
  ferrying deep-space asteroid miners out to the belt -- the same
  mission Aevoria Simulator's mining levels describe Helga performing,
  just now flown directly instead of abstracted into a level-select
  card.

## Tech stack

- **Godot 4**, GDScript for UI/mission flow.
- **C++ GDExtension** (this repo's `src/`) for the flight model
  (aerodynamics, physics integration) and the flight-computer FSM --
  the performance- and correctness-critical pieces, same split as
  Aevoria Simulator.
- Godot project lives under `godot/`, C++ source under `src/`.

## Status

Stub only -- folder structure and build scaffolding in place, no
flight model or FSM yet. Not yet a git repository; init and push when
ready to start real work.

## Getting started (once real work begins)

1. `git init`, then add `godot-cpp` as a submodule (see `SConstruct`
   for the expected layout -- same pattern as Aevoria Simulator's
   `.gitmodules`).
2. `scons` to build the GDExtension into `godot/bin/`.
3. Open `godot/project.godot` in Godot 4.
