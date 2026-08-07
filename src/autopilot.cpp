#include "autopilot.h"

#include <godot_cpp/core/class_db.hpp>

#include <algorithm>

using namespace godot;

HelgaAutopilot::HelgaAutopilot() {}

HelgaAutopilot::~HelgaAutopilot() {}

double HelgaAutopilot::compute_elevator(double current_flight_path_angle_deg) {
    double error = target_flight_path_angle_deg - current_flight_path_angle_deg;
    commanded_elevator = std::max(-1.0, std::min(1.0, pitch_gain * error));
    return commanded_elevator;
}

void HelgaAutopilot::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_engaged", "value"), &HelgaAutopilot::set_engaged);
    ClassDB::bind_method(D_METHOD("is_engaged"), &HelgaAutopilot::is_engaged);
    ClassDB::bind_method(D_METHOD("set_target_flight_path_angle_deg", "value"), &HelgaAutopilot::set_target_flight_path_angle_deg);
    ClassDB::bind_method(D_METHOD("get_target_flight_path_angle_deg"), &HelgaAutopilot::get_target_flight_path_angle_deg);
    ClassDB::bind_method(D_METHOD("compute_elevator", "current_flight_path_angle_deg"), &HelgaAutopilot::compute_elevator);
    ClassDB::bind_method(D_METHOD("get_commanded_elevator"), &HelgaAutopilot::get_commanded_elevator);
    ClassDB::bind_method(D_METHOD("get_pitch_gain"), &HelgaAutopilot::get_pitch_gain);
    ClassDB::bind_method(D_METHOD("set_pitch_gain", "value"), &HelgaAutopilot::set_pitch_gain);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "engaged"), "set_engaged", "is_engaged");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "target_flight_path_angle_deg"), "set_target_flight_path_angle_deg", "get_target_flight_path_angle_deg");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch_gain"), "set_pitch_gain", "get_pitch_gain");
}
