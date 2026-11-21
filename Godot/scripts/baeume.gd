class_name Baeume
extends RefCounted
## Baeume als Instanzen: je Sektor ein MultiMesh fuer die Kronen (Laub rund aus mehreren
## Kugeln, Nadel aus gestapelten Kegeln) und eines fuer die Staemme. Die Zeilen kommen aus
## Tools/konvertiere_sektoren.py (*.baeume): x, y_fuss, z, krone_mitte, radius, halbe_hoehe,
## r, g, b (neun Zahlen je Baum, Meter, Godot-Achsen).

const SHADER := preload("res://shader/baum.gdshader")
const SPALTEN := 9
## Ab diesem Verhaeltnis Hoehe zu Radius gilt ein Baum als Nadelbaum.
const NADEL_VERHAELTNIS := 1.7
const SICHT_KRONE := 700.0
const SICHT_STAMM := 260.0

static var _laub: ArrayMesh
static var _nadel: ArrayMesh
static var _stamm: ArrayMesh
static var _mat_laub: ShaderMaterial
static var _mat_laub2: ShaderMaterial
static var _mat_nadel: ShaderMaterial
static var _mat_stamm: StandardMaterial3D


## Baut die Instanzen eines Sektors; fuegt sie an wurzel an und liefert die Zahl der Baeume.
static func stelle(wurzel: Node3D, zeilen: PackedFloat32Array, mit_stamm := true) -> int:
	var n := zeilen.size() / SPALTEN
	if n == 0:
		return 0
	_vorbereiten()
	var laub: Array[int] = []
	var nadel: Array[int] = []
	for i in n:
		var radius := zeilen[i * SPALTEN + 4]
		var hoehe := zeilen[i * SPALTEN + 5]
		(nadel if hoehe > radius * NADEL_VERHAELTNIS else laub).append(i)
	# Ein Teil der Laubbaeume traegt das hellere Lindenlaub, damit nicht alle gleich gruen sind.
	var laub_hell: Array[int] = []
	var laub_dunkel: Array[int] = []
	for i in laub:
		(laub_hell if fposmod(zeilen[i * SPALTEN] * 0.37 + zeilen[i * SPALTEN + 2] * 0.61, 1.0)
				< 0.4 else laub_dunkel).append(i)
	_krone(wurzel, zeilen, laub_dunkel, _laub, _mat_laub)
	_krone(wurzel, zeilen, laub_hell, _laub, _mat_laub2)
	_krone(wurzel, zeilen, nadel, _nadel, _mat_nadel)
	if mit_stamm:
		_staemme(wurzel, zeilen, range(n))
	return n


static func _krone(wurzel: Node3D, zeilen: PackedFloat32Array, liste: Array[int], netz: Mesh,
		material: Material) -> void:
	if liste.is_empty():
		return
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.use_colors = true
	mm.mesh = netz
	mm.instance_count = liste.size()
	for j in liste.size():
		var o := liste[j] * SPALTEN
		var gier := fposmod(zeilen[o] * 12.9898 + zeilen[o + 2] * 78.233, TAU)
		var basis := Basis(Vector3.UP, gier) * Basis.from_scale(
				Vector3(zeilen[o + 4], zeilen[o + 5], zeilen[o + 4]))
		var ort := Vector3(zeilen[o], zeilen[o + 1] + zeilen[o + 3], zeilen[o + 2])
		mm.set_instance_transform(j, Transform3D(basis, ort))
		var z := fposmod(sin(zeilen[o] * 3.7 + zeilen[o + 2] * 5.1) * 43758.5, 1.0)
		mm.set_instance_color(j, Color(0.85 + 0.3 * z, 0.9 + 0.2 * fposmod(z * 7.0, 1.0),
				0.85 + 0.3 * fposmod(z * 13.0, 1.0)))
	var inst := MultiMeshInstance3D.new()
	inst.multimesh = mm
	inst.material_override = material
	inst.visibility_range_end = SICHT_KRONE
	wurzel.add_child(inst)


static func _staemme(wurzel: Node3D, zeilen: PackedFloat32Array, liste: Array) -> void:
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.mesh = _stamm
	mm.instance_count = liste.size()
	for j in liste.size():
		var o := int(liste[j]) * SPALTEN
		# Der Stamm reicht von Fuss bis in die Krone hinein.
		var lang := maxf(zeilen[o + 3] - zeilen[o + 5] * 0.4, 1.2)
		var dick := clampf(zeilen[o + 4] * 0.22, 0.7, 1.8)
		var basis := Basis.from_scale(Vector3(dick, lang, dick))
		var fuss := Vector3(zeilen[o], zeilen[o + 1], zeilen[o + 2])
		mm.set_instance_transform(j, Transform3D(basis, fuss))
	var inst := MultiMeshInstance3D.new()
	inst.multimesh = mm
	inst.material_override = _mat_stamm
	inst.visibility_range_end = SICHT_STAMM
	wurzel.add_child(inst)


