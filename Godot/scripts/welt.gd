extends Node3D
## Wurzel des Spiels: baut Licht, Stadt, Spieler, Auto, Kamera und HUD zusammen
## und kuemmert sich um Ein- und Aussteigen.
##
## Film ansehen (Bilder):   ... -- --filmtest --bild=/pfad/film.png
## Rauchtest ohne Fenster:  godot --path Godot --headless -- --rauchtest
## Mit Screenshot:          ... -- --rauchtest --bild=/pfad/bild.png
## Ohne Schatten:           ... -- --rauchtest --ohneschatten

const EINSTEIGEN_M := 4.0
const STEHLEN_M := 8.0
const SONDER_M := 9.0

var stadt := Stadt.new()
var spieler := Spieler.new()
var kamera := Kamera.new()
var hud := Hud.new()
var karte := Karte.new()
var verkehr := Verkehr.new()
var polizei := Polizei.new()
var auftraege := Auftraege.new()
var laeden := Laeden.new()
var farbdosen := Farbdosen.new()
var radio := Radio.new()
var schiessbude := Schiessbude.new()
var moebel := Strassenmoebel.new()
var musik := Musik.new()
var revier := Revier.new()
var drehbuch := Drehbuch.new()
var auto: Auto
var aktuelles_auto: Auto
var aktuelles_sonder: Sonderfahrzeug
var panzer: Sonderfahrzeug
var hubschrauber: Sonderfahrzeug
var farbtreffer := 0

var _rauchtest := false
var _filmtest := false
var _film_bilder := 0
var _spielbild := false
var _boden_y := 0.0
var _bildpfad := ""
var _rauch_zeit := 0.0


func _ready() -> void:
	add_to_group("welt")
	_eingaben()
	_umgebung()
	_maler()
	_rauchtest = "--rauchtest" in OS.get_cmdline_user_args()
	_filmtest = "--filmtest" in OS.get_cmdline_user_args()
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--bild="):
			_bildpfad = arg.substr(7)

	add_child(stadt)
	add_child(kamera)
	add_child(spieler)
	add_child(hud)
	add_child(karte)
	add_child(radio)
	karte.kamera = kamera
	karte.auftraege = auftraege
	add_child(verkehr)
	add_child(polizei)
	# Aufträge und Reviere brauchen das Strassennetz der Polizei (Hoehe der Ziele).
	auftraege.polizei = polizei
	revier.polizei = polizei
	add_child(auftraege)
	add_child(laeden)
	farbdosen.polizei = polizei
	add_child(farbdosen)
	add_child(schiessbude)
	moebel.stadt = stadt
	moebel.polizei = polizei
	add_child(moebel)
	musik.welt = self
	musik.karte = karte
	musik.polizei = polizei
	musik.radio = radio
	add_child(musik)
	add_child(revier)
	drehbuch.spieler = spieler
	drehbuch.polizei = polizei
	drehbuch.laeden = laeden
	drehbuch.revier = revier
	drehbuch.stadt = stadt
	drehbuch.welt = self
	drehbuch.automatisch = not _rauchtest
	add_child(drehbuch)
	spieler.kamera = kamera
	spieler.aktiv(false)  # bis unter dem Spawn der Boden steht
	kamera.ziel = spieler
	kamera.ausnehmen(spieler)
	_fokus_auf(spieler)

	var s: Dictionary = stadt.spawn
	if s.is_empty():
		return  # Daten fehlen, Fehlermeldung kam schon aus stadt.gd
	var start := Vector3(s["x"], s["y"], s["z"])
	spieler.global_position = start + Vector3(0, 3, 0)
	kamera.yaw = deg_to_rad(float(s["blick"]))
	stadt.sektor_bereit.connect(_sektor_bereit.bind(start), CONNECT_ONE_SHOT)
	if _rauchtest:
		call("_rauchtest_ablauf")   # in welt_rauchtest.gd
	else:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


