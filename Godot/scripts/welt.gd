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
		_rauchtest_ablauf()
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

	# Ampeln und Bremsen.
	ok = _pruefe_ampelplan() and ok
	ok = await _pruefe_bremsen() and ok

	# Auftraege, Laeden, Lackiererei und Reviere.
	ok = await _pruefe_wirtschaft() and ok

	# Panzer und Hubschrauber.
	ok = await _pruefe_sonder() and ok

	# Farbdosen: fuenfzig gestellt, zehn einsammeln bringt den Staffelpreis.
	ok = _pruefe_dosen() and ok

	# Ueberholen: ein Wagen faehrt auf ein stehendes Hindernis zu und schert aus.
	ok = await _pruefe_ueberholen() and ok

	# Kontakt-Einsatz: bei drei Sternen laufen Beamte los und nehmen fest.
	ok = await _pruefe_kontakt() and ok

	# Schiessbude: Runde starten, Scheiben treffen, auszahlen.
	ok = await _pruefe_bude() and ok

	# Radio: schaltet durch alle Sender und wieder aus.
	ok = _pruefe_radio() and ok

	# Karte und Menue.
	ok = await _pruefe_karte() and ok

	# Drehbuch: die ganze Hauptgeschichte im Zeitraffer, alle drei Enden.
	ok = await _pruefe_drehbuch() and ok

	await get_tree().create_timer(maxf(0.0, 22.0 - _rauch_zeit)).timeout
	ok = _pruefe("baeume=%d_gauben=%d" % [stadt.baeume, stadt.gauben],
			stadt.baeume > 20 and stadt.gauben > 5) and ok
	ok = _pruefe("moebel=%s" % [moebel.gestellt], moebel.gestellt["lampen"] > 5
			and moebel.gestellt["baeume"] > 5 and moebel.gestellt["buesche"] > 5) and ok
	var ki := verkehr.bewegung()
	ok = _pruefe("verkehr", ki > 1.0) and ok
	var knoten := verkehr.bewegung_knoten()
	ok = _pruefe("verkehr_knoten_bewegt=%.1f" % knoten, knoten > 1.0) and ok
	await _bild_am_ort()
	print("LALABERG_RAUCHTEST ende pos=%s auto=%s ki_bewegt=%.1f fps=%d ok=%s" % [
		spieler.global_position, auto.global_position, ki,
		Engine.get_frames_per_second(), ok])
	get_tree().quit(0 if ok else 1)


func _pruefe_dosen() -> bool:
	var vor_dosen := Konto.dosen()
	var vor_geld := Konto.geld()
	for i in 50:
		if Konto.dosen() >= 10:
			break
		farbdosen.sammle(i)
	var gesammelt := Konto.dosen() - vor_dosen
	var ok := _pruefe("dosen_gestellt=%d" % farbdosen.gestellt, farbdosen.gestellt == 50)
	return _pruefe("dosen_gesammelt=%d geld=%d" % [gesammelt, Konto.geld() - vor_geld],
			gesammelt >= 1 and Konto.geld() - vor_geld >= gesammelt * Farbdosen.PREIS) and ok


func _pruefe_ueberholen() -> bool:
	var start := spieler.global_position + Vector3(0, 40.0, 0)   # ueber der Stadt, ohne Verkehr
	var blocker := Verkehrsauto.new()
	add_child(blocker)
	blocker.setze_lage(start + Vector3(40, 0, 1.5), Vector3.RIGHT)
	var fahrer := Verkehrsauto.new()
	add_child(fahrer)
	fahrer.setze_lage(start, Vector3.RIGHT)
	fahrer.folge_weg(PackedVector3Array([start, start + Vector3(200, 0, 0)]), 40.0)
	var frist := 25.0
	while frist > 0.0 and fahrer.position_jetzt().x < start.x + 55.0:
		await get_tree().create_timer(0.25).timeout
		frist -= 0.25
	var vorbei := fahrer.position_jetzt().x > start.x + 55.0
	var ok := _pruefe("ueberholen_vorbei=%s_anzahl=%d" % [vorbei, fahrer.ueberholungen],
			vorbei and fahrer.ueberholungen >= 1)
	blocker.queue_free()
	fahrer.queue_free()
	return ok


