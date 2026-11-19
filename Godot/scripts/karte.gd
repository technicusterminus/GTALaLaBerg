class_name Karte
extends CanvasLayer
## Minikarte (oben rechts, Norden oben), Vollkarte auf M und das Pausenmenue auf Esc.
## Die Kartenvorlage liegt als daten/karte.png vor (3 m je Pixel); ihre Lage kommt
## aus daten/karte.json. Fehlt sie, bleibt nur das Menue.

const MINI_PX := 200.0
const MINI_SICHT_M := 420.0
const RAND := Color(0.05, 0.05, 0.07, 0.9)
const SPIELER := Color(1.0, 0.85, 0.15)
const ZIEL := Color(0.25, 0.85, 1.0)

var fokus: Node3D
var kamera: Kamera
var auftraege: Auftraege
var voll := false
var menue_offen := false

var _bild: Texture2D
var _ursprung := Vector2.ZERO   # Meter (x, z) der linken oberen Ecke
var _m_je_px := 3.0
var _mini := Control.new()
var _gross := Control.new()
var _menue := PanelContainer.new()
var _l_titel := Label.new()


func _ready() -> void:
	add_to_group("karte")
	process_mode = Node.PROCESS_MODE_ALWAYS
	layer = 5
	_lade_karte()
	_mini.custom_minimum_size = Vector2(MINI_PX, MINI_PX)
	_mini.anchor_left = 1.0
	_mini.anchor_right = 1.0
	_mini.offset_left = -MINI_PX - 16.0
	_mini.offset_right = -16.0
	_mini.offset_top = 40.0
	_mini.offset_bottom = 40.0 + MINI_PX
	_mini.clip_contents = true
	_mini.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_mini.draw.connect(_zeichne_mini)
	add_child(_mini)
	_gross.set_anchors_preset(Control.PRESET_FULL_RECT)
	_gross.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_gross.visible = false
	_gross.draw.connect(_zeichne_gross)
	add_child(_gross)
	_baue_menue()


func _lade_karte() -> void:
	var f := FileAccess.open("res://daten/karte.json", FileAccess.READ)
	if f == null or not FileAccess.file_exists("res://daten/karte.png"):
		return
	var d: Dictionary = JSON.parse_string(f.get_as_text())
	var bild := Image.load_from_file(ProjectSettings.globalize_path("res://daten/karte.png"))
	if d == null or bild == null:
		return
	_bild = ImageTexture.create_from_image(bild)
	_ursprung = Vector2(float(d["x0"]), float(d["y0"])) / 100.0
	_m_je_px = float(d["breite"]) * float(d["cmProPixel"]) / 100.0 / float(bild.get_width())


func hat_karte() -> bool:
	return _bild != null


## Bildpunkt (Pixel der Kartenvorlage) zu einem Ort in der Welt.
func bild_punkt(ort: Vector3) -> Vector2:
	return (Vector2(ort.x, ort.z) - _ursprung) / _m_je_px


func _baue_menue() -> void:
	_menue.visible = false
	_menue.set_anchors_preset(Control.PRESET_CENTER)
	_menue.offset_left = -170
	_menue.offset_right = 170
	_menue.offset_top = -170
	_menue.offset_bottom = 170
	var kasten := VBoxContainer.new()
	kasten.add_theme_constant_override("separation", 10)
	_menue.add_child(kasten)
	_l_titel.text = "GTA LaLaBerg"
	_l_titel.add_theme_font_size_override("font_size", 30)
	_l_titel.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	kasten.add_child(_l_titel)
	for eintrag: Array in [["Weiter", schliesse_menue], ["Karte", _zeige_karte],
			["Speichern", _speichern], ["Beenden", _beenden]]:
		var b := Button.new()
		b.text = eintrag[0]
		b.custom_minimum_size = Vector2(0, 48)
		b.pressed.connect(eintrag[1])
		kasten.add_child(b)
	add_child(_menue)


func _speichern() -> void:
	Konto.speichere()
	Schaden.rueckmeldung(get_tree(), "Spielstand gespeichert")
	schliesse_menue()


func _zeige_karte() -> void:
	schliesse_menue()
	schalte_vollkarte(true)


