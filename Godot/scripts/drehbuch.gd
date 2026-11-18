class_name Drehbuch
extends Node
## Das Drehbuch: die Geschichte als Daten statt als Code. Die Hauptgeschichte
## "Farbkrieg" steht in daten/drehbuch.json (Prolog, vier Akte, Finale mit drei
## Enden), die Nebenauftraege der Mannschaften in daten/missionen.json. Eine
## Mission besteht aus Stufen; Spielstufen (hin, bring, markiere, stelle, halte,
## im_wagen, warte_revier) warten auf eine Bedingung, Filmstufen (szene, titel,
## funk, wahl) laufen im Kino ab, wenn und setze lenken die Geschichte.
##
## Die Hauptgeschichte hat eine gruene Saeule, die Nebenauftraege eine orange; beide
## stehen am Startort der naechsten offenen Mission. Missionen mit start "auto"
## beginnen von selbst, sobald sie dran sind.

const GRUEN := Color(0.2, 0.9, 0.4)
const ORANGE := Color(1.0, 0.6, 0.1)
const ANNAHME_RADIUS := 6.0
const ZIEL_RADIUS := 10.0
const HOEHE_SPIEL := 15.0

var fokus: Node3D
var spieler: Spieler
var polizei: Polizei
var laeden: Laeden
var revier: Revier
var stadt: Stadt
var welt: Node
var kino := Kino.new()
var bereit := false          ## die Welt steht (Marken und Laeden gesetzt)
var automatisch := true      ## "auto"-Missionen starten von selbst
var test_modus := false      ## Rauchtest: Spielstufen gelten sofort als geschafft
var geschafft := 0
var fehlende_orte := 0

var _missionen: Array[Dictionary] = []
var _figuren := {}
var _aktiv := {}
var _lauf := 0
var _angebot := {"story": -1, "neben": -1}
var _marken := {}
var _strassen := {}
var _grund := ""
var _stufe_text := ""
var _stufe_ort := Vector3.ZERO
var _stufe_hat_ort := false
var _gegner: Verkehrsauto
var _frist := 0.0
var _erst_hinaus := false
var _proxy := Node3D.new()
var _vorheriger_fokus: Node3D
var _takt := 0.0


func _ready() -> void:
	add_to_group("drehbuch")
	add_child(kino)
	add_child(_proxy)
	_marken["story"] = Saeule.neu(GRUEN, 13.0, 40.0, 1.3)
	_marken["neben"] = Saeule.neu(ORANGE, 11.0, 30.0, 1.0)
	for k in _marken:
		_marken[k].zeige(false)
		get_tree().current_scene.add_child.call_deferred(_marken[k])
	_lade()


