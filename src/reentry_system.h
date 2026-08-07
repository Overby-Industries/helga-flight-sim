#ifndef HELGA_REENTRY_SYSTEM_H
#define HELGA_REENTRY_SYSTEM_H

// HelgaReentrySystem -- the aerobraking/reentry corridor mechanic, the
// centerpiece piloting challenge per docs/DESIGN.md: Helga reenters
// belly-first at high AoA, using its lower surface as an active
// (ionic-liquid-managed) heat shield. Too shallow and the vehicle skips
// back out instead of settling into the atmosphere; too steep and heat
// flux exceeds what the shield can manage.
//
// Lives as a sibling of HelgaAerodynamics/HelgaFlightComputer under
// HelgaAircraft (see godot/scenes/main.tscn) and reaches them by fixed
// sibling node name in _ready() -- this mirrors how HelgaPropulsion/
// HelgaAerodynamics already reach up to their parent RigidBody3D, just
// sideways instead of up, since the scene's node layout is hand-built
// and fixed rather than a reusable/instanced component.
//
// Three live readings drive the mechanic, refreshed every physics frame:
//   - dynamic pressure (0.5 * rho * v^2) -- structural/handling load,
//     surfaced for HUD/telemetry, not itself a failure trigger here.
//   - heat flux, via the Sutton-Graves stagnation-point approximation
//     (q = k * sqrt(rho) * v^3, effective nose radius folded into k) --
//     a real functional form, though k and the "critical" threshold
//     below are this sim's own tuning (no published Helga heat-shield
//     figures exist, same situation docs/DESIGN.md already notes for
//     HelgaPropulsion's thrust constants), landing in a realistic W/m^2
//     range for a LEO-return profile.
//   - flight path angle (the velocity vector's angle below the local
//     horizontal) -- too shallow for too long at high speed reads as
//     "about to skip back out."
//
// The contingency-triggering logic below only arms while the flight
// computer is in REENTRY or one of the two contingency states -- outside
// that window the readings above still update (for HUD/telemetry) but
// transition_to() is never called. Per docs/DESIGN.md's extended-state
// rule ("every extended state needs at least one path to FAULT"):
//   - STATE_THERMAL_OVERLOAD: heat flux over critical for longer than
//     thermal_overload_grace_seconds enters this state; dropping back
//     under critical returns to REENTRY; staying over critical for
//     thermal_catastrophic_seconds total (grace included) reaches FAULT.
//   - STATE_SHALLOW_REENTRY_CONTINGENCY: flight path angle under
//     shallow_angle_deg for longer than skip_out_grace_seconds while
//     still going fast enters this state; climbing back away (skipped
//     out) returns to ORBIT for another attempt; correcting back into a
//     proper descent angle returns to REENTRY; running out of
//     shallow_contingency_fault_seconds without either reaches FAULT.

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>

namespace godot {

class HelgaAerodynamics;
class HelgaFlightComputer;

class HelgaReentrySystem : public Node {
    GDCLASS(HelgaReentrySystem, Node)

private:
    RigidBody3D *body = nullptr;
    HelgaAerodynamics *aerodynamics = nullptr;
    HelgaFlightComputer *flight_computer = nullptr;

    double heat_flux_coefficient = 1.83e-4;   // Sutton-Graves k, nose radius folded in -- see class comment
    double heat_flux_critical_w_m2 = 2500000.0;
    double thermal_overload_grace_seconds = 4.0;
    double thermal_catastrophic_seconds = 12.0;

    double shallow_angle_deg = 2.0;
    double skip_out_airspeed_ms = 1500.0;
    double skip_out_grace_seconds = 5.0;
    double skip_out_recovery_vspeed_ms = 5.0;
    double shallow_contingency_fault_seconds = 30.0;

    double dynamic_pressure_pa = 0.0;
    double heat_flux_w_m2 = 0.0;
    double flight_path_angle_deg = 0.0;
    double corridor_deviation = 0.0;

    double thermal_excursion_seconds = 0.0;
    double shallow_excursion_seconds = 0.0;
    double shallow_contingency_seconds = 0.0;

    void update_readings();
    void evaluate_contingencies(double p_delta);

protected:
    static void _bind_methods();

public:
    HelgaReentrySystem();
    ~HelgaReentrySystem() override;

    void _ready() override;
    void _physics_process(double p_delta) override;

    double get_dynamic_pressure_pa() const { return dynamic_pressure_pa; }
    double get_heat_flux_w_m2() const { return heat_flux_w_m2; }
    double get_flight_path_angle_deg() const { return flight_path_angle_deg; }
    double get_corridor_deviation() const { return corridor_deviation; }
    double get_heat_flux_ratio() const { return heat_flux_critical_w_m2 > 0.0 ? heat_flux_w_m2 / heat_flux_critical_w_m2 : 0.0; }

    double get_heat_flux_coefficient() const { return heat_flux_coefficient; }
    void set_heat_flux_coefficient(double v) { heat_flux_coefficient = v; }
    double get_heat_flux_critical_w_m2() const { return heat_flux_critical_w_m2; }
    void set_heat_flux_critical_w_m2(double v) { heat_flux_critical_w_m2 = v; }
    double get_thermal_overload_grace_seconds() const { return thermal_overload_grace_seconds; }
    void set_thermal_overload_grace_seconds(double v) { thermal_overload_grace_seconds = v; }
    double get_thermal_catastrophic_seconds() const { return thermal_catastrophic_seconds; }
    void set_thermal_catastrophic_seconds(double v) { thermal_catastrophic_seconds = v; }

    double get_shallow_angle_deg() const { return shallow_angle_deg; }
    void set_shallow_angle_deg(double v) { shallow_angle_deg = v; }
    double get_skip_out_airspeed_ms() const { return skip_out_airspeed_ms; }
    void set_skip_out_airspeed_ms(double v) { skip_out_airspeed_ms = v; }
    double get_skip_out_grace_seconds() const { return skip_out_grace_seconds; }
    void set_skip_out_grace_seconds(double v) { skip_out_grace_seconds = v; }
    double get_skip_out_recovery_vspeed_ms() const { return skip_out_recovery_vspeed_ms; }
    void set_skip_out_recovery_vspeed_ms(double v) { skip_out_recovery_vspeed_ms = v; }
    double get_shallow_contingency_fault_seconds() const { return shallow_contingency_fault_seconds; }
    void set_shallow_contingency_fault_seconds(double v) { shallow_contingency_fault_seconds = v; }
};

}

#endif // HELGA_REENTRY_SYSTEM_H
