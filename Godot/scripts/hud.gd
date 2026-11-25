class_name Hud
extends CanvasLayer
## Ortsanzeige links unten (wie in der Unreal-Version: Platz vor Wahrzeichen vor
## Strasse), Tempo, Bildrate und Einsteigehinweis.

var ziel: Node3D
var tempo_ms := 0.0
var hinweis := ""
var status := ""
var leben := 100
var geld := 0
var waffe := ""
var ruf := 0
var auftrag_text := ""
var revier_text := ""
var laden_text := ""
var bude_text := ""
var drehbuch_text := ""
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
var _l_fps := Label.new()
var _l_hinweis := Label.new()
var _l_status := Label.new()
var _leiste := PanelContainer.new()
var _werte: Array[Label] = []
var _ziel_tafel := Panel.new()
var _linie := ColorRect.new()
var _tacho := Control.new()
var _l_rueck := Label.new()
var _l_fahndung := Label.new()
var _l_auftrag := Label.new()
var _l_revier := Label.new()
var _l_laden := Label.new()
var _l_bude := Label.new()
var _l_drehbuch := Label.new()
var _rueck_bis := 0.0


func _ready() -> void:
	add_to_group("hud")
	_lade_orte()
	_baue_leiste()
	_baue_ziel_tafel()
	_platziere(_l_rueck, 20, 0.5, 1.0, -350, -270, 700, 30, HORIZONTAL_ALIGNMENT_CENTER)
	_l_rueck.modulate.a = 0.0
	_platziere(_l_auftrag, 16, 0.0, 0.0, 12, 32, 760, 24, HORIZONTAL_ALIGNMENT_LEFT)
	_platziere(_l_revier, 14, 0.0, 0.0, 12, 58, 760, 22, HORIZONTAL_ALIGNMENT_LEFT)
	_platziere(_l_drehbuch, 16, 0.0, 0.0, 12, 80, 900, 48, HORIZONTAL_ALIGNMENT_LEFT)
	_platziere(_l_laden, 20, 0.5, 0.5, -300, -120, 600, 200, HORIZONTAL_ALIGNMENT_CENTER)
	_platziere(_l_bude, 22, 0.5, 0.0, -350, 110, 700, 30, HORIZONTAL_ALIGNMENT_CENTER)
	_platziere(_l_fahndung, 22, 0.5, 0.0, -300, 40, 600, 60, HORIZONTAL_ALIGNMENT_CENTER)
	_platziere(_l_ort, 46, 0.0, 1.0, 44, -176, 620, 60, HORIZONTAL_ALIGNMENT_LEFT)
	_platziere(_l_zusatz, 22, 0.0, 1.0, 47, -104, 620, 30, HORIZONTAL_ALIGNMENT_LEFT)
	_linie.color = Color(1, 1, 1, 0.85)
	_linie.anchor_top = 1.0
	_linie.anchor_bottom = 1.0
	_linie.offset_left = 48
	_linie.offset_right = 430
	_linie.offset_top = -112
	_linie.offset_bottom = -110
	_linie.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_linie)
	_baue_tacho()
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


## Die Statusleiste oben rechts: Herz, Muenze, Pistole und Stern mit ihren Zahlen (Icons vom
## Bildmodell, Godot/texturen/ui). Fehlen die Icons, bleibt der Text.
func _baue_leiste() -> void:
	_leiste.anchor_left = 1.0
	_leiste.anchor_right = 1.0
	_leiste.offset_left = -470
	_leiste.offset_right = -16
	_leiste.offset_top = 6
	_leiste.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_leiste.add_theme_stylebox_override("panel", _tafelstil())
	var reihe := HBoxContainer.new()
	reihe.add_theme_constant_override("separation", 16)
	reihe.alignment = BoxContainer.ALIGNMENT_END
	_leiste.add_child(reihe)
	for icon in ["herz", "muenze", "pistole", "stern"]:
		var paar := HBoxContainer.new()
		paar.add_theme_constant_override("separation", 6)
		var pfad := "res://texturen/ui/icon_%s.png" % icon
		if ResourceLoader.exists(pfad):
			var tr := TextureRect.new()
			tr.texture = load(pfad)
			tr.custom_minimum_size = Vector2(26, 26)
			tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
			tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
			paar.add_child(tr)
		var l := Label.new()
		l.add_theme_font_size_override("font_size", 18)
		l.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.8))
		l.add_theme_constant_override("outline_size", 4)
		paar.add_child(l)
		reihe.add_child(paar)
		_werte.append(l)
	add_child(_leiste)


