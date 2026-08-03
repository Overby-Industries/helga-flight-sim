class_name HelgaPauseMenu
extends CanvasLayer
## Esc-triggered pause menu: resume, quicksave/load the current mission
## recording (see HelgaFlightRecorder/replay_controller.gd), return to
## the main menu, or quit. process_mode is set to Always in the scene so
## this keeps responding to input while get_tree().paused freezes
## everything else.

@export var replay_controller_path: NodePath

const SAVE_PATH := "user://missions/quicksave.hflight"

var replay_controller: HelgaReplayController
var is_open: bool = false

func _ready() -> void:
	replay_controller = get_node_or_null(replay_controller_path) as HelgaReplayController
	%Root.visible = false
	%ResumeButton.pressed.connect(_on_resume_pressed)
	%SaveButton.pressed.connect(_on_save_pressed)
	%LoadButton.pressed.connect(_on_load_pressed)
	%MainMenuButton.pressed.connect(_on_main_menu_pressed)
	%QuitButton.pressed.connect(_on_quit_pressed)

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo and event.physical_keycode == KEY_ESCAPE:
		toggle()

func toggle() -> void:
	if is_open:
		close()
	else:
		open()

func open() -> void:
	is_open = true
	%Root.visible = true
	%StatusLabel.text = ""
	get_tree().paused = true

func close() -> void:
	is_open = false
	%Root.visible = false
	get_tree().paused = false

func _on_resume_pressed() -> void:
	close()

func _on_save_pressed() -> void:
	DirAccess.make_dir_recursive_absolute("user://missions")
	if replay_controller != null and replay_controller.save_mission(SAVE_PATH):
		%StatusLabel.text = "Saved to " + SAVE_PATH
	else:
		%StatusLabel.text = "Nothing to save -- record a flight first (R)."

func _on_load_pressed() -> void:
	if replay_controller != null and replay_controller.load_mission(SAVE_PATH):
		%StatusLabel.text = "Loaded. Close this menu and press P to replay."
	else:
		%StatusLabel.text = "No saved mission found at " + SAVE_PATH

func _on_main_menu_pressed() -> void:
	get_tree().paused = false
	get_tree().change_scene_to_file("res://scenes/main_menu.tscn")

func _on_quit_pressed() -> void:
	get_tree().quit()
