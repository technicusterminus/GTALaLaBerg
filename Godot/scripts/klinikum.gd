class_name Klinikum
extends RefCounted
## Außenbauteile am vorhandenen LoD2-Klinikum. Die Eingangsachse folgt der
## Nordfassade und dem Karl-Mohrenweis-Platz im Stadtmesh.

const SEKTOR := Vector2i(-4, 0)
const EINGANG := Vector3(-1613.6, 28.73, 122.0)
const ACHSE := 0.322
const VORPLATZ := Vector3(-1627.25, 28.66, 109.0)

var _metall := _material(Color("#627686"), 0.53, 0.35)
var _aluminium := _material(Color("#c4cbd0"), 0.49, 0.44)
var _glas := _material(Color("#315f72"), 0.23, 0.30)
var _glas_dunkel := _material(Color("#213941"), 0.31, 0.21)
var _sockel := _material(Color("#a4a8a7"), 0.85)
var _blau := _material(Color("#226480"), 0.52)
var _gelb := _material(Color("#dca82c"), 0.59)


func baue(sektor: Node3D) -> void:
	var detail := Node3D.new()
	detail.name = "KlinikumHaupteingang"
	detail.position = EINGANG
	detail.rotation.y = ACHSE
	sektor.add_child(detail)
	_verbindung(detail)
	_eingang(detail)
	_dach(detail)
	_schriftzug(detail)
	_vorplatz(sektor)


func _verbindung(eltern: Node3D) -> void:
	# Glasgalerien links und rechts vom vorspringenden Atrium.
	for x in [-14.4, 14.4]:
		_quader(eltern, Vector3(x, 1.35, -2.65), Vector3(15.0, 2.75, 5.45), _sockel)
		_quader(eltern, Vector3(x, 4.11, -3.0), Vector3(15.0, 2.68, 5.68), _glas)
		_quader(eltern, Vector3(x, 5.52, -3.0), Vector3(15.25, 0.25, 6.00), _aluminium)
		_quader(eltern, Vector3(x, 2.79, -5.86), Vector3(15.2, 0.16, 0.14), _metall)
		for i in 10:
			var fenster_x := float(x) - 7.11 + i * 1.58
			_quader(eltern, Vector3(fenster_x, 4.13, -5.85), Vector3(0.073, 2.60, 0.13), _aluminium)
			if i < 9:
				_quader(eltern, Vector3(fenster_x + 0.78, 1.48, -5.41),
						Vector3(1.23, 0.98, 0.07), _glas_dunkel)
		_quader(eltern, Vector3(x, 4.52, -5.86), Vector3(15.0, 0.058, 0.13), _aluminium)


func _eingang(eltern: Node3D) -> void:
	# Zurückliegende Türzone und auskragende, zum Vorplatz geneigte Glasfront.
	_quader(eltern, Vector3(0, 1.45, -5.23), Vector3(7.2, 2.90, 2.80), _glas_dunkel)
	_quader(eltern, Vector3(0, 2.95, -6.13), Vector3(8.1, 0.24, 1.67), _metall)
	_quader(eltern, Vector3(0, 2.84, -6.94), Vector3(5.3, 0.14, 0.07), _gelb)
	for x in [-2.62, -1.31, 0.0, 1.31, 2.62]:
		_quader(eltern, Vector3(x, 1.48, -6.66), Vector3(0.078, 2.65, 0.08), _aluminium)
	_quader(eltern, Vector3(0, 2.58, -6.67), Vector3(5.3, 0.065, 0.08), _aluminium)
	_quader(eltern, Vector3(0, 3.20, -4.7), Vector3(13.15, 0.66, 8.24), _aluminium)
	_quader(eltern, Vector3(0, 5.35, -4.32), Vector3(12.96, 3.58, 7.95), _glas)
	_quader(eltern, Vector3(0, 7.37, -4.95), Vector3(13.55, 0.25, 9.0), _aluminium)

	var glasfront := Node3D.new()
	glasfront.position = Vector3(0, 5.33, -8.82)
	glasfront.rotation.x = -0.14
	eltern.add_child(glasfront)
	_quader(glasfront, Vector3.ZERO, Vector3(13.12, 3.76, 0.12), _glas)
	_quader(glasfront, Vector3(0, 1.41, -0.09), Vector3(13.07, 0.81, 0.10), _gelb)
	_quader(glasfront, Vector3(0, -1.49, -0.10), Vector3(13.10, 0.69, 0.10), _blau)
	for x in [-6.57, -4.35, -2.17, 0.0, 2.17, 4.35, 6.57]:
		_quader(glasfront, Vector3(x, 0.0, -0.17), Vector3(0.075, 3.82, 0.12), _aluminium)
	for y in [-1.87, -1.08, -0.57, 0.99, 1.88]:
		_quader(glasfront, Vector3(0, y, -0.16), Vector3(13.22, 0.075, 0.12), _aluminium)
	for x in [-6.52, 6.52]:
		_quader(eltern, Vector3(x, 6.83, -6.49), Vector3(0.11, 0.76, 4.49), _gelb)
		_quader(eltern, Vector3(x, 2.91, -7.73), Vector3(0.16, 0.85, 0.19), _metall)
	for x in [-2.1, 2.1]:
		var oberlicht := _quader(eltern, Vector3(x, 7.57, -3.63), Vector3(1.12, 0.19, 0.93), _glas)
		oberlicht.rotation.x = 0.13


