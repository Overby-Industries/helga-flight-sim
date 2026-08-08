#ifndef HELGA_ATMOSPHERE_H
#define HELGA_ATMOSPHERE_H

// HelgaAtmosphere -- altitude-driven sky/space transition, plus a
// procedural starfield. Before this the sim had no WorldEnvironment/sky
// at all, so nothing visually distinguished sitting on the runway from
// being in orbit -- a real gap given the whole point of the sim (per the
// pilot) is testing ion-thruster and re-entry approach maneuvers from
// LEO/MEO. This closes that gap with a lightweight altitude-keyed
// interpolation, not a physically simulated atmosphere -- real Rayleigh-
// scattering-accurate sky is Godot's PhysicalSkyMaterial's job and a
// heavier art task, out of scope per docs/DESIGN.md's "top-tier visuals
// are separate, additional work" note. This uses the simpler
// ProceduralSkyMaterial, interpolating sky/ground colors and energy
// between a day-sky palette and near-black "space" as altitude crosses
// space_transition_start_m..space_transition_end_m.
//
// Subclasses WorldEnvironment directly (rather than owning one as a
// sibling) so it drops straight into the scene wherever a WorldEnvironment
// normally goes, while still following the sim's sibling-node pattern to
// reach the aircraft (follow_target_path) and read its altitude each
// physics frame.
//
// The starfield is a MultiMeshInstance3D of small billboard quads spread
// over a fixed-radius sphere, re-centered on the follow target's position
// every frame -- the standard "skybox" trick, always the same apparent
// distance regardless of where the aircraft actually is -- and faded in
// via material alpha as the sky darkens rather than toggled on/off.

#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/procedural_sky_material.hpp>
#include <godot_cpp/classes/sky.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace godot {

class HelgaAtmosphere : public WorldEnvironment {
    GDCLASS(HelgaAtmosphere, WorldEnvironment)

private:
    NodePath follow_target_path;
    Node3D *follow_target = nullptr;

    Ref<Environment> environment_res;
    Ref<ProceduralSkyMaterial> sky_material;
    Ref<Sky> sky;

    MultiMeshInstance3D *star_field = nullptr;
    Ref<MultiMesh> star_multimesh;
    Ref<StandardMaterial3D> star_material;

    double space_transition_start_m = 20000.0;
    double space_transition_end_m = 100000.0;
    int star_count = 700;
    double star_field_radius_m = 3000.0;

    double space_fraction = 0.0; // 0 = full day sky, 1 = full space

protected:
    static void _bind_methods();

public:
    HelgaAtmosphere();
    ~HelgaAtmosphere() override;

    void _ready() override;
    void _physics_process(double p_delta) override;

    NodePath get_follow_target_path() const { return follow_target_path; }
    void set_follow_target_path(const NodePath &p_path) { follow_target_path = p_path; }

    double get_space_transition_start_m() const { return space_transition_start_m; }
    void set_space_transition_start_m(double v) { space_transition_start_m = v; }
    double get_space_transition_end_m() const { return space_transition_end_m; }
    void set_space_transition_end_m(double v) { space_transition_end_m = v; }

    // 0 at/below space_transition_start_m, 1 at/above space_transition_end_m.
    double get_space_fraction() const { return space_fraction; }
};

}

#endif // HELGA_ATMOSPHERE_H
