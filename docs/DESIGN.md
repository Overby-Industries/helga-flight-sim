# Design notes

## Vision

A small, focused **flight simulator**, not a rocket-part sandbox: one
real vehicle -- Overby Industries' Starlifter II ("Project Helga") --
flown by the player as a test pilot through its full envelope, with
true-to-life flight dynamics as the entire point. Think DCS-level
fidelity (real lift/drag/AoA, stall, spin, no arcade shortcuts) applied
to a hypersonic SSTO instead of a jet fighter.

Canon for the vehicle itself lives on the public site, not in this
repo -- treat these as source material:
- https://overbyindustries.space/aerospace (program overview)
- https://overbyindustries.space/aerospace/propulsion (hybrid ABEP /
  MHD-Lorentz / ionic-liquid afterburner / solar-wind propulsion)
- https://overbyindustries.space/aerospace/operating-systems
  (triple-redundant flight computer, sensor fusion, 1kHz control loop)
- https://overbyindustries.space/aerospace/flight-controls (fly-by-wire,
  dynamic control-surface mixing across the speed envelope)
- https://overbyindustries.space/aerospace/airframe (blended wing body,
  double-delta, high-AoA reentry posture, ionic-liquid thermal
  management)

## What we cut from the original pitch

- **No assembly/VAB.** The original pitch reused Aevoria Simulator's
  `PartDefinition`/`PartAssembler`/socket system to let the player
  build Helga. That's gone. In this game, a handful of Starlifter II
  prototypes already exist in-universe -- the player is a test pilot
  assigned to fly a finished airframe, not an engineer assembling one.
- **No mining economy.** The original "ferry asteroid miners to the
  belt" mission arc is out of scope. Missions are short **prototype
  flight-test sorties**, not logistics runs.
- **No multi-vehicle sandbox.** Still one vehicle, one flight model.

## Core loop

A single sortie, playable start to finish in one sitting:

1. **Preflight** -- systems checklist against Helga's actual avionics
   (confirm FC-A/FC-B/FC-C health, sensor fusion lock, propulsion
   controller status) before requesting taxi.
2. **Ground ops & comms** -- taxi clearance from ground, runway
   handoff to tower, departure clearance. Real phraseology, real
   readbacks.
3. **Takeoff & climb** -- conventional runway departure, becomes a
   climb-out under departure control.
4. **Cruise -> ascent** -- subsonic cruise through transonic/supersonic
   into the hypersonic upper-atmosphere transition, propulsion
   handoff from ABEP-dominant to MHD-Lorentz-boost-dominant per the
   real propulsion phase table.
5. **Orbital insertion & brief LEO ops** -- level off, insertion burn,
   a short on-orbit segment. This is a prototype test program, not a
   mission arc -- LEO ops are brief and may include simple data-
   collection objectives (station-keeping, a sensor pass) rather than
   cargo/mining tasks.
6. **Reentry & aerobraking** -- the centerpiece (see below).
7. **Approach & landing** -- comms-cleared approach, possibly through
   dynamic weather, to a conventional runway landing.
8. **Debrief** -- a flight-test data summary, not a score screen (see
   Data Collection below).

## Flight computer FSM

Do **not** reuse Aevoria Simulator's CUR compliance FSM (table-driven
transitions over compliance/governance axes) -- that pattern is fully
bespoke to legal/compliance logic and doesn't fit a flight computer.

Instead, Helga's flight computer follows the **integer-based
extensible FSM architecture** (see the separate FSM Concept & Reference
Guide and FSM Extensibility Guide -- the actual C++ implementation is
being built independently as a C++/FSM learning exercise, not authored
in this doc):

- A small, closed, compiler-checked **core enum** holds the certified
  baseline flight phases -- the states every sortie is guaranteed to
  pass through in order. For Helga that's phase-of-flight, not a
  generic rocket profile:
  `PREFLIGHT, TAXI, TAKEOFF, CLIMB, CRUISE, ASCENT, ORBIT, REENTRY,
  APPROACH, LANDING, FAULT`
  (`ASCENT` covers the upper-atmosphere transition through orbital
  insertion burn; there's no discrete "MECO" the way a staged rocket
  has one, since Helga's hybrid propulsion tapers/hands off rather
  than cutting off).
- The FSM's state variable is a plain `int`, not the enum type itself.
  New states discovered during development -- contingencies the core
  enum was never meant to enumerate -- get added as `constexpr int`
  values numbered past the enum's range, **without editing or
  recompiling the core enum**. Real GA/test-flight contingencies are
  natural fits here: `ABORT_TAKEOFF`, `GO_AROUND`, `HOLDING_PATTERN`,
  `DIVERT`, `SHALLOW_REENTRY_CONTINGENCY`, `POST_LANDING_SYSTEMS_CHECK`.
