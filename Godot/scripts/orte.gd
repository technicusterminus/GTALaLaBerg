class_name Orte
extends RefCounted
## Die Orte aus orte.json (Wahrzeichen und Plaetze) als Namen: Mitte des Ortes und
## naechster Punkt auf einer Strasse davor. Die Wahrzeichen liegen mitten im
## Gebaeude; als Ziel taugt der naechste Strassenpunkt, dort kommt man mit dem
## Wagen hin. Die Hoehe kommt vom naechsten Knoten des Strassennetzes.

static var _mitten := {}      # Name -> Vector3 (x, 0, z) in Metern
static var _zuege: Array[PackedVector2Array] = []
static var _geladen := false


static func laden() -> void:
	if _geladen:
		return
	var o = JSON.parse_string(FileAccess.get_file_as_string("res://daten/orte.json"))
	if o == null:
		push_warning("daten/orte.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
		return
	for m: Dictionary in o["marken"]:
		if not _mitten.has(m["n"]):
			_mitten[m["n"]] = Vector3(m["x"], 0.0, m["y"]) / 100.0
	# Plaetze stehen als Umriss da; als Ort zaehlt die Mitte des Umrisses.
	for p: Dictionary in o["plaetze"]:
		var punkte: Array = p["p"]
		if _mitten.has(p["n"]) or punkte.size() < 6:
			continue
		var summe := Vector2.ZERO
		var ecken := punkte.size() / 2
		for i in ecken:
			summe += Vector2(punkte[i * 2], punkte[i * 2 + 1])
		summe /= maxi(1, ecken)
		_mitten[p["n"]] = Vector3(summe.x, 0.0, summe.y) / 100.0
	for s: Dictionary in o["strassen"]:
		for t: Array in s["teile"]:
			var zug := PackedVector2Array()
			for i in range(0, t.size() - 1, 2):
				zug.append(Vector2(t[i], t[i + 1]) / 100.0)
			_zuege.append(zug)
	_geladen = true


static func gibt_es(name: String) -> bool:
	laden()
	return _mitten.has(name)


static func mitte(name: String) -> Vector3:
	laden()
	return _mitten.get(name, Vector3.ZERO)


## Naechster Punkt einer Strasse zum Ort, mit Hoehe vom Strassennetz.
static func strasse(name: String, polizei: Polizei) -> Vector3:
	laden()
	var m := mitte(name)
	var mitte2 := Vector2(m.x, m.z)
	var beste := mitte2
	var beste_d := INF
	for zug in _zuege:
		for i in range(zug.size() - 1):
			var p := Geometry2D.get_closest_point_to_segment(mitte2, zug[i], zug[i + 1])
			var d := mitte2.distance_to(p)
			if d < beste_d:
				beste_d = d
				beste = p
	var ort := Vector3(beste.x, 0.0, beste.y)
	if polizei != null:
		ort.y = polizei.naechster_punkt(ort).y
	return ort
