class_name HelgaGroundTrackingCamera
extends Camera3D
## A fixed ground-based camera that continuously points at a target and
## zooms its FOV to keep the target a roughly constant apparent size --
## like a real launch tracking telescope (wide at liftoff, progressively
## more telephoto as the vehicle climbs away and shrinks with distance)
## rather than a chase camera that follows the target's position.
##
## This is a cinematic/gameplay camera, not an optical simulation: it
## will keep "tracking" (pointing at and zooming toward) the target even
## at LEO/re-entry distances a real ground telescope could never
## actually resolve. That's intentional -- the point is a usable,
## always-locked-on shot for replay cinematography, not realism.

@export var target_path: NodePath
@export var assumed_target_span_m: float = 14.0 ## Helga's rough overall length/span, for the apparent-size estimate
@export var desired_screen_fraction: float = 0.15 ## how much of the vertical FOV the target should occupy
@export var min_fov_deg: float = 0.8 ## extreme telephoto, for tracking at high altitude/orbit
@export var max_fov_deg: float = 60.0
@export var zoom_speed: float = 1.5 ## how fast FOV eases toward the desired value, per second

var target: Node3D

func _ready() -> void:
	target = get_node_or_null(target_path) as Node3D

func _process(delta: float) -> void:
	if target == null:
		return

	look_at(target.global_position, Vector3.UP)

	var distance := maxf(global_position.distance_to(target.global_position), 1.0)
	var object_angular_size_deg := rad_to_deg(2.0 * atan2(assumed_target_span_m * 0.5, distance))
	var desired_fov := clampf(object_angular_size_deg / desired_screen_fraction, min_fov_deg, max_fov_deg)
	fov = lerpf(fov, desired_fov, clampf(delta * zoom_speed, 0.0, 1.0))
