class_name HelgaCameraRig
extends Node
## Switches between a fixed set of Camera3D views with a single key
## (default: C). Cameras are referenced by NodePath rather than direct
## node exports so this can be hand-authored/edited in a .tscn without
## depending on editor-only node-reference serialization.

@export var camera_paths: Array[NodePath] = []

var cameras: Array[Camera3D] = []
var current_index: int = 0

func _ready() -> void:
	for path in camera_paths:
		var cam := get_node_or_null(path) as Camera3D
		if cam != null:
			cameras.append(cam)
	_activate(current_index)

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and event.physical_keycode == KEY_C:
		next_camera()

func next_camera() -> void:
	if cameras.is_empty():
		return
	current_index = (current_index + 1) % cameras.size()
	_activate(current_index)

func get_active_camera() -> Camera3D:
	if cameras.is_empty():
		return null
	return cameras[current_index]

func _activate(index: int) -> void:
	if cameras.is_empty():
		return
	for i in cameras.size():
		cameras[i].current = (i == index)
