class_name Stadt
extends Node3D
## Laedt die Stadtsektoren rund um einen Fokus nach und gibt entfernte wieder frei.
## Das Einlesen laeuft in Arbeitsthreads, im Hauptthread werden pro Bild nur
## ein Netz und eine Kollisionsform zusammengesetzt.

signal sektor_bereit(id: String)

const RADIUS := 2       ## Sektoren (Chebyshev) um den Fokus, die geladen werden
const ENTLADEN := 3     ## Weiter entfernte Sektoren werden freigegeben
const SICHTWEITE := 1600.0
const SHADER := preload("res://shader/flach.gdshader")

var fokus: Node3D
var sektorgroesse := 500.0
var spawn := {}
var baeume := 0   ## wie viele Baum-Instanzen bisher gestellt wurden
var gauben := 0   ## ... und Dachgauben

var _eintraege := {}    # Vector2i -> Verzeichniseintrag
var _geladen := {}      # id -> Node3D
var _in_arbeit := {}    # id -> true
var _fertig: Array = []
var _sperre := Mutex.new()
var _materialien := {}
var _kopf_typ := -1     # Variant-Typkennung von PackedVector3Array, -1 = langsamer Weg
var _takt := 0.0


func _ready() -> void:
	var text := FileAccess.get_file_as_string("res://daten/sektoren.json")
	var verz = JSON.parse_string(text)
	if verz == null:
		push_error("daten/sektoren.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
		return
	sektorgroesse = verz["sektorgroesse"]
	spawn = verz["spawn"]
	for e in verz["sektoren"]:
		_eintraege[Vector2i(int(e["index"][0]), int(e["index"][1]))] = e
	_kopf_typ = _pruefe_schnellen_weg()
	print("LALABERG_STADT sektoren=%d schneller_weg=%s" % [_eintraege.size(), _kopf_typ >= 0])


func _process(delta: float) -> void:
	_takt -= delta
	if _takt <= 0.0 and fokus != null:
		_takt = 0.25
		_aktualisiere()
	_sperre.lock()
	var daten = _fertig.pop_front() if not _fertig.is_empty() else null
	_sperre.unlock()
	if daten != null:
		_baue_sektor(daten)


func sektor_an(pos: Vector3) -> Vector2i:
	return Vector2i(floori(pos.x / sektorgroesse), floori(pos.z / sektorgroesse))


func ist_geladen(id: String) -> bool:
	return _geladen.has(id)


func eintrag_fuer(pos: Vector3) -> Dictionary:
	return _eintraege.get(sektor_an(pos), {})


func _aktualisiere() -> void:
	var mitte := sektor_an(fokus.global_position)
	var gewuenscht: Array = []
	for dx in range(-RADIUS, RADIUS + 1):
		for dy in range(-RADIUS, RADIUS + 1):
			var k := mitte + Vector2i(dx, dy)
			if _eintraege.has(k):
				gewuenscht.append(k)
	gewuenscht.sort_custom(func(a: Vector2i, b: Vector2i) -> bool:
		return (a - mitte).length_squared() < (b - mitte).length_squared())
	for k: Vector2i in gewuenscht:
		var e: Dictionary = _eintraege[k]
		var id: String = e["id"]
		if _geladen.has(id) or _in_arbeit.has(id):
			continue
		if _in_arbeit.size() >= 2:
			break
		_in_arbeit[id] = true
		WorkerThreadPool.add_task(_lese.bind(e))
	for id: String in _geladen.keys():
		var knoten: Node3D = _geladen[id]
		var k: Vector2i = knoten.get_meta("index")
		if maxi(absi(k.x - mitte.x), absi(k.y - mitte.y)) > ENTLADEN:
			knoten.queue_free()
			_geladen.erase(id)


# --- Arbeitsthread ---------------------------------------------------------

func _lese(e: Dictionary) -> void:
	var abschnitte: Array = []
	var f := FileAccess.open("res://daten/sektoren/%s.msh" % e["id"], FileAccess.READ)
	if f == null:
		push_error("Sektor fehlt: %s" % e["id"])
	else:
		for a: Dictionary in e["abschnitte"]:
			var ecken := f.get_buffer(int(a["v"]) * 12).to_float32_array()
			var indizes := f.get_buffer(int(a["i"]) * 4).to_int32_array()
			abschnitte.append({"pos": _zu_vec3(ecken), "idx": indizes})
	var flaechen := PackedVector3Array()
	var c := FileAccess.open("res://daten/sektoren/%s.col" % e["id"], FileAccess.READ)
	if c != null and int(e["kollision_ecken"]) > 0:
		flaechen = _zu_vec3(c.get_buffer(int(e["kollision_ecken"]) * 12).to_float32_array())
	var gauben := PackedFloat32Array()
	var gf := FileAccess.open("res://daten/sektoren/%s.gauben" % e["id"], FileAccess.READ)
	if gf != null and int(e.get("gauben", 0)) > 0:
		gauben = gf.get_buffer(int(e["gauben"]) * Gauben.SPALTEN * 4).to_float32_array()
	var baeume := PackedFloat32Array()
	var bf := FileAccess.open("res://daten/sektoren/%s.baeume" % e["id"], FileAccess.READ)
	if bf != null and int(e.get("baeume", 0)) > 0:
		baeume = bf.get_buffer(int(e["baeume"]) * Baeume.SPALTEN * 4).to_float32_array()
	_sperre.lock()
	_fertig.append({"eintrag": e, "abschnitte": abschnitte, "flaechen": flaechen, "baeume": baeume,
			"gauben": gauben})
	_sperre.unlock()


## Fester Weg ueber bytes_to_var; Schleife als Rueckfall, falls das Format abweicht.
func _zu_vec3(f: PackedFloat32Array) -> PackedVector3Array:
	var n := f.size() / 3
	if _kopf_typ >= 0 and n > 0:
		var b := PackedByteArray()
		b.resize(8)
		b.encode_u32(0, _kopf_typ)
		b.encode_u32(4, n)
		b.append_array(f.to_byte_array())
		var r = bytes_to_var(b)
		if r is PackedVector3Array and r.size() == n \
				and r[0].is_equal_approx(Vector3(f[0], f[1], f[2])) \
				and r[n - 1].is_equal_approx(Vector3(f[n * 3 - 3], f[n * 3 - 2], f[n * 3 - 1])):
			return r
	var v := PackedVector3Array()
	v.resize(n)
	for i in n:
		v[i] = Vector3(f[i * 3], f[i * 3 + 1], f[i * 3 + 2])
	return v


func _pruefe_schnellen_weg() -> int:
	var probe := var_to_bytes(PackedVector3Array([Vector3(1, 2, 3), Vector3(4, 5, 6)]))
	if probe.size() != 8 + 24:
		return -1
	var typ := probe.decode_u32(0)
	var test := PackedFloat32Array([1, 2, 3, 4, 5, 6])
	var vorher := _kopf_typ
	_kopf_typ = typ
	var r := _zu_vec3(test)
	_kopf_typ = vorher
	return typ if r.size() == 2 and r[1].is_equal_approx(Vector3(4, 5, 6)) else -1


# --- Hauptthread -----------------------------------------------------------

func _baue_sektor(d: Dictionary) -> void:
	var e: Dictionary = d["eintrag"]
	var id: String = e["id"]
	_in_arbeit.erase(id)
	var mitte := sektor_an(fokus.global_position) if fokus != null else Vector2i.ZERO
	var k := Vector2i(int(e["index"][0]), int(e["index"][1]))
	if maxi(absi(k.x - mitte.x), absi(k.y - mitte.y)) > ENTLADEN:
		return  # Fokus ist inzwischen weitergezogen
	var wurzel := Node3D.new()
	wurzel.name = id
	wurzel.set_meta("index", k)

	var netz := ArrayMesh.new()
	var flaeche := 0
	for i in d["abschnitte"].size():
		var a: Dictionary = d["abschnitte"][i]
		var felder := []
		felder.resize(Mesh.ARRAY_MAX)
		felder[Mesh.ARRAY_VERTEX] = a["pos"]
		felder[Mesh.ARRAY_INDEX] = a["idx"]
		netz.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, felder)
		var meta: Dictionary = e["abschnitte"][i]
		netz.surface_set_material(flaeche, _material(meta["a"], meta["c"]))
		flaeche += 1
	var instanz := MeshInstance3D.new()
	instanz.mesh = netz
	instanz.visibility_range_end = SICHTWEITE
	wurzel.add_child(instanz)

	baeume += Baeume.stelle(wurzel, d["baeume"])
	gauben += Gauben.stelle(wurzel, d["gauben"])

	var flaechen: PackedVector3Array = d["flaechen"]
	if flaechen.size() > 0:
		var form := ConcavePolygonShape3D.new()
		form.set_faces(flaechen)
		var koerper := StaticBody3D.new()
		var cs := CollisionShape3D.new()
		cs.shape = form
		koerper.add_child(cs)
		wurzel.add_child(koerper)

	add_child(wurzel)
	_geladen[id] = wurzel
	sektor_bereit.emit(id)