func _pruefe_kontakt() -> bool:
	var vor := polizei.festnahmen
	polizei.setze_sterne(3)
	var p := Polizist.new()
	p.polizei = polizei
	add_child(p)
	p.global_position = _boden(spieler.global_position + Vector3(5, 0, 0)) + Vector3(0, 0.3, 0)
	var frist := 6.0
	while polizei.festnahmen == vor and frist > 0.0:
		await get_tree().create_timer(0.25).timeout
		frist -= 0.25
	var ok := _pruefe("kontakt_festnahme=%d" % (polizei.festnahmen - vor),
			polizei.festnahmen == vor + 1 and polizei.hole_sterne() == 0)
	await get_tree().create_timer(1.0).timeout
	return ok


## Eine echte Kugel aus der Pistole, vom Stand aus auf eine hochstehende Scheibe.
func _pruefe_bude_schuss() -> bool:
	spieler.global_position = _boden(schiessbude.stand()) + Vector3(0, 0.3, 0)
	schiessbude.test_oben = true
	schiessbude.starte()
	var frist := 6.0
	while schiessbude.hochstehende().is_empty() and frist > 0.0:
		await get_tree().create_timer(0.1).timeout
		frist -= 0.1
	var versuche := 0
	while schiessbude.treffer_runde == 0 and versuche < 8 and schiessbude.laeuft:
		var oben := schiessbude.hochstehende()
		if not oben.is_empty():
			spieler.feuere_auf(oben[0].global_position)
			versuche += 1
		# Physikschritte statt Zeit: im Render-Lauf mit Schatten laeuft das Spiel mit 1 bis 2
		# Bildern je Sekunde, die Kugel braucht aber ihre Schritte, um anzukommen.
		for i in 40:
			await get_tree().physics_frame
	var treffer := schiessbude.treffer_runde
	schiessbude.test_oben = false
	schiessbude.beende()
	return _pruefe("bude_echter_schuss_treffer=%d_versuche=%d" % [treffer, versuche], treffer >= 1)


func _pruefe_bude() -> bool:
	var echt := await _pruefe_bude_schuss()
	var gestartet := schiessbude.starte()
	var frist := 6.0
	while schiessbude.hochstehende().is_empty() and frist > 0.0:
		await get_tree().create_timer(0.1).timeout
		frist -= 0.1
	var oben := schiessbude.hochstehende()
	for k in oben:
		schiessbude.treffer(k)
	var treffer := schiessbude.treffer_runde
	# Geld unmittelbar vor und nach dem Abschluss: dazwischen kann nichts anderes buchen.
	var geld0 := Konto.geld()
	schiessbude.beende()
	var lohn := Konto.geld() - geld0
	return _pruefe("bude_start=%s_treffer=%d_lohn=%d" % [gestartet, treffer, lohn],
			gestartet and treffer >= 1 and lohn == treffer * Schiessbude.PREIS
			and Konto.beste_bude() >= treffer) and echt


func _pruefe_radio() -> bool:
	radio.nummer = 0
	radio.fahrt(true)
	radio.weiter()
	var an := radio.laeuft()
	var erster := radio.sender_name()
	radio.weiter()
	radio.weiter()
	radio.weiter()
	var aus := not radio.laeuft() and radio.nummer == 0
	radio.fahrt(false)
	return _pruefe("radio_%s_laeuft=%s_aus=%s" % [erster, an, aus],
			erster == "Blasmusik" and an and aus)


func _pruefe_karte() -> bool:
	var ok := _pruefe("karte_geladen", karte.hat_karte())
	var p := karte.bild_punkt(spieler.global_position)
	ok = _pruefe("karte_spieler_im_bild=%s" % p, p.x > 0.0 and p.y > 0.0 and p.x < 1043.0
			and p.y < 1113.0) and ok
	karte.oeffne_menue()
	await get_tree().process_frame
	var pausiert := get_tree().paused and karte.menue_offen
	karte.schliesse_menue()
	await get_tree().process_frame
	ok = _pruefe("menue_pausiert", pausiert and not get_tree().paused) and ok
	karte.schalte_vollkarte(true)
	await get_tree().process_frame
	var voll := karte.voll and get_tree().paused
	karte.schalte_vollkarte(false)
	await get_tree().process_frame
	return _pruefe("vollkarte", voll and not get_tree().paused) and ok


