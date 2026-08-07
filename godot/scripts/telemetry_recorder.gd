class_name HelgaTelemetryRecorder
extends Node
## Flight-test telemetry aggregation for the post-flight debrief (see
## docs/DESIGN.md's Data Collection section) -- narrative/feedback
## dressing, not a scoring system. Polls the same getters the HUD and
## HelgaReentrySystem already expose rather than owning any physics
## itself, and tracks session peaks: max dynamic pressure, peak reentry
## heat flux, max g-load, max aerobraking corridor deviation, and sink
## rate at touchdown.

@export var aircraft_path: NodePath
@export var flight_computer_path: NodePath
@export var reentry_system_path: NodePath

const GRAVITY_MS2 := 9.81
const TOUCHDOWN_ALTITUDE_M := 3.5 # matches HelgaFlightComputer's own grounded_altitude_m default

var aircraft: HelgaAircraftControl
var flight_computer: HelgaFlightComputer
var reentry_system: HelgaReentrySystem

var max_dynamic_pressure_pa: float = 0.0
var peak_heat_flux_w_m2: float = 0.0
var max_g_load: float = 0.0
var max_corridor_deviation: float = 0.0
var touchdown_sink_rate_ms: float = 0.0

var _previous_velocity: Vector3 = Vector3.ZERO
var _has_previous_velocity: bool = false
var _was_airborne_during_landing: bool = false

const REENTRY_ARMED_STATES: Array[int] = [
	HelgaFlightComputer.REENTRY,
	HelgaFlightComputer.STATE_SHALLOW_REENTRY_CONTINGENCY,
	HelgaFlightComputer.STATE_THERMAL_OVERLOAD,
]

func _ready() -> void:
	aircraft = get_node_or_null(aircraft_path) as HelgaAircraftControl
	flight_computer = get_node_or_null(flight_computer_path) as HelgaFlightComputer
	reentry_system = get_node_or_null(reentry_system_path) as HelgaReentrySystem

func reset() -> void:
	max_dynamic_pressure_pa = 0.0
	peak_heat_flux_w_m2 = 0.0
	max_g_load = 0.0
	max_corridor_deviation = 0.0
	touchdown_sink_rate_ms = 0.0
	_has_previous_velocity = false
	_was_airborne_during_landing = false

func _physics_process(delta: float) -> void:
	if aircraft == null:
		return

	var velocity: Vector3 = aircraft.linear_velocity
	if _has_previous_velocity and delta > 0.0:
		# Finite-difference estimate of the pilot's felt acceleration
		# (actual acceleration minus gravity, the way a strapped-in
		# accelerometer would read it) -- a flavor debrief stat, not a
		# control input, so frame-to-frame noise is an acceptable
		# tradeoff against not needing a dedicated C++ filter for it.
		var accel: Vector3 = (velocity - _previous_velocity) / delta
		var felt_accel: Vector3 = accel - Vector3(0.0, -GRAVITY_MS2, 0.0)
		max_g_load = max(max_g_load, felt_accel.length() / GRAVITY_MS2)
	_previous_velocity = velocity
	_has_previous_velocity = true

	if reentry_system != null:
		max_dynamic_pressure_pa = max(max_dynamic_pressure_pa, reentry_system.get_dynamic_pressure_pa())
		peak_heat_flux_w_m2 = max(peak_heat_flux_w_m2, reentry_system.get_heat_flux_w_m2())

		if flight_computer != null and flight_computer.get_current_state() in REENTRY_ARMED_STATES:
			max_corridor_deviation = max(max_corridor_deviation, reentry_system.get_corridor_deviation())

	if flight_computer != null:
		var altitude: float = aircraft.global_position.y
		var grounded := altitude <= TOUCHDOWN_ALTITUDE_M
		if flight_computer.get_current_state() == HelgaFlightComputer.LANDING:
			if not grounded:
				_was_airborne_during_landing = true
			elif _was_airborne_during_landing:
				touchdown_sink_rate_ms = -velocity.y
				_was_airborne_during_landing = false
		else:
			_was_airborne_during_landing = false
