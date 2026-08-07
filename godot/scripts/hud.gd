class_name HelgaHud
extends CanvasLayer
## In-flight instrument HUD: flight-computer phase, airspeed/altitude/
## vertical speed/AoA/heading/throttle, active camera, and record/replay
## status. Purely a readout -- no interactive elements -- so it never
## competes with aircraft_control.gd/camera_rig.gd/replay_controller.gd
## for keyboard input.

@export var aircraft_path: NodePath
@export var aerodynamics_path: NodePath
@export var flight_computer_path: NodePath
@export var camera_rig_path: NodePath
@export var replay_controller_path: NodePath
@export var landing_gear_path: NodePath
@export var propulsion_path: NodePath
@export var reentry_system_path: NodePath
@export var comms_controller_path: NodePath
@export var gravity_path: NodePath
@export var autopilot_path: NodePath

var aircraft: HelgaAircraftControl
var aerodynamics: HelgaAerodynamics
var flight_computer: HelgaFlightComputer
var camera_rig: HelgaCameraRig
var replay_controller: HelgaReplayController
var landing_gear: HelgaLandingGear
var propulsion: HelgaPropulsion
var reentry_system: HelgaReentrySystem
var comms_controller: HelgaCommsController
var gravity: HelgaGravity
var autopilot: HelgaAutopilot

## States where the reentry corridor instruments are relevant -- see
## src/reentry_system.h's class comment for what arms the mechanic itself.
const REENTRY_RELEVANT_STATES: Array[int] = [
	HelgaFlightComputer.REENTRY,
	HelgaFlightComputer.STATE_SHALLOW_REENTRY_CONTINGENCY,
	HelgaFlightComputer.STATE_THERMAL_OVERLOAD,
]

## States where "how close to real orbital velocity am I" is relevant --
## see src/gravity.h and flight_computer.cpp's ASCENT->ORBIT guard.
const ORBITAL_VELOCITY_RELEVANT_STATES: Array[int] = [
	HelgaFlightComputer.ASCENT,
	HelgaFlightComputer.ORBIT,
]

func _ready() -> void:
	aircraft = get_node_or_null(aircraft_path) as HelgaAircraftControl
	aerodynamics = get_node_or_null(aerodynamics_path) as HelgaAerodynamics
	flight_computer = get_node_or_null(flight_computer_path) as HelgaFlightComputer
	camera_rig = get_node_or_null(camera_rig_path) as HelgaCameraRig
	replay_controller = get_node_or_null(replay_controller_path) as HelgaReplayController
	landing_gear = get_node_or_null(landing_gear_path) as HelgaLandingGear
	propulsion = get_node_or_null(propulsion_path) as HelgaPropulsion
	reentry_system = get_node_or_null(reentry_system_path) as HelgaReentrySystem
	gravity = get_node_or_null(gravity_path) as HelgaGravity
	autopilot = get_node_or_null(autopilot_path) as HelgaAutopilot
	comms_controller = get_node_or_null(comms_controller_path) as HelgaCommsController

