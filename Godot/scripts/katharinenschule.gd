class_name Katharinenschule
extends RefCounted
## Hofseite des früheren Kasernengebäudes an der Waldheimer Straße.

const SEKTOR := Vector2i(-2, 2)
const FRONT := Vector3(-744.20, 24.72, 1068.75)
const WINKEL := -1.309

var _putz := _material("#d8cdbc")
var _stein := _material("#af9f88")
var _weiss := _material("#e6ddc9")
var _ziegel := _material("#9c492b")
var _glas := _material("#3a5861")
var _metall := _material("#68635b")


func baue(sektor: Node3D) -> void:
	var bau := Node3D.new()
	bau.name = "Katharinenschule"
	bau.position = FRONT
	bau.rotation.y = WINKEL
	sektor.add_child(bau)
	_dach(bau)
	_eingang(bau)
	_uhr(bau)
	_sportplatz(sektor)


func _dach(bau: Node3D) -> void:
	var netz := SurfaceTool.new()
	netz.begin(Mesh.PRIMITIVE_TRIANGLES)
	var vorne_l := Vector3(-26.65, 7.94, 0.44)
	var vorne_r := Vector3(26.65, 7.94, 0.44)
	var hinten_l := Vector3(-26.65, 7.94, -17.96)
	var hinten_r := Vector3(26.65, 7.94, -17.96)
	var first_l := Vector3(-26.65, 15.42, -8.76)
	var first_r := Vector3(26.65, 15.42, -8.76)
	for punkt in [vorne_l, first_l, first_r, vorne_l, first_r, vorne_r,
			hinten_r, first_r, first_l, hinten_r, first_l, hinten_l]:
		netz.set_uv(Vector2(punkt.x, punkt.z + punkt.y * 0.54) * 0.37)
		netz.set_smooth_group(-1)
		netz.add_vertex(punkt)
	netz.generate_normals()
	var dach := netz.commit()
	var pfad := "res://daten/texturen/T_Ziegel_D.png"
	if ResourceLoader.exists(pfad, "Texture2D"):
		_ziegel.albedo_texture = load(pfad)
	var instanz := MeshInstance3D.new()
	instanz.mesh = dach
	instanz.material_override = _ziegel
	bau.add_child(instanz)
	var koerper := StaticBody3D.new()
	var form := CollisionShape3D.new()
	var schale := dach.create_trimesh_shape()
	schale.backface_collision = true
	form.shape = schale
	koerper.add_child(form)
	instanz.add_child(koerper)
	for x in [-26.3, 26.3]:
		var abschluss := SurfaceTool.new()
		abschluss.begin(Mesh.PRIMITIVE_TRIANGLES)
		for v in [Vector3(x, 7.92, -17.55), Vector3(x, 15.14, -8.76), Vector3(x, 7.92, 0)]:
			abschluss.add_vertex(v)
		abschluss.generate_normals()
		var giebel := MeshInstance3D.new()
		giebel.mesh = abschluss.commit()
		giebel.material_override = _putz
		bau.add_child(giebel)
	for x in [-18.6, -7.0, 3.65, 14.8]:
		_quader(bau, Vector3(x, 10.30, -2.56), Vector3(3.63, 2.41, 2.56), _putz)
		var gaubendach := _quader(bau, Vector3(x, 11.53, -2.62), Vector3(3.91, 0.18, 3.18), _ziegel)
		gaubendach.rotation.x = 0.30
		_quader(bau, Vector3(x, 10.24, -1.24), Vector3(3.24, 1.48, 0.09), _weiss)
		for s in range(7):
			for r in [9.94, 10.54]:
				_quader(bau, Vector3(x - 1.34 + s * 0.445, r, -1.18),
						Vector3(0.35, 0.49, 0.038), _glas)


func _eingang(bau: Node3D) -> void:
	for x in [-5.8, -0.55]:
		_quader(bau, Vector3(x, 1.18, 2.06), Vector3(0.32, 2.38, 0.34), _stein)
	_quader(bau, Vector3(-3.20, 1.30, 0.18), Vector3(5.75, 2.60, 0.15), _metall)
	for x in [-4.95, -3.18, -1.41]:
		_quader(bau, Vector3(x, 1.39, 0.30), Vector3(1.54, 2.42, 0.09), _glas)
	var vordach := _quader(bau, Vector3(-3.2, 2.90, 1.10), Vector3(8.98, 0.19, 2.41), _ziegel)
	vordach.rotation.x = 0.40


func _uhr(bau: Node3D) -> void:
	var mitte := Vector3(8.27, 12.60, 0.54)
	_quader(bau, mitte + Vector3(0, -4.50, -0.32), Vector3(3.30, 11.30, 0.50), _stein)
	var decke := _quader(bau, mitte + Vector3(0, 1.22, -0.31), Vector3(3.59, 0.18, 1.02), _ziegel)
	decke.rotation.x = 0.13
	var blatt := CylinderMesh.new()
	blatt.top_radius = 1.15
	blatt.bottom_radius = 1.15
	blatt.height = 0.09
	blatt.radial_segments = 40
	var uhr := MeshInstance3D.new()
	uhr.mesh = blatt
	uhr.position = mitte
	uhr.rotation.x = PI / 2.0
	uhr.material_override = _weiss
	bau.add_child(uhr)
	for i in 12:
		var winkel := i * TAU / 12.0
		var strich := _quader(bau, mitte + Vector3(sin(winkel), cos(winkel), 0.06) * 0.96,
				Vector3(0.052, 0.17, 0.04), _metall)
		strich.rotation.z = -winkel
	for zeiger in [Vector2(-0.23, 0.39), Vector2(0.28, -0.68)]:
		var v := Vector3(zeiger.x, zeiger.y, 0)
		var teil := _quader(bau, mitte + v * 0.50 + Vector3(0, 0, 0.09),
				Vector3(0.072, v.length(), 0.078), _metall)
		teil.rotation.z = atan2(-v.x, v.y)


func _sportplatz(sektor: Node3D) -> void:
	for ort: Vector3 in [Vector3(-791.2, 25.45, 1059.8), Vector3(-786.4, 25.22, 1078.4)]:
		_quader(sektor, ort + Vector3.UP * 1.57, Vector3(0.115, 3.14, 0.115), _metall)
		_quader(sektor, ort + Vector3.UP * 3.0, Vector3(1.24, 0.76, 0.09), _weiss)
		var ring := TorusMesh.new()
		ring.outer_radius = 0.25
		ring.inner_radius = 0.21
		ring.rings = 20
		ring.ring_segments = 8
		var instanz := MeshInstance3D.new()
		instanz.mesh = ring
		instanz.position = ort + Vector3(0, 2.74, 0.34)
		instanz.material_override = _ziegel
		sektor.add_child(instanz)


func _quader(eltern: Node3D, ort: Vector3, masse: Vector3, material: Material) -> MeshInstance3D:
	var netz := BoxMesh.new()
	netz.size = masse
	var instanz := MeshInstance3D.new()
	instanz.mesh = netz
	instanz.material_override = material
	instanz.position = ort
	eltern.add_child(instanz)
	return instanz


static func _material(farbe: String) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = Color(farbe)
	material.roughness = 0.79
	material.cull_mode = BaseMaterial3D.CULL_DISABLED
	return material