## Wer gerade gesteuert wird (Figur oder Wagen): darauf schauen Stadt, Polizei,
## Verkehr, Auftraege, Laeden, Reviere und HUD.
func _fokus_auf(wer: Node3D) -> void:
	stadt.fokus = wer
	polizei.fokus = wer
	Verkehrsauto.fokus = wer
	auftraege.fokus = wer
	laeden.fokus = wer
	farbdosen.fokus = wer
	schiessbude.fokus = wer
	revier.fokus = wer
	drehbuch.fokus = wer
	hud.ziel = wer
	karte.fokus = wer


## Vom Drehbuch bei Szenen: Film an, die Spielfigur und der Wagen stehen still, das
## HUD ist weg und die Polizei ruht. Nach dem Film kommt die Spielkamera zurueck.
func kino_modus(an: bool) -> void:
	spieler.gesperrt = an
	if auto != null:
		auto.gesperrt = an
	for a: Auto in get_tree().get_nodes_in_group("auto"):
		a.gesperrt = an
	for s: Sonderfahrzeug in get_tree().get_nodes_in_group("sonderfahrzeug"):
		s.gesperrt = an
	hud.visible = not an
	polizei.pausiert = an
	if not an:
		kamera.aktivieren()


func zaehle_farbtreffer() -> void:
	farbtreffer += 1


## Von der Polizei bei einer Festnahme: aus dem Wagen aussteigen.
func aussteigen_erzwingen() -> void:
	if aktuelles_auto != null or aktuelles_sonder != null:
		_aussteigen()


## Von der Polizei bei einer Festnahme: die Figur an einen Ort setzen.
func versetze_spieler(ort: Vector3) -> void:
	spieler.setze_auf_boden(Vector3(ort.x, 0.0, ort.z))


func _sektor_bereit(id: String, start: Vector3) -> void:
	# Der erste fertige Sektor kann ein anderer sein als der unter dem Spawn.
	if stadt.eintrag_fuer(start).get("id", "") != id:
		stadt.sektor_bereit.connect(_sektor_bereit.bind(start), CONNECT_ONE_SHOT)
		return
	# Die Kollisionsform steht erst nach einem Physikschritt im Raum.
	await get_tree().physics_frame
	await get_tree().physics_frame
	var boden := _boden(start)
	_boden_y = boden.y
	spieler.global_position = boden + Vector3(0, 0.1, 0)
	spieler.aktiv(true)
	auto = Auto.new()
	add_child(auto)
	var voraus := kamera.vorwaerts() * 7.0
	auto.global_position = _boden(boden + voraus) + Vector3(0, 0.9, 0)
	auto.rotation.y = kamera.yaw
	_stelle_marken(start)
	print("LALABERG_START boden=%s auto=%s" % [boden, auto.global_position])


## Die erste blaue Saeule zwischen Startpunkt und Wagen, die Lackiererei nah am
## Start und der Paintball-Laden am Weg in die Stadt (beim Bahnhof), alle auf einer
## Fahrbahn.
func _stelle_marken(start: Vector3) -> void:
	var mitte := (start + auto.global_position) * 0.5
	auftraege.setze_start_ort(polizei.naechster_punkt(mitte))
	laeden.stelle(Laeden.Ladenart.LACKIEREREI, polizei.naechster_punkt(Vector3(-1500.0, 0, 120.0)))
	laeden.stelle(Laeden.Ladenart.PAINTBALL, polizei.naechster_punkt(Vector3(-376.3, 0, 260.0)))
	# Die Schiessbude steht an der Strasse hinter dem Paintball-Laden, die Scheiben stehen
	# quer ueber der Strasse.
	schiessbude.stelle(polizei.naechster_punkt(Vector3(-300.0, 0, 330.0)), Vector3.RIGHT)
	drehbuch.bereit = true
	_stelle_sonderfahrzeuge()


