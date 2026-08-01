#include "flight_computer.h"

#include <godot_cpp/core/class_db.hpp>

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
        default: break;
    }
}

void HelgaFlightComputer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("transition_to", "next_state"), &HelgaFlightComputer::transition_to);
    ClassDB::bind_method(D_METHOD("get_current_state"), &HelgaFlightComputer::get_current_state);
    ClassDB::bind_method(D_METHOD("is_known_state", "state"), &HelgaFlightComputer::is_known_state);
    ClassDB::bind_method(D_METHOD("state_name", "state"), &HelgaFlightComputer::state_name);

    ADD_SIGNAL(MethodInfo("state_changed",
                          PropertyInfo(Variant::INT, "previous_state"),
                          PropertyInfo(Variant::INT, "current_state")));

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
}
