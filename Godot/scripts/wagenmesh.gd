class_name Wagenmesh
extends RefCounted
## Karosserie-Netz aus daten/wagen.json (Quelle der Unreal-Version), geteilt
## zwischen dem fahrbaren Auto, den KI-Autos und den geparkten Autos.
## Die Quelldaten schauen nach +X, das Netz behaelt diese Richtung.

const SHADER := preload("res://shader/flach.gdshader")
const SHADER_INSTANZ := preload("res://shader/flach_instanz.gdshader")

static var _cache := {}
static var _wagen = null


## lack: Farbe der lackierten Teile. instanziert: Lack kommt aus der
## Instanzfarbe eines MultiMesh (dann ist "lack" ohne Bedeutung).
static func netz(lack: Color, instanziert := false) -> ArrayMesh:
	var schluessel := "%s|%s" % [lack.to_html(false), instanziert]
	if _cache.has(schluessel):
		return _cache[schluessel]
	if _wagen == null:
		_wagen = JSON.parse_string(FileAccess.get_file_as_string("res://daten/wagen.json"))
	var netz := ArrayMesh.new()
	if _wagen == null:
		push_error("daten/wagen.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
		return netz
	for teil: Dictionary in _wagen["teile"]:
		var roh: Array = teil["positions"]
		var ecken := PackedVector3Array()
		ecken.resize(roh.size() / 3)
		for i in ecken.size():
			# Zentimeter, (x vorwaerts, y rechts, z oben) -> Meter, (x, y oben, z)
			ecken[i] = Vector3(roh[i * 3], roh[i * 3 + 2], roh[i * 3 + 1]) / 100.0
		var roh_idx: Array = teil["indices"]
		var indizes := PackedInt32Array()
		indizes.resize(roh_idx.size())
		for i in indizes.size():
			indizes[i] = int(roh_idx[i])
		var felder := []
		felder.resize(Mesh.ARRAY_MAX)
		felder[Mesh.ARRAY_VERTEX] = ecken
		felder[Mesh.ARRAY_INDEX] = indizes
		netz.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, felder)

		var lackiert: bool = teil.get("lack", false)
		var c: Array = teil["color"]
		var m := ShaderMaterial.new()
		m.shader = SHADER_INSTANZ if instanziert else SHADER
		m.set_shader_parameter("farbe", Vector3(c[0], c[1], c[2]))
		if instanziert:
			m.set_shader_parameter("lackiert", lackiert)
		elif lackiert:
			m.set_shader_parameter("farbe", Vector3(lack.r, lack.g, lack.b))
		var glatt: bool = teil["klasse"] == "Glas" or lackiert
		m.set_shader_parameter("rauheit", 0.25 if glatt else 0.8)
		m.set_shader_parameter("glanz", 1.0 if glatt else 0.0)
		netz.surface_set_material(netz.get_surface_count() - 1, m)
	_cache[schluessel] = netz
	return netz
