class_name HelgaMainMenu
extends Control

func _ready() -> void:
	%StartButton.pressed.connect(_on_start_pressed)
	%QuitButton.pressed.connect(_on_quit_pressed)
	%StartButton.grab_focus()

func _on_start_pressed() -> void:
	get_tree().change_scene_to_file("res://scenes/main.tscn")

func _on_quit_pressed() -> void:
	get_tree().quit()
