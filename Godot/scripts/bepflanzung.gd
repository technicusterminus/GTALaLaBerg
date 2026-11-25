class_name Bepflanzung
extends RefCounted
## Handgesetzte Pflanzungen folgen der jeweiligen Platz- oder Geländehöhe.
## Die Teile einer Pflanzung werden je Sektor nach Form zusammengefasst.

enum Form { BLATT, HALM, KUEBEL, AST, LAUB_BREIT, LAUB_SCHLANK, LAUB_IRREGULAER, FICHTE }
const SHADER := preload("res://shader/pflanzen.gdshader")

const HAUPTPLATZ := [
	Vector3(-19, 7.960, 15), Vector3(-21, 7.839, 17), Vector3(-5, 8.857, 7),
	Vector3(-29, 7.374, 27), Vector3(-26, 7.412, -35), Vector3(-12, 8.212, -31),
	Vector3(23, 10.779, -19),
]
const KLINIKUM := [
	Vector3(-1610, 28.023, 203), Vector3(-1600, 28.094, 196), Vector3(-1625, 28.146, 198),
	Vector3(-1662, 27.924, 152), Vector3(-1671, 28.664, 159), Vector3(-1676, 28.702, 157),
	Vector3(-1678, 28.615, 143), Vector3(-1688, 28.593, 146),
]
const LECH := [
	Vector3(-247, 3.396, 227), Vector3(-253, 2.514, 206), Vector3(-266, 2.554, 198),
	Vector3(-274, 2.672, 185), Vector3(-273, 1.292, 166), Vector3(-276, 0.108, 151),
	Vector3(-286, 1.793, 143), Vector3(-297, 1.975, 101),
]
const BLUETEN := [
	[Color("#8e79b7"), Color("#b7a1d6"), Color("#e7d4ee")],
	[Color("#c77b63"), Color("#e4a17a"), Color("#f4d795")],
	[Color("#af6787"), Color("#da99a9"), Color("#f3dfc1")],
]
const LAUB := [Color("#315437"), Color("#51784a"), Color("#6b8b52"), Color("#81965e")]
const WIESE := [Color("#526a38"), Color("#637f46"), Color("#839955"), Color("#aca968")]
const BAUM := [
	[Color("#44633e"), Color("#607b46"), Color("#749153")],
	[Color("#3c684a"), Color("#538258"), Color("#719668")],
	[Color("#52683d"), Color("#73884b"), Color("#8a9a5d")],
	[Color("#415f49"), Color("#4c7455"), Color("#608168")],
]

var _formen: Array[Mesh] = []
var _teile: Array[Array]
var _zufall := RandomNumberGenerator.new()


func baue(wurzel: Node3D, k: Vector2i, groesse: float, wiese: Array, kronen: Array) -> void:
	_teile = [[], [], [], [], [], [], [], []]
	for i in HAUPTPLATZ.size():
		var ort: Vector3 = HAUPTPLATZ[i]
		if _im_sektor(ort, k, groesse):
			_zufall.seed = hash(ort)
			_kuebel(wurzel, ort, i)
	if DisplayServer.get_name() == "headless":
		return
	for ort: Vector3 in KLINIKUM:
		if _im_sektor(ort, k, groesse):
			_zufall.seed = hash(ort)
			_beet(ort)
	for ort: Vector3 in LECH:
		if _im_sektor(ort, k, groesse):
			_zufall.seed = hash(ort)
			for i in 19:
				_gras(ort + _versatz(2.0), _zufall.randf_range(0.37, 1.10), true)
			_strauch(ort + Vector3(0.75, -0.08, 0), 0.72)
	for punkt: Array in wiese:
		var ort := Vector3(punkt[0], punkt[1], punkt[2])
		_zufall.seed = hash(ort)
		_gras(ort, _zufall.randf_range(0.25, 0.58))
		if _zufall.randf() < 0.11:
			_strauch(ort, _zufall.randf_range(0.40, 0.88))
	for krone: Array in kronen:
		_baumkrone(Vector3(krone[0], krone[1], krone[2]),
				Vector3(krone[3], krone[4], krone[5]), bool(krone[6]))
	if _teile.all(func(teile: Array) -> bool: return teile.is_empty()):
		return
	if _formen.is_empty():
		_baue_formen()
	for i in _formen.size():
		if _teile[i].is_empty():
			continue
		var baum := i >= Form.LAUB_BREIT
		_buendel(wurzel, _formen[i], _teile[i], baum, 0.0 if baum else 425.0)


func _im_sektor(ort: Vector3, k: Vector2i, groesse: float) -> bool:
	return Vector2i(floori(ort.x / groesse), floori(ort.z / groesse)) == k


