class_name Revier
extends Node
## Die vier Reviere des Farbkriegs (siehe Docs/Geschichte.md). Jedes gehoert einer
## Mannschaft und haengt an vier Wahrzeichen aus orte.json. Wer alle vier mit
## Farbe trifft, die Bewaehrung des Kapitels besteht und genug Ruf hat, ruft den
## Kopf der Mannschaft auf den Plan: ein fliehender Wagen in ihrer Farbe. Wer ihn
## stellt, nimmt das Revier und findet, was er hinterlaesst.
##
## Ein Wahrzeichen ist im Spiel kein eigenes Gebaeude, sondern ein Ort in den
## Daten; markiert wird deshalb eine Saeule, die dort steht: von weitem sichtbar,
## aus jeder Richtung zu treffen.

const ORANGE := Color(0.95, 0.45, 0.05)   ## so sieht ein uebernommenes Revier aus
## Reihenfolge wie in Docs/Geschichte.md: erst die Gelben an der Bahn, dann die
## Blauen am Lech, dann die Gruenen ums Klinikum, zuletzt die Weissen in der
## Altstadt. Das eigene Viertel faellt spaet: man nimmt es den Gruenen ab.
## Bewaehrungsart: 0 Lieferung, 1 Taxi, 2 Rennen, 3 Verfolgung.
const VORGABEN := [
	{"revier": "Vorstadt-Nord", "mannschaft": "Die Gelben", "kopf": "Der Dispatcher",
			"farbe": Color(0.95, 0.78, 0.10), "leben": 100.0, "tempo": 58.0, "polizei": false,
			"bew_art": 1, "bew_zahl": 3,
			"marken": ["Bahnhof", "Polizeiinspektion", "Sankt Katharina", "Stadtverwaltung"]},
	{"revier": "Lechviertel", "mannschaft": "Die Blauen", "kopf": "Die Fährfrau",
			"farbe": Color(0.10, 0.35, 0.92), "leben": 160.0, "tempo": 72.0, "polizei": false,
			"bew_art": 2, "bew_zahl": 3,
			"marken": ["Mutterturm", "Johanniskirche", "Färbertor", "Stadttheater"]},
	{"revier": "Klinikum und Süd", "mannschaft": "Die Grünen", "kopf": "Der Förster",
			"farbe": Color(0.10, 0.62, 0.24), "leben": 220.0, "tempo": 64.0, "polizei": true,
			"bew_art": 3, "bew_zahl": 3,
			"marken": ["Klinikum", "kbo-Lech-Mangfall-Klinik", "Christuskirche",
					"Friedhofskirche zur Heiligen Dreifaltigkeit"]},
	{"revier": "Altstadt", "mannschaft": "Die Weißen", "kopf": "Der Wirt vom Hauptplatz",
			"farbe": Color(0.92, 0.92, 0.90), "leben": 300.0, "tempo": 68.0, "polizei": true,
			"bew_art": 0, "bew_zahl": 8,
			"marken": ["Historisches Rathaus", "Schmalzturm", "Bayertor",
					"Stadtpfarrkirche Mariae Himmelfahrt"]},
]
const ARTNAMEN := ["Lieferungen", "Taxifahrten", "Rennen", "Verfolgungen"]
const KOPF_FRIST := 300.0

var fokus: Node3D
var polizei: Polizei
var reviere: Array[Dictionary] = []

var _kopf: Verkehrsauto
var _kopf_revier := -1
var _kopf_frist := 0.0
var _geladen := false
var _takt := 0.0


func _ready() -> void:
	add_to_group("revier")
	_lade_marken()


