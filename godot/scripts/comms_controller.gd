class_name HelgaCommsController
extends Node
## Ground/Tower/Departure/Approach comms: a small curated phrase set that
## reacts to the flight computer's real state changes (see
## src/flight_computer.h's state_changed signal) rather than a scripted
## mission timeline or a full ATC simulator, per docs/DESIGN.md's ATC &
## comms section.
##
## Each line below only ever fires in direct response to
## HelgaFlightComputer.state_changed actually reaching that state, so the
## design doc's "a tower clearance only counts once the FSM is in TAXI"
## rule holds by construction -- there is no code path that can issue a
## clearance for a phase the aircraft isn't actually in.
##
## STATE_HOLDING_PATTERN and STATE_DIVERT are deliberately not wired to
## an automatic trigger here -- those represent ATC issuing a hold/
## diversion for its own reasons (traffic, weather), which this sim
## doesn't model. They stay available for a future scripted-mission layer
## to invoke directly via flight_computer.transition_to().

@export var flight_computer_path: NodePath

const MAX_LOG_LINES := 6
const READBACK_KEY := KEY_T

var flight_computer: HelgaFlightComputer
var log_lines: Array[String] = []
var pending_readback: String = ""
var pending_station: String = ""

# HelgaFlightComputer state (int) -> {station, line, readback}. "readback"
# is "" where a plain instruction doesn't call for one (flavor-only lines).
var _phrases: Dictionary = {}

func _build_phrases() -> void:
	_phrases = {
		HelgaFlightComputer.TAXI: {
			"station": "GROUND",
			"line": "Helga, Ground, taxi to Runway 27 via Alpha, hold short.",
			"readback": "Ground, Helga, taxi to Runway 27 via Alpha, hold short, Helga.",
		},
		HelgaFlightComputer.TAKEOFF: {
			"station": "TOWER",
			"line": "Helga, Tower, wind calm, Runway 27, cleared for takeoff.",
			"readback": "Cleared for takeoff, Runway 27, Helga.",
		},
		HelgaFlightComputer.CLIMB: {
			"station": "DEPARTURE",
			"line": "Helga, Departure, radar contact, climb via the ascent profile.",
			"readback": "Climb via the ascent profile, Helga.",
		},
		HelgaFlightComputer.ASCENT: {
			"station": "DEPARTURE",
			"line": "Helga, Departure, radar service terminated, cleared for the ascent profile, contact Approach for the return.",
			"readback": "",
		},
		HelgaFlightComputer.APPROACH: {
			"station": "APPROACH",
			"line": "Helga, Approach, radar contact, expect vectors for Runway 27, number one.",
			"readback": "Vectors for Runway 27, Helga.",
		},
		HelgaFlightComputer.LANDING: {
			"station": "TOWER",
			"line": "Helga, Tower, Runway 27, cleared to land.",
			"readback": "Cleared to land, Runway 27, Helga.",
		},
		HelgaFlightComputer.STATE_POST_LANDING_CHECK: {
			"station": "GROUND",
			"line": "Helga, Ground, taxi to parking when able.",
			"readback": "Taxi to parking, Helga.",
		},
		HelgaFlightComputer.STATE_ABORT_TAKEOFF: {
			"station": "TOWER",
			"line": "Helga, Tower, roger the abort, say intentions when ready.",
			"readback": "",
		},
		HelgaFlightComputer.STATE_GO_AROUND: {
			"station": "TOWER",
			"line": "Helga, Tower, go around acknowledged, fly runway heading, climb and maintain one thousand.",
			"readback": "Going around, runway heading, one thousand, Helga.",
		},
		HelgaFlightComputer.STATE_SHALLOW_REENTRY_CONTINGENCY: {
			"station": "APPROACH",
			"line": "Helga, Approach, understand you're extending for another entry attempt.",
			"readback": "",
		},
		HelgaFlightComputer.STATE_THERMAL_OVERLOAD: {
			"station": "APPROACH",
			"line": "Helga, Approach, say your status.",
			"readback": "",
		},
		HelgaFlightComputer.FAULT: {
			"station": "TOWER",
			"line": "Helga, say again your status.",
			"readback": "",
		},
	}

func _ready() -> void:
	_build_phrases()
	flight_computer = get_node_or_null(flight_computer_path) as HelgaFlightComputer
	if flight_computer != null:
		flight_computer.state_changed.connect(_on_state_changed)

func _on_state_changed(_previous_state: int, current_state: int) -> void:
	var entry: Dictionary = _phrases.get(current_state, {})
	if entry.is_empty():
		return
	_append_log("%s: %s" % [entry["station"], entry["line"]])
	var readback: String = entry.get("readback", "")
	pending_readback = readback
	pending_station = entry["station"] if readback != "" else ""

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and event.physical_keycode == READBACK_KEY:
		if pending_readback != "":
			_append_log("HELGA: %s" % pending_readback)
			pending_readback = ""
			pending_station = ""

func _append_log(line: String) -> void:
	log_lines.append(line)
	while log_lines.size() > MAX_LOG_LINES:
		log_lines.pop_front()

func get_log_lines() -> Array[String]:
	return log_lines

func has_pending_readback() -> bool:
	return pending_readback != ""

func get_pending_station() -> String:
	return pending_station