## Mit --bildort=<Ortsname> steht am Ende ein Bild von diesem Ort (nur fuer Screenshots).
func _bild_am_ort() -> void:
	if _bildpfad.is_empty():
		return
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--bildort="):
			var name := arg.substr(10)
			get_tree().call_group("welt", "versetze_spieler",
					polizei.naechster_punkt(Orte.mitte(name)))
			await get_tree().create_timer(14.0).timeout
			_bild("ort")


func _pruefe_sonder() -> bool:
	var ok := _pruefe("sonder_da", panzer != null and hubschrauber != null)
	if panzer == null or hubschrauber == null:
		return false
	# --- Panzer: ohne Zuendschluessel verschlossen, dann fahren und feuern
	await _gehe_zu(panzer.position_jetzt() + Vector3(0, 0, 6))
	if not Konto.hat_fund(Konto.Fund.ZUENDUNG):
		_einsteigen()
		ok = _pruefe("panzer_verschlossen", aktuelles_sonder == null) and ok
	Konto.gib_fund(Konto.Fund.ZUENDUNG)
	_einsteigen()
	ok = _pruefe("panzer_eingestiegen", aktuelles_sonder == panzer) and ok
	var start := panzer.position_jetzt()
	panzer.test_steuerung(1.0, 0.0, 0.0)
	await get_tree().create_timer(3.0).timeout
	var weg := panzer.position_jetzt().distance_to(start)
	ok = _pruefe("panzer_faehrt weg=%.1f kmh=%.0f" % [weg, panzer.tempo_kmh()],
			weg > 5.0 and panzer.tempo_kmh() > 10.0) and ok
	var vor := farbtreffer
	ok = _pruefe("kanone_feuert", panzer.feuern()) and ok
	await get_tree().create_timer(0.4).timeout
	_bild("panzer")
	ok = _pruefe("kanone_laedt", not panzer.feuern()) and ok
	await get_tree().create_timer(6.0).timeout
	ok = _pruefe("kanone_trifft", farbtreffer > vor) and ok
	var leben := panzer.leben
	panzer.verletze(100.0, Vector3.UP, Schaden.Art.ANPRALL)
	ok = _pruefe("panzer_farbe_prallt_ab", is_equal_approx(panzer.leben, leben - 12.0)) and ok
	panzer.test_steuerung(0.0, 0.0, 0.0)
	panzer.verletze(700.0, Vector3.UP, Schaden.Art.SPRENGUNG)
	ok = _pruefe("panzer_kaputt", panzer.ist_ausgeschaltet()) and ok
	var frist := 8.0
	while aktuelles_sonder != null and frist > 0.0:
		await get_tree().create_timer(0.25).timeout
		frist -= 0.25
	ok = _pruefe("panzer_fahrer_steigt_aus", aktuelles_sonder == null) and ok

	# --- Hubschrauber: steigen, fliegen, abstuerzen
	await _gehe_zu(hubschrauber.position_jetzt() + Vector3(0, 0, 5))
	var abstand := hubschrauber.position_jetzt().distance_to(spieler.global_position)
	Konto.gib_fund(Konto.Fund.ROTOR)
	_einsteigen()
	ok = _pruefe("heli_eingestiegen abstand=%.1f heli=%s figur=%s" % [
			abstand, hubschrauber.position_jetzt(), spieler.global_position],
			aktuelles_sonder == hubschrauber) and ok
	var hoehe := hubschrauber.position_jetzt().y
	hubschrauber.test_steuerung(0.0, 0.0, 1.0)
	await get_tree().create_timer(3.0).timeout
	var gestiegen := hubschrauber.position_jetzt().y - hoehe
	ok = _pruefe("heli_steigt hoehe=%.1f" % gestiegen, gestiegen > 8.0) and ok
	hubschrauber.test_steuerung(0.6, 0.0, 1.0)
	await get_tree().create_timer(2.0).timeout
	var kmh := hubschrauber.tempo_kmh()
	_bild("heli")
	ok = _pruefe("heli_fliegt kmh=%.0f" % kmh, kmh > 20.0) and ok
	hubschrauber.test_steuerung(0.0, 0.0, 0.0)
	hubschrauber.verletze(300.0, Vector3.UP, Schaden.Art.BESCHUSS)
	ok = _pruefe("heli_kaputt", hubschrauber.ist_ausgeschaltet()) and ok
	frist = 20.0
	while aktuelles_sonder != null and frist > 0.0:
		await get_tree().create_timer(0.25).timeout
		frist -= 0.25
	ok = _pruefe("heli_stuerzt_fahrer_steigt_aus", aktuelles_sonder == null) and ok
	return ok


