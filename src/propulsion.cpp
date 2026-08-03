#include "propulsion.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

HelgaPropulsion::HelgaPropulsion() {}

HelgaPropulsion::~HelgaPropulsion() {}

void HelgaPropulsion::_ready() {
    body = Object::cast_to<RigidBody3D>(get_parent());
}

void HelgaPropulsion::_physics_process(double p_delta) {
    if (body == nullptr) {
        return;
    }

    double altitude = std::max(static_cast<double>(body->get_global_transform().origin.y), 0.0);
    double density_ratio = std::exp(-altitude / atmosphere_scale_height);

    abep_thrust = abep_max_thrust_newtons * throttle * density_ratio;
    mhd_thrust = mhd_max_thrust_newtons * throttle;

    if (throttle > afterburner_throttle_threshold && power_reserve > 0.0) {
        double afterburner_fraction = (throttle - afterburner_throttle_threshold) / (1.0 - afterburner_throttle_threshold);
        afterburner_thrust = il_afterburner_max_thrust_newtons * afterburner_fraction * std::min(power_reserve, 1.0);
        power_reserve = std::max(0.0, power_reserve - afterburner_drain_per_second * p_delta);
    } else {
        afterburner_thrust = 0.0;
        power_reserve = std::min(1.0, power_reserve + generation_recharge_per_second * (1.0 - throttle) * p_delta);
    }

    // Vanishingly small this close to Earth (real thrust is in deep
    // space, beyond this sim's LEO/MEO scope) -- included so the HUD/
    // debug readout reflects that all four systems are always active.
    solar_wind_thrust = solar_wind_max_thrust_newtons * (1.0 - density_ratio);

    double total = abep_thrust + mhd_thrust + afterburner_thrust + solar_wind_thrust;
    Vector3 forward = body->get_global_transform().basis.xform(Vector3(0.0, 0.0, -1.0));
    body->apply_central_force(forward * static_cast<real_t>(total));
}

void HelgaPropulsion::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_throttle"), &HelgaPropulsion::get_throttle);
    ClassDB::bind_method(D_METHOD("set_throttle", "value"), &HelgaPropulsion::set_throttle);
    ClassDB::bind_method(D_METHOD("get_abep_thrust"), &HelgaPropulsion::get_abep_thrust);
    ClassDB::bind_method(D_METHOD("get_mhd_thrust"), &HelgaPropulsion::get_mhd_thrust);
    ClassDB::bind_method(D_METHOD("get_afterburner_thrust"), &HelgaPropulsion::get_afterburner_thrust);
    ClassDB::bind_method(D_METHOD("get_solar_wind_thrust"), &HelgaPropulsion::get_solar_wind_thrust);
    ClassDB::bind_method(D_METHOD("get_total_thrust"), &HelgaPropulsion::get_total_thrust);
    ClassDB::bind_method(D_METHOD("get_power_reserve"), &HelgaPropulsion::get_power_reserve);
    ClassDB::bind_method(D_METHOD("is_afterburner_active"), &HelgaPropulsion::is_afterburner_active);

    ClassDB::bind_method(D_METHOD("get_abep_max_thrust_newtons"), &HelgaPropulsion::get_abep_max_thrust_newtons);
    ClassDB::bind_method(D_METHOD("set_abep_max_thrust_newtons", "value"), &HelgaPropulsion::set_abep_max_thrust_newtons);
    ClassDB::bind_method(D_METHOD("get_mhd_max_thrust_newtons"), &HelgaPropulsion::get_mhd_max_thrust_newtons);
    ClassDB::bind_method(D_METHOD("set_mhd_max_thrust_newtons", "value"), &HelgaPropulsion::set_mhd_max_thrust_newtons);
    ClassDB::bind_method(D_METHOD("get_il_afterburner_max_thrust_newtons"), &HelgaPropulsion::get_il_afterburner_max_thrust_newtons);
    ClassDB::bind_method(D_METHOD("set_il_afterburner_max_thrust_newtons", "value"), &HelgaPropulsion::set_il_afterburner_max_thrust_newtons);
    ClassDB::bind_method(D_METHOD("get_solar_wind_max_thrust_newtons"), &HelgaPropulsion::get_solar_wind_max_thrust_newtons);
    ClassDB::bind_method(D_METHOD("set_solar_wind_max_thrust_newtons", "value"), &HelgaPropulsion::set_solar_wind_max_thrust_newtons);
    ClassDB::bind_method(D_METHOD("get_afterburner_throttle_threshold"), &HelgaPropulsion::get_afterburner_throttle_threshold);
    ClassDB::bind_method(D_METHOD("set_afterburner_throttle_threshold", "value"), &HelgaPropulsion::set_afterburner_throttle_threshold);
    ClassDB::bind_method(D_METHOD("get_atmosphere_scale_height"), &HelgaPropulsion::get_atmosphere_scale_height);
    ClassDB::bind_method(D_METHOD("set_atmosphere_scale_height", "value"), &HelgaPropulsion::set_atmosphere_scale_height);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "throttle", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_throttle", "get_throttle");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "abep_max_thrust_newtons"), "set_abep_max_thrust_newtons", "get_abep_max_thrust_newtons");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mhd_max_thrust_newtons"), "set_mhd_max_thrust_newtons", "get_mhd_max_thrust_newtons");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "il_afterburner_max_thrust_newtons"), "set_il_afterburner_max_thrust_newtons", "get_il_afterburner_max_thrust_newtons");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "solar_wind_max_thrust_newtons"), "set_solar_wind_max_thrust_newtons", "get_solar_wind_max_thrust_newtons");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "afterburner_throttle_threshold", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_afterburner_throttle_threshold", "get_afterburner_throttle_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "atmosphere_scale_height"), "set_atmosphere_scale_height", "get_atmosphere_scale_height");
}
