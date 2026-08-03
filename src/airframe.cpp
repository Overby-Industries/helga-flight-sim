#include "airframe.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>

#include <cmath>

using namespace godot;

namespace {
constexpr double DEG_TO_RAD = 0.017453292519943295;

double smoothstep01(double t) {
    t = t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t);
    return t * t * (3.0 - 2.0 * t);
}

// Two triangles covering quad a-b-c-d, in order. With cull disabled on
// the material, triangle winding only affects computed-normal
// direction, not visibility -- see the "make it look good later" note
// in airframe.h.
void add_quad(SurfaceTool *st, const Vector3 &a, const Vector3 &b, const Vector3 &c, const Vector3 &d) {
    st->add_vertex(a);
    st->add_vertex(b);
    st->add_vertex(c);
    st->add_vertex(a);
    st->add_vertex(c);
    st->add_vertex(d);
}
}

HelgaAirframe::HelgaAirframe() {}

HelgaAirframe::~HelgaAirframe() {}

double HelgaAirframe::leading_edge_z(double span_x) const {
    double cx = crank_x();
    double inner_slope = std::tan(inner_sweep_deg * DEG_TO_RAD);
    double outer_slope = std::tan(outer_sweep_deg * DEG_TO_RAD);
    if (span_x <= cx) {
        return nose_z() + span_x * inner_slope;
    }
    return nose_z() + cx * inner_slope + (span_x - cx) * outer_slope;
}

double HelgaAirframe::trailing_edge_z(double span_x) const {
    double cant_slope = std::tan(trailing_edge_cant_deg * DEG_TO_RAD);
    return tail_z() - span_x * cant_slope;
}

double HelgaAirframe::thickness_at(double span_x) const {
    double hs = half_span();
    double t = hs > 0.0 ? span_x / hs : 0.0;
    return body_height + (tip_thickness - body_height) * smoothstep01(t);
}

void HelgaAirframe::rebuild_mesh() {
    Ref<SurfaceTool> st;
    st.instantiate();
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    double hs = half_span();
    double cx = crank_x();
    double stations[5] = {-hs, -cx, 0.0, cx, hs};

    for (int i = 0; i < 4; i++) {
        double x0 = stations[i];
        double x1 = stations[i + 1];
        double s0 = std::fabs(x0);
        double s1 = std::fabs(x1);

        double le0 = leading_edge_z(s0);
        double te0 = trailing_edge_z(s0);
        double th0 = thickness_at(s0);
        double le1 = leading_edge_z(s1);
        double te1 = trailing_edge_z(s1);
        double th1 = thickness_at(s1);

        Vector3 tf0(x0, camber + th0 * 0.5, le0);
        Vector3 tb0(x0, camber + th0 * 0.5, te0);
        Vector3 bb0(x0, camber - th0 * 0.5, te0);
        Vector3 bf0(x0, camber - th0 * 0.5, le0);

        Vector3 tf1(x1, camber + th1 * 0.5, le1);
        Vector3 tb1(x1, camber + th1 * 0.5, te1);
        Vector3 bb1(x1, camber - th1 * 0.5, te1);
        Vector3 bf1(x1, camber - th1 * 0.5, le1);

        add_quad(*st, tf0, tb0, tb1, tf1); // top
        add_quad(*st, bf1, bb1, bb0, bf0); // bottom
        add_quad(*st, tf1, bf1, bf0, tf0); // leading edge
        add_quad(*st, tb0, bb0, bb1, tb1); // trailing edge
        add_quad(*st, tf0, bf0, bb0, tb0); // root-end cap
        add_quad(*st, tf1, tb1, bb1, bf1); // tip-end cap
    }

    st->generate_normals();
    Ref<ArrayMesh> mesh = st->commit();

    Ref<StandardMaterial3D> material;
    material.instantiate();
    material->set_albedo(Color(0.55, 0.57, 0.6, 1.0));
    material->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
    mesh->surface_set_material(0, material);

    set_mesh(mesh);
}

void HelgaAirframe::_ready() {
    rebuild_mesh();
}

Vector3 HelgaAirframe::get_nose_point() const {
    return Vector3(0.0, camber, nose_z());
}

Vector3 HelgaAirframe::get_nose_gear_point() const {
    double z = nose_z() * 0.7;
    double th = thickness_at(0.0);
    return Vector3(0.0, camber - th * 0.5, z);
}

