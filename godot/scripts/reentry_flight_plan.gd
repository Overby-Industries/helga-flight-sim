class_name HelgaReentryFlightPlan
extends Node
## A pre-approved reentry profile: a curated, altitude-keyed target
## flight-path-angle curve the pilot is expected to fly, rather than
## free-form descent within HelgaReentrySystem's shallow/steep corridor
## limits (see src/reentry_system.h). Matches real spaceflight-ops
## language -- a flight plan gets approved before you fly it -- and the
## project's existing "curated, not deeply simulated" pattern already
## used for comms_controller.gd's phrase table.
##
## This is deliberately GDScript, not C++: it's mission/profile data and
## a comparison against it, not a physics system in its own right (the
## actual flight-path-angle it's compared against still comes from
## HelgaReentrySystem). Keyframes are linearly interpolated by altitude.
## Values stay clear of HelgaReentrySystem's shallow_angle_deg (2.0) for
## most of the descent, only approaching it near the bottom where
## airspeed has already bled below skip_out_airspeed_ms and the shallow-
## contingency check no longer applies -- see reentry_system.h.
##
## HUD reads get_deviation_deg() as a fly-to indicator (like a glideslope
## needle) while HelgaFlightComputer is in a reentry-relevant state.

## (altitude_m, target_flight_path_angle_deg), highest altitude first.
const PROFILE: Array[Vector2] = [
	Vector2(120000.0, 1.5),
	Vector2(80000.0, 4.0),
	Vector2(40000.0, 6.0),
	Vector2(10000.0, 3.0),
	Vector2(3000.0, 1.0),
]

## Interpolated target flight-path-angle at a given altitude. Clamped to
## the profile's own endpoints outside its altitude range rather than
## extrapolated -- there's no "correct" angle to fly above/below the
## profile's own defined corridor.
static func get_target_flight_path_angle_deg(altitude_m: float) -> float:
	if altitude_m >= PROFILE[0].x:
		return PROFILE[0].y
	if altitude_m <= PROFILE[PROFILE.size() - 1].x:
		return PROFILE[PROFILE.size() - 1].y

	for i in range(PROFILE.size() - 1):
		var hi := PROFILE[i]
		var lo := PROFILE[i + 1]
		if altitude_m <= hi.x and altitude_m >= lo.x:
			var span := hi.x - lo.x
			var t := (altitude_m - lo.x) / span if span > 0.0 else 0.0
			return lerpf(lo.y, hi.y, t)

	return PROFILE[PROFILE.size() - 1].y

## Positive = flying steeper than the plan, negative = flying shallower.
static func get_deviation_deg(altitude_m: float, actual_flight_path_angle_deg: float) -> float:
	return actual_flight_path_angle_deg - get_target_flight_path_angle_deg(altitude_m)
