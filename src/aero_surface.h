#ifndef HELGA_AERO_SURFACE_H
#define HELGA_AERO_SURFACE_H

// HelgaAeroSurface -- one discrete lifting panel (a wing half, a
// stabilizer, a vertical fin) of the aircraft's aerodynamic model.
//
// Real flight dynamics -- including roll/pitch/yaw damping and a
// genuine, un-scripted post-stall autorotation (spin) -- come from
// treating the airframe as several independent panels rather than one
// combined lift curve for the whole aircraft. Because each panel sits
// at its own offset from the center of mass, HelgaAerodynamics (the
// per-frame driver, see aerodynamics.h) gives each panel the actual
// local velocity at its position (including the velocity added by the
// aircraft's own rotation, v_point = v_com + omega x r). A rolling
// aircraft therefore increases the effective angle of attack on the
// down-going wing and decreases it on the up-going wing for free, out
// of geometry, not a hand-tuned "roll damping coefficient" -- and if
// one wing stalls before the other, that asymmetry is a real spin, not
// a scripted state.
//
// This class is a pure calculator: given the relative wind already
// expressed in this surface's own local frame plus a combined control
// input, it returns a force in that same local frame. It knows nothing
// about the rigid body, the scene tree beyond its own Node3D transform,
// or the flight computer.
//
// Control response is a weighted mix of pilot inputs
// (elevator_gain/aileron_gain), not a single exclusive axis, since a
// wing panel can respond to more than one command at once. Per the
// pilot: the main wings are clamshell flaperons (roll authority) that
// ALSO provide yaw authority as drag rudders -- opening the clamshell
// on one wing only, adding drag on that side without a symmetric
// side-force surface -- rather than a conventional vertical-fin
// rudder. That's yaw_drag_gain below, handled separately from
// elevator/aileron/rudder_gain because it changes drag, not effective
// angle of attack. The primary pitch-control surface is a separate
// "bird tail elevator" hinged at the fuselage tail (large
// elevator_gain, aileron/rudder/yaw_drag_gain=0); the main wings keep a
// small elevator_gain since their inner flaps can double as elevons if
// needed, per the pilot, but aren't the primary pitch authority.

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {

class HelgaAeroSurface : public Node3D {
    GDCLASS(HelgaAeroSurface, Node3D)

private:
    double area_m2 = 1.0;
    double lift_slope_per_rad = 5.5;      // pre-stall dCl/dAlpha
    double stall_angle_deg = 16.0;
    double stall_blend_range_deg = 8.0;   // AoA past stall over which Cl/Cd blend to flat-plate behavior
    double parasite_drag_coefficient = 0.02; // Cd0
    double induced_drag_factor = 0.05;       // k in Cd = Cd0 + k*Cl^2
    double incidence_deg = 0.0;              // fixed mounting angle relative to the fuselage reference line
    double control_effectiveness = 0.4;      // radians of effective AoA shift per unit combined control input
    double elevator_gain = 0.0;              // this surface's response to elevator input, -1..1
    double aileron_gain = 0.0;               // this surface's response to aileron input, -1..1
    double rudder_gain = 0.0;                // this surface's response to rudder input, -1..1
    double yaw_drag_gain = 0.0;               // -1..1, clamshell drag-rudder response to rudder input
    double leading_edge_flap_gain = 0.0;     // 0..1, how much of this surface is LE flap/slat
    double trailing_edge_flap_gain = 0.0;    // 0..1, how much of this surface is TE flap
    double yaw_drag_coefficient = 0.6;        // added Cd at full clamshell deployment

protected:
    static void _bind_methods();

public:
    HelgaAeroSurface();
    ~HelgaAeroSurface() override;

    // local_wind: relative wind at this surface's position, already
    // expressed in this surface's own local (rotated) coordinate frame.
    // control_input: the combined, already-gain-weighted control value
    // for this surface (see HelgaAerodynamics::_physics_process) --
    // typically in roughly -1..1 but not hard-clamped, since a surface
    // that mixes e.g. full elevator and full aileron can exceed that.
    // flap_deflection: 0..1 flap lever position, deployed symmetrically
    // (not part of the elevator/aileron/rudder mix). Leading-edge flaps
    // extend the usable stall angle (their real job -- delaying flow
    // separation); trailing-edge flaps add effective camber/lift at the
    // cost of extra drag. A surface with both gains at 0 (e.g. the tail
    // elevator) is completely unaffected by flap_deflection.
    // rudder_input: the raw (not gain-weighted) yaw command, -1..1 --
    // needed separately from control_input because the clamshell drag
    // rudder only opens for ONE sign of rudder_input per side (see
    // yaw_drag_gain); it cannot be folded into the same additive mix
    // used for elevator/aileron/rudder side-force since drag can't go
    // negative.
    // Returns the aerodynamic force on this surface, in the same local frame.
    Vector3 compute_force(const Vector3 &local_wind, double air_density, double control_input, double flap_deflection = 0.0, double rudder_input = 0.0) const;

    double get_elevator_gain() const { return elevator_gain; }
    void set_elevator_gain(double p_value) { elevator_gain = p_value; }
    double get_aileron_gain() const { return aileron_gain; }
    void set_aileron_gain(double p_value) { aileron_gain = p_value; }
    double get_rudder_gain() const { return rudder_gain; }
    void set_rudder_gain(double p_value) { rudder_gain = p_value; }
    double get_yaw_drag_gain() const { return yaw_drag_gain; }
    void set_yaw_drag_gain(double p_value) { yaw_drag_gain = p_value; }
    double get_leading_edge_flap_gain() const { return leading_edge_flap_gain; }
    void set_leading_edge_flap_gain(double p_value) { leading_edge_flap_gain = p_value; }
    double get_trailing_edge_flap_gain() const { return trailing_edge_flap_gain; }
    void set_trailing_edge_flap_gain(double p_value) { trailing_edge_flap_gain = p_value; }
    double get_yaw_drag_coefficient() const { return yaw_drag_coefficient; }
    void set_yaw_drag_coefficient(double p_value) { yaw_drag_coefficient = p_value; }

    double get_area() const { return area_m2; }
    void set_area(double p_area) { area_m2 = p_area; }

    double get_lift_slope_per_rad() const { return lift_slope_per_rad; }
    void set_lift_slope_per_rad(double p_value) { lift_slope_per_rad = p_value; }

    double get_stall_angle_deg() const { return stall_angle_deg; }
    void set_stall_angle_deg(double p_value) { stall_angle_deg = p_value; }

    double get_stall_blend_range_deg() const { return stall_blend_range_deg; }
    void set_stall_blend_range_deg(double p_value) { stall_blend_range_deg = p_value; }

    double get_parasite_drag_coefficient() const { return parasite_drag_coefficient; }
    void set_parasite_drag_coefficient(double p_value) { parasite_drag_coefficient = p_value; }

    double get_induced_drag_factor() const { return induced_drag_factor; }
    void set_induced_drag_factor(double p_value) { induced_drag_factor = p_value; }

    double get_incidence_deg() const { return incidence_deg; }
    void set_incidence_deg(double p_value) { incidence_deg = p_value; }

    double get_control_effectiveness() const { return control_effectiveness; }
    void set_control_effectiveness(double p_value) { control_effectiveness = p_value; }
};

}

#endif // HELGA_AERO_SURFACE_H
