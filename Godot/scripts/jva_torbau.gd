class_name JvaTorbau
extends RefCounted
## Straßenfassade des denkmalgeschützten Torbaus am Hindenburgring.
## Die Fassadenachse und die Rundtürme sind am vorhandenen Stadtmesh ausgerichtet.

const SEKTOR := Vector2i(-2, -1)
const DACHSEKTOR := Vector2i(-2, -2)
const FRONT := Vector3(-651.2, 15.55, -454.3)
const WINKEL := 1.033
const FASSADENBREITE := 14.44
const TURMMITTEN := [Vector3(-646.58, 15.52, -463.23), Vector3(-656.85, 15.64, -446.12)]

var _putz := _material(Color("#aba398"))
var _fels := _material(Color("#bdb4a6"))
var _sockel := _material(Color("#8d8274"))
var _glas := _material(Color("#26373a"), 0.39)
var _dachziegel := _material(Color("#6f4a41"))
var _kupfer := _material(Color("#6c9c8a"), 0.56, 0.2)
var _dunkler_kupfer := _material(Color("#445b50"), 0.72)


func baue(sektor: Node3D) -> void:
	var front := Node3D.new()
	front.name = "HistorischerJVATorbau"
	front.position = FRONT
	front.rotation.y = WINKEL
	sektor.add_child(front)
	_quader(front, Vector3(0, 3.70, 0.035), Vector3(FASSADENBREITE, 7.60, 0.19), _putz)
	_quader(front, Vector3(0, 0.28, 0.16), Vector3(FASSADENBREITE, 0.55, 0.19), _sockel)
	_quader(front, Vector3(0, 7.51, 0.16), Vector3(FASSADENBREITE + 0.35, 0.23, 0.38), _fels)
	_portal(front)
	for x in [-5.05, -3.07, 3.07, 5.05]:
		_fenster(front, Vector3(x, 2.43, 0.15), 1.12, 1.72)
		_fenster(front, Vector3(x, 5.63, 0.15), 1.10, 1.60)
	_walmdach(front)
	for ort: Vector3 in TURMMITTEN:
		_turm(sektor, ort)


func baue_dachlaterne(sektor: Node3D) -> void:
	var ort := Vector3(-764, 30.17, -521)
	_zylinder(sektor, ort, 3.99, 2.65, _putz)
	_zylinder(sektor, ort + Vector3.UP * 0.78, 4.06, 0.21, _fels)
	var dach := CylinderMesh.new()
	dach.top_radius = 0.11
	dach.bottom_radius = 4.54
	dach.height = 2.87
	dach.radial_segments = 8
	dach.rings = 0
	_modell(sektor, dach, ort + Vector3.UP * 2.70, _dachziegel)
	for i in 8:
		var a := TAU * i / 8.0
		var fenster := _quader(sektor, ort + Vector3(sin(a) * 4.0, 0.23, cos(a) * 4.0),
				Vector3(1.45, 0.89, 0.14), _glas)
		fenster.rotation.y = a


func _turm(sektor: Node3D, ort: Vector3) -> void:
	var turm := Node3D.new()
	turm.position = ort
	turm.rotation.y = WINKEL
	sektor.add_child(turm)
	_zylinder(turm, Vector3(0, 4.21, 0), 3.02, 8.63, _putz)
	_zylinder(turm, Vector3(0, 0.28, 0), 3.08, 0.57, _sockel)
	_zylinder(turm, Vector3(0, 8.54, 0), 3.13, 0.27, _fels)
	_zylinder(turm, Vector3(0, 8.76, 0), 3.21, 0.18, _dunkler_kupfer)
	_modell(turm, _haube(), Vector3(0, 8.80, 0), _kupfer)
	_zylinder(turm, Vector3(0, 11.33, 0), 0.055, 0.67, _dunkler_kupfer)
	var kugel := SphereMesh.new()
	kugel.radial_segments = 14
	kugel.rings = 7
	kugel.radius = 0.119
	kugel.height = 0.238
	_modell(turm, kugel, Vector3(0, 11.72, 0), _kupfer)
	for winkel in [-0.86, 0.0, 0.86]:
		var rahmen := Node3D.new()
		rahmen.rotation.y = winkel
		turm.add_child(rahmen)
		_fenster(rahmen, Vector3(0, 2.46, 2.97), 0.98, 1.65)
		_fenster(rahmen, Vector3(0, 5.91, 2.97), 1.01, 1.52)


func _haube() -> ArrayMesh:
	var struktur := SurfaceTool.new()
	struktur.begin(Mesh.PRIMITIVE_TRIANGLES)
	var profil := [
		Vector2(0.0, 3.12), Vector2(0.28, 3.22), Vector2(0.62, 3.24),
		Vector2(1.03, 3.06), Vector2(1.36, 2.62), Vector2(1.62, 2.03),
		Vector2(1.83, 1.36), Vector2(2.08, 0.74), Vector2(2.32, 0.38),
		Vector2(2.55, 0.065), Vector2(2.60, 0.025),
	]
	for r in profil.size() - 1:
		for s in 48:
			for e in [Vector2i(r, s), Vector2i(r + 1, s), Vector2i(r, s + 1),
					Vector2i(r, s + 1), Vector2i(r + 1, s), Vector2i(r + 1, s + 1)]:
				var a: float = TAU * e.y / 48.0
				var p: Vector2 = profil[e.x]
				var radius := p.y * (1.0 + 0.012 * cos(a * 24.0))
				var verblichen := 0.83 + 0.14 * float(e.y % 2)
				struktur.set_color(Color(verblichen, verblichen, verblichen))
				struktur.add_vertex(Vector3(cos(a) * radius, p.x, sin(a) * radius))
	struktur.generate_normals()
	_kupfer.vertex_color_use_as_albedo = true
	return struktur.commit()


