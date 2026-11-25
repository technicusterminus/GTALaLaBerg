class_name Verkehr
extends Node3D
## Liest daten/verkehr.json und stellt KI-Autos, Passanten und geparkte Autos
## in die Stadt. Geparkte Autos sind je Sektor ein MultiMesh (ein Zeichenaufruf
## je Karosserieteil und Sektor, Lack aus der Instanzfarbe).

const SEKTOR_M := 500.0
const PARKSICHT := 380.0
const LACKE := [
	Color(0.55, 0.06, 0.05), Color(0.08, 0.12, 0.30), Color(0.75, 0.75, 0.77),
	Color(0.10, 0.10, 0.11), Color(0.80, 0.78, 0.70), Color(0.12, 0.30, 0.18),
	Color(0.92, 0.92, 0.90), Color(0.45, 0.47, 0.50), Color(0.62, 0.34, 0.10),
	Color(0.20, 0.38, 0.55), Color(0.70, 0.55, 0.12), Color(0.35, 0.06, 0.20),
]
## Fahrzeugmodelle im Verkehr mit Gewicht: meist Alltagsautos, seltener Sportwagen und
## Supercars, ganz selten ein Rennwagen. Geparkt stehen keine Rennwagen.
const MISCHUNG := {
	"klein": 24, "limousine": 22, "kombi": 22, "bus": 9, "sport": 12, "super": 7, "renn": 4,
}
## Lack fuer die Sportler: kraeftiger als der Alltagslack.
const SPORTLACKE := [
	Color(0.85, 0.08, 0.04), Color(0.95, 0.75, 0.05), Color(0.05, 0.25, 0.75),
	Color(0.92, 0.92, 0.92), Color(0.06, 0.06, 0.07), Color(0.20, 0.65, 0.25),
	Color(0.95, 0.40, 0.05),
]

var autos: Array[Verkehrsauto] = []
var ampeln: Array[Ampel] = []
var passanten: Array[Passant] = []
var geparkt := 0
var _start_auto := Vector3.ZERO