func _process(_delta: float) -> void:
	if aircraft == null or aerodynamics == null:
		return

	if flight_computer != null:
		%FlightStateLabel.text = flight_computer.state_name(flight_computer.get_current_state())

	var airspeed := aerodynamics.get_airspeed()
	var altitude := aircraft.global_position.y
	var vertical_speed := aircraft.linear_velocity.y
	var aoa := aerodynamics.get_angle_of_attack_deg()

	%AirspeedLabel.text = "SPD  %5.1f m/s" % airspeed
	%AltitudeLabel.text = "ALT  %6.1f m" % altitude
	%VSpeedLabel.text = "V/S  %+5.1f m/s" % vertical_speed
	%AoaLabel.text = "AOA  %+5.1f deg" % aoa
	%HeadingLabel.text = "HDG  %03d" % int(_heading_degrees())
	%ThrottleLabel.text = "THR  %3d%%" % int(aircraft.throttle * 100.0)
	%FlapsLabel.text = "FLP  %3d%%" % int(aerodynamics.flaps * 100.0)

	if landing_gear != null:
		if landing_gear.is_extended():
			%GearLabel.text = "GEAR DOWN"
		elif landing_gear.is_retracted():
			%GearLabel.text = "GEAR UP"
		else:
			%GearLabel.text = "GEAR %3d%%" % int(landing_gear.get_extension_fraction() * 100.0)

	if propulsion != null:
		%ThrustLabel.text = "THRUST %4d kN" % int(propulsion.get_total_thrust() / 1000.0)
		%PowerLabel.text = "PWR  %3d%%" % int(propulsion.get_power_reserve() * 100.0)

	if gravity != null and flight_computer != null:
		var orbit_relevant := flight_computer.get_current_state() in ORBITAL_VELOCITY_RELEVANT_STATES
		%OrbitalVelocityLabel.visible = orbit_relevant
		if orbit_relevant:
			var horizontal_speed := Vector2(aircraft.linear_velocity.x, aircraft.linear_velocity.z).length()
			var target := gravity.get_orbital_velocity_ms()
			%OrbitalVelocityLabel.text = "ORB VEL %5.0f / %5.0f m/s" % [horizontal_speed, target]
			var reached := target > 0.0 and horizontal_speed >= 0.9 * target
			%OrbitalVelocityLabel.modulate = Color(0.4, 1.0, 0.55, 1) if reached else Color(1.0, 0.85, 0.3, 1)

	if camera_rig != null:
		var cam := camera_rig.get_active_camera()
		%CameraLabel.text = "CAM  %s" % (cam.name if cam != null else "--")

	if replay_controller != null:
		if replay_controller.recorder.is_recording():
			%StatusLabel.text = "● REC"
			%StatusLabel.modulate = Color(1.0, 0.3, 0.3)
		elif replay_controller.replaying:
			%StatusLabel.text = "▶ REPLAY"
			%StatusLabel.modulate = Color(0.4, 0.7, 1.0)
		else:
			%StatusLabel.text = ""

	if reentry_system != null and flight_computer != null:
		var relevant := flight_computer.get_current_state() in REENTRY_RELEVANT_STATES
		%ReentryPanel.visible = relevant
		if relevant:
			var corridor := reentry_system.get_corridor_deviation()
			%DynamicPressureLabel.text = "Q         %5.0f Pa" % reentry_system.get_dynamic_pressure_pa()
			%HeatFluxLabel.text = "HEAT   %6.0f kW/m2" % (reentry_system.get_heat_flux_w_m2() / 1000.0)
			%CorridorLabel.text = "CORRIDOR   %3.0f%%" % (corridor * 100.0)
			var corridor_color := Color(1.0, 0.3, 0.2, 1) if corridor > 0.0 else Color(1.0, 0.75, 0.3, 1)
			%DynamicPressureLabel.modulate = corridor_color
			%HeatFluxLabel.modulate = corridor_color
			%CorridorLabel.modulate = corridor_color

			# Fly-to indicator against the pre-approved reentry profile --
			# see reentry_flight_plan.gd. Positive = flying steeper than
			# planned, negative = shallower.
			var deviation := HelgaReentryFlightPlan.get_deviation_deg(altitude, reentry_system.get_flight_path_angle_deg())
			var deviation_word := "STEEP" if deviation > 0.0 else "SHALLOW"
			%ProfileLabel.text = "PROFILE   %+4.1f deg %s" % [deviation, deviation_word if absf(deviation) > 0.3 else ""]
			%ProfileLabel.modulate = Color(0.4, 1.0, 0.55, 1) if absf(deviation) <= 1.0 else Color(1.0, 0.85, 0.3, 1)

			%AutopilotLabel.visible = autopilot != null and autopilot.is_engaged()

	if comms_controller != null:
		%CommsLogLabel.text = "\n".join(comms_controller.get_log_lines())
		if comms_controller.has_pending_readback():
			%CommsPromptLabel.text = "[T] READBACK TO %s" % comms_controller.get_pending_station()
		else:
			%CommsPromptLabel.text = ""

	var warning_text := ""
	if flight_computer != null:
		var state := flight_computer.get_current_state()
		if state == HelgaFlightComputer.STATE_THERMAL_OVERLOAD:
			warning_text = "THERMAL OVERLOAD"
		elif state == HelgaFlightComputer.STATE_SHALLOW_REENTRY_CONTINGENCY:
			warning_text = "SHALLOW ENTRY - SKIP-OUT RISK"
		elif landing_gear != null:
			var approaching_to_land := state == HelgaFlightComputer.APPROACH or state == HelgaFlightComputer.LANDING
			if approaching_to_land and not landing_gear.is_extended():
				warning_text = "GEAR UP"
	%WarningLabel.text = warning_text

func _heading_degrees() -> float:
	var forward := -aircraft.global_transform.basis.z
	var heading := rad_to_deg(atan2(forward.x, -forward.z))
	if heading < 0.0:
		heading += 360.0
	return heading
