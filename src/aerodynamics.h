#ifndef HELGA_AERODYNAMICS_H
#define HELGA_AERODYNAMICS_H

// HelgaAerodynamics -- drives the aircraft's HelgaAeroSurface children
// each physics frame.
//
// Expected scene layout: a RigidBody3D (the aircraft body) with a
// HelgaAerodynamics node as a direct child at the origin, which in turn
// has one HelgaAeroSurface child per lifting panel (left wing, right
// wing, horizontal stabilizer/elevons, vertical fin/rudder, canard --
// whatever the airframe actually has). Each surface's own Node3D
// transform in the scene *is* its position and orientation for
// aerodynamic purposes -- a dihedral wing panel tilted a few degrees in
// the editor gets that dihedral's roll stability for free, because this
// class evaluates relative wind in each surface's own local frame.
//
// Per physics frame: for each surface, compute the velocity of the air
// at that surface's specific location (the body's velocity at the
// center of mass plus the velocity contributed by the body's own
// rotation, v = v_com + omega x r) so surfaces further from the axis of
// rotation -- and surfaces on the outside of a roll or yaw -- see a
// different, physically correct relative wind than surfaces near the
// center. That's what produces real damping and real (unscripted)
// post-stall spin behavior instead of hand-tuned damping coefficients.

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <vector>

namespace godot {

class HelgaAeroSurface;

class HelgaAerodynamics : public Node3D {
    GDCLASS(HelgaAerodynamics, Node3D)

private:
    RigidBody3D *body = nullptr;
    std::vector<HelgaAeroSurface *> surfaces;

    double elevator = 0.0; // -1..1
    double aileron = 0.0;  // -1..1
    double rudder = 0.0;   // -1..1
    double flaps = 0.0;    // 0..1, symmetric flap lever position

    double air_density_sea_level = 1.225;   // kg/m^3
    double atmosphere_scale_height = 8500.0; // meters

    // Whole-aircraft readings, refreshed once per physics frame for
    // HUD/debug/FSM guard-condition use -- distinct from each surface's
    // own local angle of attack.
    double airspeed = 0.0;
    double angle_of_attack_deg = 0.0;
    double sideslip_deg = 0.0;

    static double clamp_input(double p_value) { return p_value < -1.0 ? -1.0 : (p_value > 1.0 ? 1.0 : p_value); }
    static double clamp_unit(double p_value) { return p_value < 0.0 ? 0.0 : (p_value > 1.0 ? 1.0 : p_value); }

protected:
    static void _bind_methods();

public:
    HelgaAerodynamics();
    ~HelgaAerodynamics() override;

    void _ready() override;
    void _physics_process(double p_delta) override;

    double get_elevator() const { return elevator; }
    void set_elevator(double p_value) { elevator = clamp_input(p_value); }
    double get_aileron() const { return aileron; }
    void set_aileron(double p_value) { aileron = clamp_input(p_value); }
    double get_rudder() const { return rudder; }
    void set_rudder(double p_value) { rudder = clamp_input(p_value); }
    double get_flaps() const { return flaps; }
    void set_flaps(double p_value) { flaps = clamp_unit(p_value); }

    double get_air_density_sea_level() const { return air_density_sea_level; }
    void set_air_density_sea_level(double p_value) { air_density_sea_level = p_value; }
    double get_atmosphere_scale_height() const { return atmosphere_scale_height; }
    void set_atmosphere_scale_height(double p_value) { atmosphere_scale_height = p_value; }

    double get_airspeed() const { return airspeed; }
    double get_angle_of_attack_deg() const { return angle_of_attack_deg; }
    double get_sideslip_deg() const { return sideslip_deg; }
};

}

#endif // HELGA_AERODYNAMICS_H
