#ifndef HELGA_GRAVITY_H
#define HELGA_GRAVITY_H

// HelgaGravity -- real inverse-square gravity, referenced to Earth's
// actual radius and surface gravity, applied manually each physics
// frame in place of Godot's constant default gravity (see main.tscn:
// HelgaAircraft has gravity_scale = 0, so the engine's own gravity
// integration is disabled and this is the only source of it).
//
// This deliberately does NOT switch the sim to a geocentric/curved-
// world coordinate system -- position.y is still "altitude" the same
// way HelgaAerodynamics/HelgaPropulsion's own atmosphere models already
// read it, not a distance from a planet center in 3D space. What
// changes is that gravity's *magnitude* now falls off with altitude
// the real way (g(h) = g0 * (R / (R+h))^2) instead of Godot's flat
// constant, and -- the actually important part for gameplay -- there is
// now a real, physically-derived circular-orbit speed at any given
// altitude (v = sqrt(g(h) * (R+h)), textbook vis-viva for a circular
// orbit) that HelgaFlightComputer's ASCENT->ORBIT guard checks against
// (see flight_computer.cpp's transition table). Before this, "reaching
// orbit" only required altitude and a leveled-off vertical speed, which
// a purely vertical ascent that stalls out at apogee could satisfy
// without ever building real orbital velocity. Now it can't: gravity at
// LEO/MEO altitudes is barely reduced from sea level (this matches real
// physics -- orbital "weightlessness" is freefall, not reduced gravity),
// so holding altitude for real takes real horizontal speed.
//
// Lives as a sibling of HelgaAerodynamics/HelgaPropulsion under
// HelgaAircraft, reaching its parent RigidBody3D the same way those do.

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>

namespace godot {

class HelgaGravity : public Node {
    GDCLASS(HelgaGravity, Node)

private:
    RigidBody3D *body = nullptr;

    double earth_radius_m = 6371000.0;
    double surface_gravity_ms2 = 9.81;

    double local_gravity_ms2 = 9.81;
    double orbital_velocity_ms = 0.0;

protected:
    static void _bind_methods();

public:
    HelgaGravity();
    ~HelgaGravity() override;

    void _ready() override;
    void _physics_process(double p_delta) override;

    // Current local gravitational acceleration, m/s^2.
    double get_local_gravity_ms2() const { return local_gravity_ms2; }
    // Circular-orbit speed at the current altitude, m/s -- the
    // horizontal speed at which gravity alone exactly supplies the
    // centripetal force to hold a level orbit here.
    double get_orbital_velocity_ms() const { return orbital_velocity_ms; }

    double get_earth_radius_m() const { return earth_radius_m; }
    void set_earth_radius_m(double v) { earth_radius_m = v; }
    double get_surface_gravity_ms2() const { return surface_gravity_ms2; }
    void set_surface_gravity_ms2(double v) { surface_gravity_ms2 = v; }
};

}

#endif // HELGA_GRAVITY_H
