#ifndef HELGA_PROPULSION_H
#define HELGA_PROPULSION_H

// HelgaPropulsion -- the SSTO-44 Starlifter II's unified hybrid drive.
//
// Per overbyindustries.space/aerospace/propulsion, this is deliberately
// NOT a set of discrete switchable engines -- it's one propulsion
// controller continuously blending four subsystems every physics frame,
// the same way the real design's controller does across every flight
// regime:
//
//   1. ABEP (air-breathing electric propulsion) -- ionizes ambient
//      atmospheric mass for continuous thrust with no propellant draw.
//      Strong at sea level, tapering with air density exactly like
//      HelgaAerodynamics' own atmosphere model (0-80km per the site;
//      falls out naturally from the same exponential falloff rather
//      than a hard cutoff).
//   2. MHD-Lorentz rail accelerator -- dual-mode: in boost mode it
//      accelerates onboard ionic liquid propellant through magnetic
//      rails for thrust that doesn't care about ambient air at all
//      (works to orbit and beyond); in generation mode (low/cruise
//      throttle) it reverses to recharge the power reserve instead of
//      producing thrust.
//   3. IL afterburner -- injects ionic liquid into the MHD exhaust for
//      a big extra kick during launch and orbital insertion, at the
//      cost of draining the power reserve MHD generation mode fills
//      back up. Disabled once the reserve runs dry.
//   4. Solar wind capture -- electromagnetic sail collection, real but
//      vanishingly small this close to Earth; included for HUD/flavor
//      completeness rather than meaningful thrust in this sim's
//      LEO/MEO scope.
//
// No specific thrust/ISP/power figures are published for the real
// vehicle, so the constants below are this sim's own tuning, sized for
// a ~44,000 kg (97,000 lb) airframe -- see docs/DESIGN.md.
//
// throttle is set externally each physics frame (see
// aircraft_control.gd), the same pattern HelgaAerodynamics uses for
// elevator/aileron/rudder.

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>

namespace godot {

class HelgaPropulsion : public Node {
    GDCLASS(HelgaPropulsion, Node)

private:
    RigidBody3D *body = nullptr;

    double throttle = 0.0; // 0..1

    double abep_max_thrust_newtons = 150000.0;
    double mhd_max_thrust_newtons = 200000.0;
    double il_afterburner_max_thrust_newtons = 150000.0;
    double solar_wind_max_thrust_newtons = 500.0;
    double afterburner_throttle_threshold = 0.85;

    double atmosphere_scale_height = 8500.0; // meters, matches HelgaAerodynamics

    double power_reserve = 1.0; // 0..1, drained by afterburner use, refilled by MHD generation mode
    double afterburner_drain_per_second = 0.12;
    double generation_recharge_per_second = 0.08;

    double abep_thrust = 0.0;
    double mhd_thrust = 0.0;
    double afterburner_thrust = 0.0;
    double solar_wind_thrust = 0.0;

protected:
    static void _bind_methods();

public:
    HelgaPropulsion();
    ~HelgaPropulsion() override;

    void _ready() override;
    void _physics_process(double p_delta) override;

    double get_throttle() const { return throttle; }
    void set_throttle(double p_value) { throttle = p_value < 0.0 ? 0.0 : (p_value > 1.0 ? 1.0 : p_value); }

    double get_abep_thrust() const { return abep_thrust; }
    double get_mhd_thrust() const { return mhd_thrust; }
    double get_afterburner_thrust() const { return afterburner_thrust; }
    double get_solar_wind_thrust() const { return solar_wind_thrust; }
    double get_total_thrust() const { return abep_thrust + mhd_thrust + afterburner_thrust + solar_wind_thrust; }
    double get_power_reserve() const { return power_reserve; }
    bool is_afterburner_active() const { return afterburner_thrust > 0.0; }

    double get_abep_max_thrust_newtons() const { return abep_max_thrust_newtons; }
    void set_abep_max_thrust_newtons(double v) { abep_max_thrust_newtons = v; }
    double get_mhd_max_thrust_newtons() const { return mhd_max_thrust_newtons; }
    void set_mhd_max_thrust_newtons(double v) { mhd_max_thrust_newtons = v; }
    double get_il_afterburner_max_thrust_newtons() const { return il_afterburner_max_thrust_newtons; }
    void set_il_afterburner_max_thrust_newtons(double v) { il_afterburner_max_thrust_newtons = v; }
    double get_solar_wind_max_thrust_newtons() const { return solar_wind_max_thrust_newtons; }
    void set_solar_wind_max_thrust_newtons(double v) { solar_wind_max_thrust_newtons = v; }
    double get_afterburner_throttle_threshold() const { return afterburner_throttle_threshold; }
    void set_afterburner_throttle_threshold(double v) { afterburner_throttle_threshold = v; }
    double get_atmosphere_scale_height() const { return atmosphere_scale_height; }
    void set_atmosphere_scale_height(double v) { atmosphere_scale_height = v; }
};

}

#endif // HELGA_PROPULSION_H