func _lade_marken() -> void:
	var o = JSON.parse_string(FileAccess.get_file_as_string("res://daten/orte.json"))
	if o == null:
		push_warning("daten/orte.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
		return
	for v: Dictionary in VORGABEN:
		var r := {"name": v["revier"], "mannschaft": v["mannschaft"], "kopf": v["kopf"],
				"farbe": v["farbe"], "leben": v["leben"], "tempo": v["tempo"],
				"polizei": v["polizei"], "bew_art": v["bew_art"], "bew_zahl": v["bew_zahl"],
				"marken": []}
		for gesucht: String in v["marken"]:
			# Doppelte Namen (es gibt drei "Klinikum") nimmt der erste.
			for m: Dictionary in o["marken"]:
				if m["n"] != gesucht:
					continue
				var ort := Vector3(m["x"], 0.0, m["y"]) / 100.0
				# Hoehe vom naechsten Knoten des Strassennetzes; die Saeule ist 30 m
				# hoch, kleine Abweichungen fallen nicht auf.
				if polizei != null:
					ort.y = polizei.naechster_punkt(ort).y
				var s := Saeule.neu(v["farbe"], 14.0, 30.0, 1.6, Vector3.ZERO, true)
				s.position = ort   # die Szene liegt im Ursprung
				get_tree().current_scene.add_child.call_deferred(s)
				r["marken"].append({"name": gesucht, "ort": ort, "markiert": false, "saeule": s})
				break
		reviere.append(r)
		print("LALABERG_REVIER %s (%s) marken=%d" % [r["name"], r["mannschaft"], r["marken"].size()])
	_geladen = true
	# Schon uebernommene Reviere faerben ihre Saeulen sofort um.
	for i in reviere.size():
		if Konto.hat_revier(i):
			for m: Dictionary in reviere[i]["marken"]:
				m["markiert"] = true
				m["saeule"].setze_farbe(ORANGE)


## Welches Revier gerade dran ist (Kapitel 1 = Vorstadt-Nord ...), sonst -1.
func offenes() -> int:
	for i in reviere.size():
		if not Konto.hat_revier(i):
			return i
	return -1


## Wie viele Wahrzeichen des Reviers schon markiert sind.
func markiert(nr: int) -> int:
	if nr < 0 or nr >= reviere.size():
		return 0
	var zahl := 0
	for m: Dictionary in reviere[nr]["marken"]:
		if m["markiert"]:
			zahl += 1
	return zahl


## Wie weit die Bewaehrung des Reviers gediehen ist; [erledigt, noetig].
func bewaehrung(nr: int) -> Array[int]:
	if nr < 0 or nr >= reviere.size():
		return [0, 0]
	var noetig: int = reviere[nr]["bew_zahl"]
	return [mini(noetig, Konto.hole_art_zahl(reviere[nr]["bew_art"])), noetig]


func kopf_laeuft() -> bool:
	return is_instance_valid(_kopf)


func kopf_name() -> String:
	return reviere[_kopf_revier]["kopf"] if _kopf_revier >= 0 else ""


func kopf_ort() -> Vector3:
	return _kopf.position_jetzt() if is_instance_valid(_kopf) else Vector3.ZERO


func _melde(text: String) -> void:
	Schaden.rueckmeldung(get_tree(), text)


## Von der Farbkugel gerufen: ein Treffer auf diesen Koerper.
func markiere(koerper: Object, farbe: Color) -> void:
	var offen := offenes()
	if offen < 0:
		return
	for m: Dictionary in reviere[offen]["marken"]:
		if m["saeule"].koerper() != koerper or m["markiert"]:
			continue
		m["markiert"] = true
		m["saeule"].setze_farbe(farbe)
		var zahl := markiert(offen)
		_melde("%s markiert – %d von %d im Revier %s" % [
				m["name"], zahl, reviere[offen]["marken"].size(), reviere[offen]["name"]])
		print("LALABERG_MARKE %s revier=%s stand=%d/%d" % [
				m["name"], reviere[offen]["name"], zahl, reviere[offen]["marken"].size()])
		_pruefe_uebernahme(offen)
		return


## Alle Wahrzeichen markiert, Bewaehrung geschafft, genug Ruf: jetzt kommt der Kopf.
func _pruefe_uebernahme(nr: int) -> void:
	if nr < 0 or nr >= reviere.size():
		return
	if markiert(nr) < reviere[nr]["marken"].size():
		return
	var b := bewaehrung(nr)
	if b[0] < b[1]:
		_melde("%s markiert – aber erst %d von %d %s" % [
				reviere[nr]["name"], b[0], b[1], ARTNAMEN[clampi(reviere[nr]["bew_art"], 0, 3)]])
		return
	var noetiger_ruf := nr * 150
	if Konto.hole_wert(Konto.Wert.RUF) < noetiger_ruf:
		_melde("%s markiert – aber %s nehmen dich noch nicht ernst (Ruf %d von %d)" % [
				reviere[nr]["name"], reviere[nr]["mannschaft"],
				Konto.hole_wert(Konto.Wert.RUF), noetiger_ruf])
		return
	_rufe_kopf(nr)


## Der Kopf ist ein fahrender Wagen in der Farbe der Mannschaft: weit genug weg,
## um ihn suchen zu muessen, zaeh genug, um ihn nicht mit drei Treffern zu
## erledigen. Der Foerster und der Wirt rufen zusaetzlich die Polizei.
func _rufe_kopf(nr: int) -> void:
	if is_instance_valid(_kopf):
		return
	var r: Dictionary = reviere[nr]
	var wo := fokus.global_position if fokus != null else Vector3.ZERO
	var beste: Verkehrsauto = null
	var beste_d := 0.0
	for a: Verkehrsauto in get_tree().get_nodes_in_group("verkehrsauto"):
		if a.geparkt or a.ist_ausgeschaltet() or a.polizei or not a.weg.gueltig():
			continue
		var d := Vector2(a.position_jetzt().x - wo.x, a.position_jetzt().z - wo.z).length()
		if d < 200.0 or d > 1500.0:
			continue
		if d > beste_d:
			beste_d = d
			beste = a
	if beste == null:
		_melde("Kein Wagen der Mannschaft unterwegs – später noch einmal versuchen")
		return
	_kopf = beste
	_kopf_revier = nr
	_kopf_frist = Time.get_ticks_msec() / 1000.0 + KOPF_FRIST
	beste.setze_lack(r["farbe"])
	beste.setze_leben(r["leben"])
	if r["polizei"] and polizei != null:
		polizei.setze_sterne(4 if nr == 3 else 2)
	# Die Belagerung des Hauptplatzes: der Wirt faehrt nicht allein. Drei Wagen
	# der Weissen in seiner Naehe werden mitgefaerbt und zaeher; sie stehen im Weg,
	# sie jagen nicht.
	if nr == 3:
		var begleitung := 0
		for a: Verkehrsauto in get_tree().get_nodes_in_group("verkehrsauto"):
			if begleitung >= 3 or a == beste or a.geparkt or a.ist_ausgeschaltet() or a.polizei:
				continue
			if a.position_jetzt().distance_to(beste.position_jetzt()) > 300.0:
				continue
			a.setze_lack(r["farbe"])
			a.setze_leben(140.0)
			begleitung += 1
		_melde("Die Weißen halten den Hauptplatz – und sie sind nicht allein")
		print("LALABERG_KOPF belagerung begleitung=%d" % begleitung)
	_melde("%s ist unterwegs – stell ihn, dann gehört dir %s" % [r["kopf"], r["name"]])
	print("LALABERG_KOPF %s revier=%d leben=%.0f abstand=%.0fm" % [r["kopf"], nr, r["leben"], beste_d])


func _kopf_gestellt(nr: int) -> void:
	if nr < 0 or nr >= reviere.size():
		return
	Konto.nimm_revier(nr)
	Konto.setze_kapitel(nr + 2)
	Konto.uebe(Konto.Wert.RUF, 120.0)
	Konto.gutschrift(800)
	# Was der Kopf hinterlaesst: kein Gegenstand, ein Schalter in der Welt.
	var funde := ["den Werkstattschlüssel", "den Rotorschlüssel", "den Zündschlüssel", "die Stadt"]
	if nr < 3:
		Konto.gib_fund(nr as Konto.Fund)
	_melde("%s gestellt – %s gehört dir, und du hast %s" % [
			reviere[nr]["kopf"], reviere[nr]["name"], funde[mini(nr, 3)]])
	print("LALABERG_KOPF gestellt revier=%d kapitel=%d werkstatt=%s" % [
			nr, Konto.kapitel(), Konto.hat_fund(Konto.Fund.WERKSTATT)])
	_kopf = null
	_kopf_revier = -1
	# Die Saeulen des Reviers faerben sich um.
	for m: Dictionary in reviere[nr]["marken"]:
		m["saeule"].setze_farbe(ORANGE)


## Fuer den Rauchtest: alle Wahrzeichen des offenen Reviers treffen.
func test_markiere_alle() -> void:
	var offen := offenes()
	if offen < 0:
		return
	for m: Dictionary in reviere[offen]["marken"]:
		markiere(m["saeule"].koerper(), ORANGE)
	_pruefe_uebernahme(offen)


## Fuer den Rauchtest: den Kopf sofort ausser Gefecht setzen.
func test_stelle_kopf() -> void:
	if not is_instance_valid(_kopf):
		return
	_kopf.verletze(9999.0, Vector3.RIGHT, Schaden.Art.BESCHUSS)
	if _kopf.ist_ausgeschaltet() and _kopf_revier >= 0:
		_kopf_gestellt(_kopf_revier)


## Die Zeile fuers HUD: Kapitel, Revier, Stand der Markierungen und der Bewaehrung.
func hud_text() -> String:
	var offen := offenes()
	if not _geladen or offen < 0:
		return "Ganz Landsberg gehört dir"
	var r: Dictionary = reviere[offen]
	if is_instance_valid(_kopf):
		return "%s: %s ist unterwegs" % [r["name"], r["kopf"]]
	var b := bewaehrung(offen)
	return "Kapitel %d  %s – %d/%d markiert, %s %d/%d" % [
			offen + 1, r["name"], markiert(offen), r["marken"].size(),
			ARTNAMEN[clampi(r["bew_art"], 0, 3)], b[0], b[1]]


func _physics_process(delta: float) -> void:
	if not _geladen:
		return
	_takt += delta
	if _takt < 0.5:
		return
	_takt = 0.0
	# Laeuft eine Jagd? Der Kopf gilt als gestellt, sobald sein Wagen steht.
	if _kopf_revier >= 0:
		if not is_instance_valid(_kopf):
			_kopf_revier = -1
		elif _kopf.ist_ausgeschaltet():
			_kopf_gestellt(_kopf_revier)
		elif Time.get_ticks_msec() / 1000.0 > _kopf_frist:
			_melde("%s ist entkommen – die Wahrzeichen bleiben markiert" % kopf_name())
			_kopf = null
			_kopf_revier = -1
	# Nur die Saeulen des offenen Reviers stehen sichtbar in der Stadt: die
	# spaeteren Kapitel sollen nicht von Anfang an als Wald von Saeulen herumstehen.
	var offen := offenes()
	for i in reviere.size():
		for m: Dictionary in reviere[i]["marken"]:
			m["saeule"].zeige(i == offen)