func _kuebel(wurzel: Node3D, ort: Vector3, i: int) -> void:
	var hoch := i % 3 == 1
	var durchmesser := 1.10 if hoch else 1.48
	var hoehe := 0.62 if hoch else 0.40
	var ton := Color("#8c5a48") if hoch else Color("#777d74")
	_teil(Form.KUEBEL, ort + Vector3.UP * (hoehe * 0.5 - 0.035),
			Vector3(durchmesser, hoehe, durchmesser), ton)
	_teil(Form.KUEBEL, ort + Vector3.UP * (hoehe - 0.06),
			Vector3(durchmesser * 1.06, 0.09, durchmesser * 1.06), ton.lightened(0.10))
	var erde := ort + Vector3.UP * (hoehe - 0.012)
	_teil(Form.KUEBEL, erde, Vector3(durchmesser * 0.88, 0.035, durchmesser * 0.88), Color("#40372e"))
	_strauch(erde, 0.44 if hoch else 0.52)
	for j in 11:
		var punkt := erde + _versatz(durchmesser * 0.34)
		_gras(punkt, _zufall.randf_range(0.19, 0.38))
		_bluete(punkt, _zufall.randf_range(0.24, 0.58), BLUETEN[i % BLUETEN.size()])

	# Die feste Kante gehört zur Weltkollision, die Pflanzen selbst bleiben durchlässig.
	var koerper := StaticBody3D.new()
	var cs := CollisionShape3D.new()
	var form := CylinderShape3D.new()
	form.radius = durchmesser * 0.51
	form.height = hoehe
	cs.shape = form
	koerper.position = ort + Vector3.UP * hoehe * 0.5
	koerper.add_child(cs)
	wurzel.add_child(koerper)


func _beet(ort: Vector3) -> void:
	var palette: Array = BLUETEN[_zufall.randi_range(0, BLUETEN.size() - 1)]
	for i in 4:
		_strauch(ort + _versatz(1.2), _zufall.randf_range(0.47, 0.86))
	for i in 17:
		var punkt := ort + _versatz(1.55)
		_gras(punkt, _zufall.randf_range(0.22, 0.53))
		_bluete(punkt, _zufall.randf_range(0.35, 0.73), palette)


func _strauch(ort: Vector3, groesse: float) -> void:
	for i in 8:
		var breite := (1.0 - i / 12.0) * groesse
		var punkt := ort + _versatz(breite * 0.58) + Vector3.UP * groesse * (0.12 + i * 0.067)
		_teil(Form.BLATT, punkt, Vector3(breite, breite * 0.83, breite * 0.97),
				LAUB[_zufall.randi_range(0, LAUB.size() - 1)], Basis(Vector3.UP, _zufall.randf() * TAU))


func _baumkrone(mitte: Vector3, masse: Vector3, nadelbaum: bool) -> void:
	_zufall.seed = hash(mitte)
	var art := 3 if nadelbaum else _zufall.randi_range(0, 2)
	var palette: Array = BAUM[art]
	var drehung := _zufall.randf() * TAU
	var basis := Basis(Vector3.UP, drehung)
	var wurzel := mitte - Vector3.UP * masse.y * 0.44
	var farbe: Color = palette[0].lerp(palette[2], _zufall.randf_range(0.33, 0.77))
	if nadelbaum:
		var breite := _zufall.randf_range(1.14, 1.38)
		_teil(Form.FICHTE, mitte, masse * Vector3(breite, 1.0, breite), farbe, basis)
		_ast(wurzel, mitte + Vector3.UP * masse.y * 0.41, clampf(masse.x * 0.062, 0.09, 0.25))
		return

	var formen: Array[Form] = [Form.LAUB_BREIT, Form.LAUB_SCHLANK, Form.LAUB_IRREGULAER]
	var krone := masse * Vector3(_zufall.randf_range(1.15, 1.39), 1.08, _zufall.randf_range(1.11, 1.32))
	_teil(formen[art], mitte, krone, farbe, basis)
	var gabel := wurzel.lerp(mitte, 0.51)
	_ast(wurzel, gabel, clampf(krone.x * 0.064, 0.105, 0.30))
	for i in 4:
		var winkel := drehung + i * TAU / 4.0 + _zufall.randf_range(-0.32, 0.32)
		var ende := mitte + krone * Vector3(cos(winkel) * 0.26,
				_zufall.randf_range(0.02, 0.27), sin(winkel) * 0.26)
		_ast(gabel, ende, clampf(krone.x * 0.035, 0.07, 0.19))


func _ast(von: Vector3, bis: Vector3, durchmesser: float) -> void:
	var richtung := bis - von
	var normal := richtung.normalized()
	var drehachse := Vector3.UP.cross(normal)
	var basis := Basis.IDENTITY
	if drehachse.length_squared() > 0.0001:
		basis = Basis(drehachse.normalized(), Vector3.UP.angle_to(normal))
	_teil(Form.AST, (von + bis) * 0.5, Vector3(durchmesser, richtung.length(), durchmesser),
			Color("#62503e"), basis)


