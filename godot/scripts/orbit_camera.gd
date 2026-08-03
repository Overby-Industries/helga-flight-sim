class_name HelgaOrbitCamera
extends Camera3D
## Free-look camera for inspecting the aircraft from any angle, e.g.
## while parked in the editor/dev-testing before the world environment
## exists. Right-mouse-drag orbits, scroll wheel zooms. Deliberately
## uses no keyboard input at all, so it never competes with
## aircraft_control.gd's WASD/arrow throttle+control bindings even when
## both are active in the same scene.

@export var target_path: NodePath
@export var distance: float = 40.0
@export var min_distance: float = 5.0
@export var max_distance: float = 300.0
@export var orbit_speed: float = 0.005
@export var zoom_step: float = 3.0

var target: Node3D
var yaw: float = 0.0
var pitch: float = -0.2
var _dragging: bool = false

func _ready() -> void:
	target = get_node_or_null(target_path) as Node3D

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_RIGHT:
			_dragging = event.pressed
		elif event.button_index == MOUSE_BUTTON_WHEEL_UP and event.pressed:
			distance = maxf(min_distance, distance - zoom_step)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN and event.pressed:
			distance = minf(max_distance, distance + zoom_step)
	elif event is InputEventMouseMotion and _dragging:
		yaw -= event.relative.x * orbit_speed
		pitch = clampf(pitch - event.relative.y * orbit_speed, -1.4, 1.4)

func _process(_delta: float) -> void:
	if target == null:
		return
	var offset := Vector3(
		distance * cos(pitch) * sin(yaw),
		distance * sin(pitch),
		distance * cos(pitch) * cos(yaw)
	)
	global_position = target.global_position + offset
	look_at(target.global_position, Vector3.UP)
