class_name Auto
extends VehicleBody3D
## Ein fahrbares Auto. Die Karosserie kommt aus Fahrzeug/wagen.json (Quelle der
## Unreal-Version); die Fahrphysik ist Godots VehicleBody3D.
## Vorwaerts ist in Godot -Z, die Quelldaten schauen nach +X: die Huelle ist
## deshalb um 90 Grad gedreht.

const SHADER := preload("res://shader/flach.gdshader")

const MOTOR := 7500.0
const RUECKWAERTS := 4000.0
const BREMSE := 45.0
const HOECHST_MS := 48.0

@export var lack := Color(0.55, 0.06, 0.05)

var fahrer: Node3D
var tempo_ms := 0.0
var _lenkung := 0.0


func _ready() -> void:
	add_to_group("auto")
	mass = 1300.0
	center_of_mass_mode = RigidBody3D.CENTER_OF_MASS_MODE_CUSTOM
	center_of_mass = Vector3(0, -0.15, 0)
	linear_damp = 0.05
	angular_damp = 1.0

	var form := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = Vector3(1.8, 0.9, 4.3)
	form.shape = box
	form.position = Vector3(0, 0.45, 0)
	add_child(form)

	var huelle := Node3D.new()
	huelle.rotation.y = PI / 2.0
	huelle.position.y = -0.315  # Boden der Quelldaten liegt auf z = 0, Radmitte 31,5 cm
	huelle.add_child(_baue_karosserie())
	add_child(huelle)

	# Radstand und Spur aus den Quelldaten: achsen +-131 cm, spur 76 cm
	for vorne in [true, false]:
		for links in [true, false]:
			var rad := VehicleWheel3D.new()
			rad.position = Vector3(-0.76 if links else 0.76, 0.0, -1.31 if vorne else 1.31)
			rad.wheel_radius = 0.315
			rad.suspension_travel = 0.25
			rad.suspension_stiffness = 45.0
			rad.suspension_max_force = 9000.0
			rad.wheel_rest_length = 0.12
			rad.damping_compression = 2.2
			rad.damping_relaxation = 2.6
			rad.wheel_friction_slip = 3.2
			rad.wheel_roll_influence = 0.35
			rad.use_as_steering = vorne
			rad.use_as_traction = true
			add_child(rad)


func _physics_process(delta: float) -> void:
	tempo_ms = linear_velocity.length()
	var vorwaerts_ms := -global_basis.z.dot(linear_velocity)
	if fahrer == null:
		engine_force = 0.0
		brake = BREMSE * 0.2
		steering = move_toward(steering, 0.0, delta)
		return
	var gas := Input.get_axis("rueckwaerts", "vorwaerts")
	var lenk := Input.get_axis("rechts", "links")
	var bremse := 0.0
	var kraft := 0.0
	if gas > 0.0:
		kraft = gas * MOTOR
	elif gas < 0.0:
		if vorwaerts_ms > 1.0:
			bremse = BREMSE * -gas
		else:
			kraft = gas * RUECKWAERS
	if tempo_ms > HOECHST_MS:
		kraft = 0.0
	if Input.is_action_pressed("handbremse"):
		bremse = BREMSE * 1.5
	engine_force = kraft
	brake = bremse
	var maximal := lerpf(0.5, 0.1, clampf(tempo_ms / 30.0, 0.0, 1.0))
	_lenkung = move_toward(_lenkung, lenk * maximal, 2.5 * delta)
	steering = _lenkung


func _baue_karosserie() -> MeshInstance3D:
	var text := FileAccess.get_file_as_string("res://daten/wagen.json")
	var wagen = JSON.parse_string(text)
	var netz := ArrayMesh.new()
	if wagen == null:
		push_error("daten/wagen.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
	else:
		for teil: Dictionary in wagen["teile"]:
			var roh: Array = teil["positions"]
			var ecken := PackedVector3Array()
			ecken.resize(roh.size() / 3)
			for i in ecken.size():
				# Zentimeter, (x vorwaerts, y rechts, z oben) -> Meter, (x, y oben, z)
				ecken[i] = Vector3(roh[i * 3], roh[i * 3 + 2], roh[i * 3 + 1]) / 100.0
			var roh_idx: Array = teil["indices"]
			var indizes := PackedInt32Array()
			indizes.resize(roh_idx.size())
			for i in indizes.size():
				indizes[i] = int(roh_idx[i])
			var felder := []
			felder.resize(Mesh.ARRAY_MAX)
			felder[Mesh.ARRAY_VERTEX] = ecken
			felder[Mesh.ARRAY_INDEX] = indizes
			netz.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, felder)
			var m := ShaderMaterial.new()
			m.shader = SHADER
			var c: Array = teil["color"]
			var lackiert: bool = teil.get("lack", false)
			var farbe := Vector3(lack.r, lack.g, lack.b) if lackiert else Vector3(c[0], c[1], c[2])
			m.set_shader_parameter("farbe", farbe)
			var glatt: bool = teil["klasse"] == "Glas" or lackiert
			m.set_shader_parameter("rauheit", 0.25 if glatt else 0.8)
			m.set_shader_parameter("glanz", 1.0 if glatt else 0.0)
			netz.surface_set_material(netz.get_surface_count() - 1, m)
	var instanz := MeshInstance3D.new()
	instanz.mesh = netz
	return instanz