func _lade() -> void:
	Orte.laden()
	var story = JSON.parse_string(FileAccess.get_file_as_string("res://daten/drehbuch.json"))
	if story != null:
		_figuren = story["figuren"]
		for k in _figuren:
			if _figuren[k].get("gestalt") == null:
				_figuren[k]["gestalt"] = ""
		var nr := 0
		for m: Dictionary in story["missionen"]:
			m["art_der_mission"] = "story"
			m["reihe"] = nr
			nr += 1
			_missionen.append(m)
	else:
		push_warning("daten/drehbuch.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
	var neben = JSON.parse_string(FileAccess.get_file_as_string("res://daten/missionen.json"))
	if neben != null:
		for m: Dictionary in neben["missionen"]:
			m["art_der_mission"] = "neben"
			m["voraus"] = []
			m["akt"] = m["kapitel"]
			_missionen.append(m)
	for m in _missionen:
		if m["start"] != "auto" and not _kenne_ort(m["start"]):
			fehlende_orte += 1
		for s in _alle_stufen(m["stufen"]):
			if s.has("ort") and not _kenne_ort(s["ort"]):
				fehlende_orte += 1
	print("LALABERG_DREHBUCH missionen=%d fehlende_orte=%d" % [_missionen.size(), fehlende_orte])


func _kenne_ort(name: String) -> bool:
	return Orte.gibt_es(name) or name in ["Lackiererei", "Paintball-Laden"]


func _alle_stufen(stufen: Array) -> Array:
	var alle := []
	for s: Dictionary in stufen:
		alle.append(s)
		if s["art"] == "wenn":
			alle.append_array(_alle_stufen(s["dann"]))
			alle.append_array(_alle_stufen(s["sonst"]))
	return alle


func laeuft() -> bool:
	return not _aktiv.is_empty()


func missionen_zahl() -> int:
	return _missionen.size()


func figur(id: String) -> Dictionary:
	return _figuren.get(id, {})


func spieler_sichtbar() -> bool:
	return spieler != null and spieler.visible


func _jetzt() -> float:
	return Time.get_ticks_msec() / 1000.0


func _melde(text: String) -> void:
	Schaden.rueckmeldung(get_tree(), text)


# --- Orte -------------------------------------------------------------------

## Ort einer Mission als Punkt auf einer Strasse; die Laeden stehen schon dort.
func ort_von(name: String) -> Vector3:
	if _strassen.has(name):
		return _strassen[name]
	var ort: Vector3
	if name == "Lackiererei":
		ort = laeden.ort_von(Laeden.Ladenart.LACKIEREREI)
	elif name == "Paintball-Laden":
		ort = laeden.ort_von(Laeden.Ladenart.PAINTBALL)
	else:
		ort = Orte.strasse(name, polizei)
	_strassen[name] = ort
	return ort


func boden_y(pos: Vector3, rueckfall: float) -> float:
	if spieler == null:
		return rueckfall
	var abfrage := PhysicsRayQueryParameters3D.create(pos + Vector3(0, 30, 0), pos - Vector3(0, 30, 0))
	abfrage.collision_mask = 1
	var treffer := spieler.get_world_3d().direct_space_state.intersect_ray(abfrage)
	return treffer["position"].y if not treffer.is_empty() else rueckfall


## Zielpunkt eines Kameraschnitts. Ohne Ortsnamen spielt die Szene dort, wo die
## Figur steht. Liegt der Ort weit weg, laedt die Stadt dort erst nach.
func kamera_ziel(name: String) -> Dictionary:
	var ist_spieler := name == "" or name == "spieler"
	var pos: Vector3
	if ist_spieler:
		pos = fokus.global_position
	elif name == "auto":
		pos = welt.auto.global_position if welt != null and welt.auto != null else fokus.global_position
	else:
		pos = ort_von(name)
	if not ist_spieler and not kino.test_schnell:
		var weit := Vector2(pos.x - fokus.global_position.x, pos.z - fokus.global_position.z).length()
		if weit > 150.0:
			await _lade_dort(pos)
	return {"pos": pos, "ist_spieler": ist_spieler}


## Die Stadt folgt kurz der Kamera statt der Figur, bis der Sektor dort steht.
func _lade_dort(pos: Vector3) -> void:
	if _vorheriger_fokus == null:
		_vorheriger_fokus = stadt.fokus
	_proxy.global_position = pos
	stadt.fokus = _proxy
	var id: String = stadt.eintrag_fuer(pos).get("id", "")
	var frist := 6.0
	while not stadt.ist_geladen(id) and frist > 0.0:
		await get_tree().create_timer(0.2).timeout
		frist -= 0.2
	await get_tree().create_timer(0.4).timeout


## Vom Kino gerufen: Film an oder aus. Die Welt friert ein und blendet das HUD aus.
func kino_modus(an: bool) -> void:
	if welt != null:
		welt.kino_modus(an)
	if not an and _vorheriger_fokus != null:
		stadt.fokus = _vorheriger_fokus
		_vorheriger_fokus = null


# --- Angebote ---------------------------------------------------------------

func _kapitel() -> int:
	var o := revier.offenes()
	return 5 if o < 0 else o + 1


## Die naechste offene Mission der Hauptgeschichte (nach Reihenfolge, wenn alle
## Voraussetzungen erfuellt sind) und der Nebenauftraege des laufenden Kapitels.
func _suche_angebote() -> void:
	var story := -1
	var neben := -1
	var beste_reihe := 999999
	for i in _missionen.size():
		var m := _missionen[i]
		if Konto.hat_mission(m["id"]):
			continue
		if m["art_der_mission"] == "story":
			if story < 0 and m["voraus"].all(func(v: String) -> bool: return Konto.hat_mission(v)):
				story = i
		elif m["kapitel"] == _kapitel() and m["reihe"] < beste_reihe:
			beste_reihe = m["reihe"]
			neben = i
	_angebot["story"] = story
	_angebot["neben"] = neben
	for art: String in ["story", "neben"]:
		var i: int = _angebot[art]
		var marke: Saeule = _marken[art]
		if i < 0 or _missionen[i]["start"] == "auto" or not bereit or laeuft():
			marke.zeige(false)
			continue
		marke.position = ort_von(_missionen[i]["start"])
		marke.zeige(true)


# --- Ablauf -----------------------------------------------------------------

func _physics_process(delta: float) -> void:
	_takt += delta
	if _takt < 0.25:
		return
	_takt = 0.0
	if fokus == null or not bereit or laeuft() or kino.laeuft():
		return
	_suche_angebote()
	var auto_start: int = _angebot["story"]
	if auto_start >= 0 and _missionen[auto_start]["start"] == "auto" and automatisch:
		_starte(_missionen[auto_start])
		return
	var wo := fokus.global_position
	var innen := false
	for art: String in ["story", "neben"]:
		var i: int = _angebot[art]
		if i < 0 or _missionen[i]["start"] == "auto":
			continue
		var start := ort_von(_missionen[i]["start"])
		if Vector2(wo.x - start.x, wo.z - start.z).length() < ANNAHME_RADIUS \
				and absf(wo.y - start.y) < HOEHE_SPIEL:
			innen = true
			if not _erst_hinaus:
				_starte(_missionen[i])
				return
	if not innen:
		_erst_hinaus = false


## Fuehrt eine Mission aus; kehrt zurueck, wenn sie geschafft oder gescheitert ist.
func _starte(m: Dictionary) -> void:
	_aktiv = m
	_lauf += 1
	var id := _lauf
	_grund = ""
	_gegner = null
	_suche_angebote()
	print("LALABERG_MISSION start %s stufen=%d" % [m["id"], m["stufen"].size()])
	var ok: bool = await _stufen(m["stufen"], id)
	if id != _lauf:
		return
	if ok:
		_vollende(m)
	else:
		_scheitere(m)


func _vollende(m: Dictionary) -> void:
	Konto.gutschrift(m["lohn"])
	Konto.uebe(Konto.Wert.RUF, float(m["ruf"]))
	Konto.setze_mission(m["id"])
	geschafft += 1
	if m["lohn"] > 0:
		_melde("%s geschafft – %d €" % [m["name"], m["lohn"]])
	print("LALABERG_MISSION fertig %s lohn=%d" % [m["id"], m["lohn"]])
	_ende_der_mission()


func _scheitere(m: Dictionary) -> void:
	_melde("%s gescheitert – %s. Neuer Versuch an der Säule" % [m["name"], _grund])
	print("LALABERG_MISSION gescheitert %s grund=%s" % [m["id"], _grund])
	_ende_der_mission()


func _ende_der_mission() -> void:
	_aktiv = {}
	_gegner = null
	_stufe_text = ""
	_stufe_hat_ort = false
	_frist = 0.0
	# Wer noch in der Saeule steht, bekommt nicht gleich die naechste Mission.
	_erst_hinaus = true
	_suche_angebote()


## Bricht eine laufende Mission ab, etwa nach einer Festnahme in einem Spielstueck.
func abbrechen() -> void:
	if laeuft():
		_lauf += 1
		_ende_der_mission()


func _stufen(stufen: Array, id: int) -> bool:
	for s: Dictionary in stufen:
		if id != _lauf:
			return false
		var ok: bool = await _stufe(s, id)
		if not ok:
			return false
	return true


func _stufe(s: Dictionary, id: int) -> bool:
	_stufe_hat_ort = false
	_frist = 0.0
	if s["art"] in ["szene", "titel", "funk", "wahl", "wenn", "setze", "gib"]:
		return await _stufe_film(s, id)
	return await _stufe_spiel(s, id)


## Filmstufen und Zustand: laufen im Kino ab oder lenken die Geschichte.
func _stufe_film(s: Dictionary, id: int) -> bool:
	var ok := true
	match s["art"]:
		"szene":
			await kino.spiele_szene(s["schnitte"], self)
		"titel":
			await kino.zeige_titel(s["text"], s["unter"], float(s["dauer"]))
		"funk":
			var f := figur(s["wer"])
			kino.funk(f.get("name", s["wer"]), f.get("farbe", "#ffffff"), s["text"])
			if not test_modus:
				var dauer := clampf(s["text"].length() * 0.055 + 1.5, 3.0, 7.0)
				await get_tree().create_timer(dauer).timeout
		"wahl":
			var nr: int = await kino.frage(s["frage"], s["optionen"])
			var gewaehlt: Dictionary = s["optionen"][nr]
			for flag: String in gewaehlt["setze"]:
				Konto.setze_flag(flag, gewaehlt["setze"][flag])
		"wenn":
			var zweig: Array = s["sonst"]
			if Konto.hole_flag(s["flag"], "") == s["ist"]:
				zweig = s["dann"]
			ok = await _stufen(zweig, id)
		"setze":
			Konto.setze_flag(s["flag"], s["wert"])
		"gib":
			Konto.gutschrift(int(s["geld"]))
			Konto.uebe(Konto.Wert.RUF, float(s["ruf"]))
			if s.has("waffe"):
				Konto.gib_waffe(int(s["waffe"]))
	return ok


## Spielstufen: warten auf eine Bedingung in der Welt.
func _stufe_spiel(s: Dictionary, id: int) -> bool:
	var ok := true
	match s["art"]:
		"hin", "bring":
			var ort := ort_von(s["ort"])
			var wagen: bool = s["art"] == "bring" or s.get("imWagen", false)
			_zeige_stufe(s["text"], ort, float(s["zeit"]))
			var frist := float(s["zeit"])
			ok = await _warte_bis(func() -> bool: return _am_ort(ort, wagen), frist, id)
		"markiere":
			var zeit := float(s.get("zeit", 0.0))
			_zeige_stufe(s["text"], revier.marke_ort(s["ort"]), zeit)
			ok = await _warte_bis(func() -> bool: return revier.ist_markiert(s["ort"]), zeit, id)
		"stelle":
			ok = await _stelle(s, id)
		"halte":
			_zeige_stufe(s["text"], Vector3.ZERO, 0.0)
			if int(s["sterne"]) > 0:
				polizei.setze_sterne(int(s["sterne"]))
			var bis := _jetzt() + float(s["zeit"])
			_frist = bis
			ok = await _warte_bis(func() -> bool: return _jetzt() >= bis, 0.0, id)
		"im_wagen":
			_zeige_stufe(s["text"], Vector3.ZERO, 0.0)
			ok = await _warte_bis(func() -> bool: return fokus is Auto, 0.0, id)
		"warte_revier":
			_zeige_stufe(s["text"], Vector3.ZERO, 0.0)
			ok = await _warte_bis(func() -> bool: return Konto.hat_revier(int(s["nr"])),
					0.0, id, false)
		_:
			push_error("Unbekannte Stufe: %s" % s["art"])
	return ok


func _zeige_stufe(text: String, ort: Vector3, zeit: float) -> void:
	_stufe_text = text
	_stufe_ort = ort
	_stufe_hat_ort = ort != Vector3.ZERO
	_frist = _jetzt() + zeit if zeit > 0.0 else 0.0
	_melde(text)


func _am_ort(ort: Vector3, wagen: bool) -> bool:
	if wagen and not (fokus is Auto):
		return false
	var wo := fokus.global_position
	var waagerecht := Vector2(wo.x - ort.x, wo.z - ort.z).length()
	return waagerecht < ZIEL_RADIUS and absf(wo.y - ort.y) < HOEHE_SPIEL


## Wartet auf eine Bedingung. Scheitert bei abgelaufener Frist, wenn der Spieler
## umgehauen oder festgenommen wird (streng) oder die Mission abgebrochen wird.
func _warte_bis(bedingung: Callable, zeit: float, id: int, streng := true) -> bool:
	if test_modus:
		await get_tree().process_frame
		return id == _lauf
	var ende := _jetzt() + zeit
	var besuche := spieler.krankenhausbesuche
	var haft := polizei.festnahmen
	while not bedingung.call():
		await get_tree().create_timer(0.1).timeout
		if id != _lauf:
			return false
		if zeit > 0.0 and _jetzt() > ende:
			_grund = "die Zeit ist um"
			return false
		if streng and spieler.krankenhausbesuche != besuche:
			_grund = "umgehauen"
			return false
		if streng and polizei.festnahmen != haft:
			_grund = "festgenommen"
			return false
	return id == _lauf


## Einen Wagen kennzeichnen und ausser Gefecht setzen.
func _stelle(s: Dictionary, id: int) -> bool:
	_gegner = _suche_gegner()
	if _gegner == null:
		_melde("Niemand unterwegs – weiter")
		return true
	_stufe_text = s["text"]
	_stufe_hat_ort = true
	_frist = _jetzt() + float(s["zeit"]) if float(s["zeit"]) > 0.0 else 0.0
	_melde(s["text"])
	var g := _gegner
	if test_modus:
		g.verletze(9999.0, Vector3.RIGHT, Schaden.Art.BESCHUSS)
	return await _warte_bis(func() -> bool: return not is_instance_valid(g) or g.ist_ausgeschaltet(),
			float(s["zeit"]), id)


func _suche_gegner() -> Verkehrsauto:
	var wo := fokus.global_position
	var beste: Verkehrsauto = null
	var beste_d := INF
	for a: Verkehrsauto in get_tree().get_nodes_in_group("verkehrsauto"):
		if a.geparkt or a.ist_ausgeschaltet() or a.polizei or not a.weg.gueltig():
			continue
		var d := Vector2(a.position_jetzt().x - wo.x, a.position_jetzt().z - wo.z).length()
		if d < 80.0 or d > 900.0:
			continue
		if d < beste_d:
			beste_d = d
			beste = a
	if beste != null:
		beste.setze_lack(Color(0.95, 0.12, 0.45))
		beste.setze_leben(120.0)
	return beste


# --- HUD --------------------------------------------------------------------

## Die Missionszeilen fuers HUD: laufende Stufe mit Pfeil, Entfernung und Frist;
## sonst die offenen Angebote mit der Saeule, zu der man muss.
func hud_text(vorwaerts: Vector3) -> String:
	if fokus == null:
		return ""
	var wo := fokus.global_position
	if laeuft():
		var zeile := "%s – %s" % [_aktiv["name"], _stufe_text]
		var ziel := _stufe_ort
		if is_instance_valid(_gegner) and _stufe_hat_ort and _stufe_text != "":
			ziel = _gegner.position_jetzt()
		if _stufe_hat_ort:
			zeile += "   " + _pfeil(wo, ziel, vorwaerts)
		if _frist > 0.0:
			var rest := maxf(0.0, _frist - _jetzt())
			zeile += "   %d:%02d" % [floori(rest / 60.0), floori(rest) % 60]
		return zeile
	var zeilen: Array[String] = []
	for art: String in ["story", "neben"]:
		var i: int = _angebot[art]
		if i < 0 or _missionen[i]["start"] == "auto":
			continue
		var m := _missionen[i]
		zeilen.append("%s: %s – %s   %s" % [
				"Grüne Säule" if art == "story" else "Orange Säule", m["name"], m["start"],
				_pfeil(wo, ort_von(m["start"]), vorwaerts)])
	return "\n".join(zeilen)


func _pfeil(von: Vector3, nach: Vector3, vorwaerts: Vector3) -> String:
	var d := Vector3(nach.x - von.x, 0, nach.z - von.z)
	var pfeile := ["↑", "↖", "←", "↙", "↓", "↘", "→", "↗"]
	var ang := vorwaerts.signed_angle_to(d, Vector3.UP)
	return "%s %d m" % [pfeile[posmod(roundi(ang / (PI / 4.0)), 8)], roundi(d.length())]


# --- Rauchtest --------------------------------------------------------------

## Spielt die ganze Hauptgeschichte im Zeitraffer durch und liefert, wie viele
## Missionen fertig wurden. Spielstufen gelten sofort als geschafft, Szenen laufen
## ohne Warten, Entscheidungen waehlt kino.test_wahl.
func test_durchlauf() -> int:
	test_modus = true
	kino.test_schnell = true
	Konto.test_story_zuruecksetzen()
	var fertig := 0
	for runde in 60:
		_suche_angebote()
		var i: int = _angebot["story"]
		if i < 0:
			break
		await _starte(_missionen[i])
		fertig += 1
	test_modus = false
	kino.test_schnell = false
	return fertig


## Eine Hauptmission noch einmal spielen, mit einer bestimmten Wahl.
func test_wiederhole(id: String, wahl: int) -> void:
	test_modus = true
	kino.test_schnell = true
	kino.test_wahl = wahl
	Konto.test_mission_loeschen(id)
	for m in _missionen:
		if m["id"] == id:
			await _starte(m)
	kino.test_wahl = 0
	test_modus = false
	kino.test_schnell = false
