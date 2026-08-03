#ifndef HELGA_AIRFRAME_H
#define HELGA_AIRFRAME_H

// HelgaAirframe -- procedurally builds the SSTO-44 Starlifter II's
// blended-wing-body hull mesh.
//
// Shape and proportions come from three sources: overbyindustries.space/
// aerospace ("blended wing body... double-delta planform with a cranked
// leading edge... canted trailing edges... shallow camber") for the
// qualitative shape; the pilot's own aircraft-design-tool sketches for
// the reference numbers -- 100 ft (30.5 m) span, 0.067 taper ratio,
// 4.167 aspect ratio, ~35.75 deg root-tip sweep, 97,000 lb (~44,000 kg)
// mass; and the pilot's own follow-up corrections for the true layout:
// the fuselage is 120 ft (36.6 m) long overall, mid-wing mounted, with
// the double-delta root chord (60 ft / half the fuselage) starting at
// the fuselage's longitudinal midpoint and running aft to the tail --
// so the forward half of the fuselage is a nose section ahead of the
// wing, not part of the double-delta's own chord. A large diamond-
// planform "bird tail elevator" (the primary pitch control surface,
// per the pilot) hinges right at that same tail station, between the
// two wing roots, and extends aft of it tapering to a point.
//
// The main double-delta loft is one continuous surface (not a separate
// fuselage-plus-wing assembly): 5 spanwise stations
// (-halfspan, -crank_x, 0, +crank_x, +halfspan) with its LE anchored at
// wing_root_le_z() (the fuselage midpoint) rather than the true nose,
// so at span_x=0 its own chord *is* the 60 ft root chord, matching the
// pilot's reported dimension. The true nose is built as a separate
// tapering nose-cone segment ahead of that, and the bird tail elevator
// as a separate two-segment diamond aft of the tail -- see
// rebuild_mesh().
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
class SurfaceTool;

class HelgaAirframe : public MeshInstance3D {
    GDCLASS(HelgaAirframe, MeshInstance3D)

private:
    double fuselage_length = 36.6;          // meters, nose to tail (120 ft)
    double nose_boom_length = 18.3;         // meters, nose to the wing root's own LE (60 ft)
    double wingspan = 30.5;                 // meters, tip to tip (100 ft)
    double crank_fraction = 0.3;            // fraction of half-span where LE sweep changes
    double inner_sweep_deg = 60.0;          // strake leading-edge sweep, root to crank
    double outer_sweep_deg = 16.0;          // wing-panel leading-edge sweep, crank to tip
    double trailing_edge_cant_deg = 22.0;   // trailing edge sweeps forward toward the tip
    double body_height = 3.0;               // thickness at centerline
    double tip_thickness = 0.3;             // thickness at wingtip
    double camber = 0.3;                    // shallow upward camber offset, meters

    double nose_half_width = 1.6;           // nose-cone base half-width, at the wing root LE station
    double tail_elevator_extension = 7.0;   // meters aft of the fuselage tail the elevator's tip reaches
    double tail_elevator_half_width = 4.0;  // max half-width of the diamond, at its midpoint
    double tail_elevator_thickness = 0.4;   // constant thickness of the elevator slab

    double half_span() const { return wingspan * 0.5; }
    double crank_x() const { return crank_fraction * half_span(); }
    double nose_z() const { return -fuselage_length * 0.5; }
    double tail_z() const { return fuselage_length * 0.5; }
    double wing_root_le_z() const { return nose_z() + nose_boom_length; }

    double leading_edge_z(double span_x) const;
    double trailing_edge_z(double span_x) const;
    double thickness_at(double span_x) const;

    void add_nose_cone(SurfaceTool *st) const;
    void add_tail_elevator(SurfaceTool *st) const;
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
    Vector3 get_tail_elevator_hinge_point() const;

    double get_fuselage_length() const { return fuselage_length; }
    void set_fuselage_length(double v) { fuselage_length = v; }
    double get_nose_boom_length() const { return nose_boom_length; }
    void set_nose_boom_length(double v) { nose_boom_length = v; }
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
    double get_nose_half_width() const { return nose_half_width; }
    void set_nose_half_width(double v) { nose_half_width = v; }
    double get_tail_elevator_extension() const { return tail_elevator_extension; }
    void set_tail_elevator_extension(double v) { tail_elevator_extension = v; }
    double get_tail_elevator_half_width() const { return tail_elevator_half_width; }
    void set_tail_elevator_half_width(double v) { tail_elevator_half_width = v; }
    double get_tail_elevator_thickness() const { return tail_elevator_thickness; }
    void set_tail_elevator_thickness(double v) { tail_elevator_thickness = v; }
};

}

#endif // HELGA_AIRFRAME_H
