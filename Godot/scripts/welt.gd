extends Node3D
## Wurzel des Spiels: baut Licht, Stadt, Spieler, Auto, Kamera und HUD zusammen
## und kuemmert sich um Ein- und Aussteigen.
##
## Rauchtest ohne Fenster:  godot --path Godot --headless -- --rauchtest
## Mit Screenshot:          ... -- --rauchtest --bild=/pfad/bild.png
## Ohne Schatten:           ... -- --rauchtest --ohneschatten

const EINSTEIGEN_M := 4.0
const STEHLEN_M := 8.0

var stadt := Stadt.new()
var spieler := Spieler.new()
var kamera := Kamera.new()
var hud := Hud.new()
var verkehr := Verkehr.new()
var polizei := Polizei.new()
var auto: Auto
var aktuelles_auto: Auto
var farbtreffer := 0

var _rauchtest := false
var _boden_y := 0.0
var _bildpfad := ""
var _rauch_zeit := 0.0


func _ready() -> void:
	add_to_group("welt")
	_eingaben()
	_umgebung()
	_rauchtest = "--rauchtest" in OS.get_cmdline_user_args()
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--bild="):
			_bildpfad = arg.substr(7)

	add_child(stadt)
	add_child(kamera)
	add_child(spieler)
	add_child(hud)
	add_child(verkehr)
	add_child(polizei)
	polizei.fokus = spieler
	spieler.kamera = kamera
	spieler.aktiv(false)  # bis unter dem Spawn der Boden steht
	kamera.ziel = spieler
	kamera.ausnehmen(spieler)
	stadt.fokus = spieler
	hud.ziel = spieler

	var s: Dictionary = stadt.spawn
	if s.is_empty():
		return  # Daten fehlen, Fehlermeldung kam schon aus stadt.gd
	var start := Vector3(s["x"], s["y"], s["z"])
	spieler.global_position = start + Vector3(0, 3, 0)
	kamera.yaw = deg_to_rad(float(s["blick"]))
	stadt.sektor_bereit.connect(_sektor_bereit.bind(start), CONNECT_ONE_SHOT)
	if _rauchtest:
		_rauchtest_ablauf()
	else:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func zaehle_farbtreffer() -> void:
	farbtreffer += 1


## Von der Polizei bei einer Festnahme: aus dem Wagen aussteigen.
func aussteigen_erzwingen() -> void:
	if aktuelles_auto != null:
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
	print("LALABERG_START boden=%s auto=%s" % [boden, auto.global_position])


## Hoehe des Bodens unter einer Stelle; faellt auf die Stelle selbst zurueck.
func _boden(pos: Vector3) -> Vector3:
	var von := pos + Vector3(0, 300, 0)
	var abfrage := PhysicsRayQueryParameters3D.create(von, pos + Vector3(0, -300, 0))
	abfrage.collision_mask = 1
	var treffer := get_world_3d().direct_space_state.intersect_ray(abfrage)
	return treffer["position"] if not treffer.is_empty() else pos


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("menue"):
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE \
				if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED else Input.MOUSE_MODE_CAPTURED
	elif event.is_action_pressed("einsteigen") and auto != null:
		if aktuelles_auto == null:
			_einsteigen()
		else:
			_aussteigen()
	elif event is InputEventMouseButton and event.pressed:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func _einsteigen() -> void:
	# Zuerst ein freier eigener Wagen in der Naehe ...
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
	neu.global_position = ki.global_position + Vector3(0, 0.4, 0)
	# Das Netz der KI schaut nach +X, der fahrbare Wagen nach -Z.
	neu.rotation.y = ki.rotation.y - PI / 2.0
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
	spieler.aktiv(false)
	kamera.ziel = wagen
	kamera.hoehe = 1.2
	kamera.abstand_setzen(7.0)
	kamera.ausnehmen(wagen)
	stadt.fokus = wagen
	polizei.fokus = wagen
	hud.ziel = wagen
	Schaden.rueckmeldung(get_tree(), "Eingestiegen – W/S fahren, Leertaste bremsen, F aussteigen")


