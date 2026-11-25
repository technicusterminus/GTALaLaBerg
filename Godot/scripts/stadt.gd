class_name Stadt
extends Node3D
## Laedt die Stadtsektoren rund um einen Fokus nach und gibt entfernte wieder frei.
## Das Einlesen laeuft in Arbeitsthreads, im Hauptthread werden pro Bild nur
## ein Netz und eine Kollisionsform zusammengesetzt.

signal sektor_bereit(id: String)

const RADIUS := 2       ## Sektoren (Chebyshev) um den Fokus, die geladen werden
const ENTLADEN := 3     ## Weiter entfernte Sektoren werden freigegeben
const SICHTWEITE := 1600.0
const SHADER := preload("res://shader/stadt.gdshader")

var fokus: Node3D
var sektorgroesse := 500.0
var spawn := {}

var _eintraege := {}    # Vector2i -> Verzeichniseintrag
var _geladen := {}      # id -> Node3D
var _in_arbeit := {}    # id -> true
var _fertig: Array = []
var _sperre := Mutex.new()
var _materialien := {}
var _texturen := {}
var _hartmann_belag: Texture2D
var _katharinen_belag: Texture2D
var _breslauer_belag: Texture2D
var _bepflanzung := Bepflanzung.new()
var _hartmannviertel := Hartmannviertel.new()
var _kopf_typ := -1     # Variant-Typkennung von PackedVector3Array, -1 = langsamer Weg
var _takt := 0.0


func _ready() -> void:
	var text := FileAccess.get_file_as_string("res://daten/sektoren.json")
	var verz = JSON.parse_string(text)
	if verz == null:
		push_error("daten/sektoren.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
		return
	sektorgroesse = verz["sektorgroesse"]
	var hartmann_pfad := "res://daten/texturen/hartmann_freiflaechen.tga"
	if ResourceLoader.exists(hartmann_pfad, "Texture2D"):
		_hartmann_belag = load(hartmann_pfad)
	var katharinen_pfad := "res://daten/texturen/katharinen_freiflaechen.tga"
	if ResourceLoader.exists(katharinen_pfad, "Texture2D"):
		_katharinen_belag = load(katharinen_pfad)
	var breslauer_pfad := "res://daten/texturen/breslauer_freiflaechen.tga"
	if ResourceLoader.exists(breslauer_pfad, "Texture2D"):
		_breslauer_belag = load(breslauer_pfad)
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
			var abschnitt := {"pos": _zu_vec3(ecken), "idx": indizes}
			if a.get("uv", false):
				var uv_bytes := f.get_buffer(int(a["v"]) * 8)
				abschnitt["uv"] = _zu_vec2(uv_bytes)
			abschnitte.append(abschnitt)
	var flaechen := PackedVector3Array()
	var c := FileAccess.open("res://daten/sektoren/%s.col" % e["id"], FileAccess.READ)
	if c != null and int(e["kollision_ecken"]) > 0:
		flaechen = _zu_vec3(c.get_buffer(int(e["kollision_ecken"]) * 12).to_float32_array())
	_sperre.lock()
	_fertig.append({"eintrag": e, "abschnitte": abschnitte, "flaechen": flaechen})
	_sperre.unlock()


func _zu_vec2(bytes: PackedByteArray) -> PackedVector2Array:
	# Das Format speichert zwei Float32-Werte je UV ganz ohne Variant-Kopf.
	var werte := bytes.to_float32_array()
	var uv := PackedVector2Array()
	uv.resize(werte.size() >> 1)
	for i in uv.size():
		uv[i] = Vector2(werte[2 * i], werte[2 * i + 1])
	return uv


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
		if a.has("uv"):
			felder[Mesh.ARRAY_TEX_UV] = a["uv"]
		netz.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, felder)
		var meta: Dictionary = e["abschnitte"][i]
		netz.surface_set_material(flaeche, _material(meta["a"], meta["c"], a.has("uv")))
		flaeche += 1
	var instanz := MeshInstance3D.new()
	instanz.mesh = netz
	instanz.visibility_range_end = SICHTWEITE
	wurzel.add_child(instanz)

	var flaechen: PackedVector3Array = d["flaechen"]
	if flaechen.size() > 0:
		var form := ConcavePolygonShape3D.new()
		form.set_faces(flaechen)
		var koerper := StaticBody3D.new()
		var cs := CollisionShape3D.new()
		cs.shape = form
		koerper.add_child(cs)
		wurzel.add_child(koerper)

	_bepflanzung.baue(wurzel, k, sektorgroesse, e.get("wiese", []), e.get("kronen", []))
	if k == Klinikum.SEKTOR and DisplayServer.get_name() != "headless":
		Klinikum.new().baue(wurzel)
	if k == JvaTorbau.SEKTOR and DisplayServer.get_name() != "headless":
		JvaTorbau.new().baue(wurzel)
	if k == JvaTorbau.DACHSEKTOR and DisplayServer.get_name() != "headless":
		JvaTorbau.new().baue_dachlaterne(wurzel)
	if k == Platanenschule.SEKTOR and DisplayServer.get_name() != "headless":
		Platanenschule.new().baue(wurzel)
	_hartmannviertel.baue(wurzel, k, e.get("breslauer_objekte", []))
	if k == Katharinenschule.SEKTOR:
		Katharinenschule.new().baue(wurzel)
	add_child(wurzel)
	_geladen[id] = wurzel
	sektor_bereit.emit(id)