func _pruefe_drehbuch() -> bool:
	var ok := _pruefe("drehbuch_daten missionen=%d fehlende_orte=%d" % [
			drehbuch.missionen_zahl(), drehbuch.fehlende_orte],
			drehbuch.missionen_zahl() == 44 and drehbuch.fehlende_orte == 0)
	var fertig := await drehbuch.test_durchlauf()
	polizei.setze_sterne(0)
	ok = _pruefe("drehbuch_durchlauf fertig=%d" % fertig, fertig == 22) and ok
	ok = _pruefe("drehbuch_wahl ton=%s" % Konto.hole_flag("ton", "?"),
			Konto.hole_flag("ton", "") == "frech") and ok
	ok = _pruefe("ende_wahrheit", Konto.hole_flag("ende", "") == "wahrheit") and ok
	await drehbuch.test_wiederhole("s_f_hauptplatz", 1)
	ok = _pruefe("ende_frieden", Konto.hole_flag("ende", "") == "frieden") and ok
	await drehbuch.test_wiederhole("s_f_hauptplatz", 2)
	ok = _pruefe("ende_macht", Konto.hole_flag("ende", "") == "macht") and ok
	ok = _pruefe("kino_aus", not spieler.gesperrt and hud.visible and not polizei.pausiert) and ok
	return ok


## Setzt die Figur an einen Ort und wartet, bis sie auf dem Boden steht.
func _gehe_zu(ort: Vector3) -> void:
	spieler.setze_auf_boden(ort)
	var frist := 15.0
	while frist > 0.0:
		await get_tree().create_timer(0.25).timeout
		frist -= 0.25
		if absf(spieler.global_position.y - 60.0) > 0.5 and spieler.is_on_floor():
			break
	await get_tree().create_timer(0.3).timeout


