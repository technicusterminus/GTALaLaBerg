class_name Hud
extends CanvasLayer
## Ortsanzeige links unten (wie in der Unreal-Version: Platz vor Wahrzeichen vor
## Strasse), Tempo, Bildrate und Einsteigehinweis.

var ziel: Node3D
var tempo_ms := 0.0
var hinweis := ""
var status := ""
var auftrag_text := ""
var revier_text := ""
var laden_text := ""
var sterne := 0
var gesehen := true
var such_anteil := 0.0
var festnahme_anteil := 0.0

var _stadt := ""
var _altstadt: Rect2
var _strassen: Array = []   # {n, linien: Array[PackedVector2Array]}
var _plaetze: Array = []    # {n, umriss}
var _marken: Array = []     # {n, ort, r}
var _ortsteile: Array = []
var _ort_name := ""
var _ort_zusatz := ""
var _ort_seit := 0.0
var _takt := 0.0

var _l_ort := Label.new()
var _l_zusatz := Label.new()
var _l_tempo := Label.new()
var _l_fps := Label.new()
var _l_hinweis := Label.new()
var _l_status := Label.new()
var _l_rueck := Label.new()
var _l_fahndung := Label.new()
var _l_auftrag := Label.new()
var _l_revier := Label.new()
var _l_laden := Label.new()
var _rueck_bis := 0.0


func _ready() -> void:
	add_to_group("hud")
	_lade_orte()
	_platziere(_l_status, 16, 1.0, 0.0, -560, 8, 548, 24, HORIZONTAL_ALIGNMENT_RIGHT)
	_platziere(_l_rueck, 20, 0.5, 1.0, -350, -270, 700, 30, HORIZONTAL_ALIGNMENT_CENTER)
	_l_rueck.modulate.a = 0.0
	_platziere(_l_auftrag, 16, 0.0, 0.0, 12, 32, 760, 24, HORIZONTAL_ALIGNMENT_LEFT)
	_platziere(_l_revier, 14, 0.0, 0.0, 12, 58, 760, 22, HORIZONTAL_ALIGNMENT_LEFT)
	_platziere(_l_laden, 20, 0.5, 0.5, -300, -120, 600, 200, HORIZONTAL_ALIGNMENT_CENTER)
	_platziere(_l_fahndung, 22, 0.5, 0.0, -300, 40, 600, 60, HORIZONTAL_ALIGNMENT_CENTER)
	_platziere(_l_ort, 26, 0.0, 1.0, 44, -150, 520, 36, HORIZONTAL_ALIGNMENT_LEFT)
	_platziere(_l_zusatz, 14, 0.0, 1.0, 45, -110, 520, 22, HORIZONTAL_ALIGNMENT_LEFT)
	_platziere(_l_tempo, 30, 1.0, 1.0, -260, -110, 220, 40, HORIZONTAL_ALIGNMENT_RIGHT)
	_platziere(_l_fps, 13, 0.0, 0.0, 12, 8, 200, 20, HORIZONTAL_ALIGNMENT_LEFT)
	_platziere(_l_hinweis, 18, 0.5, 1.0, -250, -220, 500, 28, HORIZONTAL_ALIGNMENT_CENTER)


## Anker auf einen Punkt (ax, ay), Lage und Groesse als feste Offsets dazu.
func _platziere(l: Label, groesse: int, ax: float, ay: float, x: float, y: float,
		breite: float, hoehe: float, ausrichtung: HorizontalAlignment) -> void:
	l.add_theme_font_size_override("font_size", groesse)
	l.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.85))
	l.add_theme_constant_override("outline_size", 6)
	l.horizontal_alignment = ausrichtung
	l.anchor_left = ax
	l.anchor_right = ax
	l.anchor_top = ay
	l.anchor_bottom = ay
	l.offset_left = x
	l.offset_right = x + breite
	l.offset_top = y
	l.offset_bottom = y + hoehe
	add_child(l)


func _process(delta: float) -> void:
	_l_fps.text = "%d FPS" % Engine.get_frames_per_second()
	_l_tempo.text = "%d km/h" % roundi(tempo_ms * 3.6) if tempo_ms > 0.5 else ""
	_l_hinweis.text = hinweis
	_l_status.text = status
	_l_fahndung.text = _fahndungstext()
	_l_auftrag.text = auftrag_text
	_l_revier.text = revier_text
	_l_laden.text = laden_text
	_l_rueck.modulate.a = clampf(_rueck_bis - Time.get_ticks_msec() / 1000.0, 0.0, 1.0)
	_takt -= delta
	if _takt <= 0.0 and ziel != null:
		_takt = 0.5
		_bestimme_ort(Vector2(ziel.global_position.x, ziel.global_position.z) * 100.0)
	var deckung := clampf((Time.get_ticks_msec() / 1000.0 - _ort_seit) / 0.5, 0.0, 1.0)
	_l_ort.modulate.a = deckung
	_l_zusatz.modulate.a = deckung * 0.7
	_l_ort.text = _ort_name
	_l_zusatz.text = _ort_zusatz


