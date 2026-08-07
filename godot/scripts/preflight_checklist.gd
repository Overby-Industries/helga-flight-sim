class_name HelgaPreflightChecklist
extends CanvasLayer
## Preflight systems checklist -- "confirm FC-A/FC-B/FC-C health, sensor
## fusion lock, propulsion controller status" per docs/DESIGN.md's core
## loop step 1. Most of these have no dedicated simulated subsystem yet
## (there's one HelgaFlightComputer, not three redundant ones, and no
## sensor fusion model) -- consistent with real GA preflight checklists,
## where most items are pilot-verified rather than sensor-verified, those
## are pilot-confirmed keypresses rather than invented instrumentation.
## The one item with real backing data (propulsion controller status)
## actually checks HelgaPropulsion's power reserve rather than just
## taking the keypress on faith.
##
## aircraft_control.gd holds a reference to this and clamps throttle to
## zero while the flight computer is in PREFLIGHT and this checklist
## isn't complete -- so HelgaFlightComputer's own PREFLIGHT->TAXI auto-
## transition guard (throttle > taxi_throttle, see flight_computer.h)
## can never actually fire until the checklist is done. That keeps the
## checklist entirely in GDScript/UI territory without the C++ FSM
## needing to know it exists.

@export var propulsion_path: NodePath
@export var flight_computer_path: NodePath

const POWER_RESERVE_MIN := 0.5

const ITEM_LABELS := [
	"FC-A primary flight computer",
	"FC-B backup flight computer",
	"FC-C backup flight computer",
	"Sensor fusion",
	"Propulsion controller status",
]

var propulsion: HelgaPropulsion
var flight_computer: HelgaFlightComputer

var checked: Array[bool] = [false, false, false, false, false]
var _propulsion_flash := false

func _ready() -> void:
	propulsion = get_node_or_null(propulsion_path) as HelgaPropulsion
	flight_computer = get_node_or_null(flight_computer_path) as HelgaFlightComputer

func is_complete() -> bool:
	for c in checked:
		if not c:
			return false
	return true

func _unhandled_input(event: InputEvent) -> void:
	if not (event is InputEventKey and event.pressed):
		return
	var index := _index_for_key(event.physical_keycode)
	if index == -1 or checked[index]:
		return
	if index == 4:
		if propulsion != null and propulsion.get_power_reserve() >= POWER_RESERVE_MIN:
			checked[index] = true
			_propulsion_flash = false
		else:
			_propulsion_flash = true
	else:
		checked[index] = true

func _index_for_key(keycode: int) -> int:
	match keycode:
		KEY_1: return 0
		KEY_2: return 1
		KEY_3: return 2
		KEY_4: return 3
		KEY_5: return 4
		_: return -1

func _process(_delta: float) -> void:
	var relevant := flight_computer != null and flight_computer.get_current_state() == HelgaFlightComputer.PREFLIGHT
	%Root.visible = relevant
	if not relevant:
		return

	%Item1Label.text = _line(0)
	%Item2Label.text = _line(1)
	%Item3Label.text = _line(2)
	%Item4Label.text = _line(3)
	%Item5Label.text = _line(4)

	if is_complete():
		%StatusLabel.text = "PREFLIGHT COMPLETE -- throttle up to taxi."
		%StatusLabel.modulate = Color(0.4, 1.0, 0.55, 1)
	elif _propulsion_flash:
		%StatusLabel.text = "Propulsion controller status NOT NOMINAL -- power reserve below %d%%." % int(POWER_RESERVE_MIN * 100.0)
		%StatusLabel.modulate = Color(1.0, 0.3, 0.2, 1)
	else:
		%StatusLabel.text = "Press 1-5 to complete the preflight checklist."
		%StatusLabel.modulate = Color(1, 1, 1, 0.85)

func _line(i: int) -> String:
	var mark := "[X]" if checked[i] else "[ ]"
	return "%s %d. %s" % [mark, i + 1, ITEM_LABELS[i]]