func _material(art: String, farbe: Array) -> ShaderMaterial:
	var schluessel := "%s|%s|%s|%s" % [art, farbe[0], farbe[1], farbe[2]]
	if _materialien.has(schluessel):
		return _materialien[schluessel]
	var m := ShaderMaterial.new()
	m.shader = SHADER
	m.set_shader_parameter("farbe", Vector3(farbe[0], farbe[1], farbe[2]))
	for t in ["dach", "putz", "pflaster", "wiese"]:
		var pfad := "res://texturen/%s.jpg" % t
		if ResourceLoader.exists(pfad):
			m.set_shader_parameter("detail_" + t, load(pfad))
	if art in ["Ground", "Road", "Sidewalk", "Gehweg", "Plaza", "Rail", "Water", "Roof"]:
		m.set_shader_parameter("oben", true)
	match art:
		"Water":
			m.set_shader_parameter("stil", 2)
			m.set_shader_parameter("rauheit", 0.1)
			m.set_shader_parameter("glanz", 1.0)
		"Road", "Rail":
			m.set_shader_parameter("rauheit", 0.85)
		"Roof", "Wall", "Sockel", "Gesims":
			if art == "Wall":
				m.set_shader_parameter("stil", 3)
				var da := ResourceLoader.exists("res://texturen/fenster_a.jpg")
				if da:
					for f in ["a", "b", "c"]:
						m.set_shader_parameter("fenster_" + f, load("res://texturen/fenster_%s.jpg" % f))
					m.set_shader_parameter("fassade_foto", true)
			elif art == "Roof":
				m.set_shader_parameter("stil", 4)
			m.set_shader_parameter("rauheit", 0.8)
		"Gehweg", "Sidewalk", "Plaza":
			m.set_shader_parameter("stil", 5)
			m.set_shader_parameter("rauheit", 0.9)
		"Tree", "Trunk", "Ground":
			if art == "Ground":
				m.set_shader_parameter("stil", 1)
			elif art == "Tree":
				m.set_shader_parameter("stil", 6)
			m.set_shader_parameter("rauheit", 1.0)
	_materialien[schluessel] = m
	return m
