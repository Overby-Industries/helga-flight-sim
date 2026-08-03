class_name HelgaAirframeVisual
extends Node3D
## Swaps between the procedural placeholder mesh (HelgaAirframe, see
## src/airframe.cpp) and an imported 3D model once one exists. Until
## imported_model is assigned, the procedural mesh shows unchanged --
## this node adds zero behavior change today, it's just the seam where
## a Blender-exported model plugs in later, so the aerodynamics/
## propulsion/gear/flight-computer code never has to know or care which
## one is active.
##
## Pipeline: build/refine geometry in XFLR5 -> bring into Blender for
## detail/materials -> export glTF (.glb) into godot/models/ (Godot
## auto-imports it as a PackedScene) -> assign that PackedScene to
## imported_model here.
##
## Conventions the model should follow to match the rest of the scene:
## forward = -Z, up = +Y, origin at HelgaAircraft's own reference point
## (wherever the RigidBody3D's origin sits today), scale in meters.
## HelgaAeroSurface/gear/camera positions are all still hand-placed
## relative to that same origin and will need re-tuning once a real
## model's proportions are known -- not done yet, since there's nothing
## to match against until a model exists.

@export var imported_model: PackedScene

@onready var procedural_airframe: HelgaAirframe = $Airframe

func _ready() -> void:
	if imported_model != null:
		var instance := imported_model.instantiate()
		add_child(instance)
		procedural_airframe.visible = false
