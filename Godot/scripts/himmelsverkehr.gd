class_name Himmelsverkehr
extends Node3D
## Kleine Silhouetten über dem Lech und den Stadtvierteln.

const KREISE := [
	[Vector3(-125, 39, 60), Vector2(89, 64), 7],
	[Vector3(-1510, 76, 102), Vector2(100, 78), 6],
	[Vector3(-370, 52, 360), Vector2(113, 61), 5],
]
const FLUG_START := Vector3(-2250, 205, 490)
const FLUG_ZIEL := Vector3(1280, 278, -675)
const FLUG_DAUER := 58.0

var _zeit := 0.0
var _voegel: Array[Array] = []
var _flugzeug := Node3D.new()


func _ready() -> void:
	var feder := StandardMaterial3D.new()
	feder.albedo_color = Color("#393d41")
	feder.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	feder.cull_mode = BaseMaterial3D.CULL_DISABLED
	var linker_fluegel := _fluegel([
		Vector3.ZERO, Vector3(-0.33, 0, -0.10), Vector3(-0.65, 0, 0.03),
		Vector3(-0.30, 0, 0.17)])
	var rechter_fluegel := _fluegel([
		Vector3.ZERO, Vector3(0.33, 0, -0.10), Vector3(0.65, 0, 0.03),
		Vector3(0.30, 0, 0.17)])
	var rumpf := SphereMesh.new()
	rumpf.radius = 0.5
	rumpf.height = 1.0
	rumpf.rings = 2
	rumpf.radial_segments = 6
	for kreis: Array in KREISE:
		var schwarm: Array = []
		for i in int(kreis[2]):
			var vogel := Node3D.new()
			add_child(vogel)
			_modell(vogel, rumpf, feder, Vector3.ZERO, Vector3(0.14, 0.13, 0.43))
			var links := _modell(vogel, linker_fluegel, feder)
			var rechts := _modell(vogel, rechter_fluegel, feder)
			vogel.scale = Vector3.ONE * (0.84 + (i * 7 % 5) * 0.095)
			schwarm.append([vogel, links, rechts])
		_voegel.append(schwarm)
	_baue_flugzeug(rumpf)
	_process(0)


func _process(delta: float) -> void:
	_zeit += delta
	for k in _voegel.size():
		var kreis: Array = KREISE[k]
		var mitte: Vector3 = kreis[0]
		var radius: Vector2 = kreis[1]
		for i in _voegel[k].size():
			var vogel: Node3D = _voegel[k][i][0]
			var fluegel_l: Node3D = _voegel[k][i][1]
			var fluegel_r: Node3D = _voegel[k][i][2]
			var winkel := _zeit * (0.12 + 0.018 * k) + 1.75 * k - i * 0.072
			var abstand := float(i % 2) * 2.2 - float(i) * 1.3
			vogel.position = mitte + Vector3(cos(winkel) * (radius.x + abstand),
					sin(winkel * 2.0 + i * 0.44) * 2.3 + i * 0.32,
					sin(winkel) * (radius.y + abstand))
			var richtung := Vector3(-sin(winkel) * radius.x, 0, cos(winkel) * radius.y)
			vogel.rotation.y = atan2(-richtung.x, -richtung.z)
			var gleitet := maxf(sin(_zeit * 0.86 + i * 0.57), 0.0)
			var schlag := sin(_zeit * 14.0 + i * 1.07) * gleitet * 0.61 + 0.12
			fluegel_l.rotation.z = -schlag
			fluegel_r.rotation.z = schlag

	var seit_start := fposmod(_zeit + 14.0, 98.0)
	_flugzeug.visible = seit_start < FLUG_DAUER
	_flugzeug.position = FLUG_START.lerp(FLUG_ZIEL, clampf(seit_start / FLUG_DAUER, 0.0, 1.0))


func _baue_flugzeug(rumpf: Mesh) -> void:
	add_child(_flugzeug)
	var kurs := FLUG_ZIEL - FLUG_START
	_flugzeug.rotation.y = atan2(-kurs.x, -kurs.z)
	var lack := StandardMaterial3D.new()
	lack.albedo_color = Color("#e2e5e6")
	lack.roughness = 0.6
	lack.cull_mode = BaseMaterial3D.CULL_DISABLED
	var dunkel := lack.duplicate() as StandardMaterial3D
	dunkel.albedo_color = Color("#62758a")
	_modell(_flugzeug, rumpf, lack, Vector3.ZERO, Vector3(2.75, 2.7, 21.5))
	for seite in [-1.0, 1.0]:
		var tragflaeche := _fluegel([Vector3(0, 0, -2), Vector3(seite * 10.6, 0.63, 3.4),
				Vector3(seite * 10.1, 0.61, 4.2), Vector3(0, 0, 3.5)])
		_modell(_flugzeug, tragflaeche, lack)
		_modell(_flugzeug, rumpf, dunkel, Vector3(seite * 3.1, -0.75, 0.15),
				Vector3(1.20, 1.20, 3.7))
		var heck := _fluegel([Vector3(0, 0, 6), Vector3(seite * 3.8, 0, 8.4),
				Vector3(seite * 3.6, 0, 9.2), Vector3(0, 0, 8.4)])
		_modell(_flugzeug, heck, lack)
	var leitwerk := _fluegel([Vector3(0, 0, 5.3), Vector3(0, 3.3, 8.4),
			Vector3(0, 3.3, 9.2), Vector3(0, 0, 8.35)])
	_modell(_flugzeug, leitwerk, dunkel)


func _fluegel(ecken: Array) -> ArrayMesh:
	var netz := SurfaceTool.new()
	netz.begin(Mesh.PRIMITIVE_TRIANGLES)
	for i in [0, 1, 2, 0, 2, 3]:
		netz.add_vertex(ecken[i])
	netz.generate_normals()
	return netz.commit()


func _modell(eltern: Node3D, mesh: Mesh, material: Material, pos := Vector3.ZERO,
		masse := Vector3.ONE) -> MeshInstance3D:
	var instanz := MeshInstance3D.new()
	instanz.mesh = mesh
	instanz.material_override = material
	instanz.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	instanz.position = pos
	instanz.scale = masse
	eltern.add_child(instanz)
	return instanz
