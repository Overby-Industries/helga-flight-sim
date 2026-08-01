#ifndef HELGA_FLIGHT_COMPUTER_H
#define HELGA_FLIGHT_COMPUTER_H

// HelgaFlightComputer -- the flight-computer FSM, exposed to Godot.
//
// Follows the integer-based extensible FSM architecture (see
// docs/DESIGN.md): a small, closed, compiler-checked core enum for the
// certified baseline phases of flight, plus `constexpr int` extended
// states for contingencies discovered after the fact -- added without
// touching the core enum. The FSM's actual state variable is a plain
// int so core and extended states share one dispatch pipeline.
//
// This class owns state bookkeeping, entry/exit dispatch, and the
// unknown-state-falls-back-to-FAULT safety net. It does not yet decide
// *when* to transition -- there's no flight model or guard-condition
// logic (altitude, velocity, AoA thresholds) to key off of yet. Callers
// (GDScript mission/flight logic) call transition_to() once those
// conditions are met; see docs/DESIGN.md's transition-table convention
// for how each legal transition should be documented as it's added.

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

// Core baseline flight phases every sortie is guaranteed to pass
// through in order. Closed and compiler-checked -- do not add new
// members here. New contingency states go in the extended block below.
enum FlightState : int {
    PREFLIGHT = 0,
    TAXI = 1,
    TAKEOFF = 2,
    CLIMB = 3,
    CRUISE = 4,
    ASCENT = 5,   // upper-atmosphere transition through orbital insertion burn
    ORBIT = 6,
    REENTRY = 7,
    APPROACH = 8,
    LANDING = 9,
    FAULT = 10,
};

// Extended states -- real GA/test-flight contingencies, added past the
// core enum's range without modifying it. Keep MAX_KNOWN_STATE in sync
// whenever a new one is added.
constexpr int STATE_ABORT_TAKEOFF = 11;               // Rejected takeoff roll
constexpr int STATE_GO_AROUND = 12;                   // Balked landing
constexpr int STATE_HOLDING_PATTERN = 13;             // ATC-issued hold
constexpr int STATE_DIVERT = 14;                      // Diversion to an alternate
constexpr int STATE_SHALLOW_REENTRY_CONTINGENCY = 15; // Off-nominal shallow entry
constexpr int STATE_POST_LANDING_CHECK = 16;          // Systems check after touchdown

constexpr int MAX_KNOWN_STATE = STATE_POST_LANDING_CHECK;

class HelgaFlightComputer : public Node {
    GDCLASS(HelgaFlightComputer, Node)

private:
    int current_state = static_cast<int>(PREFLIGHT);

    void on_state_entry(int state);
    void on_state_exit(int state);

protected:
    static void _bind_methods();

public:
    HelgaFlightComputer();
    ~HelgaFlightComputer() override;

    // Attempts to move to next_state. An unrecognized state id (outside
    // the core enum and past MAX_KNOWN_STATE) is never accepted -- the
    // FSM forces FAULT instead, the same safety net the reference
    // architecture applies to a corrupted/unexpected state id. Returns
    // true if next_state was accepted as given, false if it was forced
    // to FAULT.
    bool transition_to(int next_state);

    int get_current_state() const;
    bool is_known_state(int state) const;
    String state_name(int state) const;
};

}

#endif // HELGA_FLIGHT_COMPUTER_H
