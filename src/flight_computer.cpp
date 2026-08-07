#include "flight_computer.h"

#include <godot_cpp/core/class_db.hpp>

#include <cmath>

using namespace godot;

HelgaFlightComputer::HelgaFlightComputer() {}

HelgaFlightComputer::~HelgaFlightComputer() {}

bool HelgaFlightComputer::transition_to(int next_state) {
    // Safety net from the integer-based extensible FSM architecture:
    // an unrecognized state id (corrupted save data, a caller passing a
    // stale constant from a future build, etc.) is never accepted as
    // given -- force FAULT instead of letting the dispatch switches
    // below fall through to nothing.
    bool known = is_known_state(next_state);
    int target = known ? next_state : static_cast<int>(FAULT);

    on_state_exit(current_state);
    int previous_state = current_state;
    current_state = target;
    on_state_entry(current_state);

    emit_signal("state_changed", previous_state, current_state);
    return known;
}

bool HelgaFlightComputer::is_grounded(double altitude_m, double vertical_speed_ms) const {
    return altitude_m <= grounded_altitude_m && std::fabs(vertical_speed_ms) <= grounded_vspeed_ms;
}

// Automatic phase-of-flight transition table, evaluated once per physics
// frame purely from flight-dynamics telemetry -- the same way a real
// squat switch/radar altimeter/airspeed-based phase annunciator would
// detect "what the airplane is physically doing." This is deliberately
// NOT where ATC clearances or other systems' contingencies get decided
// (see the class comment in flight_computer.h) -- those callers hold
// their own reference to this node and call transition_to() directly.
// At most one transition fires per call.
//
//   from                       condition                                              to
//   -------------------------  -------------------------------------------------      --------------------------
//   PREFLIGHT                  grounded AND throttle > taxi_throttle                   TAXI
//                                 (godot/scripts/preflight_checklist.gd holds
//                                  throttle at 0 until its checklist is done)
//   TAXI                       grounded AND throttle > takeoff_throttle                TAKEOFF
//   TAKEOFF                    grounded AND throttle < abort_throttle                  STATE_ABORT_TAKEOFF
//   TAKEOFF                    altitude > airborne_climb_altitude_m AND climbing       CLIMB
//   STATE_ABORT_TAKEOFF        grounded AND airspeed < rollout_airspeed_ms             TAXI
//   CLIMB                      altitude > cruise_altitude_m AND |vspeed| leveled off   CRUISE
//   CRUISE                     throttle > ascent_commit_throttle                       ASCENT
//   ASCENT                     altitude > orbit_altitude_m AND |vspeed| leveled off    ORBIT
//   ORBIT                      vertical_speed < deorbit_vspeed_ms                      REENTRY
//   REENTRY                    altitude < approach_altitude_m                          APPROACH
//   APPROACH / LANDING         airborne AND throttle > ascent_commit_throttle          STATE_GO_AROUND
//   STATE_GO_AROUND            altitude > airborne_climb_altitude_m AND climbing       CLIMB
//   APPROACH                   altitude < flare_altitude_m                             LANDING
//   LANDING                    grounded AND airspeed < rollout_airspeed_ms             STATE_POST_LANDING_CHECK
//   STATE_POST_LANDING_CHECK   airspeed < stopped_airspeed_ms                          TAXI
//
// Not handled here (call transition_to() directly instead):
// STATE_HOLDING_PATTERN and STATE_DIVERT (ATC/comms system),
// STATE_SHALLOW_REENTRY_CONTINGENCY (reentry corridor system), FAULT
// (any system detecting a genuine failure).
void HelgaFlightComputer::evaluate_auto_transition(double altitude_m, double airspeed_ms, double vertical_speed_ms, double throttle) {
    bool grounded = is_grounded(altitude_m, vertical_speed_ms);
    bool climbing_away = altitude_m > airborne_climb_altitude_m && vertical_speed_ms > 0.0;

    switch (current_state) {
        case PREFLIGHT:
            if (grounded && throttle > taxi_throttle) {
                transition_to(TAXI);
            }
            break;
        case TAXI:
            if (grounded && throttle > takeoff_throttle) {
                transition_to(TAKEOFF);
            }
            break;
        case TAKEOFF:
            if (climbing_away) {
                transition_to(CLIMB);
            } else if (grounded && throttle < abort_throttle) {
                transition_to(STATE_ABORT_TAKEOFF);
            }
            break;
        case STATE_ABORT_TAKEOFF:
            if (grounded && airspeed_ms < rollout_airspeed_ms) {
                transition_to(TAXI);
            }
            break;
        case CLIMB:
            if (altitude_m > cruise_altitude_m && std::fabs(vertical_speed_ms) < level_off_vspeed_ms) {
                transition_to(CRUISE);
            }
            break;
        case CRUISE:
            if (throttle > ascent_commit_throttle) {
                transition_to(ASCENT);
            }
            break;
        case ASCENT:
            if (altitude_m > orbit_altitude_m && std::fabs(vertical_speed_ms) < level_off_vspeed_ms) {
                transition_to(ORBIT);
            }
            break;
        case ORBIT:
            if (vertical_speed_ms < deorbit_vspeed_ms) {
                transition_to(REENTRY);
            }
            break;
        case REENTRY:
            if (altitude_m < approach_altitude_m) {
                transition_to(APPROACH);
            }
            break;
        case APPROACH:
            if (!grounded && throttle > ascent_commit_throttle) {
                transition_to(STATE_GO_AROUND);
            } else if (altitude_m < flare_altitude_m) {
                transition_to(LANDING);
            }
            break;
        case LANDING:
            if (!grounded && throttle > ascent_commit_throttle) {
                transition_to(STATE_GO_AROUND);
            } else if (grounded && airspeed_ms < rollout_airspeed_ms) {
                transition_to(STATE_POST_LANDING_CHECK);
            }
            break;
        case STATE_GO_AROUND:
            if (climbing_away) {
                transition_to(CLIMB);
            }
            break;
        case STATE_POST_LANDING_CHECK:
            if (airspeed_ms < stopped_airspeed_ms) {
                transition_to(TAXI);
            }
            break;
        default:
            break;
    }
}