func _aussteigen() -> void:
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
	stadt.fokus = spieler
	polizei.fokus = spieler
	hud.ziel = spieler
	hud.tempo_ms = 0.0


func _process(delta: float) -> void:
	if aktuelles_auto != null:
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
	hud.status = "Leben %d    %d €    %s    Ruf %d" % [
		ceili(spieler.leben), Konto.geld(), spieler.waffe.art_name(),
		Konto.stufe(Konto.Wert.RUF)]
	if _rauchtest:
		_rauch_zeit += delta
		if not _bildpfad.is_empty() and _rauch_zeit > 15.0:
			var bild := get_viewport().get_texture().get_image()
			if bild != null:
				bild.save_png(_bildpfad)
				print("LALABERG_BILD %s %s" % [_bildpfad, bild.get_size()])
			_bildpfad = ""


# --- Rauchtest ----------------------------------------------------------------

func _pruefe(name: String, bedingung: bool) -> bool:
	print("LALABERG_PRUEFUNG %s ok=%s" % [name, bedingung])
	return bedingung


## Laeuft nur mit --rauchtest: prueft die Spielsysteme der Reihe nach und beendet
## das Spiel mit Exit-Code 0, wenn alles stimmt.
func _rauchtest_ablauf() -> void:
	var ok := true
	while auto == null:
		await get_tree().process_frame
	await get_tree().create_timer(6.0).timeout

	# Stehen Spieler und Auto auf dem Boden, ist niemand durchgefallen?
	var p := spieler.global_position
	var steht := absf(p.y - _boden_y) < 15.0 and auto.global_position.y > _boden_y - 15.0
	ok = _pruefe("steht", steht) and ok

	# Konto: gutschreiben, bezahlen, nicht ueberziehen.
	var geld0 := Konto.geld()
	Konto.gutschrift(100)
	var bezahlt := Konto.bezahle(30)
	var ueberzogen := Konto.bezahle(1000000)
	ok = _pruefe("konto", bezahlt and not ueberzogen and Konto.geld() == geld0 + 70) and ok

	# Treffer: ein Passant und ein KI-Auto ohne Route stehen vor dem Spieler.
	var vorn := spieler.global_position + kamera.vorwaerts() * 8.0
	var passant := Passant.new()
	add_child(passant)
	passant.global_position = _boden(vorn)
	var wagen := Verkehrsauto.new()
	add_child(wagen)
	wagen.global_position = _boden(vorn + kamera.rechts() * 5.0)
	for i in 4:
		await get_tree().physics_frame
	spieler.feuere_auf(passant.global_position + Vector3(0, 1.0, 0))
	await get_tree().create_timer(1.0).timeout
	ok = _pruefe("treffer_passant", passant.leben < 100.0 and farbtreffer > 0) and ok
	ok = _pruefe("zielsicherheit", Konto.hole_wert(Konto.Wert.ZIELSICHERHEIT) > 0) and ok
	spieler.feuere_auf(wagen.global_position + Vector3(0, 0.8, 0))
	await get_tree().create_timer(1.0).timeout
	ok = _pruefe("treffer_auto", wagen.leben < 100.0) and ok

	# Zusammenbruch: Krankenhaus, Leben voll, ein Viertel des Geldes weg.
	var vor_geld := Konto.geld()
	spieler.verletze(150.0, Vector3.UP, Schaden.Art.ANPRALL)
	for i in 10:
		await get_tree().physics_frame
	var am_klinikum := Vector2(spieler.global_position.x - Spieler.KLINIKUM.x,
			spieler.global_position.z - Spieler.KLINIKUM.z).length() < 15.0
	ok = _pruefe("krankenhaus", spieler.krankenhausbesuche == 1 and spieler.leben == 100.0
			and am_klinikum and Konto.geld() == vor_geld - vor_geld / 4) and ok

	# Fahndung: zwei Sterne, die Streife kommt, wer stehen bleibt, wird festgenommen.
	ok = await _pruefe_fahndung() and ok

	await get_tree().create_timer(maxf(0.0, 22.0 - _rauch_zeit)).timeout
	var ki := verkehr.bewegung()
	ok = _pruefe("verkehr", ki > 1.0) and ok
	print("LALABERG_RAUCHTEST ende pos=%s auto=%s ki_bewegt=%.1f fps=%d ok=%s" % [
		spieler.global_position, auto.global_position, ki,
		Engine.get_frames_per_second(), ok])
	get_tree().quit(0 if ok else 1)


