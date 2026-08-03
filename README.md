# Project Helga (working title)

[![Version](https://img.shields.io/badge/Version-1.0.2--Alpha-blue?style=for-the-badge&logo=github)](https://github.com/Overby-Industries/helga-flight-sim/releases)
[![Play on itch.io](https://img.shields.io/badge/Play_on-itch.io-fa5c5c?style=for-the-badge&logo=itchdotio)](https://aevoria-simulator.itch.io/project-helga-ssto-44-starlifter-ii-flight-simulator)

A small, focused **flight simulator** built around Overby Industries'
Starlifter II ("Helga") -- a real-aerodynamics SSTO. This is not a
rocket-part sandbox: there's no assembly. A handful of Starlifter II
prototypes already exist in-universe, and the player is a test pilot
flying a finished airframe through its full envelope -- preflight,
ATC-cleared taxi and departure, ascent to LEO, aerobraking reentry,
and a comms-cleared landing. Real flight dynamics -- lift, drag, angle
of attack, stall, spin -- are the entire point, closer in spirit to
DCS-grade flight modeling than to an arcade space-launch toy.

The vehicle and its systems (hybrid ABEP/MHD-Lorentz/ionic-liquid
propulsion, triple-redundant flight computer, fly-by-wire controls,
blended-wing-body airframe) are canon on the public site --
https://overbyindustries.space/aerospace and its Propulsion,
Operating Systems, Flight Controls, and Airframe subpages -- and
should be treated as source material for this sim, not reinvented.

This is one of two small titles meant to help establish the studio
(the other is [Asteroid Miner](../asteroid-miner)) -- scoped
intentionally small so it can actually ship, rather than growing into
another Aevoria-sized project.

## Concept

- **Fly, don't build.** No assembly/part-sandbox -- Helga is already a
  complete prototype. The loop is one sortie: preflight checklist,
  ATC comms for taxi/departure/approach, ascent to LEO, brief on-orbit
  ops, reentry and aerobraking, landing, and a flight-test debrief.
- **Real flight aerodynamics** through the full envelope -- lift,
  drag, angle of attack, stall, spin, all modeled for real.
- **Real ATC comms** for ground, tower, departure, and approach, with
  authentic phraseology -- a small curated phrase set, not a full ATC
  sim.
- **Reentry & aerobraking is the centerpiece**: a pilot-flown, high-AoA
  belly-first entry corridor using Helga's active thermal management,
  tuned as a genuine piloting skill rather than a cutscene.
- **Flight computer = C++ FSM**, using an integer-based extensible
  state architecture (closed core enum for certified baseline flight
  phases, `constexpr int` extended states for contingencies like
  aborts/go-arounds/holds) rather than a generic rocket-profile or
  compliance-style state machine. See `docs/DESIGN.md` for the full
  phase model.
- **Prototype flight-test framing** instead of a mining economy --
  short sorties that log flight-test-style telemetry (max Q, peak
  heat flux, g-load, touchdown sink rate) for a post-flight debrief.

Full design rationale, including what was cut from the original pitch
(assembly, the mining mission arc) and why, lives in `docs/DESIGN.md`.

## Tech stack

- **Godot 4**, GDScript for UI/mission flow.
- **C++ GDExtension** (this repo's `src/`) for the flight model
  (aerodynamics, physics integration) and the flight-computer FSM --
  the performance- and correctness-critical pieces, same split as
  Aevoria Simulator.
- Procedural surface/skin generation adapted from Aevoria Simulator's
  `ProceduralArtGenerator`, for visual consistency across both titles.
- Godot project lives under `godot/`, C++ source under `src/`.

## Status

Scaffolding in place: `godot-cpp` wired up as a submodule, GDExtension
builds cleanly, no flight model, FSM, or comms yet.

## Getting started

1. `git submodule update --init` to pull in `godot-cpp` (already
   configured as a submodule on branch `4.5` in `.gitmodules`).
2. `scons` to build the GDExtension into `godot/bin/`.
3. Open `godot/project.godot` in Godot 4.
