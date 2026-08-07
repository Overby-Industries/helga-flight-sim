#include "gravity.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

HelgaGravity::HelgaGravity() {}

HelgaGravity::~HelgaGravity() {}

void HelgaGravity::_ready() {
    body = Object::cast_to<RigidBody3D>(get_parent());
}

void HelgaGravity::_physics_process(double p_delta) {
    (void)p_delta;
    if (body == nullptr) {
        return;
    }

    double altitude = std::max(static_cast<double>(body->get_global_transform().origin.y), 0.0);
    double radius = earth_radius_m + altitude;

    double radius_ratio = earth_radius_m / radius;
    local_gravity_ms2 = surface_gravity_ms2 * radius_ratio * radius_ratio;
    orbital_velocity_ms = std::sqrt(local_gravity_ms2 * radius);

    double weight = local_gravity_ms2 * body->get_mass();
    body->apply_central_force(Vector3(0.0, static_cast<real_t>(-weight), 0.0));
}

void HelgaGravity::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_local_gravity_ms2"), &HelgaGravity::get_local_gravity_ms2);
    ClassDB::bind_method(D_METHOD("get_orbital_velocity_ms"), &HelgaGravity::get_orbital_velocity_ms);

    ClassDB::bind_method(D_METHOD("get_earth_radius_m"), &HelgaGravity::get_earth_radius_m);
    ClassDB::bind_method(D_METHOD("set_earth_radius_m", "value"), &HelgaGravity::set_earth_radius_m);
    ClassDB::bind_method(D_METHOD("get_surface_gravity_ms2"), &HelgaGravity::get_surface_gravity_ms2);
    ClassDB::bind_method(D_METHOD("set_surface_gravity_ms2", "value"), &HelgaGravity::set_surface_gravity_ms2);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "earth_radius_m"), "set_earth_radius_m", "get_earth_radius_m");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "surface_gravity_ms2"), "set_surface_gravity_ms2", "get_surface_gravity_ms2");
}
