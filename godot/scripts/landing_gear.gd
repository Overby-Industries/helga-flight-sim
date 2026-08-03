class_name HelgaLandingGear
extends Node3D
## Retractable landing gear: three legs (nose + left/right main) that
## animate between extended and retracted over a few seconds. G toggles.
##
## Deliberately just state plus a visual animation -- there's no
## collision-shape swap for a gear-up landing yet (a genuine DCS-style
## "bellied it in" consequence is future work). The safety-relevant part
## built now is the HUD warning if the flight computer reaches LANDING
## with the gear not fully down (see hud.gd).

## Each leg is a small Node3D whose local Y position this script
## animates -- see main.tscn's NoseGear/LeftMainGear/RightMainGear.
@export var leg_paths: Array[NodePath] = []
@export var extend_seconds: float = 3.0
@export var gear_travel: float = 1.0 ## meters the strut assembly retracts upward

var _legs: Array[Node3D] = []
var _extended: bool = true
var _fraction: float = 1.0 ## 0 = fully retracted, 1 = fully extended

func _ready() -> void:
	for path in leg_paths:
		var leg := get_node_or_null(path) as Node3D
		if leg != null:
			_legs.append(leg)

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and event.physical_keycode == KEY_G:
		_extended = not _extended

func _process(delta: float) -> void:
	var target := 1.0 if _extended else 0.0
	var step := delta / maxf(extend_seconds, 0.01)
	_fraction = move_toward(_fraction, target, step)
	for leg in _legs:
		leg.position.y = gear_travel * (1.0 - _fraction)

func is_extended() -> bool:
	return _fraction >= 0.999

func is_retracted() -> bool:
	return _fraction <= 0.001

func get_extension_fraction() -> float:
	return _fraction