- `transitionTo()`, `onStateEntry()`, `onStateExit()` all take `int`
  so core and extended states share one pipeline; a `default:` case in
  the main dispatch always falls back to `FAULT` on an unrecognized
  state id.
- Every legal transition (from-state, condition, to-state) gets
  documented in a transition table as it's added, and every extended
  state needs at least one path to `FAULT`.
- Trade-off, explicitly accepted: the core enum keeps compiler
  switch-exhaustiveness checking for the certified baseline; the
  extended states trade that safety for the ability to add
  operationally-discovered contingencies without a recompile-and-
  reverify pass on the whole state machine.

This state machine is what departure/approach comms, mission scoring,
and the aerobraking sequence all key off of -- e.g. a tower clearance
only "counts" once the FSM is in `TAXI` with preflight complete; the
aerobraking minigame only arms once the FSM enters `REENTRY`.

## ATC & comms

The player is a private pilot in real life -- comms should read as
authentic to someone who already knows real phraseology, not a
simplified guessing game:

- **Ground**: taxi clearance, routing, hold-short instructions.
- **Tower**: runway assignment, takeoff clearance, landing clearance.
- **Departure**: climb-out handoff, vectors/altitude assignments up
  through the upper-atmosphere transition.
- **Approach**: handoff back from orbital/reentry ops into the
  terminal area, sequencing, approach clearance.
- Real clearance/readback structure, but a **small, curated phrase
  set** -- this is not a full ATC simulator, just enough exchanges to
  make taxi-out, departure, and arrival feel like real flying instead
  of a menu.

## Flight dynamics fidelity target

- Full 6DOF aerodynamics: lift, drag, angle of attack, stall, spin,
  all modeled for real -- comparable in spirit to DCS's flight
  modeling, not an arcade space-launch toy.
- Helga's actual aerodynamic quirks are gameplay, not flavor: the
  double-delta/cranked-leading-edge planform's high-AoA control
  authority, passive dihedral stability, and the fly-by-wire system's
  continuous blending of aerodynamic and thrust-vectored control
  across the speed envelope (per the Flight Controls page) should all
  be felt by the pilot, not hidden behind a flight-assist layer.
- **Reentry & aerobraking is the hardest, most impactful skill in the
  game.** Helga reenters belly-first at a controlled high AoA, using
  its lower surface as an active (ionic-liquid-managed) heat shield
  rather than a disposable ablative one. The player flies the entry
  corridor: too shallow and the vehicle skips out and has to try
  again; too steep and thermal loads exceed the shield's management
  capacity. This should be tuned as a genuine, replayable piloting
  challenge, not a scripted cutscene.

## Graphics & environment

- **Procedural surface generation**: port/adapt Aevoria Simulator's
  `ProceduralArtGenerator` (CPU-side, `FastNoiseLite` cellular-noise
  field posterized into a dark/base/highlight palette, baked once to
  an `ImageTexture`) for Helga's skin/material generation, so ship
  visuals stay consistent with the flagship title and any purchased
  skin "recipes" reproduce identically here. This is a texturing tool,
  not a renderer -- it doesn't by itself deliver the "top-tier
  graphics" bar.
- **Dynamic weather and top-tier visuals are separate, additional
  work**: volumetric clouds/atmosphere for the terminal-area weather
  the approach/landing comms should react to, and a proper re-entry
  plasma/heating visual treatment for the aerobraking sequence. Plan
  on Godot's Forward+ renderer, volumetric fog/sky, and custom shaders
  for these -- not something the ported procedural-texture tool
  covers.

## Data collection (flight-test framing)

Instead of a mining/resource economy, each sortie logs real
flight-test-style telemetry -- max dynamic pressure, peak heat flux
during reentry, g-load, corridor deviation during aerobraking, sink
rate at touchdown -- and surfaces it as a post-flight debrief. This is
narrative/feedback dressing (a prototype program collecting real
engineering data), not a gameplay economy, and should stay lightweight
enough that it doesn't turn into a second project.

## Scope discipline

Small and shippable, on purpose:
- One vehicle, one flight model, one mission shape (preflight -> taxi
  -> depart -> ascend -> brief LEO ops -> reenter/aerobrake -> approach
  -> land -> debrief).
- Comms are a curated phrase set, not a full ATC sim.
- LEO ops and data collection are brief flavor, not a mission-design
  surface of their own.
- Resist scope creep back toward assembly, economy, or multi-vehicle
  play until this core loop is built and fun.
