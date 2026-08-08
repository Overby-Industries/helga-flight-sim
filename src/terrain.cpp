#include "terrain.h"

#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cmath>

using namespace godot;

namespace {
double smoothstep01(double t) {
    t = t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t);
    return t * t * (3.0 - 2.0 * t);
}

// Posterized dark/base/highlight/peak bands, echoing the palette
// approach docs/DESIGN.md describes for the ported ProceduralArtGenerator
// texturing tool -- the "base" band deliberately matches main.tscn's
// existing flat Ground plane's albedo so the flattened zone (where both
// coexist) reads as one continuous surface.
Color color_for_height(double h) {
    if (h < -20.0) return Color(0.20, 0.32, 0.22);
    if (h < 80.0) return Color(0.30, 0.45, 0.28);
    if (h < 180.0) return Color(0.42, 0.38, 0.30);
    return Color(0.78, 0.78, 0.80);
}
}

HelgaTerrain::HelgaTerrain() {}

HelgaTerrain::~HelgaTerrain() {}

double HelgaTerrain::height_at(double world_x, double world_z) const {
    if (noise.is_null()) {
        return 0.0;
    }
    double r = std::sqrt(world_x * world_x + world_z * world_z);
    double blend;
    if (r <= flatten_radius_m) {
        blend = 0.0;
    } else if (r >= flatten_radius_m + flatten_blend_m) {
        blend = 1.0;
    } else {
        blend = smoothstep01((r - flatten_radius_m) / flatten_blend_m);
    }
    double raw = static_cast<double>(noise->get_noise_2d(static_cast<float>(world_x), static_cast<float>(world_z)));
    return raw * height_amplitude * blend;
}

void HelgaTerrain::build_chunk(Chunk &chunk, const Vector2i &cell) {
    double origin_x = static_cast<double>(cell.x) * chunk_size;
    double origin_z = static_cast<double>(cell.y) * chunk_size;
    double step = chunk_size / static_cast<double>(subdivisions);
    int stride = subdivisions + 1;

    Ref<SurfaceTool> st;
    st.instantiate();
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    for (int j = 0; j < stride; ++j) {
        for (int i = 0; i < stride; ++i) {
            double wx = origin_x + static_cast<double>(i) * step;
            double wz = origin_z + static_cast<double>(j) * step;
            double h = height_at(wx, wz);
            st->set_color(color_for_height(h));
            st->add_vertex(Vector3(static_cast<real_t>(i) * static_cast<real_t>(step),
                                    static_cast<real_t>(h),
                                    static_cast<real_t>(j) * static_cast<real_t>(step)));
        }
    }

    // Winding verified to produce an upward (+Y) normal for a Y-up grid:
    // for corners a=(i,j) b=(i+1,j) c=(i,j+1) d=(i+1,j+1), (a,c,b) and
    // (b,c,d) both give edge1 x edge2 = (0,1,0).
    for (int j = 0; j < subdivisions; ++j) {
        for (int i = 0; i < subdivisions; ++i) {
            int a = j * stride + i;
            int b = a + 1;
            int c = a + stride;
            int d = c + 1;
            st->add_index(a);
            st->add_index(c);
            st->add_index(b);
            st->add_index(b);
            st->add_index(c);
            st->add_index(d);
        }
    }

    st->generate_normals();
    Ref<ArrayMesh> mesh = st->commit();

    chunk.mesh_instance->set_mesh(mesh);
    chunk.mesh_instance->set_position(Vector3(static_cast<real_t>(origin_x), 0.0, static_cast<real_t>(origin_z)));
    chunk.body->set_position(Vector3(static_cast<real_t>(origin_x), 0.0, static_cast<real_t>(origin_z)));

    Ref<ConcavePolygonShape3D> trimesh_shape = mesh->create_trimesh_shape();
    // The physics engine treats a concave trimesh as single-sided by
    // default (collidable only from the winding's front face). Terrain
    // should stop the aircraft from any approach angle -- a botched
    // landing or an odd attitude near a hillside shouldn't clip through
    // from the "back" -- so this is enabled explicitly rather than
    // relying on getting triangle winding exactly right.
    trimesh_shape->set_backface_collision_enabled(true);
    chunk.collision->set_shape(trimesh_shape);
}

void HelgaTerrain::rebuild_grid() {
    int span = grid_radius * 2 + 1;
    for (int j = 0; j < span; ++j) {
        for (int i = 0; i < span; ++i) {
            Vector2i cell = center_cell + Vector2i(i - grid_radius, j - grid_radius);
            build_chunk(chunks[static_cast<size_t>(j * span + i)], cell);
        }
    }
}

