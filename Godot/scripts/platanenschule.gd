class_name Platanenschule
extends RefCounted
## Sichtbare Aufbauten und Freianlagen auf den Grundrissen der Stadtdaten.

const SEKTOR := Vector2i(-3, -2)

var _blech := _material("#cbd0d1", 0.55)
var _glas := _material("#415968", 0.30)
var _rahmen := _material("#636f72")
var _holz := _material("#967352")
var _stein := _material("#ab9b85")
var _wasser := _material("#315c56", 0.28)


func baue(sektor: Node3D) -> void:
	var areal := Node3D.new()
	areal.name = "PlatanenschuleFreianlagen"
	sektor.add_child(areal)
	var mitte := Vector3(-1147.15, 0, -498.8)
	_rund(areal, mitte + Vector3(0, 27.06, 0), 21.63, 21.63, 0.22, _blech, 64)
	_rund(areal, mitte + Vector3(0, 29.07, 0), 3.15, 5.17, 2.10, _glas)
	_rund(areal, mitte + Vector3(0, 30.16, 0), 3.25, 3.25, 0.13, _blech)
	for i in 18:
		var a := TAU * i / 18.0
		var richtung := Vector3(cos(a), 0, sin(a))
		_strebe(areal, mitte + richtung * 5.20 + Vector3.UP * 28.04,
				mitte + richtung * 3.17 + Vector3.UP * 30.16, 0.051, _blech)
		if i % 6 == 0:
			_strebe(areal, mitte + richtung * 2.82 + Vector3.UP * 30.17,
					mitte + Vector3.UP * 33.75, 0.045, _blech)
	for z in [-542.1, -552.7]:
		_quader(areal, Vector3(-1147.2, 28.21, z), Vector3(2.10, 0.30, 6.82), _blech)
		_quader(areal, Vector3(-1147.2, 28.40, z), Vector3(1.69, 0.11, 6.39), _glas)
		for i in range(-2, 3):
			_quader(areal, Vector3(-1147.2, 28.47, z + float(i) * 1.09),
					Vector3(1.74, 0.065, 0.075), _rahmen)
	_quader(areal, Vector3(-1146.0, 23.19, -526.6), Vector3(2.28, 0.24, 13.3), _blech)
	for z in [-531.7, -525.3, -520.8]:
		for x in [-1147.01, -1144.99]:
			_quader(areal, Vector3(x, 21.6, z), Vector3(0.105, 3.24, 0.105), _rahmen)
	_freianlagen(areal)


func _freianlagen(areal: Node3D) -> void:
	var teich := Vector3(-1182.95, 19.87, -590.1)
	_rund(areal, teich, 3.24, 3.32, 0.42, _stein)
	_rund(areal, teich + Vector3.UP * 0.225, 2.79, 2.79, 0.045, _wasser)
	for i in 7:
		var a := 0.26 + float(i) * 0.38
		var sitz := teich + Vector3(cos(a) * 4.06, 0.23, sin(a) * 4.06)
		var bank := _quader(areal, sitz, Vector3(0.72, 0.38, 1.28), _stein)
		bank.rotation.y = -a
		var auflage := _quader(areal, sitz + Vector3.UP * 0.22, Vector3(0.67, 0.08, 1.32), _holz)
		auflage.rotation.y = -a
	for z in [-559.8, -536.6]:
		var y := 19.72 if z < -550.0 else 20.02
		for x in [-1116.90, -1112.30]:
			_quader(areal, Vector3(x, y + 0.94, z), Vector3(0.09, 1.88, 0.09), _blech)
		_quader(areal, Vector3(-1114.6, y + 1.88, z), Vector3(4.69, 0.09, 0.09), _blech)
	# Die Sandgrube schließt südlich an das Rasenspielfeld an.
	_quader(areal, Vector3(-1106.77, 19.60, -569.1), Vector3(4.7, 0.13, 3.6), _stein)


func _rund(eltern: Node3D, ort: Vector3, oben: float, unten: float, hoehe: float,
		material: Material, segmente := 48) -> MeshInstance3D:
	var netz := CylinderMesh.new()
	netz.top_radius = oben
	netz.bottom_radius = unten
	netz.height = hoehe
	netz.radial_segments = segmente
	netz.rings = 0
	return _modell(eltern, ort, netz, material)


func _strebe(eltern: Node3D, a: Vector3, b: Vector3, radius: float, material: Material) -> void:
	var oben := (b - a).normalized()
	var rechts := oben.cross(Vector3.FORWARD).normalized()
	var strebe := _rund(eltern, (a + b) * 0.5, radius, radius, a.distance_to(b), material, 8)
	strebe.basis = Basis(rechts, oben, rechts.cross(oben))


func _quader(eltern: Node3D, ort: Vector3, masse: Vector3, material: Material) -> MeshInstance3D:
	var netz := BoxMesh.new()
	netz.size = masse
	return _modell(eltern, ort, netz, material)


func _modell(eltern: Node3D, ort: Vector3, netz: Mesh, material: Material) -> MeshInstance3D:
	var instanz := MeshInstance3D.new()
	instanz.mesh = netz
	instanz.position = ort
	instanz.material_override = material
	eltern.add_child(instanz)
	return instanz


static func _material(farbcode: String, rauheit := 0.86) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = Color(farbcode)
	material.roughness = rauheit
	return material
