#ifndef HELGA_AUTOPILOT_H
#define HELGA_AUTOPILOT_H

// HelgaAutopilot -- a pitch-hold recovery aid for a missed reentry
// approach. When the aircraft has skipped out of the reentry corridor
// (STATE_SHALLOW_REENTRY_CONTINGENCY, see flight_computer.h), this can
// fly the pitch axis toward the pre-approved flight plan's target
// flight-path-angle (see godot/scripts/reentry_flight_plan.gd) while the
// pilot manages throttle/roll/yaw. This is deliberately a single-axis
// (pitch) proportional controller, not a full 6DOF autoflight system --
// a real, honest recovery aid rather than an autopilot that could
// quietly fly the whole sortie. It also simplifies by commanding
// elevator directly off flight-path-angle error rather than going
// through an intermediate target-AoA/attitude stage the way a real
// autopilot's inner pitch loop would -- a reasonable simplification
// given the scope, same spirit as this sim's other estimated-not-
// published tuning constants (see docs/DESIGN.md).
//
// engaged is driven externally (see aircraft_control.gd, which arms it
// automatically on entering STATE_SHALLOW_REENTRY_CONTINGENCY via
// HelgaFlightComputer's state_changed signal, and lets the pilot
// disengage manually) rather than this class watching the FSM itself --
// the same "systems don't reach into the FSM, callers reach into them"
// pattern the rest of the sim uses.
//
// compute_elevator() only computes and returns a value; it doesn't
// write to HelgaAerodynamics itself. The caller still owns that write,
// same as manual pilot input, so switching between manual and autopilot
// is just "whose number the caller writes this frame."

#include <godot_cpp/classes/node.hpp>

namespace godot {

class HelgaAutopilot : public Node {
    GDCLASS(HelgaAutopilot, Node)

private:
    bool engaged = false;
    double target_flight_path_angle_deg = 0.0;
    double pitch_gain = 0.08; // elevator command per degree of angle error
    double commanded_elevator = 0.0;

protected:
    static void _bind_methods();

public:
    HelgaAutopilot();
    ~HelgaAutopilot() override;

    void set_engaged(bool v) { engaged = v; }
    bool is_engaged() const { return engaged; }

    void set_target_flight_path_angle_deg(double v) { target_flight_path_angle_deg = v; }
    double get_target_flight_path_angle_deg() const { return target_flight_path_angle_deg; }

    double compute_elevator(double current_flight_path_angle_deg);
    double get_commanded_elevator() const { return commanded_elevator; }

    double get_pitch_gain() const { return pitch_gain; }
    void set_pitch_gain(double v) { pitch_gain = v; }
};

}

#endif // HELGA_AUTOPILOT_H