func _dach(eltern: Node3D) -> void:
	# Flacher, geböschter Blechaufbau oberhalb der Hauptachse.
	var netz := SurfaceTool.new()
	netz.begin(Mesh.PRIMITIVE_TRIANGLES)
	var unten := [Vector3(-12.2, 0, -6.4), Vector3(12.2, 0, -6.4),
			Vector3(12.2, 0, 6.4), Vector3(-12.2, 0, 6.4)]
	var oben := [Vector3(-10.3, 2.65, -4.7), Vector3(10.3, 2.65, -4.7),
			Vector3(10.3, 2.65, 4.7), Vector3(-10.3, 2.65, 4.7)]
	for i in 4:
		var j := (i + 1) % 4
		for punkt in [unten[i], oben[i], unten[j], unten[j], oben[i], oben[j]]:
			netz.set_smooth_group(-1)
			netz.add_vertex(punkt)
	for i in [0, 2, 1, 0, 3, 2]:
		netz.set_smooth_group(-1)
		netz.add_vertex(oben[i])
	netz.generate_normals()
	_modell(eltern, netz.commit(), Vector3(0.35, 14.96, 7.25), _metall)
	_quader(eltern, Vector3(-5.23, 17.81, 7.7), Vector3(0.73, 1.02, 0.91), _aluminium)
	_quader(eltern, Vector3(5.40, 17.66, 9.3), Vector3(1.85, 0.62, 1.13), _aluminium)


func _schriftzug(eltern: Node3D) -> void:
	var platte := _material(Color("#9baab6"), 0.75)
	_quader(eltern, Vector3(0, 11.48, -0.08), Vector3(8.2, 3.16, 0.20), platte)
	_text(eltern, "Klinikum", Vector3(0.64, 11.78, -0.20), 66, 0.024)
	_text(eltern, "Landsberg am Lech", Vector3(0.66, 10.66, -0.205), 44, 0.016)
	var emblem := CylinderMesh.new()
	emblem.top_radius = 0.34
	emblem.bottom_radius = 0.34
	emblem.height = 0.080
	emblem.radial_segments = 20
	var punkt := _modell(eltern, emblem, Vector3(-3.31, 11.61, -0.22), _gelb)
	punkt.rotation.x = PI * 0.5


func _vorplatz(sektor: Node3D) -> void:
	var stein := _material(Color("#969997"), 0.96)
	var dunkler_stein := _material(Color("#666c70"), 0.96)
	for i in 3:
		var ring := TorusMesh.new()
		ring.outer_radius = 1.38 + i * 0.78
		ring.inner_radius = ring.outer_radius - 0.19
		ring.rings = 40
		ring.ring_segments = 6
		_modell(sektor, ring, VORPLATZ + Vector3.UP * (0.17 - i * 0.038),
				stein if i != 1 else dunkler_stein)


func _text(eltern: Node3D, text: String, ort: Vector3, groesse: int, pixel: float) -> void:
	var schild := Label3D.new()
	schild.text = text
	schild.font_size = groesse
	schild.pixel_size = pixel
	schild.outline_size = 0
	schild.modulate = Color("#234c62")
	schild.position = ort
	schild.rotation.y = PI
	eltern.add_child(schild)


func _quader(eltern: Node3D, ort: Vector3, groesse: Vector3, material: Material) -> MeshInstance3D:
	var box := BoxMesh.new()
	box.size = groesse
	return _modell(eltern, box, ort, material)


func _modell(eltern: Node3D, mesh: Mesh, ort: Vector3, material: Material) -> MeshInstance3D:
	var instanz := MeshInstance3D.new()
	instanz.mesh = mesh
	instanz.material_override = material
	instanz.position = ort
	eltern.add_child(instanz)
	return instanz


static func _material(farbe: Color, rauheit: float, metallic := 0.0) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = farbe
	m.roughness = rauheit
	m.metallic = metallic
	m.cull_mode = BaseMaterial3D.CULL_DISABLED
	return m
