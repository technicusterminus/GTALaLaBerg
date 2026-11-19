class_name Kino
extends CanvasLayer
## Die Filmdarstellung des Drehbuchs: Kamerafahrten mit Letterbox, Untertitel mit
## Sprecherfarbe, Figuren am Schauplatz, Titelkarten, Entscheidungen und
## Funksprueche. Im Spiel laufen Szenen von selbst ab; Enter, Leertaste oder ein
## Mausklick springen zur naechsten Zeile. Im Rauchtest (test_schnell) entfaellt
## jedes Warten.

const BALKEN_ANTEIL := 0.12
const ZEILE_MIN := 2.2
const ZEILE_MAX := 8.0

var test_schnell := false
var test_wahl := 0          ## welche Option im Rauchtest gewaehlt wird

var _kam := Camera3D.new()
var _oben := ColorRect.new()
var _unten := ColorRect.new()
var _untertitel := RichTextLabel.new()
var _karte := ColorRect.new()
var _karte_text := Label.new()
var _karte_unter := Label.new()
var _wahl_text := Label.new()
var _funk := RichTextLabel.new()
var _funk_bis := 0.0

var _weiter := false
var _wahl_nr := -1
var _wahl_anzahl := 0
var _aktiv := false
var _ziel := Vector3.ZERO
var _cam := {}
var _cam_t := 0.0
var _figuren: Array[Node3D] = []


func _ready() -> void:
	layer = 20
	process_mode = Node.PROCESS_MODE_ALWAYS
	_kam.far = 3000.0
	_kam.near = 0.3
	_kam.fov = 55.0
	add_child(_kam)

	for balken: ColorRect in [_oben, _unten]:
		balken.color = Color.BLACK
		balken.mouse_filter = Control.MOUSE_FILTER_IGNORE
		add_child(balken)
	_oben.anchor_right = 1.0
	_unten.anchor_top = 1.0
	_unten.anchor_bottom = 1.0
	_unten.anchor_right = 1.0
	_balken_hoehe(0.0)

	_untertitel.bbcode_enabled = true
	_untertitel.fit_content = true
	_untertitel.scroll_active = false
	_untertitel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_untertitel.add_theme_font_size_override("normal_font_size", 22)
	_untertitel.add_theme_font_size_override("bold_font_size", 22)
	_untertitel.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.9))
	_untertitel.add_theme_constant_override("outline_size", 6)
	_untertitel.anchor_left = 0.12
	_untertitel.anchor_right = 0.88
	_untertitel.anchor_top = 1.0
	_untertitel.anchor_bottom = 1.0
	_untertitel.offset_top = -135
	_untertitel.offset_bottom = -20
	_untertitel.visible = false
	add_child(_untertitel)

	_karte.color = Color(0, 0, 0, 0)
	_karte.anchor_right = 1.0
	_karte.anchor_bottom = 1.0
	_karte.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_karte)
	for l: Label in [_karte_text, _karte_unter, _wahl_text]:
		l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		l.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.9))
		l.add_theme_constant_override("outline_size", 6)
		l.anchor_left = 0.1
		l.anchor_right = 0.9
		l.visible = false
		add_child(l)
	_karte_text.add_theme_font_size_override("font_size", 64)
	_karte_text.anchor_top = 0.38
	_karte_text.anchor_bottom = 0.52
	_karte_unter.add_theme_font_size_override("font_size", 24)
	_karte_unter.anchor_top = 0.54
	_karte_unter.anchor_bottom = 0.7
	_wahl_text.add_theme_font_size_override("font_size", 26)
	_wahl_text.anchor_top = 0.3
	_wahl_text.anchor_bottom = 0.7

	_funk.bbcode_enabled = true
	_funk.fit_content = true
	_funk.scroll_active = false
	_funk.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_funk.add_theme_font_size_override("normal_font_size", 18)
	_funk.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.9))
	_funk.add_theme_constant_override("outline_size", 5)
	_funk.anchor_left = 0.2
	_funk.anchor_right = 0.8
	_funk.offset_top = 120
	_funk.visible = false
	add_child(_funk)


func laeuft() -> bool:
	return _aktiv


func _balken_hoehe(anteil: float) -> void:
	var h := get_viewport().get_visible_rect().size.y * anteil
	_oben.offset_bottom = h
	_unten.offset_top = -h


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		var k := (event as InputEventKey).keycode
		if _wahl_anzahl > 0 and k >= KEY_1 and k < KEY_1 + _wahl_anzahl:
			_wahl_nr = k - KEY_1
			get_viewport().set_input_as_handled()
		elif k == KEY_ENTER or k == KEY_KP_ENTER or k == KEY_SPACE:
			_weiter = true
	elif event is InputEventMouseButton and event.pressed:
		_weiter = true