func _material(art: String, farbe: Array, hat_uv: bool) -> ShaderMaterial:
	var schluessel := "%s|%s|%s|%s|%s" % [art, farbe[0], farbe[1], farbe[2], hat_uv]
	if _materialien.has(schluessel):
		return _materialien[schluessel]
	var m := ShaderMaterial.new()
	m.shader = SHADER
	var grundfarbe := Vector3(farbe[0], farbe[1], farbe[2])
	var rauheit := 0.86
	var stil := 0
	var oben := false
	var textur := ""
	var kachel_m := 2.0
	match art:
		"Wall":
			grundfarbe = grundfarbe * 0.56 + Vector3(0.27, 0.21, 0.16)
			grundfarbe *= grundfarbe
			if hat_uv:
				textur = "Fassade"
				stil = 3
		"Ground":
			oben = true
			stil = 1
			grundfarbe = Vector3(0.12, 0.17, 0.065) if farbe[1] > farbe[0] else (
				Vector3(0.18, 0.17, 0.14) if farbe[0] == farbe[2] else Vector3(0.17, 0.19, 0.087))
			textur = "Wiese"
			kachel_m = 3.0
			rauheit = 1.0
		"Water":
			stil = 2
			oben = true
			grundfarbe = Vector3(0.028, 0.085, 0.075)
			rauheit = 0.23
		"Roof":
			oben = true
			if farbe[0] > farbe[1]:
				textur = "Ziegel"
				grundfarbe = Vector3(0.22, 0.075, 0.043)
			else:
				grundfarbe *= 0.37
		"Road", "Rail":
			oben = true
			textur = "Asphalt_Real"
			grundfarbe = Vector3(0.098, 0.105, 0.113)
		"Markierung":
			oben = true
			textur = "Asphalt_Real"
			grundfarbe = Vector3(0.52, 0.49, 0.42)
		"Sidewalk", "Gehweg", "Plaza":
			oben = true
			textur = "Pflaster" if art == "Plaza" else "Gehweg"
			grundfarbe = Vector3(0.29, 0.26, 0.22) if art == "Plaza" else Vector3(0.32, 0.29, 0.24)
		"Tree":
			grundfarbe = Vector3(0.068, 0.15, 0.047) if farbe[1] > farbe[0] else Vector3(0.046, 0.10, 0.034)
		"Trunk":
			grundfarbe = Vector3(0.15, 0.092, 0.049)
		"Sockel":
			stil = 4
			grundfarbe = grundfarbe * Vector3(0.31, 0.28, 0.23)
		"Laden":
			stil = 5 if hat_uv else 0
			grundfarbe = Vector3(0.054, 0.070, 0.072)
			rauheit = 0.38
		"Tuer":
			grundfarbe = grundfarbe * Vector3(0.27, 0.17, 0.12)
		_:
			grundfarbe = grundfarbe * 0.66
	m.set_shader_parameter("farbe", grundfarbe)
	m.set_shader_parameter("rauheit", rauheit)
	m.set_shader_parameter("oben", oben)
	m.set_shader_parameter("stil", stil)
	m.set_shader_parameter("ist_dach", art == "Roof")
	if art in ["Ground", "Road", "Sidewalk", "Gehweg", "Plaza"]:
		if _hartmann_belag != null:
			m.set_shader_parameter("hat_hartmann_belag", true)
			m.set_shader_parameter("hartmann_belag", _hartmann_belag)
		if _katharinen_belag != null:
			m.set_shader_parameter("hat_katharinen_belag", true)
			m.set_shader_parameter("katharinen_belag", _katharinen_belag)
		if _breslauer_belag != null:
			m.set_shader_parameter("hat_breslauer_belag", true)
			m.set_shader_parameter("breslauer_belag", _breslauer_belag)
	if not textur.is_empty():
		var detail := _textur(textur)
		if detail != null:
			m.set_shader_parameter("detail", detail)
			m.set_shader_parameter("kachel_m", kachel_m)
			m.set_shader_parameter("hat_detail", true)
	_materialien[schluessel] = m
	return m


func _textur(name: String) -> Texture2D:
	if not _texturen.has(name):
		var pfad := "res://daten/texturen/T_%s_D.png" % name
		_texturen[name] = load(pfad) if ResourceLoader.exists(pfad, "Texture2D") else null
	return _texturen[name]