void HelgaTerrain::_ready() {
    follow_target = Object::cast_to<Node3D>(get_node_or_null(follow_target_path));

    noise.instantiate();
    noise->set_noise_type(FastNoiseLite::TYPE_SIMPLEX_SMOOTH);
    noise->set_fractal_type(FastNoiseLite::FRACTAL_FBM);
    noise->set_fractal_octaves(3);
    noise->set_frequency(static_cast<float>(noise_frequency));
    noise->set_seed(1);

    material.instantiate();
    material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
    // Cull disabled rather than hand-verified winding trusted alone --
    // same defensive choice src/airframe.h documents for its own
    // procedural mesh: correctness of shape/collision is the goal here,
    // not polish, and this can't be visually confirmed in this dev
    // environment.
    material->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
    material->set_roughness(0.95);

    int span = grid_radius * 2 + 1;
    chunks.resize(static_cast<size_t>(span * span));
    for (auto &chunk : chunks) {
        chunk.mesh_instance = memnew(MeshInstance3D);
        add_child(chunk.mesh_instance);
        chunk.mesh_instance->set_material_override(material);

        chunk.body = memnew(StaticBody3D);
        add_child(chunk.body);
        chunk.collision = memnew(CollisionShape3D);
        chunk.body->add_child(chunk.collision);
    }

    grid_built = false;
}

void HelgaTerrain::_physics_process(double p_delta) {
    (void)p_delta;
    if (follow_target == nullptr || chunks.empty()) {
        return;
    }

    Vector3 target_pos = follow_target->get_global_transform().origin;
    bool above_hide_altitude = static_cast<double>(target_pos.y) > hide_above_altitude_m;
    set_visible(!above_hide_altitude);
    if (above_hide_altitude) {
        return;
    }

    Vector2i new_center(
        static_cast<int32_t>(std::floor(static_cast<double>(target_pos.x) / chunk_size)),
        static_cast<int32_t>(std::floor(static_cast<double>(target_pos.z) / chunk_size)));

    if (!grid_built || new_center != center_cell) {
        center_cell = new_center;
        rebuild_grid();
        grid_built = true;
    }
}

void HelgaTerrain::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_follow_target_path"), &HelgaTerrain::get_follow_target_path);
    ClassDB::bind_method(D_METHOD("set_follow_target_path", "path"), &HelgaTerrain::set_follow_target_path);
    ClassDB::bind_method(D_METHOD("get_grid_radius"), &HelgaTerrain::get_grid_radius);
    ClassDB::bind_method(D_METHOD("set_grid_radius", "value"), &HelgaTerrain::set_grid_radius);
    ClassDB::bind_method(D_METHOD("get_chunk_size"), &HelgaTerrain::get_chunk_size);
    ClassDB::bind_method(D_METHOD("set_chunk_size", "value"), &HelgaTerrain::set_chunk_size);
    ClassDB::bind_method(D_METHOD("get_subdivisions"), &HelgaTerrain::get_subdivisions);
    ClassDB::bind_method(D_METHOD("set_subdivisions", "value"), &HelgaTerrain::set_subdivisions);
    ClassDB::bind_method(D_METHOD("get_height_amplitude"), &HelgaTerrain::get_height_amplitude);
    ClassDB::bind_method(D_METHOD("set_height_amplitude", "value"), &HelgaTerrain::set_height_amplitude);
    ClassDB::bind_method(D_METHOD("get_flatten_radius_m"), &HelgaTerrain::get_flatten_radius_m);
    ClassDB::bind_method(D_METHOD("set_flatten_radius_m", "value"), &HelgaTerrain::set_flatten_radius_m);
    ClassDB::bind_method(D_METHOD("get_flatten_blend_m"), &HelgaTerrain::get_flatten_blend_m);
    ClassDB::bind_method(D_METHOD("set_flatten_blend_m", "value"), &HelgaTerrain::set_flatten_blend_m);
    ClassDB::bind_method(D_METHOD("get_hide_above_altitude_m"), &HelgaTerrain::get_hide_above_altitude_m);
    ClassDB::bind_method(D_METHOD("set_hide_above_altitude_m", "value"), &HelgaTerrain::set_hide_above_altitude_m);
    ClassDB::bind_method(D_METHOD("get_height_at", "world_x", "world_z"), &HelgaTerrain::get_height_at);

    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "follow_target_path"), "set_follow_target_path", "get_follow_target_path");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_radius"), "set_grid_radius", "get_grid_radius");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "chunk_size"), "set_chunk_size", "get_chunk_size");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "subdivisions"), "set_subdivisions", "get_subdivisions");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_amplitude"), "set_height_amplitude", "get_height_amplitude");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flatten_radius_m"), "set_flatten_radius_m", "get_flatten_radius_m");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "flatten_blend_m"), "set_flatten_blend_m", "get_flatten_blend_m");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hide_above_altitude_m"), "set_hide_above_altitude_m", "get_hide_above_altitude_m");
}
