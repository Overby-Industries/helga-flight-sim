class_name HelgaAircraftControl
extends RigidBody3D
## Pilot input for the aerodynamics model (see src/aerodynamics.h /
## src/aero_surface.h) and the unified propulsion controller (see
## src/propulsion.h).
##
## Reads a HOTAS if one is connected (tuned for a Thrustmaster T.16000M
## FCS stick + TWCS Throttle pair), falling back to keyboard so the game
## is still playable without the hardware attached. Joystick axis
## numbering can vary by OS/driver/whether the stick+throttle enumerate
## as one device or two -- if a control feels wrong or unresponsive,
## press F12 in-game to print every connected joypad's live axis values
## to the console and adjust the constants below to match.
##
## Real control mapping (rebindable settings, curves) is future work --
## throttle is a plain 0..1 lever handed to HelgaPropulsion, which is
## where the actual ABEP/MHD-Lorentz/IL-afterburner blending happens.
##
## First-flight note: elevator/aileron sign was derived from the wing
## placement aft of the center of mass (see aero_surface.h/aerodynamics
## comments), but hasn't been visually confirmed in-editor. If pitch or
## roll feels inverted, flip the corresponding sign on elevator_gain/
## aileron_gain in the wing nodes.

@onready var aerodynamics: HelgaAerodynamics = $Aerodynamics
@onready var propulsion: HelgaPropulsion = $Propulsion
@onready var flight_computer: HelgaFlightComputer = $FlightComputer
@onready var gravity: HelgaGravity = $Gravity
@onready var reentry_system: HelgaReentrySystem = $ReentrySystem
@onready var autopilot: HelgaAutopilot = $Autopilot
@onready var terrain: Node = get_node_or_null("../Terrain")

@export var preflight_checklist_path: NodePath
var preflight_checklist: HelgaPreflightChecklist

## Autopilot recovery mode: engages automatically on a missed reentry
## approach (STATE_SHALLOW_REENTRY_CONTINGENCY, see src/autopilot.h and
## src/flight_computer.h) and disengages automatically on leaving that
## state. O also toggles it manually at any time, the way a real
## autopilot disconnect works, in case the pilot wants to fly the
## recovery by hand or re-engage early.
func _ready() -> void:
	preflight_checklist = get_node_or_null(preflight_checklist_path) as HelgaPreflightChecklist
	flight_computer.state_changed.connect(_on_flight_state_changed)

func _on_flight_state_changed(_previous_state: int, current_state: int) -> void:
	if current_state == HelgaFlightComputer.STATE_SHALLOW_REENTRY_CONTINGENCY:
		autopilot.engaged = true
	elif autopilot.engaged:
		autopilot.engaged = false

## Flap lever stages -- F cycles through these in order (up, takeoff/
## approach, landing), like the discrete flap settings on a real
## airliner rather than a continuously variable lever.
const FLAP_STAGES: Array[float] = [0.0, 0.5, 1.0]
var flap_stage_index: int = 0

## Godot's named JOY_AXIS_* constants (LEFT_X/LEFT_Y/RIGHT_X/RIGHT_Y/
## TRIGGER_LEFT/TRIGGER_RIGHT) describe a standard gamepad, not a HOTAS --
## a flight stick's raw axis indices don't line up with that naming, so
## these are plain integers matching the commonly-reported raw axis order
## for a T.16000M FCS (X=0, Y=1, twist/yaw=2) instead.
##
## Stick is assumed to be joypad device 0 -- the common case when a
## T.16000M FCS + TWCS Throttle pair enumerates as two separate devices.
const STICK_DEVICE := 0
const STICK_ROLL_AXIS := 0  ## X axis
const STICK_PITCH_AXIS := 1 ## Y axis
const STICK_YAW_AXIS := 2   ## Z-rotation / twist axis (rudder)

## TWCS Throttle is assumed to be joypad device 1. Its main slider axis
## index varies more than the stick's -- this is the value most likely
## to need adjusting per-system. Use F12 to confirm before assuming
## this default is wrong.
const THROTTLE_DEVICE := 1
const THROTTLE_AXIS := 1

const JOY_DEADZONE := 0.08

var throttle: float = 0.0

func _get_joy_axis_or_zero(device: int, axis: int) -> float:
	if device >= Input.get_connected_joypads().size():
		return 0.0
	var v := Input.get_joy_axis(device, axis)
	return v if absf(v) > JOY_DEADZONE else 0.0