func _pruefe_wirtschaft() -> bool:
	var ok := true
	# --- Lieferung zu Fuss: annehmen, hinlaufen, kassieren, naechste Saeule
	var ziele := auftraege.ziele_zahl()
	ok = _pruefe("auftrag_ziele ziele=%d" % ziele, ziele >= 20) and ok
	var geld_vor := Konto.geld()
	auftraege.test_erzwinge(Auftraege.Art.LIEFERUNG)
	auftraege.setze_start_ort(spieler.global_position)
	await get_tree().create_timer(0.6).timeout
	ok = _pruefe("auftrag_angenommen", auftraege.unterwegs) and ok
	var lohn := auftraege.lohn
	var ziel_vor := auftraege.ziel_ort()
	# Direkt an den Zielpunkt: geprueft wird die Ankunft, nicht das Laden der Stadt.
	spieler.global_position = ziel_vor + Vector3(0, 0.5, 0)
	spieler.velocity = Vector3.ZERO
	var frist := 6.0
	while auftraege.erledigt == 0 and frist > 0.0:
		await get_tree().create_timer(0.25).timeout
		frist -= 0.25
	var abstand := Vector2(spieler.global_position.x - ziel_vor.x,
			spieler.global_position.z - ziel_vor.z).length()
	ok = _pruefe("auftrag_erledigt lohn=%d abstand=%.1f dy=%.1f" % [
			lohn, abstand, spieler.global_position.y - ziel_vor.y],
			auftraege.erledigt == 1 and Konto.geld() == geld_vor + lohn) and ok
	ok = _pruefe("naechste_saeule", auftraege.angebot) and ok

	# --- Paintball-Laden: zu Fuss hinein, MP kaufen und waehlen
	await _gehe_zu(laeden.ort_von(Laeden.Ladenart.PAINTBALL))
	ok = _pruefe("laden_offen", laeden.offen == Laeden.Ladenart.PAINTBALL) and ok
	Konto.gutschrift(2000)
	var vor := Konto.geld()
	var gekauft := laeden.kaufe(0)
	ok = _pruefe("laden_kauf", gekauft and Konto.hat_waffe(1) and Konto.geld() == vor - 300) and ok
	spieler.waehle_waffe(1)
	ok = _pruefe("waffe_gewaehlt", spieler.waffe.art == Waffe.Art.MASCHINE) and ok
	ok = _pruefe("laden_nicht_doppelt", not laeden.kaufe(0)) and ok

	# --- Lackiererei: mit dem Wagen hinein, umlackieren
	# Die Figur steht ausserhalb des Rings (so ist der Boden geladen); der Wagen
	# faehrt von aussen hinein, wie im Spiel: wer zu Fuss in die Lackiererei laeuft,
	# muss erst wieder hinaus.
	var lack_ort := laeden.ort_von(Laeden.Ladenart.LACKIEREREI)
	await _gehe_zu(lack_ort + Vector3(25.0, 0, 0))
	auto.linear_velocity = Vector3.ZERO
	auto.angular_velocity = Vector3.ZERO
	auto.global_position = lack_ort + Vector3(0, 1.5, 0)
	auto.rotation = Vector3.ZERO
	await get_tree().create_timer(1.0).timeout
	_steige_ein(auto)
	await get_tree().create_timer(1.0).timeout
	ok = _pruefe("lackiererei_offen", laeden.offen == Laeden.Ladenart.LACKIEREREI) and ok
	polizei.setze_sterne(1)
	await get_tree().create_timer(0.5).timeout   # erst jetzt gilt man als ungesehen
	var rot := Color(0.6, 0.03, 0.03)
	ok = _pruefe("lackiererei_kauf", laeden.kaufe(2) and Konto.hat_lack() and auto.lack == rot) and ok
	# Die Streife war nicht in Sicht: die Fahndung ist vorbei.
	ok = _pruefe("lackiererei_fahndung_weg", polizei.hole_sterne() == 0) and ok

	# --- Verfolgung: einen fahrenden Wagen stellen
	var geld_v := Konto.geld()
	auftraege.test_erzwinge(Auftraege.Art.VERFOLGUNG)
	auftraege.setze_start_ort(auto.global_position)
	await get_tree().create_timer(0.8).timeout
	var beute := auftraege.beute()
	ok = _pruefe("verfolgung_beute", auftraege.unterwegs and beute != null
			and auftraege.art == Auftraege.Art.VERFOLGUNG) and ok
	if beute != null:
		beute.verletze(9999.0, Vector3.RIGHT, Schaden.Art.BESCHUSS)
	await get_tree().create_timer(1.0).timeout
	ok = _pruefe("verfolgung_gestellt", Konto.hole_art_zahl(3) == 1 and Konto.geld() > geld_v) and ok
	_aussteigen()

	# --- Revier: alle Wahrzeichen, Bewaehrung, Kopf, Uebernahme
	ok = _pruefe("revier_marken", revier.reviere.size() == 4
			and revier.reviere[0]["marken"].size() == 4) and ok
	revier.test_markiere_alle()
	ok = _pruefe("revier_markiert", revier.markiert(0) == 4 and not revier.kopf_laeuft()) and ok
	for i in 3:
		Konto.zaehle_art(1)   # drei Taxifahrten: die Bewaehrung des ersten Reviers
	revier.test_markiere_alle()
	ok = _pruefe("revier_kopf", revier.kopf_laeuft()) and ok
	var geld_r := Konto.geld()
	revier.test_stelle_kopf()
	ok = _pruefe("revier_uebernommen", Konto.hat_revier(0) and Konto.kapitel() == 2
			and Konto.hat_fund(Konto.Fund.WERKSTATT) and Konto.geld() == geld_r + 800) and ok
	ok = _pruefe("revier_naechstes", revier.offenes() == 1) and ok
	return ok


## Es gibt die 44 Ampeln und die 1726 Kreuzungen, und an keiner Kreuzung zeigen zwei
## Phasen gleichzeitig Gruen.
func _pruefe_ampelplan() -> bool:
	var ok := _pruefe("ampeln", verkehr.ampeln.size() == 44
			and Verkehrsauto.kreuzung_orte.size() == 1726)
	var konflikte := 0
	for a in verkehr.ampeln:
		for b in verkehr.ampeln:
			if a.gruppe != b.gruppe or a.phase >= b.phase:
				continue
			for i in 400:
				var t := i * 0.5
				if Ampel.zustand_bei(t, a.phase, a.anzahl_phasen) == Ampel.Zustand.GRUEN \
						and Ampel.zustand_bei(t, b.phase, b.anzahl_phasen) == Ampel.Zustand.GRUEN:
					konflikte += 1
	return _pruefe("ampelplan konflikte=%d" % konflikte, konflikte == 0) and ok


