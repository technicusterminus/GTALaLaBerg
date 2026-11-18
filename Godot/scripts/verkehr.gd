class_name Verkehr
extends Node3D
## Liest daten/verkehr.json und stellt KI-Autos, Passanten und geparkte Autos
## in die Stadt. Geparkte Autos sind je Sektor ein MultiMesh (ein Zeichenaufruf
## je Karosserieteil und Sektor, Lack aus der Instanzfarbe).
## Noch nicht portiert: Ampeln (44) und Kreuzungsbremsen (1726 Kreuzungen).

const SEKTOR_M := 500.0
const PARKSICHT := 700.0
const LACKE := [
	Color(0.55, 0.06, 0.05), Color(0.08, 0.12, 0.30), Color(0.75, 0.75, 0.77),
	Color(0.10, 0.10, 0.11), Color(0.80, 0.78, 0.70), Color(0.12, 0.30, 0.18),
]

var autos: Array[Verkehrsauto] = []
var passanten: Array[Passant] = []
var geparkt := 0
var _start_auto := Vector3.ZERO


func _ready() -> void:
	var d = JSON.parse_string(FileAccess.get_file_as_string("res://daten/verkehr.json"))
	if d == null:
		push_warning("daten/verkehr.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
		return
	for a: Dictionary in d["autos"]:
		var route := _route(a["p"])
		if route.size() < 2:
			continue
		var auto := Verkehrsauto.new()
		auto.tempo_ms = randf_range(28.0, 46.0) / 3.6
		auto.lack = LACKE[autos.size() % LACKE.size()]
		auto.breiten = PackedFloat32Array(a.get("bp", []))
		auto.weg.setze(route, a.get("rund", false))
		autos.append(auto)
		add_child(auto)
	for p: Dictionary in d["passanten"]:
		var route := _route(p["p"])
		if route.size() < 2:
			continue
		var passant := Passant.new()
		passant.tempo_ms = randf_range(4.2, 5.6) / 3.6
		passant.farbe = passanten.size()
		passant.weg.setze(route, false)
		passanten.append(passant)
		add_child(passant)
	_stelle_geparkte(d["geparkt"])
	if not autos.is_empty():
		_start_auto = autos[0].weg.ort
	print("LALABERG_VERKEHR autos=%d passanten=%d geparkt=%d ampeln=%d (noch ohne Wirkung)"
			% [autos.size(), passanten.size(), geparkt, d["ampeln"].size()])


## Wie weit sich das erste KI-Auto seit dem Start bewegt hat (Rauchtest).
func bewegung() -> float:
	return 0.0 if autos.is_empty() else autos[0].weg.ort.distance_to(_start_auto)


## Quelle: Zentimeter (x, y, z oben) in dreier-Gruppen -> Meter (x, z, y).
func _route(zahlen: Array) -> PackedVector3Array:
	var r := PackedVector3Array()
	for i in range(0, zahlen.size() - 2, 3):
		r.append(Vector3(zahlen[i], zahlen[i + 2], zahlen[i + 1]) / 100.0)
	return r


func _stelle_geparkte(liste: Array) -> void:
	var gruppen := {}  # Vector2i -> Array
	for g: Dictionary in liste:
		var k := Vector2i(floori(g["x"] / 100.0 / SEKTOR_M), floori(g["y"] / 100.0 / SEKTOR_M))
		if not gruppen.has(k):
			gruppen[k] = []
		gruppen[k].append(g)
	var netz := Wagenmesh.netz(Color.WHITE, true)
	for k: Vector2i in gruppen:
		var teil: Array = gruppen[k]
		var mm := MultiMesh.new()
		mm.transform_format = MultiMesh.TRANSFORM_3D
		mm.use_colors = true
		mm.mesh = netz
		mm.instance_count = teil.size()
		for i in teil.size():
			var g: Dictionary = teil[i]
			var ort := Vector3(g["x"], g["z"], g["y"]) / 100.0
			# Gier (Grad) dreht in Unreal +X Richtung +Y; auf Godots Achsen heisst das -Y-Drehung.
			var basis := Basis(Vector3.UP, -deg_to_rad(float(g["gier"])))
			mm.set_instance_transform(i, Transform3D(basis, ort))
			var c: Array = g["lack"]
			mm.set_instance_color(i, Color(c[0], c[1], c[2]))
		var instanz := MultiMeshInstance3D.new()
		instanz.multimesh = mm
		instanz.visibility_range_end = PARKSICHT
		add_child(instanz)
		geparkt += teil.size()
