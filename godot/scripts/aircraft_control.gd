extends RigidBody3D
## Placeholder pilot input + propulsion for smoke-testing the
## aerodynamics model (see src/aerodynamics.h / src/aero_surface.h).
##
## Controls are a placeholder scheme using Godot's built-in UI input
## actions (no InputMap editing required): arrow keys for pitch/roll,
## A/D for rudder, W/S for throttle. Real control mapping, and Helga's
## actual hybrid ABEP/MHD-Lorentz/ionic-liquid propulsion (see
## docs/DESIGN.md), are future work -- this is a plain forward thrust
## force so the flight model can actually be flown and tuned now.
##
## First-flight note: elevator/aileron sign was derived from the wing
## placement aft of the center of mass (see aero_surface.h/aerodynamics
## comments), but hasn't been visually confirmed in-editor. If pitch or
## roll feels inverted, flip the corresponding sign on elevator_gain/
## aileron_gain in the wing nodes.

@onready var aerodynamics: HelgaAerodynamics = $Aerodynamics

@export var max_thrust_newtons: float = 180000.0

var throttle: float = 0.0

func _physics_process(delta: float) -> void:
	var pitch_input := Input.get_axis("ui_down", "ui_up")
	var roll_input := Input.get_axis("ui_left", "ui_right")
	var yaw_input := 0.0
	if Input.is_key_pressed(KEY_A):
		yaw_input -= 1.0
	if Input.is_key_pressed(KEY_D):
		yaw_input += 1.0
	if Input.is_key_pressed(KEY_W):
		throttle = min(throttle + delta, 1.0)
	if Input.is_key_pressed(KEY_S):
		throttle = max(throttle - delta, 0.0)

	aerodynamics.elevator = pitch_input
	aerodynamics.aileron = roll_input
	aerodynamics.rudder = yaw_input

	var forward := -global_transform.basis.z
	apply_central_force(forward * throttle * max_thrust_newtons)