## Nichts weist auf sie hin: keine Saeule, kein Eintrag auf der Karte. Der Panzer
## steht im Waldstueck westlich der Stadt, der Hubschrauber auf dem Dach des Klinikums,
## dort, wo ein Rettungshubschrauber auch landen wuerde. Beide liegen im Sektor des
## Spawns, die Kollision steht also schon.
func _stelle_sonderfahrzeuge() -> void:
	panzer = _setze_sonder(Sonderfahrzeug.Art.PANZER, Vector2(-1870.7, 407.6), -deg_to_rad(35.0))
	# Das Klinikum hat in der Mitte einen Innenhof; das Flachdach beginnt 14 m weiter
	# noerdlich (y = 43,6), dort steht der Hubschrauber.
	hubschrauber = _setze_sonder(Sonderfahrzeug.Art.HUBSCHRAUBER, Vector2(-1590.0, 156.0), PI)


func _setze_sonder(art: Sonderfahrzeug.Art, xz: Vector2, gier: float) -> Sonderfahrzeug:
	var boden := _boden(Vector3(xz.x, 0.0, xz.y))
	if absf(boden.y) < 0.001:
		push_warning("LALABERG_SONDER kein Boden bei %s" % xz)
		return null
	var s := Sonderfahrzeug.new()
	s.art = art
	add_child(s)
	s.setze_lage(boden + Vector3(0, 0.05, 0), gier)
	return s


## Hoehe des Bodens unter einer Stelle; faellt auf die Stelle selbst zurueck.
func _boden(pos: Vector3) -> Vector3:
	var von := pos + Vector3(0, 300, 0)
	var abfrage := PhysicsRayQueryParameters3D.create(von, pos + Vector3(0, -300, 0))
	abfrage.collision_mask = 1
	var treffer := get_world_3d().direct_space_state.intersect_ray(abfrage)
	return treffer["position"] if not treffer.is_empty() else pos


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("einsteigen") and auto != null:
		if aktuelles_auto == null and aktuelles_sonder == null:
			_einsteigen()
		elif aktuelles_sonder != null:
			var hinweis := aktuelles_sonder.aussteigen_hinweis()
			if hinweis.is_empty():
				_aussteigen()
			elif hinweis != "…":
				Schaden.rueckmeldung(get_tree(), hinweis)
		else:
			_aussteigen()
	elif event is InputEventMouseButton and event.pressed and not karte.menue_offen \
			and not karte.voll:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func _einsteigen() -> void:
	# Panzer und Hubschrauber zuerst: wer vor dem Panzer steht, will in den Panzer, auch
	# wenn daneben ein Auto parkt.
	var sonder: Sonderfahrzeug = null
	var bestes_sonder := SONDER_M
	for s: Sonderfahrzeug in get_tree().get_nodes_in_group("sonderfahrzeug"):
		if s.hat_fahrer():
			continue
		var abstand := s.position_jetzt().distance_to(spieler.global_position)
		if abstand < bestes_sonder:
			bestes_sonder = abstand
			sonder = s
	if sonder != null:
		if sonder.gesperrt_fuer_spieler():
			Schaden.rueckmeldung(get_tree(), sonder.sperrtext())
		else:
			_steige_ein_sonder(sonder)
		return
	# Dann ein freier eigener Wagen in der Naehe ...
	var nah: Auto = null
	var bester := EINSTEIGEN_M
	for a: Auto in get_tree().get_nodes_in_group("auto"):
		if a.fahrer != null:
			continue
		var d := a.global_position.distance_to(spieler.global_position)
		if d < bester:
			bester = d
			nah = a
	# ... sonst ein fahrendes oder parkendes KI-Auto: wer es uebernimmt, stiehlt es.
	if nah == null:
		var ki: Verkehrsauto = null
		var beste := STEHLEN_M
		for v: Verkehrsauto in get_tree().get_nodes_in_group("verkehrsauto"):
			if v.ist_ausgeschaltet():
				continue
			var d := v.global_position.distance_to(spieler.global_position)
			if d < beste:
				beste = d
				ki = v
		if ki == null:
			Schaden.rueckmeldung(get_tree(), "Kein freies Fahrzeug in Reichweite – näher herangehen")
			return
		nah = _uebernimm(ki)
	_steige_ein(nah)


