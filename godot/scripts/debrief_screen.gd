class_name HelgaDebriefScreen
extends CanvasLayer
## Post-flight debrief -- a flight-test data summary (see
## docs/DESIGN.md's Data Collection section), not a score screen. Opens
## automatically once the flight computer reaches
## STATE_POST_LANDING_CHECK (see src/flight_computer.h), the natural
## "sortie complete" moment given this sim's single-sortie scope.
## process_mode is Always in the scene so Enter/the button still work
## while get_tree().paused freezes everything else, matching pause_menu.gd.

@export var flight_computer_path: NodePath
@export var telemetry_recorder_path: NodePath

var flight_computer: HelgaFlightComputer
var telemetry_recorder: HelgaTelemetryRecorder

func _ready() -> void:
	flight_computer = get_node_or_null(flight_computer_path) as HelgaFlightComputer
	telemetry_recorder = get_node_or_null(telemetry_recorder_path) as HelgaTelemetryRecorder
	%Root.visible = false
	%DismissButton.pressed.connect(close)
	if flight_computer != null:
		flight_computer.state_changed.connect(_on_state_changed)

func _on_state_changed(_previous_state: int, current_state: int) -> void:
	if current_state == HelgaFlightComputer.STATE_POST_LANDING_CHECK:
		open()

func _unhandled_input(event: InputEvent) -> void:
	if %Root.visible and event is InputEventKey and event.pressed and event.physical_keycode == KEY_ENTER:
		close()

func open() -> void:
	if telemetry_recorder != null:
		%MaxQLabel.text = "Max dynamic pressure     %6.0f Pa" % telemetry_recorder.max_dynamic_pressure_pa
		%HeatFluxLabel.text = "Peak reentry heat flux   %6.0f kW/m2" % (telemetry_recorder.peak_heat_flux_w_m2 / 1000.0)
		%GLoadLabel.text = "Max g-load               %6.2f g" % telemetry_recorder.max_g_load
		%CorridorLabel.text = "Max corridor deviation   %5.0f%%" % (telemetry_recorder.max_corridor_deviation * 100.0)
		%SinkRateLabel.text = "Touchdown sink rate      %6.2f m/s" % telemetry_recorder.touchdown_sink_rate_ms
	%Root.visible = true
	get_tree().paused = true

func close() -> void:
	%Root.visible = false
	get_tree().paused = false