## Runder Tacho unten rechts: Skala von 0 bis 160 km/h ueber 270 Grad, Nadel, Zahl und Einheit.
func _baue_tacho() -> void:
	_tacho.anchor_left = 1.0
	_tacho.anchor_right = 1.0
	_tacho.anchor_top = 1.0
	_tacho.anchor_bottom = 1.0
	_tacho.offset_left = -196
	_tacho.offset_right = -20
	_tacho.offset_top = -196
	_tacho.offset_bottom = -20
	_tacho.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_tacho.draw.connect(_zeichne_tacho)
	add_child(_tacho)


func _zeichne_tacho() -> void:
	var kmh := tempo_ms * 3.6
	if kmh < 0.5:
		return
	var mitte := _tacho.size / 2.0
	var r := minf(mitte.x, mitte.y) - 4.0
	_tacho.draw_circle(mitte, r, Color(0.04, 0.09, 0.20, 0.7))
	_tacho.draw_arc(mitte, r, 0.0, TAU, 64, Color(0.65, 0.8, 1.0, 0.8), 3.0, true)
	var von := deg_to_rad(135.0)
	var spanne := deg_to_rad(270.0)
	for i in 17:
		var w := von + spanne * float(i) / 16.0
		var lang := 14.0 if i % 4 == 0 else 7.0
		var farbe := Color(1, 0.35, 0.3) if i >= 13 else Color(0.85, 0.92, 1.0)
		_tacho.draw_line(mitte + Vector2(cos(w), sin(w)) * (r - 5.0 - lang),
				mitte + Vector2(cos(w), sin(w)) * (r - 5.0), farbe, 2.0, true)
	var anteil := clampf(kmh / 160.0, 0.0, 1.0)
	var wn := von + spanne * anteil
	_tacho.draw_line(mitte, mitte + Vector2(cos(wn), sin(wn)) * (r - 18.0), Color.WHITE, 3.0, true)
	_tacho.draw_circle(mitte, 5.0, Color(0.8, 0.9, 1.0))
	var schrift := ThemeDB.fallback_font
	var zahl := str(roundi(kmh))
	var breite := schrift.get_string_size(zahl, HORIZONTAL_ALIGNMENT_LEFT, -1, 36).x
	_tacho.draw_string(schrift, mitte + Vector2(-breite / 2.0, r * 0.62), zahl,
			HORIZONTAL_ALIGNMENT_LEFT, -1, 36, Color.WHITE)
	var einheit := schrift.get_string_size("km/h", HORIZONTAL_ALIGNMENT_LEFT, -1, 16).x
	_tacho.draw_string(schrift, mitte + Vector2(-einheit / 2.0, r * 0.62 + 20.0), "km/h",
			HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0.75, 0.85, 1.0))


func _tafelstil() -> StyleBoxFlat:
	var st := StyleBoxFlat.new()
	st.bg_color = Color(0.04, 0.09, 0.20, 0.62)
	st.border_color = Color(0.55, 0.72, 0.95, 0.55)
	st.set_border_width_all(1)
	st.set_corner_radius_all(10)
	st.content_margin_left = 12
	st.content_margin_right = 12
	st.content_margin_top = 4
	st.content_margin_bottom = 4
	return st


## Eine durchscheinende Tafel hinter den Auftragszeilen oben links; sie waechst mit dem Text.
func _baue_ziel_tafel() -> void:
	_ziel_tafel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_ziel_tafel.add_theme_stylebox_override("panel", _tafelstil())
	_ziel_tafel.position = Vector2(6, 28)
	_ziel_tafel.visible = false
	add_child(_ziel_tafel)
	move_child(_ziel_tafel, 0)


func _aktualisiere_tafeln() -> void:
	if _werte.size() == 4:
		_werte[0].text = str(leben)
		_werte[1].text = "%d €" % geld
		_werte[2].text = waffe
		_werte[3].text = str(ruf)
	var breite := 0.0
	var zeilen := 0
	for l: Label in [_l_auftrag, _l_revier, _l_drehbuch]:
		if l.text != "":
			breite = maxf(breite, l.get_minimum_size().x)
			zeilen += l.text.count("\n") + 1
	_ziel_tafel.visible = zeilen > 0
	_ziel_tafel.size = Vector2(minf(breite, 900.0) + 24.0, 12.0 + 24.0 * zeilen)


func _process(delta: float) -> void:
	_aktualisiere_tafeln()
	_l_fps.text = "%d FPS" % Engine.get_frames_per_second()
	_tacho.queue_redraw()
	_l_hinweis.text = hinweis
	_l_fahndung.text = _fahndungstext()
	_l_auftrag.text = auftrag_text
	_l_revier.text = revier_text
	_l_laden.text = laden_text
	_l_bude.text = bude_text
	_l_drehbuch.text = drehbuch_text
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
