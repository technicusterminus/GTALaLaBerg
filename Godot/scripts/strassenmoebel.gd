class_name Strassenmoebel
extends Node
## Leben an der Strasse: Laternen, Strassenbaeume und Straeucher entlang des Strassennetzes.
## Die Plaetze werden einmal aus den Strassenabschnitten der Polizei berechnet und je Sektor
## gestellt, sobald er steht. Ein Bodenstrahl prueft jeden Platz: nur wo ungefaehr auf
## Strassenhoehe fester Boden ist (nicht in Haeusern, auf Daechern oder im Fluss), wird
## etwas gestellt. Alles haengt am Sektorknoten und verschwindet mit ihm.

const LAMPE_ABSTAND := 32.0
const BAUM_ABSTAND := 12.0
const BUSCH_ABSTAND := 4.5
const BANK_ABSTAND := 55.0
const PRUEFE_JE_BILD := 160

var stadt: Stadt
var polizei: Polizei
var gestellt := {"lampen": 0, "baeume": 0, "buesche": 0, "baenke": 0, "zaeune": 0, "obst": 0,
		"huetten": 0}

var _plaetze := {}   # Vector2i -> {"lampen": Array, "baeume": Array, "buesche": Array}
var _lampe: ArrayMesh
var _bank: ArrayMesh
var _zaun: ArrayMesh
var _huette: ArrayMesh
var _mat_metall: StandardMaterial3D
var _mat_licht: StandardMaterial3D


func _ready() -> void:
	if stadt == null or polizei == null:
		return
	_berechne()
	stadt.sektor_bereit.connect(_sektor_bereit)


func _hash(a: float, b: float) -> float:
	return fposmod(sin(a * 12.9898 + b * 78.233) * 43758.5453, 1.0)


func _berechne() -> void:
	var knoten := polizei.netz_knoten()
	var kanten := polizei.netz_kanten()
	for i in range(0, kanten.size() - 1, 2):
		var a := knoten[kanten[i]]
		var b := knoten[kanten[i + 1]]
		var flach := Vector3(b.x - a.x, 0.0, b.z - a.z)
		var laenge := flach.length()
		if laenge < 10.0:
			continue
		var r := flach / laenge
		var rechts := Vector3(-r.z, 0.0, r.x)
		var samen := _hash(a.x, a.z)
		var zaehler := 0
		var s := 3.0 + samen * 4.0
		while s < laenge - 3.0:
			var p := a.lerp(b, s / laenge)
			var seite := 1.0 if (zaehler + int(samen * 10.0)) % 2 == 0 else -1.0
			_merke(p, rechts, seite, 4.4, "lampen", r)
			zaehler += 1
			s += LAMPE_ABSTAND
		s = 5.0 + samen * 9.0
		zaehler = 0
		while s < laenge - 4.0:
			var p := a.lerp(b, s / laenge)
			var seite := 1.0 if zaehler % 2 == 0 else -1.0
			_merke(p, rechts, seite, 5.6 + 1.2 * _hash(p.x, p.z), "baeume", r)
			zaehler += 1
			s += BAUM_ABSTAND
		s = 2.0
		while s < laenge - 2.0:
			var p := a.lerp(b, s / laenge)
			var wuerfel := _hash(p.x * 1.7, p.z * 2.3)
			if wuerfel < 0.6:
				var seite := 1.0 if wuerfel < 0.3 else -1.0
				_merke(p, rechts, seite, 6.3 + 1.4 * wuerfel, "buesche", r)
			s += BUSCH_ABSTAND
		# Gaerten (nicht in der Altstadt): Lattenzaun hinter dem Gehweg, Obstbaeume und
		# Gartenhaeuschen auf dem Grundstueck dahinter.
		if not _in_altstadt(a):
			s = 3.0
			while s < laenge - 3.0:
				var p := a.lerp(b, s / laenge)
				for seite in [1.0, -1.0]:
					if _hash(p.x * 0.7 + seite, p.z * 1.3) < 0.92:
						_merke(p, rechts, seite, 8.2, "zaeune", r)
				s += 6.0
			s = 7.0 + samen * 6.0
			while s < laenge - 5.0:
				var p := a.lerp(b, s / laenge)
				var w := _hash(p.x * 2.1, p.z * 0.9)
				if w < 0.55:
					_merke(p, rechts, 1.0 if w < 0.27 else -1.0, 13.0 + 8.0 * w, "obst", r)
				s += 11.0
			s = 30.0 + samen * 30.0
			while s < laenge - 8.0:
				var p := a.lerp(b, s / laenge)
				if _hash(p.x * 3.3, p.z * 2.7) < 0.45:
					_merke(p, rechts, 1.0 if _hash(p.z, p.x) < 0.5 else -1.0, 19.0, "huetten", r)
				s += 45.0
		s = 20.0 + samen * 25.0
		zaehler = 0
		while s < laenge - 6.0:
			var p := a.lerp(b, s / laenge)
			_merke(p, rechts, 1.0 if zaehler % 2 == 0 else -1.0, 4.6, "baenke", r)
			zaehler += 1
			s += BANK_ABSTAND