func _pruefe_fahndung() -> bool:
	var ok := true
	polizei.einsatz_min_m = 30.0
	polizei.einsatz_max_m = 60.0
	var geld_vor := Konto.geld()
	polizei.melde(Schaden.Tat.PASSANT_ANGEFAHREN)
	polizei.melde(Schaden.Tat.AUTO_GESTOHLEN)
	polizei.melde(Schaden.Tat.PASSANT_BESCHOSSEN)
	# 2 + 2 + 1 Punkte: ein Stern ab 1, zwei ab 4.
	ok = _pruefe("sterne", polizei.hole_sterne() == 2) and ok
	var frist := 60.0
	var wartet := 0.0
	while polizei.festnahmen == 0 and frist > 0.0:
		await get_tree().create_timer(0.5).timeout
		frist -= 0.5
		wartet += 0.5
		if fmod(wartet, 3.0) < 0.5:
			print("LALABERG_DIAGNOSE sterne=%d gesehen=%s balken=%.2f %s" % [
				polizei.hole_sterne(), polizei.wird_gesehen(),
				polizei.festnahme_anteil(), polizei.debug_streifen()])
	ok = _pruefe("festnahme", polizei.festnahmen == 1 and polizei.hole_sterne() == 0) and ok
	ok = _pruefe("strafe", Konto.geld() < geld_vor) and ok
	await get_tree().create_timer(3.0).timeout
	var zur_inspektion := Vector2(spieler.global_position.x - Polizei.INSPEKTION.x,
			spieler.global_position.z - Polizei.INSPEKTION.z).length() < 25.0
	ok = _pruefe("inspektion", zur_inspektion) and ok
	# Ab drei Sternen steht voraus eine Sperre.
	polizei.test_setze_sterne(3)
	await get_tree().create_timer(1.0).timeout
	ok = _pruefe("sperre", polizei.sperre_steht) and ok
	polizei.test_setze_sterne(0)
	await get_tree().create_timer(0.5).timeout
	ok = _pruefe("sperre_weg", not polizei.sperre_steht) and ok
	return ok


# --- Aufbau -----------------------------------------------------------------

func _umgebung() -> void:
	var himmel := ProceduralSkyMaterial.new()
	himmel.sky_top_color = Color(0.25, 0.45, 0.78)
	himmel.sky_horizon_color = Color(0.65, 0.76, 0.88)
	himmel.ground_horizon_color = Color(0.6, 0.64, 0.68)
	var sky := Sky.new()
	sky.sky_material = himmel
	var env := Environment.new()
	env.background_mode = Environment.BG_SKY
	env.sky = sky
	env.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	env.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	env.fog_enabled = true
	env.fog_light_color = Color(0.7, 0.78, 0.88)
	env.fog_density = 0.0004
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)

	var sonne := DirectionalLight3D.new()
	sonne.rotation_degrees = Vector3(-50, 152, 0)
	sonne.shadow_enabled = not "--ohneschatten" in OS.get_cmdline_user_args()
	sonne.directional_shadow_max_distance = 250.0
	sonne.light_energy = 1.2
	add_child(sonne)


func _eingaben() -> void:
	var tasten := {
		"vorwaerts": [KEY_W, KEY_UP], "rueckwaerts": [KEY_S, KEY_DOWN],
		"links": [KEY_A, KEY_LEFT], "rechts": [KEY_D, KEY_RIGHT],
		"rennen": [KEY_SHIFT], "springen": [KEY_SPACE], "handbremse": [KEY_SPACE],
		"einsteigen": [KEY_F, KEY_ENTER], "menue": [KEY_ESCAPE],
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
