#include "atmosphere.h"

#include "godot_cpp/classes/quad_mesh.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <cmath>
#include <random>

using namespace godot;

namespace {
double smoothstep01(double t) {
    t = t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t);
    return t * t * (3.0 - 2.0 * t);
}

Color lerp_color(const Color &a, const Color &b, double t) {
    return Color(
        static_cast<float>(a.r + (b.r - a.r) * t),
        static_cast<float>(a.g + (b.g - a.g) * t),
        static_cast<float>(a.b + (b.b - a.b) * t),
        static_cast<float>(a.a + (b.a - a.a) * t));
}
}

HelgaAtmosphere::HelgaAtmosphere() {}

HelgaAtmosphere::~HelgaAtmosphere() {}

void HelgaAtmosphere::_ready() {
    follow_target = Object::cast_to<Node3D>(get_node_or_null(follow_target_path));

    sky_material.instantiate();
    sky.instantiate();
    sky->set_material(sky_material);

    environment_res.instantiate();
    environment_res->set_background(Environment::BG_SKY);
    environment_res->set_sky(sky);
    environment_res->set_ambient_source(Environment::AMBIENT_SOURCE_SKY);
    environment_res->set_fog_enabled(true);
    environment_res->set_fog_light_color(Color(0.75f, 0.82f, 0.9f));
    environment_res->set_fog_density(0.0008f);
    set_environment(environment_res);

    star_material.instantiate();
    star_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
    star_material->set_billboard_mode(BaseMaterial3D::BILLBOARD_ENABLED);
    star_material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
    star_material->set_albedo(Color(1.0f, 1.0f, 1.0f, 0.0f));

    Ref<QuadMesh> star_quad;
    star_quad.instantiate();
    star_quad->set_size(Vector2(6.0f, 6.0f));

    star_multimesh.instantiate();
    star_multimesh->set_transform_format(MultiMesh::TRANSFORM_3D);
    star_multimesh->set_use_colors(false);
    star_multimesh->set_mesh(star_quad);
    star_multimesh->set_instance_count(star_count);

    std::mt19937 rng(1337);
    std::uniform_real_distribution<double> theta_dist(0.0, 6.283185307179586);
    std::uniform_real_distribution<double> cos_phi_dist(-1.0, 1.0);
    for (int i = 0; i < star_count; ++i) {
        double theta = theta_dist(rng);
        double cos_phi = cos_phi_dist(rng);
        double sin_phi = std::sqrt(std::max(0.0, 1.0 - cos_phi * cos_phi));
        Vector3 dir(
            static_cast<real_t>(sin_phi * std::cos(theta)),
            static_cast<real_t>(cos_phi),
            static_cast<real_t>(sin_phi * std::sin(theta)));
        star_multimesh->set_instance_transform(i, Transform3D(Basis(), dir * static_cast<real_t>(star_field_radius_m)));
    }

    star_field = memnew(MultiMeshInstance3D);
    star_field->set_multimesh(star_multimesh);
    star_field->set_material_override(star_material);
    add_child(star_field);
}

void HelgaAtmosphere::_physics_process(double p_delta) {
    (void)p_delta;
    if (follow_target == nullptr) {
        return;
    }

    Vector3 pos = follow_target->get_global_transform().origin;
    double altitude = static_cast<double>(pos.y);

    double span = space_transition_end_m - space_transition_start_m;
    double t;
    if (span > 0.0) {
        t = smoothstep01((altitude - space_transition_start_m) / span);
    } else {
        t = altitude > space_transition_start_m ? 1.0 : 0.0;
    }
    space_fraction = t;

    static const Color DAY_SKY_TOP(0.30f, 0.55f, 0.85f);
    static const Color DAY_SKY_HORIZON(0.75f, 0.85f, 0.95f);
    static const Color DAY_GROUND_HORIZON(0.55f, 0.55f, 0.55f);
    static const Color DAY_GROUND_BOTTOM(0.35f, 0.35f, 0.35f);
    static const Color SPACE_SKY_TOP(0.01f, 0.01f, 0.03f);
    static const Color SPACE_SKY_HORIZON(0.05f, 0.05f, 0.10f);
    static const Color SPACE_GROUND(0.0f, 0.0f, 0.0f);

    sky_material->set_sky_top_color(lerp_color(DAY_SKY_TOP, SPACE_SKY_TOP, t));
    sky_material->set_sky_horizon_color(lerp_color(DAY_SKY_HORIZON, SPACE_SKY_HORIZON, t));
    sky_material->set_ground_horizon_color(lerp_color(DAY_GROUND_HORIZON, SPACE_GROUND, t));
    sky_material->set_ground_bottom_color(lerp_color(DAY_GROUND_BOTTOM, SPACE_GROUND, t));

    float energy = static_cast<float>(1.0 - 0.9 * t);
    sky_material->set_sky_energy_multiplier(energy);
    sky_material->set_ground_energy_multiplier(energy);

    environment_res->set_ambient_light_energy(static_cast<float>(1.0 - 0.85 * t));
    environment_res->set_fog_density(static_cast<float>(0.0008 * (1.0 - t)));

    star_material->set_albedo(Color(1.0f, 1.0f, 1.0f, static_cast<float>(t)));
    star_field->set_global_position(pos);
}

void HelgaAtmosphere::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_follow_target_path"), &HelgaAtmosphere::get_follow_target_path);
    ClassDB::bind_method(D_METHOD("set_follow_target_path", "path"), &HelgaAtmosphere::set_follow_target_path);
    ClassDB::bind_method(D_METHOD("get_space_transition_start_m"), &HelgaAtmosphere::get_space_transition_start_m);
    ClassDB::bind_method(D_METHOD("set_space_transition_start_m", "value"), &HelgaAtmosphere::set_space_transition_start_m);
    ClassDB::bind_method(D_METHOD("get_space_transition_end_m"), &HelgaAtmosphere::get_space_transition_end_m);
    ClassDB::bind_method(D_METHOD("set_space_transition_end_m", "value"), &HelgaAtmosphere::set_space_transition_end_m);
    ClassDB::bind_method(D_METHOD("get_space_fraction"), &HelgaAtmosphere::get_space_fraction);

    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "follow_target_path"), "set_follow_target_path", "get_follow_target_path");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "space_transition_start_m"), "set_space_transition_start_m", "get_space_transition_start_m");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "space_transition_end_m"), "set_space_transition_end_m", "get_space_transition_end_m");
}