func _merke(p: Vector3, rechts: Vector3, seite: float, abstand: float, art: String,
		richtung: Vector3) -> void:
	var ort := p + rechts * seite * abstand
	var k := stadt.sektor_an(ort)
	if not _plaetze.has(k):
		_plaetze[k] = _leer()
	# Zur Strasse hin zeigt die Laterne: Gegenrichtung der Seite.
	_plaetze[k][art].append({"ort": ort, "y": p.y, "zur_strasse": -rechts * seite,
			"richtung": richtung})


func _sektor_bereit(id: String) -> void:
	var wurzel := stadt.get_node_or_null(NodePath(id))
	if wurzel == null:
		return
	var k: Vector2i = wurzel.get_meta("index")
	if not _plaetze.has(k):
		return
	_stelle(wurzel, _plaetze[k])


func _stelle(wurzel: Node3D, plaetze: Dictionary) -> void:
	# Der Sektor hat gerade erst seine Kollision bekommen; die Strahlen brauchen sie.
	await get_tree().physics_frame
	await get_tree().physics_frame
	var raum := stadt.get_world_3d().direct_space_state
	var gueltig := _leer()
	var zaehler := 0
	for art: String in plaetze:
		for p: Dictionary in plaetze[art]:
			zaehler += 1
			if zaehler % PRUEFE_JE_BILD == 0:
				await get_tree().process_frame
				if not is_instance_valid(wurzel):
					return
			var ort: Vector3 = p["ort"]
			var abfrage := PhysicsRayQueryParameters3D.create(
					Vector3(ort.x, p["y"] + 20.0, ort.z), Vector3(ort.x, p["y"] - 6.0, ort.z))
			abfrage.collision_mask = 1
			var treffer := raum.intersect_ray(abfrage)
			# Ungefaehr Strassenhoehe und kein steiler Hang: sonst Haus, Dach oder Hang.
			if treffer.is_empty() or absf(treffer["position"].y - float(p["y"])) > 0.7 \
					or treffer["normal"].y < 0.85:
				continue
			p["boden"] = treffer["position"].y
			if art in ["zaeune", "huetten"] and not _haus_dahinter(raum, p):
				continue
			gueltig[art].append(p)
	if not is_instance_valid(wurzel):
		return
	_lampen(wurzel, gueltig["lampen"])
	_baeume(wurzel, gueltig["baeume"], false)
	_baeume(wurzel, gueltig["buesche"], true)
	_baenke(wurzel, gueltig["baenke"])
	_zaeune(wurzel, gueltig["zaeune"])
	_obst(wurzel, gueltig["obst"])
	_huetten(wurzel, gueltig["huetten"])