func _warte(sekunden: float) -> void:
	if test_schnell:
		return
	_weiter = false
	var t := 0.0
	while t < sekunden and not _weiter:
		await get_tree().process_frame
		t += get_process_delta_time()
	_weiter = false


# --- Szenen -----------------------------------------------------------------

## Spielt die Schnitte einer Szene. db liefert Orte, Figuren und die Stadt.
func spiele_szene(schnitte: Array, db: Drehbuch) -> void:
	_beginne(db)
	for sch: Dictionary in schnitte:
		await _spiele_schnitt(sch, db)
	await _beende(db)


func _beginne(db: Drehbuch) -> void:
	_aktiv = true
	db.kino_modus(true)
	_kam.current = true
	_untertitel.visible = false
	if test_schnell:
		_balken_hoehe(BALKEN_ANTEIL)
		return
	var tween := create_tween()
	tween.tween_method(_balken_hoehe, 0.0, BALKEN_ANTEIL, 0.6)
	await tween.finished


func _beende(db: Drehbuch) -> void:
	_untertitel.visible = false
	_raeume_figuren()
	if not test_schnell:
		var tween := create_tween()
		tween.tween_method(_balken_hoehe, BALKEN_ANTEIL, 0.0, 0.5)
		await tween.finished
	_balken_hoehe(0.0)
	_cam = {}
	_aktiv = false
	db.kino_modus(false)


func _spiele_schnitt(sch: Dictionary, db: Drehbuch) -> void:
	var ziel: Dictionary = await db.kamera_ziel(sch.get("ort", ""))
	_ziel = ziel["pos"]
	var zeilen: Array = sch["zeilen"]
	_baue_figuren(sch, zeilen, ziel, db)
	_cam = {"art": sch.get("art", "statisch"), "hoehe": float(sch.get("hoehe", 2.0)),
			"abstand": float(sch.get("abstand", 6.0)), "winkel": deg_to_rad(float(sch.get("winkel", 30.0)))}
	_cam_t = 0.0
	var gesamt := 0.0
	for z: Dictionary in zeilen:
		gesamt += _zeilen_dauer(z["text"])
	_cam["dauer"] = maxf(float(sch.get("dauer", 0.0)), gesamt)
	_kamera_setzen(0.0)
	var verstrichen := 0.0
	for z: Dictionary in zeilen:
		var f: Dictionary = db.figur(z["wer"])
		var name: String = f.get("name", z["wer"])
		var farbe: String = f.get("farbe", "#ffffff")
		_untertitel.text = "[color=%s][b]%s[/b][/color]   %s" % [farbe, name, z["text"]]
		_untertitel.visible = true
		var dauer := _zeilen_dauer(z["text"])
		await _warte(dauer)
		verstrichen += dauer
	_untertitel.visible = false
	var rest: float = _cam["dauer"] - verstrichen
	if rest > 0.05:
		await _warte(rest)
	_raeume_figuren()


func _zeilen_dauer(text: String) -> float:
	return clampf(text.length() * 0.055 + 1.2, ZEILE_MIN, ZEILE_MAX)


func _process(delta: float) -> void:
	if _funk.visible and Time.get_ticks_msec() / 1000.0 > _funk_bis:
		_funk.visible = false
	if not _aktiv or _cam.is_empty():
		return
	_cam_t += delta
	_kamera_setzen(_cam_t)


## Setzt die Kamera zum Zeitpunkt t des Schnitts. statisch: fester Standpunkt;
## umkreise: langsam um das Ziel; fahrt: dem Ziel entgegen; kran: von oben herab.
func _kamera_setzen(t: float) -> void:
	var art: String = _cam["art"]
	var winkel: float = _cam["winkel"]
	var abstand: float = _cam["abstand"]
	var hoehe: float = _cam["hoehe"]
	var fortschritt := clampf(t / maxf(_cam["dauer"], 0.1), 0.0, 1.0)
	match art:
		"umkreise":
			winkel += t * 0.2
		"fahrt":
			abstand *= 1.0 - 0.35 * fortschritt
			winkel += t * 0.05
		"kran":
			hoehe = lerpf(hoehe, hoehe * 0.35, fortschritt)
	var pos := _ziel + Vector3(sin(winkel) * abstand, hoehe, cos(winkel) * abstand)
	_kam.global_position = pos
	var blick := _ziel + Vector3(0, minf(1.4 + _cam["hoehe"] * 0.05, 6.0), 0)
	if pos.distance_to(blick) > 0.1:
		_kam.look_at(blick)


# --- Figuren ----------------------------------------------------------------

