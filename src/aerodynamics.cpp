#include "aerodynamics.h"
#include "aero_surface.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace {
constexpr double RAD_TO_DEG = 57.29577951308232;
}

HelgaAerodynamics::HelgaAerodynamics() {}

HelgaAerodynamics::~HelgaAerodynamics() {}

void HelgaAerodynamics::_ready() {
    body = Object::cast_to<RigidBody3D>(get_parent());

    surfaces.clear();
    for (int i = 0; i < get_child_count(); i++) {
        HelgaAeroSurface *surface = Object::cast_to<HelgaAeroSurface>(get_child(i));
        if (surface != nullptr) {
            surfaces.push_back(surface);
        }
    }
}

void HelgaAerodynamics::_physics_process(double p_delta) {
    if (body == nullptr || surfaces.empty()) {
        return;
    }

    Vector3 com_velocity = body->get_linear_velocity();
    Vector3 angular_velocity = body->get_angular_velocity();
    Vector3 com_position = body->get_global_transform().origin;

    double altitude = std::max(static_cast<double>(com_position.y), 0.0);
    double air_density = air_density_sea_level * std::exp(-altitude / atmosphere_scale_height);

    for (HelgaAeroSurface *surface : surfaces) {
        Transform3D surface_xform = surface->get_global_transform();
        Basis surface_basis = surface_xform.basis;
        Vector3 offset = surface_xform.origin - com_position;

        // Velocity of the air at this specific point on the airframe --
        // the body's velocity at the center of mass plus the velocity
        // contributed by the body's own rotation. This is what lets
        // roll/yaw/pitch rate change each surface's effective angle of
        // attack on its own, without a hand-tuned damping coefficient.
        Vector3 point_velocity = com_velocity + angular_velocity.cross(offset);
        Vector3 local_wind = surface_basis.xform_inv(point_velocity);

        double control_input = elevator * surface->get_elevator_gain()
            + aileron * surface->get_aileron_gain()
            + rudder * surface->get_rudder_gain();
        Vector3 local_force = surface->compute_force(local_wind, air_density, control_input, flaps, rudder);
        Vector3 world_force = surface_basis.xform(local_force);

        body->apply_force(world_force, offset);
    }

    // Whole-aircraft readings for HUD/debug/future FSM guard conditions --
    // separate from each surface's own local angle of attack above.
    Basis body_basis = body->get_global_transform().basis;
    Vector3 body_local_vel = body_basis.xform_inv(com_velocity);
    airspeed = body_local_vel.length();
    if (airspeed > 0.01) {
        angle_of_attack_deg = std::atan2(-body_local_vel.y, -body_local_vel.z) * RAD_TO_DEG;
        double sin_beta = std::min(std::max(body_local_vel.x / airspeed, -1.0), 1.0);
        sideslip_deg = std::asin(sin_beta) * RAD_TO_DEG;
    } else {
        angle_of_attack_deg = 0.0;
        sideslip_deg = 0.0;
    }
}

void HelgaAerodynamics::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_elevator"), &HelgaAerodynamics::get_elevator);
    ClassDB::bind_method(D_METHOD("set_elevator", "value"), &HelgaAerodynamics::set_elevator);
    ClassDB::bind_method(D_METHOD("get_aileron"), &HelgaAerodynamics::get_aileron);
    ClassDB::bind_method(D_METHOD("set_aileron", "value"), &HelgaAerodynamics::set_aileron);
    ClassDB::bind_method(D_METHOD("get_rudder"), &HelgaAerodynamics::get_rudder);
    ClassDB::bind_method(D_METHOD("set_rudder", "value"), &HelgaAerodynamics::set_rudder);
    ClassDB::bind_method(D_METHOD("get_flaps"), &HelgaAerodynamics::get_flaps);
    ClassDB::bind_method(D_METHOD("set_flaps", "value"), &HelgaAerodynamics::set_flaps);

    ClassDB::bind_method(D_METHOD("get_air_density_sea_level"), &HelgaAerodynamics::get_air_density_sea_level);
    ClassDB::bind_method(D_METHOD("set_air_density_sea_level", "value"), &HelgaAerodynamics::set_air_density_sea_level);
    ClassDB::bind_method(D_METHOD("get_atmosphere_scale_height"), &HelgaAerodynamics::get_atmosphere_scale_height);
    ClassDB::bind_method(D_METHOD("set_atmosphere_scale_height", "value"), &HelgaAerodynamics::set_atmosphere_scale_height);

    ClassDB::bind_method(D_METHOD("get_airspeed"), &HelgaAerodynamics::get_airspeed);
    ClassDB::bind_method(D_METHOD("get_angle_of_attack_deg"), &HelgaAerodynamics::get_angle_of_attack_deg);
    ClassDB::bind_method(D_METHOD("get_sideslip_deg"), &HelgaAerodynamics::get_sideslip_deg);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "elevator", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_elevator", "get_elevator");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "aileron", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_aileron", "get_aileron");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rudder", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_rudder", "get_rudder");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flaps", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_flaps", "get_flaps");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "air_density_sea_level"), "set_air_density_sea_level", "get_air_density_sea_level");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "atmosphere_scale_height"), "set_atmosphere_scale_height", "get_atmosphere_scale_height");
}