## Ein KI-Auto wird zum fahrbaren Wagen; das KI-Auto verschwindet. Das ist
## Diebstahl, und ein Streifenwagen wiegt doppelt.
func _uebernimm(ki: Verkehrsauto) -> Auto:
	var neu := Auto.new()
	neu.lack = ki.lack
	neu.uebernommen = true
	add_child(neu)
	neu.global_position = ki.position_jetzt() + Vector3(0, 0.4, 0)
	# Das Netz der KI schaut nach +X, der fahrbare Wagen nach -Z.
	neu.rotation.y = ki.gier() - PI / 2.0
	ki.remove_from_group("verkehrsauto")
	ki.remove_from_group("verletzbar")
	Schaden.melde(get_tree(), Schaden.Tat.AUTO_GESTOHLEN)
	if ki.polizei:
		Schaden.melde(get_tree(), Schaden.Tat.POLIZEI_BESCHOSSEN)
	ki.queue_free()
	return neu


func _steige_ein(wagen: Auto) -> void:
	aktuelles_auto = wagen
	wagen.fahrer = spieler
	radio.fahrt(true)
	spieler.aktiv(false)
	kamera.ziel = wagen
	kamera.hoehe = 1.2
	kamera.abstand_setzen(7.0)
	kamera.ausnehmen(wagen)
	_fokus_auf(wagen)
	Schaden.rueckmeldung(get_tree(), "Eingestiegen – W/S fahren, Leertaste bremsen, F aussteigen")


func _steige_ein_sonder(s: Sonderfahrzeug) -> void:
	aktuelles_sonder = s
	s.setze_fahrer(spieler)
	s.kamera = kamera
	spieler.aktiv(false)
	kamera.ziel = s
	kamera.hoehe = 4.2 if s.art == Sonderfahrzeug.Art.PANZER else 2.2
	kamera.abstand_setzen(16.0 if s.art == Sonderfahrzeug.Art.PANZER else 11.0)
	kamera.ausnehmen(s)
	_fokus_auf(s)
	Schaden.rueckmeldung(get_tree(), s.steuerungstext())
	print("LALABERG_SONDER eingestiegen art=%d" % s.art)


func _aussteigen() -> void:
	radio.fahrt(false)
	if aktuelles_sonder != null:
		var s := aktuelles_sonder
		aktuelles_sonder = null
		s.fahrer = null
		var seite := s.rechts() * 4.5
		spieler.global_position = s.position_jetzt() + seite + Vector3(0, 0.8, 0)
		spieler.aktiv(true)
		kamera.ziel = spieler
		kamera.hoehe = 1.5
		kamera.abstand_setzen(4.5)
		kamera.ausnehmen(spieler)
		kamera.folge_yaw = NAN
		_fokus_auf(spieler)
		hud.tempo_ms = 0.0
		return
	var a := aktuelles_auto
	aktuelles_auto = null
	a.fahrer = null
	spieler.global_position = a.global_position - a.global_basis.x * 2.2 + Vector3(0, 0.6, 0)
	spieler.aktiv(true)
	kamera.ziel = spieler
	kamera.hoehe = 1.5
	kamera.abstand_setzen(4.5)
	kamera.ausnehmen(spieler)
	kamera.folge_yaw = NAN
	_fokus_auf(spieler)
	hud.tempo_ms = 0.0