func _gras(ort: Vector3, hoehe: float, schilf := false) -> void:
	for i in (6 if schilf else 5):
		var winkel := _zufall.randf() * TAU
		var richtung := Vector3(cos(winkel), 0.0, sin(winkel))
		var punkt := ort + richtung * _zufall.randf_range(0.02, 0.17)
		var farbe: Color = WIESE[_zufall.randi_range(0, WIESE.size() - 1)]
		var basis := Basis(Vector3.UP, winkel) * Basis(Vector3.RIGHT, _zufall.randf_range(-0.27, 0.27))
		_teil(Form.HALM, punkt, Vector3(hoehe * 0.11, hoehe * _zufall.randf_range(0.6, 1.17), hoehe),
				farbe, basis)
		if schilf and i % 3 == 0:
			var spitze := punkt + Vector3.UP * hoehe * 0.88
			_teil(Form.BLATT, spitze, Vector3(0.05, hoehe * 0.25, 0.055), Color("#9b8a59"))


func _bluete(ort: Vector3, hoehe: float, palette: Array) -> void:
	_teil(Form.KUEBEL, ort + Vector3.UP * hoehe * 0.50, Vector3(0.023, hoehe, 0.023), LAUB[1])
	var kopf := ort + Vector3.UP * hoehe
	var r := _zufall.randf_range(0.045, 0.080)
	var farbe: Color = palette[_zufall.randi_range(0, palette.size() - 1)]
	for i in 5:
		var winkel := TAU * i / 5.0
		var punkt := kopf + Vector3(cos(winkel), 0.1, sin(winkel)) * r
		_teil(Form.BLATT, punkt, Vector3(r * 1.48, r * 0.65, r),
				farbe, Basis(Vector3.UP, -winkel))
	_teil(Form.BLATT, kopf + Vector3.UP * r * 0.3, Vector3(r, r * 0.65, r), Color("#ddb860"))


func _versatz(radius: float) -> Vector3:
	var winkel := _zufall.randf() * TAU
	return Vector3(cos(winkel), 0.0, sin(winkel)) * sqrt(_zufall.randf()) * radius


func _teil(form: Form, pos: Vector3, groesse: Vector3, farbe: Color, basis := Basis.IDENTITY) -> void:
	_teile[form].append([Transform3D(basis.scaled_local(groesse), pos), farbe])


func _baue_formen() -> void:
	var material := ShaderMaterial.new()
	material.shader = SHADER
	var blatt := SphereMesh.new()
	blatt.radius = 0.5
	blatt.height = 1.0
	blatt.radial_segments = 7
	blatt.rings = 3
	blatt.material = material
	var halm_material := material.duplicate() as ShaderMaterial
	halm_material.set_shader_parameter("wind", true)
	var halm := SurfaceTool.new()
	halm.begin(Mesh.PRIMITIVE_TRIANGLES)
	halm.set_material(halm_material)
	var unten_l := Vector3(-0.32, 0, 0)
	var unten_r := Vector3(0.32, 0, 0)
	var mitte_l := Vector3(-0.42, 0.54, 0.12)
	var mitte_r := Vector3(0.42, 0.54, 0.12)
	var spitze := Vector3(0.05, 1, 0.39)
	for punkt in [unten_l, mitte_l, unten_r, unten_r, mitte_l, mitte_r, mitte_l, spitze, mitte_r]:
		halm.add_vertex(punkt)
	halm.generate_normals()
	var topf := CylinderMesh.new()
	topf.top_radius = 0.5
	topf.bottom_radius = 0.43
	topf.height = 1.0
	topf.radial_segments = 12
	topf.rings = 0
	topf.material = material
	var ast := topf.duplicate() as CylinderMesh
	ast.top_radius = 0.22
	ast.bottom_radius = 0.50
	ast.radial_segments = 6
	_formen = [blatt, halm.commit(), topf, ast]
	_formen.append_array(Baumkronen.formen())


func _buendel(wurzel: Node3D, mesh: Mesh, teile: Array, schatten: bool, sichtweite: float) -> void:
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.use_colors = true
	mm.mesh = mesh
	mm.instance_count = teile.size()
	for i in teile.size():
		mm.set_instance_transform(i, teile[i][0])
		mm.set_instance_color(i, teile[i][1])
	var instanz := MultiMeshInstance3D.new()
	instanz.multimesh = mm
	instanz.visibility_range_end = sichtweite
	if not schatten:
		instanz.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	wurzel.add_child(instanz)