## Parkbaenke (Holzlatten, Gusseisenfuesse) mit Muelleimer daneben, zur Strasse gedreht.
func _baenke(wurzel: Node3D, liste: Array) -> void:
	if liste.is_empty():
		return
	if _bank == null:
		_bank = ArrayMesh.new()
		var holz := StandardMaterial3D.new()
		holz.albedo_color = Color(0.42, 0.26, 0.13)
		holz.roughness = 0.8
		var eisen := StandardMaterial3D.new()
		eisen.albedo_color = Color(0.08, 0.09, 0.08)
		eisen.roughness = 0.5
		var gruen := StandardMaterial3D.new()
		gruen.albedo_color = Color(0.10, 0.28, 0.16)
		gruen.roughness = 0.6
		_teil_in(_bank, [[Vector3(1.8, 0.05, 0.14), Vector3(0, 0.45, -0.16)],
				[Vector3(1.8, 0.05, 0.14), Vector3(0, 0.45, 0.0)],
				[Vector3(1.8, 0.05, 0.14), Vector3(0, 0.45, 0.16)],
				[Vector3(1.8, 0.14, 0.04), Vector3(0, 0.72, 0.26)],
				[Vector3(1.8, 0.14, 0.04), Vector3(0, 0.9, 0.28)]], holz)
		_teil_in(_bank, [[Vector3(0.06, 0.45, 0.5), Vector3(-0.8, 0.22, 0.05)],
				[Vector3(0.06, 0.45, 0.5), Vector3(0.8, 0.22, 0.05)],
				[Vector3(0.06, 0.55, 0.06), Vector3(-0.8, 0.72, 0.28)],
				[Vector3(0.06, 0.55, 0.06), Vector3(0.8, 0.72, 0.28)]], eisen)
		_teil_in(_bank, [[Vector3(0.38, 0.75, 0.38), Vector3(1.4, 0.38, 0.1)]], gruen)
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.mesh = _bank
	mm.instance_count = liste.size()
	for i in liste.size():
		var p: Dictionary = liste[i]
		var zur: Vector3 = (p["zur_strasse"] as Vector3).normalized()
		var basis := Basis.looking_at(zur, Vector3.UP)
		var o: Vector3 = p["ort"]
		mm.set_instance_transform(i, Transform3D(basis, Vector3(o.x, p["boden"], o.z)))
	var inst := MultiMeshInstance3D.new()
	inst.multimesh = mm
	inst.visibility_range_end = 160.0
	wurzel.add_child(inst)
	gestellt["baenke"] += liste.size()


static func _teil_in(netz: ArrayMesh, kaesten: Array, stoff: Material) -> void:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	for k: Array in kaesten:
		var b := BoxMesh.new()
		b.size = k[0]
		st.append_from(b, 0, Transform3D(Basis(), k[1]))
	st.generate_normals()
	st.commit(netz)
	netz.surface_set_material(netz.get_surface_count() - 1, stoff)


## Steht hinter dem Platz (weg von der Strasse) innerhalb von 25 m eine Hauswand? Nur dann ist
## dort ein Grundstueck, das einen Zaun oder ein Gartenhaeuschen bekommt.
func _haus_dahinter(raum: PhysicsDirectSpaceState3D, p: Dictionary) -> bool:
	var weg: Vector3 = -(p["zur_strasse"] as Vector3).normalized()
	var von := Vector3((p["ort"] as Vector3).x, float(p["boden"]) + 1.5, (p["ort"] as Vector3).z)
	var abfrage := PhysicsRayQueryParameters3D.create(von, von + weg * 25.0)
	abfrage.collision_mask = 1
	var t := raum.intersect_ray(abfrage)
	return not t.is_empty() and absf(t["normal"].y) < 0.3


static func _leer() -> Dictionary:
	return {"lampen": [], "baeume": [], "buesche": [], "baenke": [], "zaeune": [], "obst": [],
			"huetten": []}


func _in_altstadt(p: Vector3) -> bool:
	var r: Vector4 = stadt._altstadt_m
	return r.z > r.x and p.x > r.x and p.x < r.z and p.z > r.y and p.z < r.w


