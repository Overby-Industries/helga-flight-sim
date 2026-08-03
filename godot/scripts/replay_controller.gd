class_name HelgaReplayController
extends Node
## Records a flight and can play it back later, DCS/Tacview-style: during
## replay the aircraft's physics is frozen and its transform is driven
## directly from the recording every frame, while every camera (see
## camera_rig.gd) stays completely independent -- switch or fly cameras
## freely over a replayed flight and record the result (e.g. with OBS)
## without the replay itself caring what's being looked at.
##
## Default keys: R toggles recording, P plays back the current
## recording. Missions can be saved/loaded to disk with save_mission()/
## load_mission() (see HelgaFlightRecorder for the file format).

@export var aircraft_path: NodePath
@export var aerodynamics_path: NodePath
@export var flight_computer_path: NodePath
@export var replay_speed: float = 1.0

var aircraft: HelgaAircraftControl
var aerodynamics: HelgaAerodynamics
var flight_computer: HelgaFlightComputer
var recorder: HelgaFlightRecorder

var replaying: bool = false
var replay_time: float = 0.0

func _ready() -> void:
	aircraft = get_node_or_null(aircraft_path) as HelgaAircraftControl
	aerodynamics = get_node_or_null(aerodynamics_path) as HelgaAerodynamics
	flight_computer = get_node_or_null(flight_computer_path) as HelgaFlightComputer

	recorder = HelgaFlightRecorder.new()
	add_child(recorder)

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed:
		if event.physical_keycode == KEY_R:
			toggle_recording()
		elif event.physical_keycode == KEY_P:
			begin_replay()

func _physics_process(delta: float) -> void:
	if aircraft == null or aerodynamics == null:
		return

	if recorder.is_recording():
		var rotation := aircraft.global_transform.basis.get_rotation_quaternion()
		var flight_state := flight_computer.get_current_state() if flight_computer != null else 0
		recorder.record_sample(
			aircraft.global_position, rotation,
			aerodynamics.elevator, aerodynamics.aileron, aerodynamics.rudder,
			aircraft.throttle, flight_state, delta
		)
	elif replaying:
		replay_time += delta * replay_speed
		var sample := recorder.sample_at_time(replay_time)
		if not sample.is_empty():
			aircraft.global_transform = Transform3D(Basis(sample["rotation"]), sample["position"])
			aerodynamics.elevator = sample["elevator"]
			aerodynamics.aileron = sample["aileron"]
			aerodynamics.rudder = sample["rudder"]
		if replay_time >= recorder.get_duration():
			stop_replay()

func toggle_recording() -> void:
	if recorder.is_recording():
		recorder.stop_recording()
	else:
		stop_replay()
		aircraft.freeze = false
		recorder.start_recording()

func begin_replay() -> void:
	if recorder.get_sample_count() == 0:
		return
	recorder.stop_recording()
	aircraft.freeze = true
	replay_time = 0.0
	replaying = true

func stop_replay() -> void:
	replaying = false
	if aircraft != null:
		aircraft.freeze = false

func save_mission(path: String) -> bool:
	return recorder.save_to_file(path)

func load_mission(path: String) -> bool:
	stop_replay()
	return recorder.load_from_file(path)
