#include "reentry_system.h"
#include "aerodynamics.h"
#include "flight_computer.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace {
constexpr double RAD_TO_DEG = 57.29577951308232;
}

HelgaReentrySystem::HelgaReentrySystem() {}

HelgaReentrySystem::~HelgaReentrySystem() {}

void HelgaReentrySystem::_ready() {
    body = Object::cast_to<RigidBody3D>(get_parent());
    aerodynamics = Object::cast_to<HelgaAerodynamics>(get_node_or_null(NodePath("../Aerodynamics")));
    flight_computer = Object::cast_to<HelgaFlightComputer>(get_node_or_null(NodePath("../FlightComputer")));
}

void HelgaReentrySystem::update_readings() {
    if (body == nullptr || aerodynamics == nullptr) {
        return;
    }

    double density = aerodynamics->get_air_density();
    double airspeed = aerodynamics->get_airspeed();

    dynamic_pressure_pa = 0.5 * density * airspeed * airspeed;
    heat_flux_w_m2 = heat_flux_coefficient * std::sqrt(std::max(density, 0.0)) * airspeed * airspeed * airspeed;

    Vector3 velocity = body->get_linear_velocity();
    double horizontal_speed = std::sqrt(static_cast<double>(velocity.x) * velocity.x + static_cast<double>(velocity.z) * velocity.z);
    flight_path_angle_deg = std::atan2(static_cast<double>(-velocity.y), horizontal_speed) * RAD_TO_DEG;

    double steep_excess = std::max(0.0, get_heat_flux_ratio() - 1.0);
    double shallow_deficit = 0.0;
    if (airspeed > skip_out_airspeed_ms) {
        shallow_deficit = std::max(0.0, (shallow_angle_deg - flight_path_angle_deg) / shallow_angle_deg);
    }
    corridor_deviation = steep_excess + shallow_deficit;
}

void HelgaReentrySystem::evaluate_contingencies(double p_delta) {
    if (flight_computer == nullptr || aerodynamics == nullptr || body == nullptr) {
        return;
    }

    int state = flight_computer->get_current_state();
    bool armed = state == REENTRY || state == STATE_SHALLOW_REENTRY_CONTINGENCY || state == STATE_THERMAL_OVERLOAD;
    if (!armed) {
        thermal_excursion_seconds = 0.0;
        shallow_excursion_seconds = 0.0;
        shallow_contingency_seconds = 0.0;
        return;
    }

    double airspeed = aerodynamics->get_airspeed();

    // Thermal axis -- too steep. Skipped while riding out the shallow
    // contingency so the two axes never fight over a transition in the
    // same frame; see the class comment.
    if (state != STATE_SHALLOW_REENTRY_CONTINGENCY) {
        if (heat_flux_w_m2 > heat_flux_critical_w_m2) {
            thermal_excursion_seconds += p_delta;
        } else {
            thermal_excursion_seconds = 0.0;
            if (state == STATE_THERMAL_OVERLOAD) {
                flight_computer->transition_to(REENTRY);
            }
        }

        if (state == REENTRY && thermal_excursion_seconds > thermal_overload_grace_seconds) {
            flight_computer->transition_to(STATE_THERMAL_OVERLOAD);
        } else if (state == STATE_THERMAL_OVERLOAD && thermal_excursion_seconds > thermal_catastrophic_seconds) {
            flight_computer->transition_to(FAULT);
        }
    }

    // Shallow axis -- too flat, at risk of skipping out. Skipped while
    // riding out the thermal overload for the same reason as above.
    if (state != STATE_THERMAL_OVERLOAD) {
        bool shallow_now = flight_path_angle_deg < shallow_angle_deg && airspeed > skip_out_airspeed_ms;
        if (shallow_now) {
            shallow_excursion_seconds += p_delta;
        } else {
            shallow_excursion_seconds = 0.0;
        }

        if (state == REENTRY && shallow_excursion_seconds > skip_out_grace_seconds) {
            flight_computer->transition_to(STATE_SHALLOW_REENTRY_CONTINGENCY);
        } else if (state == STATE_SHALLOW_REENTRY_CONTINGENCY) {
            shallow_contingency_seconds += p_delta;
            double vertical_speed = body->get_linear_velocity().y;
            if (vertical_speed > skip_out_recovery_vspeed_ms) {
                // Bounced back upward -- skipped out, needs another attempt.
                flight_computer->transition_to(ORBIT);
                shallow_contingency_seconds = 0.0;
            } else if (!shallow_now) {
                // Corrected back into a proper descent angle.
                flight_computer->transition_to(REENTRY);
                shallow_contingency_seconds = 0.0;
            } else if (shallow_contingency_seconds > shallow_contingency_fault_seconds) {
                flight_computer->transition_to(FAULT);
            }
        }
    }
}