## Ein Feld Lattenzaun (6 m, weiss gestrichen) laengs der Strasse.
func _zaeune(wurzel: Node3D, liste: Array) -> void:
	if liste.is_empty():
		return
	if _zaun == null:
		_zaun = ArrayMesh.new()
		var weiss := StandardMaterial3D.new()
		weiss.albedo_color = Color(0.86, 0.84, 0.78)
		weiss.roughness = 0.8
		var teile: Array = [[Vector3(6.0, 0.08, 0.04), Vector3(0, 0.35, 0)],
				[Vector3(6.0, 0.08, 0.04), Vector3(0, 0.75, 0)]]
		for i in 25:
			teile.append([Vector3(0.09, 1.0, 0.03), Vector3(-3.0 + 0.24 * i + 0.12, 0.5, 0.03)])
		_teil_in(_zaun, teile, weiss)
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.mesh = _zaun
	mm.instance_count = liste.size()
	for i in liste.size():
		var p: Dictionary = liste[i]
		var x: Vector3 = (p["richtung"] as Vector3).normalized()
		var basis := Basis(x, Vector3.UP, x.cross(Vector3.UP))
		var o: Vector3 = p["ort"]
		mm.set_instance_transform(i, Transform3D(basis, Vector3(o.x, p["boden"], o.z)))
	var inst := MultiMeshInstance3D.new()
	inst.multimesh = mm
	inst.visibility_range_end = 150.0
	inst.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	wurzel.add_child(inst)
	gestellt["zaeune"] += liste.size()


## Obstbaeume: klein und rund, mit hellem Laub (Zeilen im Format von Baeume).
func _obst(wurzel: Node3D, liste: Array) -> void:
	if liste.is_empty():
		return
	var zeilen := PackedFloat32Array()
	for p: Dictionary in liste:
		var o: Vector3 = p["ort"]
		var w := _hash(o.x * 0.53, o.z * 0.91)
		var r := 1.6 + 0.8 * w
		zeilen.append_array(PackedFloat32Array([o.x, p["boden"], o.z, 2.4 + 0.6 * w, r, r * 0.85,
				0.3, 0.55, 0.25]))
	gestellt["obst"] += Baeume.stelle(wurzel, zeilen, true)


## Gartenhaeuschen: Holz, Satteldach mit Ziegeln, eine Tuer.
func _huetten(wurzel: Node3D, liste: Array) -> void:
	if liste.is_empty():
		return
	if _huette == null:
		_huette = ArrayMesh.new()
		var holz := StandardMaterial3D.new()
		holz.albedo_color = Color(0.48, 0.32, 0.18)
		holz.roughness = 0.9
		var dach := StandardMaterial3D.new()
		dach.albedo_color = Color(0.50, 0.20, 0.10)
		dach.roughness = 0.85
		var tuer := StandardMaterial3D.new()
		tuer.albedo_color = Color(0.18, 0.28, 0.18)
		_teil_in(_huette, [[Vector3(2.8, 2.1, 2.2), Vector3(0, 1.05, 0)]], holz)
		var st := SurfaceTool.new()
		st.begin(Mesh.PRIMITIVE_TRIANGLES)
		for s in [1.0, -1.0]:
			var b := BoxMesh.new()
			b.size = Vector3(3.2, 0.08, 1.65)
			st.append_from(b, 0, Transform3D(Basis(Vector3.RIGHT, s * 0.6),
					Vector3(0, 2.5, s * 0.66)))
		st.generate_normals()
		st.commit(_huette)
		_huette.surface_set_material(1, dach)
		_teil_in(_huette, [[Vector3(0.8, 1.7, 0.05), Vector3(0.6, 0.85, -1.11)]], tuer)
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.mesh = _huette
	mm.instance_count = liste.size()
	for i in liste.size():
		var p: Dictionary = liste[i]
		var zur: Vector3 = (p["zur_strasse"] as Vector3).normalized()
		var o: Vector3 = p["ort"]
		mm.set_instance_transform(i, Transform3D(Basis.looking_at(zur, Vector3.UP),
				Vector3(o.x, p["boden"], o.z)))
	var inst := MultiMeshInstance3D.new()
	inst.multimesh = mm
	inst.visibility_range_end = 220.0
	wurzel.add_child(inst)
	gestellt["huetten"] += liste.size()


