#ifndef HELGA_TERRAIN_H
#define HELGA_TERRAIN_H

// HelgaTerrain -- procedurally generated ground terrain that follows the
// aircraft, giving low-altitude flight (taxi/takeoff/climb/approach/
// landing) real relief to look at and fly over instead of the flat green
// plane main.tscn shipped with before this. This is the "complete C++
// procedurally generated world" the user asked for -- deliberately scoped
// to terrain relief + collision, not a full planet: the sim's coordinate
// system stays the flat-plane-not-geocentric model documented in
// gravity.h, so "the world" here means "the ground looks and collides
// like real terrain wherever the aircraft flies," not a rendered sphere.
//
// A fixed-size grid of square chunks (see grid_radius/chunk_size) is kept
// centered on a follow target (the aircraft) in whole-chunk steps: each
// physics frame checks which chunk cell the target is in, and only when
// that cell changes does the whole grid regenerate around the new center.
// This is a full-grid regenerate, not a ring/clipmap update -- simpler to
// get right, and cheap enough at this chunk count/resolution not to
// hitch, but a real clipmap would be the next step if chunk_size/
// grid_radius ever need to grow much from their current tuning.
//
// Height comes from a single FastNoiseLite field sampled in *world*
// space (not chunk-local), so adjacent chunks' shared edge vertices
// always agree exactly -- no seams. That raw noise is then blended
// toward exactly zero within flatten_radius_m of the world origin (where
// main.tscn's existing flat "Ground" plane and the aircraft's spawn/
// runway sit), ramping up to full relief by flatten_radius_m +
// flatten_blend_m out. This is what keeps HelgaFlightComputer's altitude
// guard conditions (which read the aircraft's raw world-Y position, see
// aircraft_control.gd) and the gear-contact physics honest: the ground
// under the runway is still exactly Y=0, same as before terrain existed.
//
// Hidden entirely above hide_above_altitude_m -- from orbital altitude,
// individual terrain relief isn't visible or meaningful anyway (rendering
// a curved-Earth view from orbit is a separate, much bigger art task, out
// of scope here per docs/DESIGN.md's "top-tier visuals are separate,
// additional work" note), and skipping regeneration work up there is a
// real perf win during ascent/orbit/reentry.
//
// Each chunk gets its own StaticBody3D + CollisionShape3D built straight
// from the visual mesh via Mesh::create_trimesh_shape(), so collision
// always matches what's rendered exactly, with no separate heightmap-
// sampling/scaling code path to keep in sync.

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <vector>

namespace godot {

class HelgaTerrain : public Node3D {
    GDCLASS(HelgaTerrain, Node3D)

private:
    struct Chunk {
        MeshInstance3D *mesh_instance = nullptr;
        StaticBody3D *body = nullptr;
        CollisionShape3D *collision = nullptr;
    };

    NodePath follow_target_path;
    Node3D *follow_target = nullptr;

    Ref<FastNoiseLite> noise;
    Ref<StandardMaterial3D> material;

    std::vector<Chunk> chunks;
    Vector2i center_cell = Vector2i(0, 0);
    bool grid_built = false;

    int grid_radius = 4;               // chunks out from center in each direction
    double chunk_size = 800.0;         // meters per chunk edge
    int subdivisions = 4;              // quads per chunk edge (5x5 verts)
    double height_amplitude = 260.0;   // meters, peak relief
    double noise_frequency = 1.0 / 1800.0;
    double flatten_radius_m = 1200.0;  // fully flat within this radius of world origin
    double flatten_blend_m = 800.0;    // ramps to full relief over this additional distance
    double hide_above_altitude_m = 30000.0;

    double height_at(double world_x, double world_z) const;
    void build_chunk(Chunk &chunk, const Vector2i &cell);
    void rebuild_grid();

protected:
    static void _bind_methods();

public:
    HelgaTerrain();
    ~HelgaTerrain() override;

    void _ready() override;
    void _physics_process(double p_delta) override;

    NodePath get_follow_target_path() const { return follow_target_path; }
    void set_follow_target_path(const NodePath &p_path) { follow_target_path = p_path; }

    int get_grid_radius() const { return grid_radius; }
    void set_grid_radius(int v) { grid_radius = v; }
    double get_chunk_size() const { return chunk_size; }
    void set_chunk_size(double v) { chunk_size = v; }
    int get_subdivisions() const { return subdivisions; }
    void set_subdivisions(int v) { subdivisions = v; }
    double get_height_amplitude() const { return height_amplitude; }
    void set_height_amplitude(double v) { height_amplitude = v; }
    double get_flatten_radius_m() const { return flatten_radius_m; }
    void set_flatten_radius_m(double v) { flatten_radius_m = v; }
    double get_flatten_blend_m() const { return flatten_blend_m; }
    void set_flatten_blend_m(double v) { flatten_blend_m = v; }
    double get_hide_above_altitude_m() const { return hide_above_altitude_m; }
    void set_hide_above_altitude_m(double v) { hide_above_altitude_m = v; }

    // Exposed for test harnesses / debug tooling -- not used by any
    // gameplay system (see the FSM-altitude note above, this is
    // deliberately never fed back into physics/FSM).
    double get_height_at(double world_x, double world_z) const { return height_at(world_x, world_z); }
};

}

#endif // HELGA_TERRAIN_H