Vector3 HelgaAirframe::get_left_gear_point() const {
    double span_x = crank_x() * 0.6;
    double th = thickness_at(span_x);
    double z = leading_edge_z(span_x) * 0.3 + trailing_edge_z(span_x) * 0.7;
    return Vector3(-span_x, camber - th * 0.5, z);
}

Vector3 HelgaAirframe::get_right_gear_point() const {
    Vector3 p = get_left_gear_point();
    p.x = -p.x;
    return p;
}

Vector3 HelgaAirframe::get_left_wingtip() const {
    double span_x = half_span();
    double z = (leading_edge_z(span_x) + trailing_edge_z(span_x)) * 0.5;
    return Vector3(-span_x, camber, z);
}

Vector3 HelgaAirframe::get_right_wingtip() const {
    Vector3 p = get_left_wingtip();
    p.x = -p.x;
    return p;
}

void HelgaAirframe::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_nose_point"), &HelgaAirframe::get_nose_point);
    ClassDB::bind_method(D_METHOD("get_nose_gear_point"), &HelgaAirframe::get_nose_gear_point);
    ClassDB::bind_method(D_METHOD("get_left_gear_point"), &HelgaAirframe::get_left_gear_point);
    ClassDB::bind_method(D_METHOD("get_right_gear_point"), &HelgaAirframe::get_right_gear_point);
    ClassDB::bind_method(D_METHOD("get_left_wingtip"), &HelgaAirframe::get_left_wingtip);
    ClassDB::bind_method(D_METHOD("get_right_wingtip"), &HelgaAirframe::get_right_wingtip);

    ClassDB::bind_method(D_METHOD("get_fuselage_length"), &HelgaAirframe::get_fuselage_length);
    ClassDB::bind_method(D_METHOD("set_fuselage_length", "value"), &HelgaAirframe::set_fuselage_length);
    ClassDB::bind_method(D_METHOD("get_wingspan"), &HelgaAirframe::get_wingspan);
    ClassDB::bind_method(D_METHOD("set_wingspan", "value"), &HelgaAirframe::set_wingspan);
    ClassDB::bind_method(D_METHOD("get_crank_fraction"), &HelgaAirframe::get_crank_fraction);
    ClassDB::bind_method(D_METHOD("set_crank_fraction", "value"), &HelgaAirframe::set_crank_fraction);
    ClassDB::bind_method(D_METHOD("get_inner_sweep_deg"), &HelgaAirframe::get_inner_sweep_deg);
    ClassDB::bind_method(D_METHOD("set_inner_sweep_deg", "value"), &HelgaAirframe::set_inner_sweep_deg);
    ClassDB::bind_method(D_METHOD("get_outer_sweep_deg"), &HelgaAirframe::get_outer_sweep_deg);
    ClassDB::bind_method(D_METHOD("set_outer_sweep_deg", "value"), &HelgaAirframe::set_outer_sweep_deg);
    ClassDB::bind_method(D_METHOD("get_trailing_edge_cant_deg"), &HelgaAirframe::get_trailing_edge_cant_deg);
    ClassDB::bind_method(D_METHOD("set_trailing_edge_cant_deg", "value"), &HelgaAirframe::set_trailing_edge_cant_deg);
    ClassDB::bind_method(D_METHOD("get_body_height"), &HelgaAirframe::get_body_height);
    ClassDB::bind_method(D_METHOD("set_body_height", "value"), &HelgaAirframe::set_body_height);
    ClassDB::bind_method(D_METHOD("get_tip_thickness"), &HelgaAirframe::get_tip_thickness);
    ClassDB::bind_method(D_METHOD("set_tip_thickness", "value"), &HelgaAirframe::set_tip_thickness);
    ClassDB::bind_method(D_METHOD("get_camber"), &HelgaAirframe::get_camber);
    ClassDB::bind_method(D_METHOD("set_camber", "value"), &HelgaAirframe::set_camber);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fuselage_length"), "set_fuselage_length", "get_fuselage_length");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wingspan"), "set_wingspan", "get_wingspan");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "crank_fraction", PROPERTY_HINT_RANGE, "0.05,0.95,0.01"), "set_crank_fraction", "get_crank_fraction");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "inner_sweep_deg"), "set_inner_sweep_deg", "get_inner_sweep_deg");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "outer_sweep_deg"), "set_outer_sweep_deg", "get_outer_sweep_deg");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "trailing_edge_cant_deg"), "set_trailing_edge_cant_deg", "get_trailing_edge_cant_deg");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "body_height"), "set_body_height", "get_body_height");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tip_thickness"), "set_tip_thickness", "get_tip_thickness");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "camber"), "set_camber", "get_camber");
}