void HelgaReentrySystem::_physics_process(double p_delta) {
    update_readings();
    evaluate_contingencies(p_delta);
}

void HelgaReentrySystem::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_dynamic_pressure_pa"), &HelgaReentrySystem::get_dynamic_pressure_pa);
    ClassDB::bind_method(D_METHOD("get_heat_flux_w_m2"), &HelgaReentrySystem::get_heat_flux_w_m2);
    ClassDB::bind_method(D_METHOD("get_flight_path_angle_deg"), &HelgaReentrySystem::get_flight_path_angle_deg);
    ClassDB::bind_method(D_METHOD("get_corridor_deviation"), &HelgaReentrySystem::get_corridor_deviation);
    ClassDB::bind_method(D_METHOD("get_heat_flux_ratio"), &HelgaReentrySystem::get_heat_flux_ratio);

    ClassDB::bind_method(D_METHOD("get_heat_flux_coefficient"), &HelgaReentrySystem::get_heat_flux_coefficient);
    ClassDB::bind_method(D_METHOD("set_heat_flux_coefficient", "value"), &HelgaReentrySystem::set_heat_flux_coefficient);
    ClassDB::bind_method(D_METHOD("get_heat_flux_critical_w_m2"), &HelgaReentrySystem::get_heat_flux_critical_w_m2);
    ClassDB::bind_method(D_METHOD("set_heat_flux_critical_w_m2", "value"), &HelgaReentrySystem::set_heat_flux_critical_w_m2);
    ClassDB::bind_method(D_METHOD("get_thermal_overload_grace_seconds"), &HelgaReentrySystem::get_thermal_overload_grace_seconds);
    ClassDB::bind_method(D_METHOD("set_thermal_overload_grace_seconds", "value"), &HelgaReentrySystem::set_thermal_overload_grace_seconds);
    ClassDB::bind_method(D_METHOD("get_thermal_catastrophic_seconds"), &HelgaReentrySystem::get_thermal_catastrophic_seconds);
    ClassDB::bind_method(D_METHOD("set_thermal_catastrophic_seconds", "value"), &HelgaReentrySystem::set_thermal_catastrophic_seconds);

    ClassDB::bind_method(D_METHOD("get_shallow_angle_deg"), &HelgaReentrySystem::get_shallow_angle_deg);
    ClassDB::bind_method(D_METHOD("set_shallow_angle_deg", "value"), &HelgaReentrySystem::set_shallow_angle_deg);
    ClassDB::bind_method(D_METHOD("get_skip_out_airspeed_ms"), &HelgaReentrySystem::get_skip_out_airspeed_ms);
    ClassDB::bind_method(D_METHOD("set_skip_out_airspeed_ms", "value"), &HelgaReentrySystem::set_skip_out_airspeed_ms);
    ClassDB::bind_method(D_METHOD("get_skip_out_grace_seconds"), &HelgaReentrySystem::get_skip_out_grace_seconds);
    ClassDB::bind_method(D_METHOD("set_skip_out_grace_seconds", "value"), &HelgaReentrySystem::set_skip_out_grace_seconds);
    ClassDB::bind_method(D_METHOD("get_skip_out_recovery_vspeed_ms"), &HelgaReentrySystem::get_skip_out_recovery_vspeed_ms);
    ClassDB::bind_method(D_METHOD("set_skip_out_recovery_vspeed_ms", "value"), &HelgaReentrySystem::set_skip_out_recovery_vspeed_ms);
    ClassDB::bind_method(D_METHOD("get_shallow_contingency_fault_seconds"), &HelgaReentrySystem::get_shallow_contingency_fault_seconds);
    ClassDB::bind_method(D_METHOD("set_shallow_contingency_fault_seconds", "value"), &HelgaReentrySystem::set_shallow_contingency_fault_seconds);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "heat_flux_coefficient"), "set_heat_flux_coefficient", "get_heat_flux_coefficient");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "heat_flux_critical_w_m2"), "set_heat_flux_critical_w_m2", "get_heat_flux_critical_w_m2");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "thermal_overload_grace_seconds"), "set_thermal_overload_grace_seconds", "get_thermal_overload_grace_seconds");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "thermal_catastrophic_seconds"), "set_thermal_catastrophic_seconds", "get_thermal_catastrophic_seconds");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "shallow_angle_deg"), "set_shallow_angle_deg", "get_shallow_angle_deg");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "skip_out_airspeed_ms"), "set_skip_out_airspeed_ms", "get_skip_out_airspeed_ms");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "skip_out_grace_seconds"), "set_skip_out_grace_seconds", "get_skip_out_grace_seconds");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "skip_out_recovery_vspeed_ms"), "set_skip_out_recovery_vspeed_ms", "get_skip_out_recovery_vspeed_ms");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "shallow_contingency_fault_seconds"), "set_shallow_contingency_fault_seconds", "get_shallow_contingency_fault_seconds");
}