## Am Schauplatz stehen die Sprecher im Kreis um den Zielpunkt. Die Figur des
## Spielers steht schon dort, wenn die Szene bei ihr spielt; sonst bekommt Lale eine
## eigene Gestalt.
func _baue_figuren(sch: Dictionary, zeilen: Array, ziel: Dictionary, db: Drehbuch) -> void:
	_raeume_figuren()
	var ids: Array[String] = []
	for f: String in sch.get("figuren", []):
		ids.append(f)
	for z: Dictionary in zeilen:
		var wer: String = z["wer"]
		if db.figur(wer).get("gestalt", "") != "" and wer not in ids:
			ids.append(wer)
	if ziel.get("ist_spieler", false) and db.spieler_sichtbar():
		ids.erase("lale")
	var n := ids.size()
	if n == 0:
		return
	var radius := 2.0 if n <= 2 else 2.0 + 0.45 * n
	for i in n:
		var f := db.figur(ids[i])
		if f.get("gestalt", "") == "":
			continue
		var winkel := TAU * i / n + deg_to_rad(float(sch.get("winkel", 30.0)) + 90.0)
		var pos: Vector3 = _ziel + Vector3(cos(winkel), 0, sin(winkel)) * radius
		pos.y = db.boden_y(pos, _ziel.y)
		var gestalt := _gestalt(f)
		get_tree().current_scene.add_child(gestalt)
		gestalt.global_position = pos
		gestalt.look_at(Vector3(_ziel.x, pos.y, _ziel.z))
		_figuren.append(gestalt)


func _gestalt(f: Dictionary) -> Node3D:
	var wurzel := Node3D.new()
	var figur: Figur = Figur.neu(hash(String(f.get("name", "")))) if Figur.verfuegbar() else null
	if figur != null:
		wurzel.add_child(figur)
		figur.passe_an()
		figur.spiele("Idle_Neutral")
		_name_schild(wurzel, f)
		return wurzel
	var mesh := MeshInstance3D.new()
	var kapsel := CapsuleMesh.new()
	kapsel.height = 1.75
	kapsel.radius = 0.3
	mesh.mesh = kapsel
	mesh.position.y = 0.875
	var m := StandardMaterial3D.new()
	m.albedo_color = Color.html(f["gestalt"])
	mesh.material_override = m
	wurzel.add_child(mesh)
	# Eine Nase zeigt die Blickrichtung (-Z), wie bei der Spielfigur.
	var nase := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = Vector3(0.16, 0.16, 0.26)
	nase.mesh = box
	nase.position = Vector3(0, 1.5, -0.3)
	nase.material_override = m
	wurzel.add_child(nase)
	_name_schild(wurzel, f)
	return wurzel


func _name_schild(wurzel: Node3D, f: Dictionary) -> void:
	var name := Label3D.new()
	name.text = f.get("name", "")
	name.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	name.font_size = 40
	name.pixel_size = 0.0048
	name.outline_size = 12
	name.position.y = 2.15
	name.no_depth_test = true
	wurzel.add_child(name)


func _raeume_figuren() -> void:
	for f in _figuren:
		if is_instance_valid(f):
			f.queue_free()
	_figuren.clear()


# --- Titel, Wahl, Funk --------------------------------------------------------

func zeige_titel(text: String, unter: String, dauer: float) -> void:
	if test_schnell:
		return
	_karte_text.text = text
	_karte_unter.text = unter
	_karte_text.visible = true
	_karte_unter.visible = true
	var tween := create_tween()
	tween.tween_property(_karte, "color", Color(0, 0, 0, 1), 0.6)
	await tween.finished
	await _warte(dauer)
	var aus := create_tween()
	aus.tween_property(_karte, "color", Color(0, 0, 0, 0), 0.7)
	await aus.finished
	_karte_text.visible = false
	_karte_unter.visible = false


## Stellt eine Frage mit nummerierten Antworten und liefert den gewaehlten Index.
func frage(text: String, optionen: Array) -> int:
	if test_schnell:
		return clampi(test_wahl, 0, optionen.size() - 1)
	var zeilen: Array[String] = [text, ""]
	for i in optionen.size():
		zeilen.append("%d   %s" % [i + 1, optionen[i]["text"]])
	_wahl_text.text = "\n".join(zeilen)
	_wahl_text.visible = true
	_wahl_nr = -1
	_wahl_anzahl = optionen.size()
	while _wahl_nr < 0:
		await get_tree().process_frame
	_wahl_text.visible = false
	_wahl_anzahl = 0
	return _wahl_nr


## Ein Funkspruch oben im Bild, blendet nach sechs Sekunden aus; kein Warten.
func funk(name: String, farbe: String, text: String) -> void:
	_funk.text = "[color=%s][b]📻 %s[/b][/color]   %s" % [farbe, name, text]
	_funk.visible = true
	_funk_bis = Time.get_ticks_msec() / 1000.0 + clampf(text.length() * 0.06 + 2.5, 4.0, 9.0)
