# Design notes (early)

Captured from the initial pitch -- expand as the design firms up.

- **Not a KSP clone**: one specific, fixed vehicle concept (the Helga
  SSTO), not an open-ended rocket-part sandbox. Assembly reuses/adapts
  Aevoria Simulator's existing part-socket system rather than building
  a generic VAB.
- **Real aerodynamics**: lift/drag/AoA/stall modeled for real, not an
  arcade flight model -- this is the game's core differentiator versus
  a simplified space-launch toy.
- **Flight computer = C++ FSM**: same architectural pattern as Aevoria
  Simulator's CUR compliance monitor (a real finite-state machine
  driving phase logic), applied here to ascent / orbit-insertion /
  descent flight phases instead of governance determinations. Worth
  evaluating whether the FSM *framework* itself (not the CUR-specific
  states) can be factored out and shared between the two projects
  rather than reimplemented.
- **Mission**: get Helga to orbit, then perform its actual in-universe
  job per Aevoria Simulator's fiction -- ferrying deep-space asteroid
  miners out to the belt. That interop (Helga's cargo = Aevoria's
  miner ships) is a nice-to-have connective thread between the two
  games, not a hard dependency -- this should stand alone and be fully
  playable even if a player has never touched Aevoria Simulator.
- **Scope discipline**: this is deliberately a small, shippable title.
  One vehicle, one flight model, one mission arc. Resist expanding into
  a multi-vehicle sandbox until this core loop is finished and fun.