## Sterne oben in der Mitte; darunter, ob man gesehen wird und wie weit Suche
## oder Festnahme sind.
func _fahndungstext() -> String:
	if sterne == 0:
		return ""
	var text := "★".repeat(sterne) + "☆".repeat(5 - sterne)
	if festnahme_anteil > 0.05:
		text += "\nFestnahme  %d %%" % roundi(festnahme_anteil * 100.0)
	elif gesehen:
		text += "\nGesucht"
	else:
		text += "\nUntergetaucht  %d %%" % roundi(such_anteil * 100.0)
	return text


## Kurze Meldung unten im Bild, blendet nach dreieinhalb Sekunden aus.
func zeige_rueckmeldung(text: String) -> void:
	_l_rueck.text = text
	_rueck_bis = Time.get_ticks_msec() / 1000.0 + 3.5


# --- Orte -------------------------------------------------------------------

func _punkte(zahlen: Array) -> PackedVector2Array:
	var p := PackedVector2Array()
	for i in range(0, zahlen.size() - 1, 2):
		p.append(Vector2(zahlen[i], zahlen[i + 1]))
	return p


func _lade_orte() -> void:
	var o = JSON.parse_string(FileAccess.get_file_as_string("res://daten/orte.json"))
	if o == null:
		push_warning("daten/orte.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
		return
	_stadt = o["stadt"]
	var a: Array = o["altstadt"]
	_altstadt = Rect2(Vector2(minf(a[0], a[2]), minf(a[1], a[3])),
			Vector2(absf(a[2] - a[0]), absf(a[3] - a[1])))
	for s: Dictionary in o["strassen"]:
		var linien: Array = []
		for t: Array in s["teile"]:
			linien.append(_punkte(t))
		_strassen.append({"n": s["n"], "linien": linien})
	for p: Dictionary in o["plaetze"]:
		_plaetze.append({"n": p["n"], "umriss": _punkte(p["p"])})
	for m: Dictionary in o["marken"]:
		_marken.append({"n": m["n"], "ort": Vector2(m["x"], m["y"]), "r": float(m["r"])})
	for t: Dictionary in o["ortsteile"]:
		_ortsteile.append({"n": t["n"], "ort": Vector2(t["x"], t["y"]), "r": float(t["r"])})
	print("LALABERG_ORTE strassen=%d plaetze=%d marken=%d ortsteile=%d"
			% [_strassen.size(), _plaetze.size(), _marken.size(), _ortsteile.size()])


## Vorrang: auf einem Platz steht man auf dem Platz; nahe einem Wahrzeichen
## sieht man das Wahrzeichen; sonst gilt die naechste Strasse bis 35 m.
func _bestimme_ort(p: Vector2) -> void:
	var ort := ""
	for platz: Dictionary in _plaetze:
		if Geometry2D.is_point_in_polygon(p, platz["umriss"]):
			ort = platz["n"]
			break
	if ort.is_empty():
		var beste := 1.0
		for m: Dictionary in _marken:
			var anteil: float = p.distance_to(m["ort"]) / m["r"]
			if anteil < beste:
				beste = anteil
				ort = m["n"]
	if ort.is_empty():
		var nah := 3500.0
		for s: Dictionary in _strassen:
			for linie: PackedVector2Array in s["linien"]:
				for i in range(linie.size() - 1):
					var d := p.distance_to(Geometry2D.get_closest_point_to_segment(p, linie[i], linie[i + 1]))
					if d < nah:
						nah = d
						ort = s["n"]
	var teil := ""
	if _altstadt.has_point(p):
		teil = "Altstadt"
	else:
		var beste := 1.0
		for t: Dictionary in _ortsteile:
			var anteil: float = p.distance_to(t["ort"]) / t["r"]
			if anteil < beste:
				beste = anteil
				teil = t["n"]
	var zusatz := _stadt if teil.is_empty() else "%s  ·  %s" % [teil, _stadt]
	if ort != _ort_name or zusatz != _ort_zusatz:
		_ort_name = ort
		_ort_zusatz = zusatz
		_ort_seit = Time.get_ticks_msec() / 1000.0