int HelgaFlightComputer::get_current_state() const {
    return current_state;
}

bool HelgaFlightComputer::is_known_state(int state) const {
    return state >= static_cast<int>(PREFLIGHT) && state <= MAX_KNOWN_STATE;
}

String HelgaFlightComputer::state_name(int state) const {
    switch (state) {
        // Core states
        case PREFLIGHT: return "PREFLIGHT";
        case TAXI: return "TAXI";
        case TAKEOFF: return "TAKEOFF";
        case CLIMB: return "CLIMB";
        case CRUISE: return "CRUISE";
        case ASCENT: return "ASCENT";
        case ORBIT: return "ORBIT";
        case REENTRY: return "REENTRY";
        case APPROACH: return "APPROACH";
        case LANDING: return "LANDING";
        case FAULT: return "FAULT";
        // Extended states
        case STATE_ABORT_TAKEOFF: return "ABORT_TAKEOFF";
        case STATE_GO_AROUND: return "GO_AROUND";
        case STATE_HOLDING_PATTERN: return "HOLDING_PATTERN";
        case STATE_DIVERT: return "DIVERT";
        case STATE_SHALLOW_REENTRY_CONTINGENCY: return "SHALLOW_REENTRY_CONTINGENCY";
        case STATE_POST_LANDING_CHECK: return "POST_LANDING_CHECK";
        case STATE_THERMAL_OVERLOAD: return "THERMAL_OVERLOAD";
        default: return "UNKNOWN";
    }
}

void HelgaFlightComputer::on_state_entry(int state) {
    switch (state) {
        // Core states -- hooks for systems that don't exist yet land here
        // (aerodynamics model, propulsion controller, ATC comms) as they're built.
        case PREFLIGHT: break;
        case TAXI: break;
        case TAKEOFF: break;
        case CLIMB: break;
        case CRUISE: break;
        case ASCENT: break;
        case ORBIT: break;
        case REENTRY: break;
        case APPROACH: break;
        case LANDING: break;
        case FAULT: break;
        // Extended states
        case STATE_ABORT_TAKEOFF: break;
        case STATE_GO_AROUND: break;
        case STATE_HOLDING_PATTERN: break;
        case STATE_DIVERT: break;
        case STATE_SHALLOW_REENTRY_CONTINGENCY: break;
        case STATE_POST_LANDING_CHECK: break;
        case STATE_THERMAL_OVERLOAD: break;
        default: break;
    }
}

void HelgaFlightComputer::on_state_exit(int state) {
    switch (state) {
        case PREFLIGHT: break;
        case TAXI: break;
        case TAKEOFF: break;
        case CLIMB: break;
        case CRUISE: break;
        case ASCENT: break;
        case ORBIT: break;
        case REENTRY: break;
        case APPROACH: break;
        case LANDING: break;
        case FAULT: break;
        case STATE_ABORT_TAKEOFF: break;
        case STATE_GO_AROUND: break;
        case STATE_HOLDING_PATTERN: break;
        case STATE_DIVERT: break;
        case STATE_SHALLOW_REENTRY_CONTINGENCY: break;
        case STATE_POST_LANDING_CHECK: break;
        case STATE_THERMAL_OVERLOAD: break;
        default: break;
    }
}

