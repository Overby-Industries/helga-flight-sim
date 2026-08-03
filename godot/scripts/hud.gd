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

var aircraft: HelgaAircraftControl
var aerodynamics: HelgaAerodynamics
var flight_computer: HelgaFlightComputer
var camera_rig: HelgaCameraRig
var replay_controller: HelgaReplayController

func _ready() -> void:
	aircraft = get_node_or_null(aircraft_path) as HelgaAircraftControl
	aerodynamics = get_node_or_null(aerodynamics_path) as HelgaAerodynamics
	flight_computer = get_node_or_null(flight_computer_path) as HelgaFlightComputer
	camera_rig = get_node_or_null(camera_rig_path) as HelgaCameraRig
	replay_controller = get_node_or_null(replay_controller_path) as HelgaReplayController

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

func _heading_degrees() -> float:
	var forward := -aircraft.global_transform.basis.z
	var heading := rad_to_deg(atan2(forward.x, -forward.z))
	if heading < 0.0:
		heading += 360.0
	return heading