func _beenden() -> void:
	Konto.speichere()
	get_tree().paused = false
	get_tree().quit()


func oeffne_menue() -> void:
	menue_offen = true
	voll = false
	_gross.visible = false
	_menue.visible = true
	get_tree().paused = true
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE


func schliesse_menue() -> void:
	menue_offen = false
	_menue.visible = false
	get_tree().paused = false
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func schalte_vollkarte(an: bool) -> void:
	if an and _bild == null:
		return
	voll = an
	_gross.visible = an
	_gross.queue_redraw()
	get_tree().paused = an
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if an else Input.MOUSE_MODE_CAPTURED


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("menue"):
		if menue_offen:
			schliesse_menue()
		elif voll:
			schalte_vollkarte(false)
		else:
			oeffne_menue()
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("karte") and not menue_offen:
		schalte_vollkarte(not voll)
		get_viewport().set_input_as_handled()


func _process(_delta: float) -> void:
	if _bild != null and fokus != null:
		_mini.queue_redraw()
		if voll:
			_gross.queue_redraw()


## Pfeil auf (mitte), Spitze in Blickrichtung der Kamera.
func _pfeil(zeichner: Control, mitte: Vector2, groesse: float) -> void:
	var blick := Vector2(0, -1)
	if kamera != null:
		var v := kamera.vorwaerts()
		blick = Vector2(v.x, v.z).normalized()
	var quer := Vector2(-blick.y, blick.x)
	var punkte := PackedVector2Array([mitte + blick * groesse,
			mitte - blick * groesse * 0.7 + quer * groesse * 0.6,
			mitte - blick * groesse * 0.3,
			mitte - blick * groesse * 0.7 - quer * groesse * 0.6])
	zeichner.draw_colored_polygon(punkte, SPIELER)
	zeichner.draw_polyline(punkte + PackedVector2Array([punkte[0]]), Color.BLACK, 1.5)


func _zeichne_mini() -> void:
	_mini.draw_rect(Rect2(Vector2.ZERO, _mini.size), Color(0.1, 0.12, 0.16))
	if _bild == null or fokus == null:
		return
	var halb := MINI_SICHT_M / _m_je_px / 2.0
	var mitte := bild_punkt(fokus.global_position)
	var quelle := Rect2(mitte - Vector2(halb, halb), Vector2(halb, halb) * 2.0)
	_mini.draw_texture_rect_region(_bild, Rect2(Vector2.ZERO, _mini.size), quelle)
	var faktor := _mini.size.x / (halb * 2.0)
	var mitte_mini := _mini.size / 2.0
	if auftraege != null and auftraege.unterwegs:
		var d := (bild_punkt(auftraege.ziel_ort()) - mitte) * faktor
		d = d.limit_length(MINI_PX / 2.0 - 8.0)
		_mini.draw_circle(mitte_mini + d, 6.0, ZIEL)
	_pfeil(_mini, mitte_mini, 9.0)
	_mini.draw_rect(Rect2(Vector2.ZERO, _mini.size), RAND, false, 3.0)


func _zeichne_gross() -> void:
	if _bild == null:
		return
	var flaeche := _gross.size
	_gross.draw_rect(Rect2(Vector2.ZERO, flaeche), Color(0, 0, 0, 0.82))
	var bildgroesse := _bild.get_size()
	var faktor := minf(flaeche.x / bildgroesse.x, (flaeche.y - 60.0) / bildgroesse.y)
	var oben_links := (flaeche - bildgroesse * faktor) / 2.0
	_gross.draw_texture_rect(_bild, Rect2(oben_links, bildgroesse * faktor), false)
	if fokus != null:
		_pfeil(_gross, oben_links + bild_punkt(fokus.global_position) * faktor, 12.0)
	if auftraege != null and auftraege.unterwegs:
		_gross.draw_circle(oben_links + bild_punkt(auftraege.ziel_ort()) * faktor, 8.0, ZIEL)
	_gross.draw_string(ThemeDB.fallback_font, Vector2(24, 36),
			"Landsberg am Lech – M oder Esc schließen", HORIZONTAL_ALIGNMENT_LEFT, -1, 22)
