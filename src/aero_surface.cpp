#include "aero_surface.h"

#include <godot_cpp/core/class_db.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace {
constexpr double DEG_TO_RAD = 0.017453292519943295;

double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}
}

HelgaAeroSurface::HelgaAeroSurface() {}

HelgaAeroSurface::~HelgaAeroSurface() {}

Vector3 HelgaAeroSurface::compute_force(const Vector3 &local_wind, double air_density, double control_input) const {
    double airspeed = local_wind.length();
    if (airspeed < 0.01 || air_density <= 0.0) {
        return Vector3();
    }

    // Angle of attack in this surface's own local vertical plane.
    // Godot convention: forward = -Z, up = +Y, so the forward and
    // vertical components of the relative wind are -local_wind.z and
    // -local_wind.y respectively (relative wind points opposite the
    // surface's travel through the air). Mounting incidence and control
    // deflection both act as a direct shift of the effective AoA -- a
    // physically reasonable simplification for a plain flap/elevon
    // covering part of the chord.
    double geometric_alpha = std::atan2(-local_wind.y, -local_wind.z);
    double effective_alpha = geometric_alpha
        + incidence_deg * DEG_TO_RAD
        + control_input * control_effectiveness;

    double stall_rad = stall_angle_deg * DEG_TO_RAD;
    double abs_alpha = std::fabs(effective_alpha);
    double sign_alpha = effective_alpha < 0.0 ? -1.0 : 1.0;

    double cl;
    double cd;
    if (abs_alpha <= stall_rad) {
        cl = lift_slope_per_rad * effective_alpha;
        cd = parasite_drag_coefficient + induced_drag_factor * cl * cl;
    } else {
        // Post-stall: blend from the linear pre-stall value toward flat-
        // plate behavior (Cl = sin(2a), Cd = 1 - cos(2a)) over
        // stall_blend_range_deg -- this is what makes a stall a sudden
        // lift loss / drag rise instead of lift climbing forever.
        double cl_at_stall = lift_slope_per_rad * stall_rad * sign_alpha;
        double cd_at_stall = parasite_drag_coefficient + induced_drag_factor * cl_at_stall * cl_at_stall;
        double flat_plate_cl = std::sin(2.0 * effective_alpha);
        double flat_plate_cd = 1.0 - std::cos(2.0 * effective_alpha);

        double blend_range = std::max(stall_blend_range_deg * DEG_TO_RAD, 0.0001);
        double t = std::min(std::max((abs_alpha - stall_rad) / blend_range, 0.0), 1.0);
        cl = lerp(cl_at_stall, flat_plate_cl, t);
        cd = lerp(cd_at_stall, flat_plate_cd, t);
    }

    Vector3 wind_dir = local_wind / airspeed;
    Vector3 drag_dir = -wind_dir;
    Vector3 lift_dir = Vector3(1.0, 0.0, 0.0).cross(wind_dir);
    double lift_dir_len = lift_dir.length();
    lift_dir = lift_dir_len > 0.0001 ? lift_dir / lift_dir_len : Vector3(0.0, 1.0, 0.0);

    double dynamic_pressure = 0.5 * air_density * airspeed * airspeed;
    return lift_dir * (dynamic_pressure * area_m2 * cl) + drag_dir * (dynamic_pressure * area_m2 * cd);
}

void HelgaAeroSurface::_bind_methods() {
    ClassDB::bind_method(D_METHOD("compute_force", "local_wind", "air_density", "control_input"), &HelgaAeroSurface::compute_force);

    ClassDB::bind_method(D_METHOD("get_elevator_gain"), &HelgaAeroSurface::get_elevator_gain);
    ClassDB::bind_method(D_METHOD("set_elevator_gain", "value"), &HelgaAeroSurface::set_elevator_gain);
    ClassDB::bind_method(D_METHOD("get_aileron_gain"), &HelgaAeroSurface::get_aileron_gain);
    ClassDB::bind_method(D_METHOD("set_aileron_gain", "value"), &HelgaAeroSurface::set_aileron_gain);
    ClassDB::bind_method(D_METHOD("get_rudder_gain"), &HelgaAeroSurface::get_rudder_gain);
    ClassDB::bind_method(D_METHOD("set_rudder_gain", "value"), &HelgaAeroSurface::set_rudder_gain);
    ClassDB::bind_method(D_METHOD("get_area"), &HelgaAeroSurface::get_area);
    ClassDB::bind_method(D_METHOD("set_area", "area"), &HelgaAeroSurface::set_area);
    ClassDB::bind_method(D_METHOD("get_lift_slope_per_rad"), &HelgaAeroSurface::get_lift_slope_per_rad);
    ClassDB::bind_method(D_METHOD("set_lift_slope_per_rad", "value"), &HelgaAeroSurface::set_lift_slope_per_rad);
    ClassDB::bind_method(D_METHOD("get_stall_angle_deg"), &HelgaAeroSurface::get_stall_angle_deg);
    ClassDB::bind_method(D_METHOD("set_stall_angle_deg", "value"), &HelgaAeroSurface::set_stall_angle_deg);
    ClassDB::bind_method(D_METHOD("get_stall_blend_range_deg"), &HelgaAeroSurface::get_stall_blend_range_deg);
    ClassDB::bind_method(D_METHOD("set_stall_blend_range_deg", "value"), &HelgaAeroSurface::set_stall_blend_range_deg);
    ClassDB::bind_method(D_METHOD("get_parasite_drag_coefficient"), &HelgaAeroSurface::get_parasite_drag_coefficient);
    ClassDB::bind_method(D_METHOD("set_parasite_drag_coefficient", "value"), &HelgaAeroSurface::set_parasite_drag_coefficient);
    ClassDB::bind_method(D_METHOD("get_induced_drag_factor"), &HelgaAeroSurface::get_induced_drag_factor);
    ClassDB::bind_method(D_METHOD("set_induced_drag_factor", "value"), &HelgaAeroSurface::set_induced_drag_factor);
    ClassDB::bind_method(D_METHOD("get_incidence_deg"), &HelgaAeroSurface::get_incidence_deg);
    ClassDB::bind_method(D_METHOD("set_incidence_deg", "value"), &HelgaAeroSurface::set_incidence_deg);
    ClassDB::bind_method(D_METHOD("get_control_effectiveness"), &HelgaAeroSurface::get_control_effectiveness);
    ClassDB::bind_method(D_METHOD("set_control_effectiveness", "value"), &HelgaAeroSurface::set_control_effectiveness);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "elevator_gain", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_elevator_gain", "get_elevator_gain");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "aileron_gain", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_aileron_gain", "get_aileron_gain");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rudder_gain", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_rudder_gain", "get_rudder_gain");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "area"), "set_area", "get_area");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lift_slope_per_rad"), "set_lift_slope_per_rad", "get_lift_slope_per_rad");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "stall_angle_deg"), "set_stall_angle_deg", "get_stall_angle_deg");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "stall_blend_range_deg"), "set_stall_blend_range_deg", "get_stall_blend_range_deg");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "parasite_drag_coefficient"), "set_parasite_drag_coefficient", "get_parasite_drag_coefficient");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "induced_drag_factor"), "set_induced_drag_factor", "get_induced_drag_factor");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "incidence_deg"), "set_incidence_deg", "get_incidence_deg");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "control_effectiveness"), "set_control_effectiveness", "get_control_effectiveness");
}
