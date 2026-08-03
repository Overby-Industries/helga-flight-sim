#ifndef HELGA_AIRFRAME_H
#define HELGA_AIRFRAME_H

// HelgaAirframe -- procedurally builds the SSTO-44 Starlifter II's
// blended-wing-body hull mesh.
//
// Shape and proportions come from two sources: overbyindustries.space/
// aerospace ("blended wing body... double-delta planform with a cranked
// leading edge... canted trailing edges... shallow camber") for the
// qualitative shape, and the pilot's own aircraft-design-tool sketches
// for the real numbers -- 100 ft (30.5 m) span, 60 ft (18.3 m) root
// chord, 0.067 taper ratio, 4.167 aspect ratio, ~35.75 deg root-tip
// sweep, 97,000 lb (~44,000 kg) mass. Default inner/outer sweep angles
// below are chosen so the piecewise leading edge's overall root-to-tip
// slope matches that reported sweep while still cranking noticeably
// steeper inboard, matching the sketched sawtooth planform.
//
// The hull is one continuous lofted surface rather than a separate
// fuselage-plus-wing assembly: 5 spanwise stations
// (-halfspan, -crank_x, 0, +crank_x, +halfspan), where the two inner
// segments share the centerline station, so at span_x=0 the loft's own
// chord *is* the full fuselage (nose to tail) -- "fuselage and wing
// blended into one surface" falls out of the geometry instead of being
// two overlapping meshes glued together.
//
// Rendered with a double-sided material (cull disabled) rather than
// hand-verified triangle winding, since this dev environment can't
// visually confirm face winding -- correctness of *shape*, not polish,
// is the goal of this pass; see docs/DESIGN.md's "make it look good
// later" note.
//
// Hardpoint getters return positions in this node's own local space for
// gear/engine placement -- purely computed from the same shape
// parameters, not scene children, so they carry no node-readiness-order
// dependency on anything else in the scene.

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {

class HelgaAirframe : public MeshInstance3D {
    GDCLASS(HelgaAirframe, MeshInstance3D)

private:
    double fuselage_length = 18.3;          // meters, nose to tail (60 ft root chord)
    double wingspan = 30.5;                 // meters, tip to tip (100 ft)
    double crank_fraction = 0.3;            // fraction of half-span where LE sweep changes
    double inner_sweep_deg = 60.0;          // strake leading-edge sweep, root to crank
    double outer_sweep_deg = 16.0;          // wing-panel leading-edge sweep, crank to tip
    double trailing_edge_cant_deg = 22.0;   // trailing edge sweeps forward toward the tip
    double body_height = 3.0;               // thickness at centerline
    double tip_thickness = 0.3;             // thickness at wingtip
    double camber = 0.3;                    // shallow upward camber offset, meters

    double half_span() const { return wingspan * 0.5; }
    double crank_x() const { return crank_fraction * half_span(); }
    double nose_z() const { return -fuselage_length * 0.5; }
    double tail_z() const { return fuselage_length * 0.5; }

    double leading_edge_z(double span_x) const;
    double trailing_edge_z(double span_x) const;
    double thickness_at(double span_x) const;

    void rebuild_mesh();

protected:
    static void _bind_methods();

public:
    HelgaAirframe();
    ~HelgaAirframe() override;

    void _ready() override;

    Vector3 get_nose_point() const;
    Vector3 get_nose_gear_point() const;
    Vector3 get_left_gear_point() const;
    Vector3 get_right_gear_point() const;
    Vector3 get_left_wingtip() const;
    Vector3 get_right_wingtip() const;

    double get_fuselage_length() const { return fuselage_length; }
    void set_fuselage_length(double v) { fuselage_length = v; }
    double get_wingspan() const { return wingspan; }
    void set_wingspan(double v) { wingspan = v; }
    double get_crank_fraction() const { return crank_fraction; }
    void set_crank_fraction(double v) { crank_fraction = v; }
    double get_inner_sweep_deg() const { return inner_sweep_deg; }
    void set_inner_sweep_deg(double v) { inner_sweep_deg = v; }
    double get_outer_sweep_deg() const { return outer_sweep_deg; }
    void set_outer_sweep_deg(double v) { outer_sweep_deg = v; }
    double get_trailing_edge_cant_deg() const { return trailing_edge_cant_deg; }
    void set_trailing_edge_cant_deg(double v) { trailing_edge_cant_deg = v; }
    double get_body_height() const { return body_height; }
    void set_body_height(double v) { body_height = v; }
    double get_tip_thickness() const { return tip_thickness; }
    void set_tip_thickness(double v) { tip_thickness = v; }
    double get_camber() const { return camber; }
    void set_camber(double v) { camber = v; }
};

}

#endif // HELGA_AIRFRAME_H