void HelgaFlightComputer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("transition_to", "next_state"), &HelgaFlightComputer::transition_to);
    ClassDB::bind_method(D_METHOD("evaluate_auto_transition", "altitude_m", "airspeed_ms", "vertical_speed_ms", "throttle"), &HelgaFlightComputer::evaluate_auto_transition);
    ClassDB::bind_method(D_METHOD("get_current_state"), &HelgaFlightComputer::get_current_state);
    ClassDB::bind_method(D_METHOD("is_known_state", "state"), &HelgaFlightComputer::is_known_state);
    ClassDB::bind_method(D_METHOD("state_name", "state"), &HelgaFlightComputer::state_name);

    ClassDB::bind_method(D_METHOD("get_grounded_altitude_m"), &HelgaFlightComputer::get_grounded_altitude_m);
    ClassDB::bind_method(D_METHOD("set_grounded_altitude_m", "value"), &HelgaFlightComputer::set_grounded_altitude_m);
    ClassDB::bind_method(D_METHOD("get_grounded_vspeed_ms"), &HelgaFlightComputer::get_grounded_vspeed_ms);
    ClassDB::bind_method(D_METHOD("set_grounded_vspeed_ms", "value"), &HelgaFlightComputer::set_grounded_vspeed_ms);
    ClassDB::bind_method(D_METHOD("get_airborne_climb_altitude_m"), &HelgaFlightComputer::get_airborne_climb_altitude_m);
    ClassDB::bind_method(D_METHOD("set_airborne_climb_altitude_m", "value"), &HelgaFlightComputer::set_airborne_climb_altitude_m);
    ClassDB::bind_method(D_METHOD("get_cruise_altitude_m"), &HelgaFlightComputer::get_cruise_altitude_m);
    ClassDB::bind_method(D_METHOD("set_cruise_altitude_m", "value"), &HelgaFlightComputer::set_cruise_altitude_m);
    ClassDB::bind_method(D_METHOD("get_level_off_vspeed_ms"), &HelgaFlightComputer::get_level_off_vspeed_ms);
    ClassDB::bind_method(D_METHOD("set_level_off_vspeed_ms", "value"), &HelgaFlightComputer::set_level_off_vspeed_ms);
    ClassDB::bind_method(D_METHOD("get_taxi_throttle"), &HelgaFlightComputer::get_taxi_throttle);
    ClassDB::bind_method(D_METHOD("set_taxi_throttle", "value"), &HelgaFlightComputer::set_taxi_throttle);
    ClassDB::bind_method(D_METHOD("get_takeoff_throttle"), &HelgaFlightComputer::get_takeoff_throttle);
    ClassDB::bind_method(D_METHOD("set_takeoff_throttle", "value"), &HelgaFlightComputer::set_takeoff_throttle);
    ClassDB::bind_method(D_METHOD("get_abort_throttle"), &HelgaFlightComputer::get_abort_throttle);
    ClassDB::bind_method(D_METHOD("set_abort_throttle", "value"), &HelgaFlightComputer::set_abort_throttle);
    ClassDB::bind_method(D_METHOD("get_ascent_commit_throttle"), &HelgaFlightComputer::get_ascent_commit_throttle);
    ClassDB::bind_method(D_METHOD("set_ascent_commit_throttle", "value"), &HelgaFlightComputer::set_ascent_commit_throttle);
    ClassDB::bind_method(D_METHOD("get_orbit_altitude_m"), &HelgaFlightComputer::get_orbit_altitude_m);
    ClassDB::bind_method(D_METHOD("set_orbit_altitude_m", "value"), &HelgaFlightComputer::set_orbit_altitude_m);
    ClassDB::bind_method(D_METHOD("get_deorbit_vspeed_ms"), &HelgaFlightComputer::get_deorbit_vspeed_ms);
    ClassDB::bind_method(D_METHOD("set_deorbit_vspeed_ms", "value"), &HelgaFlightComputer::set_deorbit_vspeed_ms);
    ClassDB::bind_method(D_METHOD("get_approach_altitude_m"), &HelgaFlightComputer::get_approach_altitude_m);
    ClassDB::bind_method(D_METHOD("set_approach_altitude_m", "value"), &HelgaFlightComputer::set_approach_altitude_m);
    ClassDB::bind_method(D_METHOD("get_flare_altitude_m"), &HelgaFlightComputer::get_flare_altitude_m);
    ClassDB::bind_method(D_METHOD("set_flare_altitude_m", "value"), &HelgaFlightComputer::set_flare_altitude_m);
    ClassDB::bind_method(D_METHOD("get_rollout_airspeed_ms"), &HelgaFlightComputer::get_rollout_airspeed_ms);
    ClassDB::bind_method(D_METHOD("set_rollout_airspeed_ms", "value"), &HelgaFlightComputer::set_rollout_airspeed_ms);
    ClassDB::bind_method(D_METHOD("get_stopped_airspeed_ms"), &HelgaFlightComputer::get_stopped_airspeed_ms);
    ClassDB::bind_method(D_METHOD("set_stopped_airspeed_ms", "value"), &HelgaFlightComputer::set_stopped_airspeed_ms);

    ADD_SIGNAL(MethodInfo("state_changed",
                          PropertyInfo(Variant::INT, "previous_state"),
                          PropertyInfo(Variant::INT, "current_state")));

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "grounded_altitude_m"), "set_grounded_altitude_m", "get_grounded_altitude_m");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "grounded_vspeed_ms"), "set_grounded_vspeed_ms", "get_grounded_vspeed_ms");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "airborne_climb_altitude_m"), "set_airborne_climb_altitude_m", "get_airborne_climb_altitude_m");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cruise_altitude_m"), "set_cruise_altitude_m", "get_cruise_altitude_m");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "level_off_vspeed_ms"), "set_level_off_vspeed_ms", "get_level_off_vspeed_ms");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "taxi_throttle", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_taxi_throttle", "get_taxi_throttle");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "takeoff_throttle", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_takeoff_throttle", "get_takeoff_throttle");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "abort_throttle", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_abort_throttle", "get_abort_throttle");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ascent_commit_throttle", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_ascent_commit_throttle", "get_ascent_commit_throttle");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "orbit_altitude_m"), "set_orbit_altitude_m", "get_orbit_altitude_m");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "deorbit_vspeed_ms"), "set_deorbit_vspeed_ms", "get_deorbit_vspeed_ms");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "approach_altitude_m"), "set_approach_altitude_m", "get_approach_altitude_m");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flare_altitude_m"), "set_flare_altitude_m", "get_flare_altitude_m");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rollout_airspeed_ms"), "set_rollout_airspeed_ms", "get_rollout_airspeed_ms");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "stopped_airspeed_ms"), "set_stopped_airspeed_ms", "get_stopped_airspeed_ms");

    // Core states
    ClassDB::bind_integer_constant(get_class_static(), "", "PREFLIGHT", PREFLIGHT);
    ClassDB::bind_integer_constant(get_class_static(), "", "TAXI", TAXI);
    ClassDB::bind_integer_constant(get_class_static(), "", "TAKEOFF", TAKEOFF);
    ClassDB::bind_integer_constant(get_class_static(), "", "CLIMB", CLIMB);
    ClassDB::bind_integer_constant(get_class_static(), "", "CRUISE", CRUISE);
    ClassDB::bind_integer_constant(get_class_static(), "", "ASCENT", ASCENT);
    ClassDB::bind_integer_constant(get_class_static(), "", "ORBIT", ORBIT);
    ClassDB::bind_integer_constant(get_class_static(), "", "REENTRY", REENTRY);
    ClassDB::bind_integer_constant(get_class_static(), "", "APPROACH", APPROACH);
    ClassDB::bind_integer_constant(get_class_static(), "", "LANDING", LANDING);
    ClassDB::bind_integer_constant(get_class_static(), "", "FAULT", FAULT);

    // Extended states
    ClassDB::bind_integer_constant(get_class_static(), "", "STATE_ABORT_TAKEOFF", STATE_ABORT_TAKEOFF);
    ClassDB::bind_integer_constant(get_class_static(), "", "STATE_GO_AROUND", STATE_GO_AROUND);
    ClassDB::bind_integer_constant(get_class_static(), "", "STATE_HOLDING_PATTERN", STATE_HOLDING_PATTERN);
    ClassDB::bind_integer_constant(get_class_static(), "", "STATE_DIVERT", STATE_DIVERT);
    ClassDB::bind_integer_constant(get_class_static(), "", "STATE_SHALLOW_REENTRY_CONTINGENCY", STATE_SHALLOW_REENTRY_CONTINGENCY);
    ClassDB::bind_integer_constant(get_class_static(), "", "STATE_POST_LANDING_CHECK", STATE_POST_LANDING_CHECK);
    ClassDB::bind_integer_constant(get_class_static(), "", "STATE_THERMAL_OVERLOAD", STATE_THERMAL_OVERLOAD);
}