func _physics_process(delta: float) -> void:
	var joy_count := Input.get_connected_joypads().size()

	var pitch_input := Input.get_axis("ui_down", "ui_up")
	var roll_input := Input.get_axis("ui_left", "ui_right")
	var yaw_input := 0.0
	if Input.is_key_pressed(KEY_A):
		yaw_input -= 1.0
	if Input.is_key_pressed(KEY_D):
		yaw_input += 1.0

	if autopilot.is_engaged():
		var target := HelgaReentryFlightPlan.get_target_flight_path_angle_deg(global_position.y)
		autopilot.target_flight_path_angle_deg = target
		pitch_input = autopilot.compute_elevator(reentry_system.get_flight_path_angle_deg())

	if joy_count > STICK_DEVICE:
		var jp := _get_joy_axis_or_zero(STICK_DEVICE, STICK_PITCH_AXIS)
		var jr := _get_joy_axis_or_zero(STICK_DEVICE, STICK_ROLL_AXIS)
		var jy := _get_joy_axis_or_zero(STICK_DEVICE, STICK_YAW_AXIS)
		# Stick forward is a positive Y axis value on most joysticks, and
		# should pitch the nose down -- hence the negation. Pitch is left
		# to the autopilot's number while it's engaged -- roll/yaw stay
		# manual either way, per src/autopilot.h's single-axis scope.
		if jp != 0.0 and not autopilot.is_engaged():
			pitch_input = -jp
		if jr != 0.0:
			roll_input = jr
		if jy != 0.0:
			yaw_input = jy

	if joy_count > THROTTLE_DEVICE:
		var jt := Input.get_joy_axis(THROTTLE_DEVICE, THROTTLE_AXIS)
		# Raw throttle axes are typically -1 (idle) .. 1 (full) -- remap to 0..1.
		throttle = clampf((jt + 1.0) * 0.5, 0.0, 1.0)
	else:
		if Input.is_key_pressed(KEY_W):
			throttle = min(throttle + delta, 1.0)
		if Input.is_key_pressed(KEY_S):
			throttle = max(throttle - delta, 0.0)

	aerodynamics.elevator = pitch_input
	aerodynamics.aileron = roll_input
	aerodynamics.rudder = yaw_input
	aerodynamics.flaps = FLAP_STAGES[flap_stage_index]

	# Held at zero until the preflight checklist is complete, which keeps
	# HelgaFlightComputer's own PREFLIGHT->TAXI guard condition (throttle
	# above taxi_throttle, see flight_computer.h) from ever firing early --
	# the C++ FSM doesn't need to know the checklist exists.
	var effective_throttle := throttle
	if preflight_checklist != null and flight_computer.get_current_state() == HelgaFlightComputer.PREFLIGHT and not preflight_checklist.is_complete():
		effective_throttle = 0.0
	propulsion.throttle = effective_throttle

	var horizontal_speed := Vector2(linear_velocity.x, linear_velocity.z).length()
	flight_computer.evaluate_auto_transition(global_position.y, aerodynamics.get_airspeed(), linear_velocity.y, effective_throttle, horizontal_speed, gravity.get_orbital_velocity_ms())

	# Safety net against clipping through the ground: a fast, rotating
	# rigid body can outrun Jolt's own contact resolution for a tick or
	# two right at the moment it leaves/re-touches the ground (seen
	# headless: a hard rotation off the runway at ~60 m/s could punch the
	# fuselage 40+ meters below grade, with Jolt's own penetration
	# recovery only clawing it back out over the following several
	# seconds -- technically self-correcting eventually, but "the runway
	# disappears and the world tunnels" for that whole stretch, and once
	# it's that deep, a soft one-axis nudge back toward the surface isn't
	# enough to break it out of whatever bad contact state it's in --
	# each tick's correction was getting outpaced by the next tick's fall).
	# This is a hard reset once things are unambiguously wrong (well past
	# any tolerance a real gear-contact bounce would need), not a subtle
	# correction: it zeroes all velocity and levels the aircraft out
	# rather than just nudging Y, specifically to break out of a
	# tumble-while-interpenetrating state rather than feed it another
	# tick of bad contact data. This doesn't replace fixing the
	# underlying contact behavior; it just guarantees the aircraft is
	# never left visibly and lastingly below the terrain it's standing on.
	if terrain != null:
		var ground_height: float = terrain.get_height_at(global_position.x, global_position.z)
		if global_position.y < ground_height - 2.0:
			var recovery_transform := global_transform
			recovery_transform.origin.y = ground_height + 3.0
			recovery_transform.basis = Basis.from_euler(Vector3(0.0, recovery_transform.basis.get_euler().y, 0.0))
			global_transform = recovery_transform
			linear_velocity = Vector3.ZERO
			angular_velocity = Vector3.ZERO

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed:
		if event.physical_keycode == KEY_F12:
			_print_joypad_debug_info()
		elif event.physical_keycode == KEY_F:
			flap_stage_index = (flap_stage_index + 1) % FLAP_STAGES.size()
		elif event.physical_keycode == KEY_O:
			autopilot.engaged = not autopilot.engaged

func _print_joypad_debug_info() -> void:
	var devices := Input.get_connected_joypads()
	if devices.is_empty():
		print("No joypads connected.")
		return
	for device in devices:
		print("Joypad %d: %s" % [device, Input.get_joy_name(device)])
		# Godot's named JOY_AXIS_* constants only cover a standard
		# gamepad's 6 axes -- a HOTAS can report more raw axes than that
		# (sliders, additional rotational axes), so this checks a wider
		# range of raw indices rather than relying on that enum.
		for axis in range(12):
			var v := Input.get_joy_axis(device, axis)
			if absf(v) > 0.02:
				print("  axis %d = %.2f" % [axis, v])
