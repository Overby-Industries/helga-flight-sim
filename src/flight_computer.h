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
// This class owns state bookkeeping, entry/exit dispatch, the
// unknown-state-falls-back-to-FAULT safety net, and the automatic
// phase-of-flight transition table (evaluate_auto_transition, see the
// transition table documented above that method in flight_computer.cpp).
// That table only covers what's detectable from raw flight dynamics --
// altitude, airspeed, vertical speed, throttle -- the same way a real
// squat switch/radar altimeter/airspeed-based phase annunciator would.
// ATC-gated holds (see docs/DESIGN.md's ATC & comms section) and other
// systems' contingencies (e.g. the reentry corridor system arming
// STATE_SHALLOW_REENTRY_CONTINGENCY) are not auto-detected here -- those
// callers hold a reference to this node and call transition_to()
// directly once their own conditions are met.

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
constexpr int STATE_SHALLOW_REENTRY_CONTINGENCY = 15; // Off-nominal shallow entry, at risk of skipping out
constexpr int STATE_POST_LANDING_CHECK = 16;          // Systems check after touchdown
constexpr int STATE_THERMAL_OVERLOAD = 17;            // Reentry too steep, heat flux over the shield's management capacity

constexpr int MAX_KNOWN_STATE = STATE_THERMAL_OVERLOAD;

class HelgaFlightComputer : public Node {
    GDCLASS(HelgaFlightComputer, Node)

private:
    int current_state = static_cast<int>(PREFLIGHT);

    // Guard-condition thresholds for evaluate_auto_transition(). Exposed
    // as tunable properties like the rest of the sim's physical constants
    // rather than hardcoded, since none of these are real published
    // numbers -- they're this sim's own placeholders until real data
    // (a taxi/tower clearance model, real orbital mechanics) replaces
    // the ones called out below.
    double grounded_altitude_m = 3.5;        // AGL; below this + low vspeed reads as "on the ground"
    double grounded_vspeed_ms = 1.0;         // |vertical speed| below this counts as settled
    double airborne_climb_altitude_m = 15.0; // AGL; above this while climbing reads as "away and climbing"
    double cruise_altitude_m = 3000.0;       // CLIMB->CRUISE level-off altitude
    double level_off_vspeed_ms = 1.0;        // |vspeed| below this counts as leveled off
    double taxi_throttle = 0.05;             // PREFLIGHT->TAXI -- godot/scripts/preflight_checklist.gd holds throttle at 0 until its checklist is complete, so this never fires early
    double takeoff_throttle = 0.9;           // TAXI->TAKEOFF -- throttle firewalled for the roll
    double abort_throttle = 0.05;            // TAKEOFF->ABORT_TAKEOFF -- throttle chopped during the roll
    double ascent_commit_throttle = 0.9;     // CRUISE->ASCENT, and APPROACH/LANDING->GO_AROUND
    double orbit_altitude_m = 100000.0;      // ASCENT->ORBIT altitude floor -- paired with orbital_velocity_fraction below
    double orbital_velocity_fraction = 0.9;  // ASCENT->ORBIT also needs horizontal speed >= this fraction of HelgaGravity's real circular-orbit speed -- see gravity.h
    double deorbit_vspeed_ms = -5.0;         // ORBIT->REENTRY -- sustained descent reads as a deorbit burn
    double approach_altitude_m = 3000.0;     // REENTRY->APPROACH, terminal-area handoff
    double flare_altitude_m = 15.0;          // APPROACH->LANDING
    double rollout_airspeed_ms = 5.0;        // LANDING->POST_LANDING_CHECK, roll essentially done
    double stopped_airspeed_ms = 1.0;        // POST_LANDING_CHECK->TAXI, aircraft has actually stopped

    bool is_grounded(double altitude_m, double vertical_speed_ms) const;

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

    // Called once per physics frame (see aircraft_control.gd) with the
    // aircraft's current telemetry. Applies at most one transition per
    // call -- see the transition table documented in flight_computer.cpp.
    // horizontal_speed_ms/orbital_velocity_ms are only consulted for the
    // ASCENT->ORBIT check (orbital_velocity_ms comes from HelgaGravity's
    // get_orbital_velocity_ms(), see gravity.h).
    void evaluate_auto_transition(double altitude_m, double airspeed_ms, double vertical_speed_ms, double throttle, double horizontal_speed_ms, double orbital_velocity_ms);

    int get_current_state() const;
    bool is_known_state(int state) const;
    String state_name(int state) const;

    double get_grounded_altitude_m() const { return grounded_altitude_m; }
    void set_grounded_altitude_m(double v) { grounded_altitude_m = v; }
    double get_grounded_vspeed_ms() const { return grounded_vspeed_ms; }
    void set_grounded_vspeed_ms(double v) { grounded_vspeed_ms = v; }
    double get_airborne_climb_altitude_m() const { return airborne_climb_altitude_m; }
    void set_airborne_climb_altitude_m(double v) { airborne_climb_altitude_m = v; }
    double get_cruise_altitude_m() const { return cruise_altitude_m; }
    void set_cruise_altitude_m(double v) { cruise_altitude_m = v; }
    double get_level_off_vspeed_ms() const { return level_off_vspeed_ms; }
    void set_level_off_vspeed_ms(double v) { level_off_vspeed_ms = v; }
    double get_taxi_throttle() const { return taxi_throttle; }
    void set_taxi_throttle(double v) { taxi_throttle = v; }
    double get_takeoff_throttle() const { return takeoff_throttle; }
    void set_takeoff_throttle(double v) { takeoff_throttle = v; }
    double get_abort_throttle() const { return abort_throttle; }
    void set_abort_throttle(double v) { abort_throttle = v; }
    double get_orbital_velocity_fraction() const { return orbital_velocity_fraction; }
    void set_orbital_velocity_fraction(double v) { orbital_velocity_fraction = v; }
    double get_ascent_commit_throttle() const { return ascent_commit_throttle; }
    void set_ascent_commit_throttle(double v) { ascent_commit_throttle = v; }
    double get_orbit_altitude_m() const { return orbit_altitude_m; }
    void set_orbit_altitude_m(double v) { orbit_altitude_m = v; }
    double get_deorbit_vspeed_ms() const { return deorbit_vspeed_ms; }
    void set_deorbit_vspeed_ms(double v) { deorbit_vspeed_ms = v; }
    double get_approach_altitude_m() const { return approach_altitude_m; }
    void set_approach_altitude_m(double v) { approach_altitude_m = v; }
    double get_flare_altitude_m() const { return flare_altitude_m; }
    void set_flare_altitude_m(double v) { flare_altitude_m = v; }
    double get_rollout_airspeed_ms() const { return rollout_airspeed_ms; }
    void set_rollout_airspeed_ms(double v) { rollout_airspeed_ms = v; }
    double get_stopped_airspeed_ms() const { return stopped_airspeed_ms; }
    void set_stopped_airspeed_ms(double v) { stopped_airspeed_ms = v; }
};

}

#endif // HELGA_FLIGHT_COMPUTER_H