func _lampen(wurzel: Node3D, liste: Array) -> void:
	if liste.is_empty():
		return
	_vorbereiten()
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.mesh = _lampe
	mm.instance_count = liste.size()
	for i in liste.size():
		var p: Dictionary = liste[i]
		var x: Vector3 = (p["zur_strasse"] as Vector3).normalized()
		var basis := Basis(x, Vector3.UP, x.cross(Vector3.UP))
		mm.set_instance_transform(i, Transform3D(basis, Vector3((p["ort"] as Vector3).x,
				p["boden"], (p["ort"] as Vector3).z)))
	var inst := MultiMeshInstance3D.new()
	inst.multimesh = mm
	inst.visibility_range_end = 300.0
	wurzel.add_child(inst)
	gestellt["lampen"] += liste.size()


## Zeilen im Format von Baeume: x, y_fuss, z, krone_mitte, radius, halbe_hoehe, r, g, b.
func _baeume(wurzel: Node3D, liste: Array, busch: bool) -> void:
	if liste.is_empty():
		return
	var zeilen := PackedFloat32Array()
	for p: Dictionary in liste:
		var o: Vector3 = p["ort"]
		var w := _hash(o.x * 0.31, o.z * 0.77)
		if busch:
			var r := 0.7 + 0.6 * w
			zeilen.append_array(PackedFloat32Array([o.x, p["boden"], o.z, r * 0.7, r, r * 0.72,
					0.3, 0.5, 0.3]))
		else:
			var r := 1.8 + 1.1 * w
			zeilen.append_array(PackedFloat32Array([o.x, p["boden"], o.z, 3.6 + 1.2 * w, r,
					r * 1.15, 0.25, 0.5, 0.25]))
	var a := Baeume.stelle(wurzel, zeilen, not busch)
	gestellt["buesche" if busch else "baeume"] += a


func _vorbereiten() -> void:
	if _lampe != null:
		return
	_mat_metall = StandardMaterial3D.new()
	_mat_metall.albedo_color = Color(0.10, 0.11, 0.12)
	_mat_metall.metallic = 0.5
	_mat_metall.roughness = 0.5
	_mat_licht = StandardMaterial3D.new()
	_mat_licht.albedo_color = Color(1.0, 0.92, 0.7)
	_mat_licht.emission_enabled = true
	_mat_licht.emission = Color(1.0, 0.85, 0.55)
	_mat_licht.emission_energy_multiplier = 1.2
	_lampe = ArrayMesh.new()
	var mast := CylinderMesh.new()
	mast.top_radius = 0.035
	mast.bottom_radius = 0.06
	mast.height = 5.0
	mast.radial_segments = 6
	mast.rings = 1
	var arm := BoxMesh.new()
	arm.size = Vector3(1.3, 0.08, 0.08)
	var kopf := BoxMesh.new()
	kopf.size = Vector3(0.55, 0.1, 0.3)
	_flaeche(mast, Vector3(0, 2.5, 0), _mat_metall)
	_flaeche(arm, Vector3(0.6, 5.0, 0), _mat_metall)
	_flaeche(kopf, Vector3(1.15, 4.93, 0), _mat_licht)


## Haengt ein Grundkoerper (Zylinder oder Quader) verschoben als eigene Flaeche an.
func _flaeche(m: PrimitiveMesh, versatz: Vector3, material: Material) -> void:
	var a := m.get_mesh_arrays()
	var v: PackedVector3Array = a[Mesh.ARRAY_VERTEX]
	for i in v.size():
		v[i] += versatz
	a[Mesh.ARRAY_VERTEX] = v
	_lampe.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, a)
	_lampe.surface_set_material(_lampe.get_surface_count() - 1, material)
