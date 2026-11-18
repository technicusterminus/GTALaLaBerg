class_name Kamera
extends Node3D
## Schulterkamera an einem Federarm. Zu Fuss und im Wagen dieselbe Kamera,
## nur das Ziel und der Abstand wechseln.

const EMPFINDLICHKEIT := 0.0025

var ziel: Node3D
var hoehe := 1.5
var yaw := 0.0
var pitch := -0.25
var folge_yaw := NAN    ## gesetzt: Kamera schwenkt hinter diese Blickrichtung
var _ruhe := 0.0

var _arm := SpringArm3D.new()
var _kamera := Camera3D.new()


func _ready() -> void:
	top_level = true
	_arm.spring_length = 4.5
	_arm.margin = 0.3
	_arm.collision_mask = 1
	_arm.add_child(_kamera)
	add_child(_arm)
	# Strassen liegen nur Zentimeter ueber dem Boden: eine enge Near-Plane
	# macht den Tiefenpuffer dort zu grob (Ringe durch Z-Fighting).
	_kamera.near = 0.5
	_kamera.far = 3000.0
	_kamera.fov = 70.0
	_kamera.current = true


func abstand_setzen(a: float) -> void:
	_arm.spring_length = a


func ausnehmen(koerper: CollisionObject3D) -> void:
	_arm.clear_excluded_objects()
	_arm.add_excluded_object(koerper.get_rid())


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		yaw -= event.relative.x * EMPFINDLICHKEIT
		pitch = clampf(pitch - event.relative.y * EMPFINDLICHKEIT, -1.2, 0.6)
		_ruhe = 0.0


func _process(delta: float) -> void:
	if ziel == null:
		return
	_ruhe += delta
	if not is_nan(folge_yaw) and _ruhe > 1.0:
		yaw = lerp_angle(yaw, folge_yaw, minf(2.0 * delta, 1.0))
	var soll := ziel.global_position + Vector3(0, hoehe, 0)
	global_position = global_position.lerp(soll, minf(20.0 * delta, 1.0))
	rotation = Vector3(0, yaw, 0)
	_arm.rotation = Vector3(pitch, 0, 0)


## Strahl durch die Bildmitte: Ursprung und Richtung in Weltkoordinaten.
func blick_strahl() -> Dictionary:
	return {"von": _kamera.global_position, "richtung": -_kamera.global_basis.z}


## Blickrichtung waagerecht, fuer Bewegung relativ zur Kamera.
func vorwaerts() -> Vector3:
	return Vector3(-sin(yaw), 0, -cos(yaw))


func rechts() -> Vector3:
	return Vector3(cos(yaw), 0, -sin(yaw))