func _process(delta: float) -> void:
	if aktuelles_sonder != null:
		hud.tempo_ms = absf(aktuelles_sonder.tempo)
		hud.hinweis = "F  aussteigen"
	elif aktuelles_auto != null:
		var vor := -aktuelles_auto.global_basis.z
		kamera.folge_yaw = atan2(-vor.x, -vor.z) if aktuelles_auto.tempo_ms > 2.0 else NAN
		hud.tempo_ms = aktuelles_auto.tempo_ms
		hud.hinweis = "F  aussteigen"
	elif auto != null and spieler.global_position.distance_to(auto.global_position) < EINSTEIGEN_M:
		hud.hinweis = "F  einsteigen"
	else:
		hud.hinweis = ""
	hud.sterne = polizei.hole_sterne()
	hud.gesehen = polizei.wird_gesehen()
	hud.such_anteil = polizei.such_anteil()
	hud.festnahme_anteil = polizei.festnahme_anteil()
	hud.drehbuch_text = drehbuch.hud_text(kamera.vorwaerts())
	hud.auftrag_text = _auftrag_text()
	hud.revier_text = revier.hud_text()
	hud.laden_text = laeden.menue_text()
	hud.bude_text = schiessbude.hud_text()
	hud.leben = ceili(spieler.leben)
	hud.geld = Konto.geld()
	hud.waffe = spieler.waffe.art_name()
	hud.ruf = Konto.stufe(Konto.Wert.RUF)
	if _filmtest and not _bildpfad.is_empty():
		_rauch_zeit += delta
		# Zwei Bilder aus dem Film: der Prolog beginnt von selbst.
		if _film_bilder < 4 and _rauch_zeit > 12.0 + 9.0 * _film_bilder:
			var film := get_viewport().get_texture().get_image()
			var pfad := _bildpfad.replace(".png", "_%d.png" % _film_bilder)
			film.save_png(pfad)
			print("LALABERG_BILD %s mission=%s" % [pfad, drehbuch.laeuft()])
			_film_bilder += 1
			if _film_bilder >= 4:
				get_tree().quit(0)
	if _rauchtest:
		_rauch_zeit += delta
		if not _bildpfad.is_empty() and _rauch_zeit > 15.0 and not _spielbild:
			_spielbild = true
			_bild("spiel")


## Auftragszeile mit Richtungspfeil zur Kamera, Entfernung und Restzeit.
func _auftrag_text() -> String:
	var gesteuert: Node3D = aktuelles_auto if aktuelles_auto != null else spieler
	var fokus_pos: Vector3 = gesteuert.global_position
	if not auftraege.unterwegs and not auftraege.angebot:
		return ""
	var ziel := auftraege.hole_wegpunkt()
	var d := Vector2(ziel.x - fokus_pos.x, ziel.z - fokus_pos.z)
	var pfeile := ["↑", "↖", "←", "↙", "↓", "↘", "→", "↗"]
	var ang := kamera.vorwaerts().signed_angle_to(Vector3(d.x, 0, d.y), Vector3.UP)
	var pfeil: String = pfeile[posmod(roundi(ang / (PI / 4.0)), 8)]
	if not auftraege.unterwegs:
		return "Blaue Säule: Auftrag annehmen   %s %d m" % [pfeil, roundi(d.length())]
	var rest := auftraege.restzeit()
	var art_text := auftraege.art_name()
	if auftraege.art == Auftraege.Art.RENNEN:
		art_text += " %d/%d" % [auftraege.rennen_punkt + 1, auftraege.rennen_punkte()]
	elif auftraege.art == Auftraege.Art.STREIFE:
		art_text += " (noch %d)" % auftraege.streife_rest
	return "%s → %s   %s %d m   %d:%02d   %d €" % [
			art_text, auftraege.hole_zielname(), pfeil, roundi(d.length()),
			floori(rest / 60.0), floori(rest) % 60, auftraege.lohn]


# --- Rauchtest ----------------------------------------------------------------

## Speichert ein Bild des Spielfensters neben --bild=..., mit dem Namen als Endung.
func _bild(name: String) -> void:
	if _bildpfad.is_empty() or DisplayServer.get_name() == "headless":
		return
	var pfad := _bildpfad.replace(".png", "_%s.png" % name)
	get_viewport().get_texture().get_image().save_png(pfad)
	print("LALABERG_BILD %s" % pfad)
	print("LALABERG_LAST %s aufrufe=%d dreiecke=%d objekte=%d fps=%d" % [name,
			Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),
			Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME),
			Performance.get_monitor(Performance.RENDER_TOTAL_OBJECTS_IN_FRAME),
			Engine.get_frames_per_second()])