func _ready() -> void:
	var d = JSON.parse_string(FileAccess.get_file_as_string("res://daten/verkehr.json"))
	if d == null:
		push_warning("daten/verkehr.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
		return
	_lade_kreuzungen(d["kreuzungen"])
	for a: Dictionary in d["autos"]:
		var route := _route(a["p"])
		if route.size() < 2:
			continue
		var auto := Verkehrsauto.new()
		auto.tempo_ms = randf_range(28.0, 46.0) / 3.6
		auto.modell = waehle_modell(autos.size() * 7919 + 17, true)
		auto.lack = LACKE[autos.size() % LACKE.size()]
		if auto.modell in ["sport", "super", "renn"]:
			auto.lack = SPORTLACKE[autos.size() % SPORTLACKE.size()]
			auto.tempo_ms *= 1.12
		auto.breiten = PackedFloat32Array(a.get("bp", []))
		auto.klasse = int(a.get("klasse", 3))
		auto.klassen = PackedInt32Array(a.get("kp", []))
		auto.kreuzungen = PackedInt32Array(a.get("kreuzungen", []))
		# Die Spurdaten gelten nur, wenn sie zu den Wegpunkten passen.
		if auto.breiten.size() != route.size() or auto.klassen.size() != route.size():
			auto.breiten = PackedFloat32Array()
			auto.klassen = PackedInt32Array()
		for k in auto.kreuzungen:
			if not Verkehrsauto.kreuzung_autos.has(k):
				Verkehrsauto.kreuzung_autos[k] = []
			Verkehrsauto.kreuzung_autos[k].append(auto)
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
	for a: Dictionary in d["ampeln"]:
		var ampel := Ampel.new()
		ampel.gruppe = int(a["gruppe"])
		ampel.phase = int(a["phase"])
		ampel.anzahl_phasen = int(a.get("phasen", 2))
		add_child(ampel)
		ampel.global_position = Vector3(a["x"], a["z"], a["y"]) / 100.0
		# Das Modell schaut nach +X; die Gier (Grad) dreht in Unreal +X Richtung +Y.
		ampel.rotation.y = -deg_to_rad(float(a["gier"]))
		ampeln.append(ampel)
	_stelle_geparkte(d["geparkt"])
	if not autos.is_empty():
		_start_auto = autos[0].weg.ort
	print("LALABERG_VERKEHR autos=%d passanten=%d geparkt=%d ampeln=%d kreuzungen=%d"
			% [autos.size(), passanten.size(), geparkt, ampeln.size(),
			Verkehrsauto.kreuzung_orte.size()])


## Ein Modell nach MISCHUNG, fest aus einer Zahl (gleiche Zahl, gleiches Modell).
## Fehlt ein Modell in den Daten, nimmt der Wagen den Kleinwagen.
static func waehle_modell(samen: int, mit_renn: bool) -> String:
	var summe := 0
	for m: String in MISCHUNG:
		if mit_renn or m != "renn":
			summe += MISCHUNG[m]
	var wahl := posmod(hash(samen), summe)
	for m: String in MISCHUNG:
		if not mit_renn and m == "renn":
			continue
		wahl -= MISCHUNG[m]
		if wahl < 0:
			return m if Wagenmesh.hat_modell(m) else "klein"
	return "klein"


func _lade_kreuzungen(liste: Array) -> void:
	Verkehrsauto.kreuzung_orte = PackedVector2Array()
	Verkehrsauto.kreuzung_klassen = PackedInt32Array()
	Verkehrsauto.kreuzung_breiten = PackedFloat32Array()
	Verkehrsauto.kreuzung_autos = {}
	for k: Dictionary in liste:
		Verkehrsauto.kreuzung_orte.append(Vector2(k["x"], k["y"]) / 100.0)
		Verkehrsauto.kreuzung_klassen.append(int(k["klasse"]))
		# Fehlt die Breite (alte Staende), gilt 3,8 m wie die mittlere Klasse.
		Verkehrsauto.kreuzung_breiten.append(float(k.get("breite", 3.8)))


## Wie weit sich das erste KI-Auto seit dem Start bewegt hat (Rauchtest).
func bewegung() -> float:
	if autos.is_empty() or not is_instance_valid(autos[0]):
		return 0.0
	return autos[0].weg.ort.distance_to(_start_auto)


## Wie weit sich der Knoten des ersten KI-Autos bewegt hat: prueft, dass das Auto
## wirklich fuer die Physik und das Auge faehrt und nicht nur auf seiner Route.
func bewegung_knoten() -> float:
	if autos.is_empty() or not is_instance_valid(autos[0]):
		return 0.0
	return autos[0].global_position.distance_to(_start_auto)


## Quelle: Zentimeter (x, y, z oben) in dreier-Gruppen -> Meter (x, z, y).
func _route(zahlen: Array) -> PackedVector3Array:
	var r := PackedVector3Array()
	for i in range(0, zahlen.size() - 2, 3):
		r.append(Vector3(zahlen[i], zahlen[i + 2], zahlen[i + 1]) / 100.0)
	return r


## Je Sektor und Modell ein MultiMesh.
func _stelle_geparkte(liste: Array) -> void:
	var gruppen := {}  # [Vector2i, modell] -> Array
	for g: Dictionary in liste:
		var k := Vector2i(floori(g["x"] / 100.0 / SEKTOR_M), floori(g["y"] / 100.0 / SEKTOR_M))
		var modell := waehle_modell(int(g["x"]) * 31 + int(g["y"]), false)
		var schluessel := [k, modell]
		if not gruppen.has(schluessel):
			gruppen[schluessel] = []
		gruppen[schluessel].append(g)
	for schluessel: Array in gruppen:
		var teil: Array = gruppen[schluessel]
		var mm := MultiMesh.new()
		mm.transform_format = MultiMesh.TRANSFORM_3D
		mm.use_colors = true
		mm.mesh = Wagenmesh.netz(Color.WHITE, true, schluessel[1])
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
		instanz.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_DOUBLE_SIDED
		add_child(instanz)
		geparkt += teil.size()