static func _vorbereiten() -> void:
	if _laub != null:
		return
	_laub = _runde_krone()
	_nadel = _kegel_krone()
	var zyl := CylinderMesh.new()
	zyl.top_radius = 0.10
	zyl.bottom_radius = 0.17
	zyl.height = 1.0
	zyl.radial_segments = 6
	zyl.rings = 1
	zyl.cap_bottom = false
	# Der Zylinder ist mittig; fuer die Instanz soll der Fuss bei y = 0 liegen.
	var arrays := zyl.get_mesh_arrays()
	var v: PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
	for i in v.size():
		v[i].y += 0.5
	arrays[Mesh.ARRAY_VERTEX] = v
	_stamm = ArrayMesh.new()
	_stamm.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	_mat_stamm = StandardMaterial3D.new()
	_mat_stamm.albedo_color = Color(0.30, 0.20, 0.12)
	_mat_stamm.roughness = 1.0
	var tex: Texture2D = load("res://texturen/laub.jpg") if ResourceLoader.exists(
			"res://texturen/laub.jpg") else null
	_mat_laub = _laubmaterial(tex, Vector3(0.95, 1.05, 0.8))
	_mat_laub2 = _laubmaterial(_textur("laub2", tex), Vector3(1.0, 1.05, 0.85))
	_mat_nadel = _laubmaterial(_textur("nadel", tex), Vector3(0.7, 0.9, 0.8))


## Textur aus res://texturen; fehlt sie, gilt der Ersatz.
static func _textur(name: String, ersatz: Texture2D) -> Texture2D:
	var pfad := "res://texturen/%s.jpg" % name
	return load(pfad) if ResourceLoader.exists(pfad) else ersatz


static func _laubmaterial(tex: Texture2D, ton: Vector3) -> ShaderMaterial:
	var m := ShaderMaterial.new()
	m.shader = SHADER
	if tex != null:
		m.set_shader_parameter("laub", tex)
	m.set_shader_parameter("grundton", ton)
	return m


## Mehrere sich durchdringende Kugeln ergeben eine unregelmaessige, runde Krone im
## Einheitsmass (Radius 1, Hoehe von -1 bis 1). Normalen zeigen vom Kronenmittelpunkt weg:
## so wirkt sie als eine Kugel und nicht als Kugelhaufen.
static func _runde_krone() -> ArrayMesh:
	var lappen := [
		[Vector3(0.0, 0.0, 0.0), 0.72], [Vector3(0.45, -0.15, 0.2), 0.5],
		[Vector3(-0.4, -0.1, 0.3), 0.5], [Vector3(0.1, 0.35, -0.4), 0.5],
		[Vector3(-0.25, 0.3, 0.1), 0.45], [Vector3(0.3, 0.2, 0.35), 0.4],
	]
	var pos := PackedVector3Array()
	var nor := PackedVector3Array()
	var idx := PackedInt32Array()
	for l: Array in lappen:
		var kugel := SphereMesh.new()
		kugel.radius = 1.0
		kugel.height = 2.0
		kugel.radial_segments = 10
		kugel.rings = 6
		var a := kugel.get_mesh_arrays()
		var v: PackedVector3Array = a[Mesh.ARRAY_VERTEX]
		var ni: PackedInt32Array = a[Mesh.ARRAY_INDEX]
		var start := pos.size()
		for p in v:
			var q: Vector3 = p * float(l[1]) + (l[0] as Vector3)
			pos.append(q)
			nor.append((q.normalized() * 0.7 + p.normalized() * 0.3).normalized())
		for i in ni:
			idx.append(i + start)
	return _netz(pos, nor, idx)


## Drei gestapelte Kegel, unten breit, oben schmal (Einheitsmass wie bei der Laubkrone).
static func _kegel_krone() -> ArrayMesh:
	var stufen := [[-0.15, 1.0, 1.1], [0.3, 0.75, 0.9], [0.68, 0.5, 0.7]]  # Mitte y, Radius, Hoehe
	var pos := PackedVector3Array()
	var nor := PackedVector3Array()
	var idx := PackedInt32Array()
	for s: Array in stufen:
		var kegel := CylinderMesh.new()
		kegel.top_radius = 0.0
		kegel.bottom_radius = float(s[1])
		kegel.height = float(s[2])
		kegel.radial_segments = 9
		kegel.rings = 1
		kegel.cap_top = false
		var a := kegel.get_mesh_arrays()
		var v: PackedVector3Array = a[Mesh.ARRAY_VERTEX]
		var nn: PackedVector3Array = a[Mesh.ARRAY_NORMAL]
		var ni: PackedInt32Array = a[Mesh.ARRAY_INDEX]
		var start := pos.size()
		for i in v.size():
			pos.append(v[i] + Vector3(0, float(s[0]), 0))
			nor.append(nn[i])
		for i in ni:
			idx.append(i + start)
	return _netz(pos, nor, idx)


static func _netz(pos: PackedVector3Array, nor: PackedVector3Array,
		idx: PackedInt32Array) -> ArrayMesh:
	var felder := []
	felder.resize(Mesh.ARRAY_MAX)
	felder[Mesh.ARRAY_VERTEX] = pos
	felder[Mesh.ARRAY_NORMAL] = nor
	felder[Mesh.ARRAY_INDEX] = idx
	var m := ArrayMesh.new()
	m.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, felder)
	return m