## Malerei-Filter (maler.gdshader) zwischen 3D-Bild und Oberflaeche; --ohnemaler schaltet ab.
func _maler() -> void:
	if "--ohnemaler" in OS.get_cmdline_user_args():
		return
	var ebene := CanvasLayer.new()
	ebene.layer = 0
	var flaeche := ColorRect.new()
	flaeche.set_anchors_preset(Control.PRESET_FULL_RECT)
	flaeche.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var m := ShaderMaterial.new()
	m.shader = preload("res://shader/maler.gdshader")
	flaeche.material = m
	ebene.add_child(flaeche)
	add_child(ebene)


func _umgebung() -> void:
	var himmel := ProceduralSkyMaterial.new()
	himmel.sky_top_color = Color(0.30, 0.50, 0.82)
	himmel.sky_curve = 0.25
	himmel.sky_horizon_color = Color(0.98, 0.84, 0.70)
	himmel.ground_horizon_color = Color(0.98, 0.84, 0.70)
	himmel.ground_bottom_color = Color(0.45, 0.52, 0.45)
	himmel.ground_curve = 0.05
	var sky := Sky.new()
	sky.sky_material = himmel
	var env := Environment.new()
	env.background_mode = Environment.BG_SKY
	env.sky = sky
	# Feste, warme Umgebungsfarbe statt Himmel: sonst sind die Schatten knallblau.
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.80, 0.74, 0.76)
	env.ambient_light_energy = 0.8
	env.tonemap_mode = Environment.TONE_MAPPER_AGX
	env.tonemap_exposure = 1.05
	env.tonemap_white = 6.0
	env.fog_enabled = true
	env.fog_light_color = Color(0.95, 0.80, 0.66)
	env.fog_density = 0.00035
	env.fog_aerial_perspective = 0.35
	env.fog_sky_affect = 0.08
	env.fog_sun_scatter = 0.4
	# Waermer, satter und mit leichtem Leuchten wie im Konzeptbild.
	env.adjustment_enabled = true
	env.adjustment_contrast = 1.0
	env.adjustment_saturation = 1.0
	env.glow_enabled = true
	env.glow_intensity = 0.3
	env.glow_bloom = 0.04
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)

	var sonne := DirectionalLight3D.new()
	# Tiefe, warme Abendsonne wie im Zielbild: lange, weiche Schatten.
	sonne.rotation_degrees = Vector3(-20, 150, 0)
	sonne.light_color = Color(1.0, 0.80, 0.58)
	sonne.shadow_enabled = not "--ohneschatten" in OS.get_cmdline_user_args()
	# Schatten nur in der Naehe: dort sieht man sie, weiter weg kosten sie nur.
	sonne.directional_shadow_max_distance = 180.0
	sonne.directional_shadow_mode = DirectionalLight3D.SHADOW_PARALLEL_2_SPLITS
	sonne.light_energy = 1.35
	# Gegen Schattenakne auf den senkrechten Waenden (streifige Flaechen im Streiflicht).
	sonne.shadow_bias = 0.12
	sonne.shadow_normal_bias = 3.2
	sonne.shadow_blur = 2.5
	add_child(sonne)


func _eingaben() -> void:
	var tasten := {
		"vorwaerts": [KEY_W, KEY_UP], "rueckwaerts": [KEY_S, KEY_DOWN],
		"links": [KEY_A, KEY_LEFT], "rechts": [KEY_D, KEY_RIGHT],
		"rennen": [KEY_SHIFT], "springen": [KEY_SPACE], "handbremse": [KEY_SPACE],
		"einsteigen": [KEY_F], "menue": [KEY_ESCAPE], "karte": [KEY_M], "radio": [KEY_R],
		"waffe1": [KEY_1], "waffe2": [KEY_2], "waffe3": [KEY_3], "waffe4": [KEY_4],
	}
	for aktion: String in tasten:
		if not InputMap.has_action(aktion):
			InputMap.add_action(aktion)
		for taste: int in tasten[aktion]:
			var e := InputEventKey.new()
			e.physical_keycode = taste as Key
			InputMap.action_add_event(aktion, e)
	InputMap.add_action("feuer")
	var maus := InputEventMouseButton.new()
	maus.button_index = MOUSE_BUTTON_LEFT
	InputMap.action_add_event("feuer", maus)
