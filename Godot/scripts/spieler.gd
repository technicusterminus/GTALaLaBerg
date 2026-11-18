class_name Spieler
extends CharacterBody3D
## Der Spieler zu Fuss. Einsteigen und Aussteigen uebernimmt welt.gd.

const GEHEN := 4.5
const RENNEN := 8.5
const SPRUNG := 5.2

var kamera: Kamera
var _schwerkraft: float = ProjectSettings.get_setting("physics/3d/default_gravity")
var _koerper := Node3D.new()


func _ready() -> void:
	var kapsel := CollisionShape3D.new()
	var form := CapsuleShape3D.new()
	form.height = 1.8
	form.radius = 0.35
	kapsel.shape = form
	kapsel.position.y = 0.9
	add_child(kapsel)

	var mesh := MeshInstance3D.new()
	var kapselmesh := CapsuleMesh.new()
	kapselmesh.height = 1.8
	kapselmesh.radius = 0.35
	mesh.mesh = kapselmesh
	mesh.position.y = 0.9
	var farbe := StandardMaterial3D.new()
	farbe.albedo_color = Color(0.85, 0.35, 0.15)
	mesh.material_override = farbe
	_koerper.add_child(mesh)
	# Eine Nase zeigt, wohin die Figur schaut.
	var nase := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = Vector3(0.18, 0.18, 0.3)
	nase.mesh = box
	nase.position = Vector3(0, 1.5, -0.35)
	nase.material_override = farbe
	_koerper.add_child(nase)
	add_child(_koerper)


func aktiv(an: bool) -> void:
	visible = an
	collision_layer = 1 if an else 0
	collision_mask = 1 if an else 0
	set_physics_process(an)
	velocity = Vector3.ZERO


func _physics_process(delta: float) -> void:
	var eingabe := Input.get_vector("links", "rechts", "vorwaerts", "rueckwaerts")
	var richtung := Vector3.ZERO
	if kamera != null:
		richtung = kamera.rechts() * eingabe.x - kamera.vorwaerts() * eingabe.y
	var tempo := RENNEN if Input.is_action_pressed("rennen") else GEHEN
	velocity.x = move_toward(velocity.x, richtung.x * tempo, 40.0 * delta)
	velocity.z = move_toward(velocity.z, richtung.z * tempo, 40.0 * delta)
	if is_on_floor():
		if Input.is_action_just_pressed("springen"):
			velocity.y = SPRUNG
	else:
		velocity.y -= _schwerkraft * delta
	move_and_slide()
	if richtung.length_squared() > 0.01:
		var ziel := atan2(-richtung.x, -richtung.z)
		_koerper.rotation.y = lerp_angle(_koerper.rotation.y, ziel, minf(12.0 * delta, 1.0))