func _walmdach(front: Node3D) -> void:
	var gedaech := SurfaceTool.new()
	gedaech.begin(Mesh.PRIMITIVE_TRIANGLES)
	var a := Vector3(-8.20, 7.56, 0.47)
	var b := Vector3(8.20, 7.56, 0.47)
	var c := Vector3(8.20, 7.56, -13.90)
	var d := Vector3(-8.20, 7.56, -13.90)
	var first_l := Vector3(-5.30, 11.34, -6.65)
	var first_r := Vector3(5.30, 11.34, -6.65)
	for p in [a, b, first_r, a, first_r, first_l, b, c, first_r,
			c, d, first_l, c, first_l, first_r, d, a, first_l]:
		gedaech.set_uv(Vector2(p.x * 0.37, (p.z + p.y * 0.84) * 0.37))
		gedaech.set_smooth_group(-1)
		gedaech.add_vertex(p)
	gedaech.generate_normals()
	var textur := "res://daten/texturen/T_Ziegel_D.png"
	if ResourceLoader.exists(textur, "Texture2D"):
		_dachziegel.albedo_texture = load(textur)
	_modell(front, gedaech.commit(), Vector3.ZERO, _dachziegel)
	_quader(front, Vector3(0, 8.85, -1.52), Vector3(9.15, 1.98, 2.12), _putz)
	_quader(front, Vector3(0, 9.91, -1.38), Vector3(9.49, 0.32, 2.72), _dachziegel)
	for x in [-3.13, -1.03, 1.03, 3.13]:
		_fenster(front, Vector3(x, 8.77, -0.42), 0.75, 1.02)


func _portal(front: Node3D) -> void:
	var lichte_weite := 1.62
	var ansatz := 2.02
	var flaeche := SurfaceTool.new()
	flaeche.begin(Mesh.PRIMITIVE_TRIANGLES)
	for p in [Vector3(-lichte_weite, 0, 0.151), Vector3(lichte_weite, 0, 0.151),
			Vector3(lichte_weite, ansatz, 0.151), Vector3(-lichte_weite, 0, 0.151),
			Vector3(lichte_weite, ansatz, 0.151), Vector3(-lichte_weite, ansatz, 0.151)]:
		flaeche.add_vertex(p)
	for i in 24:
		var a := PI * i / 24.0
		var b := PI * (i + 1) / 24.0
		for p in [Vector3(0, ansatz, 0.151),
				Vector3(cos(a) * lichte_weite, ansatz + sin(a) * lichte_weite, 0.151),
				Vector3(cos(b) * lichte_weite, ansatz + sin(b) * lichte_weite, 0.151)]:
			flaeche.add_vertex(p)
	flaeche.generate_normals()
	_modell(front, flaeche.commit(), Vector3.ZERO, _glas)
	for x in [-1.78, 1.78]:
		for reihe in 5:
			_quader(front, Vector3(x, 0.23 + reihe * 0.37, 0.30), Vector3(0.35, 0.34, 0.46), _fels)
	for i in 13:
		var a := (i + 0.5) * PI / 13.0
		var rad := 1.80
		var stein := _quader(front, Vector3(cos(a) * rad, ansatz + sin(a) * rad, 0.30),
				Vector3(0.36 if i != 6 else 0.52, 0.36, 0.46), _fels)
		stein.rotation.z = a


func _fenster(eltern: Node3D, ort: Vector3, breite: float, hoehe: float) -> void:
	_quader(eltern, ort, Vector3(breite + 0.18, hoehe + 0.13, 0.15), _fels)
	_quader(eltern, ort + Vector3(0, 0, 0.091), Vector3(breite, hoehe, 0.045), _glas)
	_quader(eltern, ort + Vector3(0, 0, 0.124), Vector3(0.055, hoehe, 0.05), _fels)
	_quader(eltern, ort + Vector3(0, hoehe * 0.14, 0.124), Vector3(breite, 0.055, 0.05), _fels)
	_quader(eltern, ort + Vector3(0, -hoehe * 0.50 - 0.038, 0.10),
			Vector3(breite + 0.26, 0.107, 0.21), _fels)


func _quader(eltern: Node3D, ort: Vector3, masse: Vector3, material: Material) -> MeshInstance3D:
	var box := BoxMesh.new()
	box.size = masse
	return _modell(eltern, box, ort, material)


func _zylinder(eltern: Node3D, ort: Vector3, radius: float, hoehe: float, material: Material) -> void:
	var netz := CylinderMesh.new()
	netz.top_radius = radius
	netz.bottom_radius = radius
	netz.height = hoehe
	netz.radial_segments = 38
	netz.rings = 0
	_modell(eltern, netz, ort, material)


func _modell(eltern: Node3D, mesh: Mesh, ort: Vector3, material: Material) -> MeshInstance3D:
	var instanz := MeshInstance3D.new()
	instanz.mesh = mesh
	instanz.material_override = material
	instanz.position = ort
	eltern.add_child(instanz)
	return instanz


static func _material(farbe: Color, rauheit := 0.89, metall := 0.0) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = farbe
	m.roughness = rauheit
	m.metallic = metall
	m.cull_mode = BaseMaterial3D.CULL_DISABLED
	return m