## Ein Auto faehrt auf eine rote Ampel zu und muss davor stehen bleiben, bei Gruen
## weiterfahren.
func _pruefe_bremsen() -> bool:
	var x0 := 0.0
	var y := 60.0
	var z0 := -3000.0
	var ampel := Ampel.new()
	ampel.anzahl_phasen = 1
	add_child(ampel)
	ampel.global_position = Vector3(x0 + 60.0, y, z0 + 3.0)
	ampel.test_erzwinge(Ampel.Zustand.ROT)
	var wagen := Verkehrsauto.new()
	wagen.tempo_ms = 10.0
	wagen.weg.setze(PackedVector3Array([Vector3(x0, y, z0), Vector3(x0 + 150.0, y, z0)]), false)
	add_child(wagen)
	await get_tree().create_timer(11.0).timeout
	var x := wagen.position_jetzt().x
	var steht := wagen.geschwindigkeit_ms().length() < 0.5
	var ok := _pruefe("bremst_vor_rot x=%.1f" % x, x > x0 + 40.0 and x < x0 + 58.0 and steht)
	ampel.test_erzwinge(Ampel.Zustand.GRUEN)
	await get_tree().create_timer(6.0).timeout
	ok = _pruefe("faehrt_bei_gruen x=%.1f" % wagen.position_jetzt().x,
			wagen.position_jetzt().x > x0 + 70.0) and ok
	wagen.queue_free()
	ampel.queue_free()
	return ok


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
	polizei.setze_sterne(3)
	await get_tree().create_timer(1.0).timeout
	ok = _pruefe("sperre", polizei.sperre_steht) and ok
	polizei.setze_sterne(0)
	await get_tree().create_timer(0.5).timeout
	ok = _pruefe("sperre_weg", not polizei.sperre_steht) and ok
	polizei.einsatz_min_m = 250.0
	polizei.einsatz_max_m = 450.0
	return ok


# --- Aufbau -----------------------------------------------------------------

func _umgebung() -> void:
	var himmel := ProceduralSkyMaterial.new()
	himmel.sky_top_color = Color(0.25, 0.45, 0.78)
	himmel.sky_horizon_color = Color(0.65, 0.76, 0.88)
	himmel.ground_horizon_color = Color(0.62, 0.72, 0.84)
	himmel.ground_bottom_color = Color(0.45, 0.52, 0.45)
	himmel.sky_curve = 0.12
	himmel.ground_curve = 0.05
	var sky := Sky.new()
	sky.sky_material = himmel
	var env := Environment.new()
	env.background_mode = Environment.BG_SKY
	env.sky = sky
	# Feste, warme Umgebungsfarbe statt Himmel: sonst sind die Schatten knallblau.
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.74, 0.72, 0.70)
	env.ambient_light_energy = 1.05
	env.tonemap_mode = Environment.TONE_MAPPER_ACES
	env.tonemap_exposure = 1.05
	env.tonemap_white = 6.0
	env.fog_enabled = true
	env.fog_light_color = Color(0.70, 0.74, 0.80)
	env.fog_density = 0.00028
	env.fog_sky_affect = 0.3
	env.fog_sun_scatter = 0.25
	# Waermer, satter und mit leichtem Leuchten wie im Konzeptbild.
	env.adjustment_enabled = true
	env.adjustment_contrast = 1.08
	env.adjustment_saturation = 1.08
	env.glow_enabled = true
	env.glow_intensity = 0.3
	env.glow_bloom = 0.04
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)

	var sonne := DirectionalLight3D.new()
	sonne.rotation_degrees = Vector3(-34, 150, 0)
	sonne.light_color = Color(1.0, 0.89, 0.72)
	sonne.shadow_enabled = not "--ohneschatten" in OS.get_cmdline_user_args()
	sonne.directional_shadow_max_distance = 450.0
	sonne.light_energy = 1.0
	# Gegen Schattenakne auf den senkrechten Waenden (streifige Flaechen im Streiflicht).
	sonne.shadow_bias = 0.12
	sonne.shadow_normal_bias = 3.2
	sonne.shadow_blur = 1.6
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
