class_name Auftraege
extends Node
## Auftraege: an der blauen Saeule nimmt man eine Fahrt an, faehrt unter Zeitdruck
## zu einem echten Ort in Landsberg (die Wahrzeichen aus orte.json, auf die
## naechste Strasse gesetzt), bekommt Geld und findet die naechste blaue Saeule an
## einem Ort in der Naehe. Laeuft die Zeit ab, bleibt die Saeule, wo sie war.
## Sechs Arten: Lieferung (auch zu Fuss), Taxi (Strecke, Trinkgeld), Rennen (vier
## Kontrollpunkte, Einsatz und Preisgeld), Verfolgung (einen Wagen stellen),
## Krankenwagen (Verletzter ins Klinikum, Rempler ziehen ab) und Streife (drei
## Wagen hintereinander, die Fahndung ist nach jedem erledigt).

enum Art { LIEFERUNG, TAXI, RENNEN, VERFOLGUNG, KRANKENWAGEN, STREIFE }

## Wie nah man kommen muss (waagerecht, Meter): in die Saeule zu Fuss hineinlaufen,
## ans Ziel auch mit dem Wagen auf der Fahrbahn.
const ANNAHME_RADIUS := 6.0
const ZIEL_RADIUS := 8.0
## Die Hoehe der Ziele stammt vom naechsten Knoten des Strassennetzes, nicht vom
## Boden unter dem Punkt; die Toleranz faengt Abweichungen an Hanglagen ab.
const HOEHE_SPIEL := 15.0
## Ziele nach Luftlinie: nicht um die Ecke, nicht quer durch den Landkreis.
const ZIEL_MIN := 400.0
const ZIEL_MAX := 2500.0
## Naechste Saeule nach einem erledigten Auftrag: ein Ort in der Naehe.
const NAECHSTE_MIN := 150.0
const NAECHSTE_MAX := 900.0
const BLAU := Color(0.05, 0.45, 1.0)
const GELB := Color(1.0, 0.78, 0.05)
const BEUTE_LACK := Color(0.85, 0.05, 0.55)

var fokus: Node3D
var polizei: Polizei
var art := Art.LIEFERUNG
var unterwegs := false
var angebot := false
var lohn := 0
var erledigt := 0
var gescheitert := 0
var rennen_punkt := 0
var streife_rest := 0

var _ziele: Array[Dictionary] = []   # {n, ort}
var _start_ort := Vector3.ZERO
var _start_ziel := -1
var _akt_ziel := -1
var _erst_hinaus := false
var _frist := 0.0
var _gesamt_zeit := 1.0
var _einsatz := 0
var _strecke: Array[int] = []
var _beute: Verkehrsauto
var _wagen_anfang := 100.0
var _letzter_hinweis := -10.0
var _erzwungene_art := -1
var _start_marke := Saeule.neu(BLAU, ANNAHME_RADIUS * 2.0, 60.0, 2.5)
var _ziel_marke := Saeule.neu(GELB, ZIEL_RADIUS * 2.0, 60.0, 2.5)
var _takt := 0.0


func _ready() -> void:
	add_to_group("auftraege")
	_start_marke.zeige(false)
	_ziel_marke.zeige(false)
	get_tree().current_scene.add_child.call_deferred(_start_marke)
	get_tree().current_scene.add_child.call_deferred(_ziel_marke)
	_lade_ziele()


func _jetzt() -> float:
	return Time.get_ticks_msec() / 1000.0


func _waagerecht(a: Vector3, b: Vector3) -> float:
	return Vector2(a.x - b.x, a.z - b.z).length()


## Die Wahrzeichen aus orte.json liegen mitten im Gebaeude. Als Ziel taugt der
## naechste Punkt auf einer Strasse davor, gesucht auf den Strassenzuegen aus
## derselben Datei; die Hoehe kommt vom naechsten Knoten des Strassennetzes.
func _lade_ziele() -> void:
	var o = JSON.parse_string(FileAccess.get_file_as_string("res://daten/orte.json"))
	if o == null:
		push_warning("daten/orte.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
		return
	var zuege: Array[PackedVector2Array] = []
	for s: Dictionary in o["strassen"]:
		for t: Array in s["teile"]:
			var zug := PackedVector2Array()
			for i in range(0, t.size() - 1, 2):
				zug.append(Vector2(t[i], t[i + 1]) / 100.0)
			zuege.append(zug)
	var verworfen := 0
	for m: Dictionary in o["marken"]:
		var n: String = m["n"]
		if _ziele.any(func(z: Dictionary) -> bool: return z["n"] == n):
			continue
		var mitte := Vector2(m["x"], m["y"]) / 100.0
		var beste := Vector2.ZERO
		var beste_d := INF
		for zug in zuege:
			for i in range(zug.size() - 1):
				var p := Geometry2D.get_closest_point_to_segment(mitte, zug[i], zug[i + 1])
				var d := mitte.distance_to(p)
				if d < beste_d:
					beste_d = d
					beste = p
		# Weiter als 250 m von jeder Strasse: da kommt man nicht hin.
		if beste_d > 250.0:
			verworfen += 1
			continue
		var ort := Vector3(beste.x, 0.0, beste.y)
		if polizei != null:
			ort.y = polizei.naechster_punkt(ort).y
		_ziele.append({"n": n, "ort": ort})
	print("LALABERG_AUFTRAG ziele=%d verworfen=%d" % [_ziele.size(), verworfen])


# --- Auskunft ---------------------------------------------------------------

func ziele_zahl() -> int:
	return _ziele.size()


func restzeit() -> float:
	return maxf(0.0, _frist - _jetzt()) if unterwegs else 0.0


func jagd_auf_wagen() -> bool:
	return art == Art.VERFOLGUNG or art == Art.STREIFE


## Wohin der Pfeil zeigt: im Auftrag das Ziel, sonst die blaue Saeule.
func hole_wegpunkt() -> Vector3:
	if not unterwegs:
		return _start_ort
	if jagd_auf_wagen():
		return _beute.position_jetzt() if is_instance_valid(_beute) else _start_ort
	return _ziele[_akt_ziel]["ort"] if _akt_ziel >= 0 else _start_ort


func hole_zielname() -> String:
	if not unterwegs:
		return ""
	if jagd_auf_wagen():
		return "Der nächste Wagen" if art == Art.STREIFE else "Der flüchtende Wagen"
	return _ziele[_akt_ziel]["n"] if _akt_ziel >= 0 else ""


func art_name() -> String:
	return ["Lieferung", "Taxi", "Rennen", "Verfolgung", "Krankenwagen", "Streife"][art]


## Rennen: der wievielte von wie vielen Kontrollpunkten.
func rennen_punkte() -> int:
	return _strecke.size()


func ziel_ort() -> Vector3:
	return _ziele[_akt_ziel]["ort"] if unterwegs and _akt_ziel >= 0 else _start_ort


func beute() -> Verkehrsauto:
	return _beute if is_instance_valid(_beute) else null


# --- Marken -----------------------------------------------------------------

func setze_start_ort(ort: Vector3) -> void:
	_start_ort = ort
	_start_ziel = -1
	angebot = true
	_zeige(true, true, _start_ort)
	print("LALABERG_AUFTRAG angebot %s" % _start_ort)


func _zeige(start: bool, sichtbar: bool, ort: Vector3) -> void:
	var m := _start_marke if start else _ziel_marke
	m.position = ort   # die Szene liegt im Ursprung
	m.zeige(sichtbar)


func _melde(text: String) -> void:
	Schaden.rueckmeldung(get_tree(), text)


# --- Annehmen ---------------------------------------------------------------

## Nur wer im Wagen sitzt, kann einen Fahrgast mitnehmen; die Lieferung geht
## auch zu Fuss und springt deshalb immer ein.
func _nimm_an() -> void:
	art = (_erzwungene_art if _erzwungene_art >= 0 else randi() % 6) as Art
	_erzwungene_art = -1
	var im_wagen := fokus is Auto
	if art != Art.LIEFERUNG and not im_wagen:
		var gewollt := art
		art = Art.LIEFERUNG
		if _jetzt() - _letzter_hinweis > 12.0:
			_letzter_hinweis = _jetzt()
			_melde(["", "Hier wartet auch ein Fahrgast – mit dem Wagen vorfahren",
					"Hier startet auch ein Rennen – mit dem Wagen vorfahren",
					"Hier gäbe es auch eine Verfolgung – mit dem Wagen vorfahren",
					"Hier wartet ein Verletzter – mit dem Wagen vorfahren",
					"Hier beginnt auch eine Streife – mit dem Wagen vorfahren"][gewollt])
	_strecke.clear()
	rennen_punkt = 0
	_einsatz = 0
	_beute = null
	streife_rest = 0
	_wagen_anfang = 100.0
	var moeglich: Array[int] = []
	for i in _ziele.size():
		var d := _waagerecht(_ziele[i]["ort"], _start_ort)
		if i != _start_ziel and d >= ZIEL_MIN and d <= ZIEL_MAX:
			moeglich.append(i)
	if moeglich.is_empty():
		push_warning("LALABERG_AUFTRAG kein Ziel in Reichweite")
		return
	_akt_ziel = moeglich[randi() % moeglich.size()]
	# Der Krankenwagen faehrt nicht irgendwohin, sondern in eine Klinik: die
	# naechstgelegene, die nicht die Saeule selbst ist. Gibt es keine, wird es
	# eine Lieferung.
	if art == Art.KRANKENWAGEN:
		var klinik := -1
		var naechste := INF
		for i in _ziele.size():
			var n: String = _ziele[i]["n"]
			if i == _start_ziel or not (n.begins_with("Klinikum") or n.begins_with("kbo-")):
				continue
			var d := _waagerecht(_ziele[i]["ort"], _start_ort)
			if d < 60.0 or d >= naechste:
				continue
			naechste = d
			klinik = i
		if klinik >= 0:
			_akt_ziel = klinik
		else:
			art = Art.LIEFERUNG
	var meter := _waagerecht(_ziele[_akt_ziel]["ort"], _start_ort)
	# Die Strassen sind laenger als die Luftlinie; 10 m/s ist gemaechliches
	# Stadttempo, dazu eine halbe Minute fuers Einsteigen.
	var zeit := roundf(meter * 1.5 / 10.0 + 30.0)
	match art:
		Art.TAXI:
			lohn = 160 + roundi(meter / 100.0) * 24
		Art.KRANKENWAGEN:
			lohn = 260 + roundi(meter / 100.0) * 34
			zeit = roundf(meter * 1.5 / 15.0 + 20.0)   # zwei Drittel der ueblichen Frist
		Art.STREIFE:
			lohn = 240
		_:
			lohn = 100 + roundi(meter / 5.0 / 10.0) * 10
	_frist = _jetzt() + zeit
	_gesamt_zeit = zeit
	if art == Art.RENNEN:
		zeit = _bereite_rennen()
	elif (art == Art.VERFOLGUNG or art == Art.STREIFE) and not _suche_beute():
		art = Art.LIEFERUNG   # kein Wagen in Reichweite: dann eben eine Lieferung
	if art == Art.STREIFE:
		streife_rest = 3
		_frist = _jetzt() + 90.0
	if art == Art.KRANKENWAGEN and fokus is Auto:
		_wagen_anfang = 100.0 * (fokus as Auto).lebensanteil()
	unterwegs = true
	angebot = false
	_zeige(true, false, _start_ort)
	_zeige(false, true, _ziele[_akt_ziel]["ort"])
	var minuten := floori(zeit / 60.0)
	var sekunden := floori(zeit) % 60
	var ziel_name: String = _ziele[_akt_ziel]["n"]
	match art:
		Art.RENNEN:
			_melde("Rennen: %d Kontrollpunkte, Einsatz %d €, Preisgeld %d €" % [
					_strecke.size(), _einsatz, lohn])
		Art.VERFOLGUNG:
			_melde("Stell den magentafarbenen Wagen – %d € Belohnung" % lohn)
		Art.STREIFE:
			_melde("Streife: drei Wagen, %d € die Runde" % lohn)
		Art.KRANKENWAGEN:
			_melde("Verletzter ins Klinikum – %d:%02d Minuten, %d €" % [minuten, sekunden, lohn])
		Art.TAXI:
			_melde("Fahrgast nach %s – %d:%02d Minuten, %d € plus Trinkgeld" % [
					ziel_name, minuten, sekunden, lohn])
		_:
			_melde("Lieferung zu %s – %d:%02d Minuten, %d €" % [ziel_name, minuten, sekunden, lohn])
	print("LALABERG_AUFTRAG start art=%s ziel=%s luftlinie=%.0fm zeit=%.0fs lohn=%d" % [
			art_name().to_lower(), ziel_name, meter, zeit, lohn])


## Rennstrecke: vier Kontrollpunkte, jeder 300 bis 900 m vom vorigen weg, so
## entsteht ein Rundkurs durch die Stadt statt einer Geraden.
func _bereite_rennen() -> float:
	_strecke.clear()
	var von := _akt_ziel
	var laenge := 0.0
	for runde in 4:
		var moeglich: Array[int] = []
		for i in _ziele.size():
			if i == von or i in _strecke or i == _start_ziel:
				continue
			var d := _waagerecht(_ziele[i]["ort"], _ziele[von]["ort"])
			if d >= 300.0 and d <= 900.0:
				moeglich.append(i)
		if moeglich.is_empty():
			break
		var naechster := moeglich[randi() % moeglich.size()]
		laenge += _waagerecht(_ziele[naechster]["ort"], _ziele[von]["ort"])
		_strecke.append(naechster)
		von = naechster
	if _strecke.is_empty():
		art = Art.LIEFERUNG
		return _gesamt_zeit
	_akt_ziel = _strecke[0]
	rennen_punkt = 0
	# 14 m/s ist zuegiges Stadttempo; der Umweg ueber die Strassen kostet die
	# Haelfte obendrauf, dazu zwanzig Sekunden Anlauf.
	var zeit := roundf(laenge * 1.5 / 14.0 + 20.0)
	_frist = _jetzt() + zeit
	_gesamt_zeit = zeit
	_einsatz = 150
	lohn = 150 + roundi(laenge / 100.0) * 45
	Konto.bezahle(mini(_einsatz, Konto.geld()))
	return zeit


## Verfolgung: ein fahrendes Auto in 300 bis 1200 m Entfernung, moeglichst weit
## weg vom Spieler. Es wird auffaellig lackiert, damit man weiss, wen man sucht.
func _suche_beute() -> bool:
	var wo := fokus.global_position if fokus != null else _start_ort
	var beste: Verkehrsauto = null
	var beste_d := 0.0
	for a: Verkehrsauto in get_tree().get_nodes_in_group("verkehrsauto"):
		if a.geparkt or a.ist_ausgeschaltet() or a.polizei or not a.weg.gueltig():
			continue
		var d := _waagerecht(a.position_jetzt(), wo)
		if d < 300.0 or d > 1200.0:
			continue
		if d > beste_d:
			beste_d = d
			beste = a
	if beste == null:
		return false
	_beute = beste
	beste.setze_lack(BEUTE_LACK)
	lohn = 450 + roundi(beste_d / 10.0) * 10
	return true


# --- Erledigen, Scheitern, Abbrechen ----------------------------------------

func _erledige() -> void:
	# Rennen: erst der letzte Kontrollpunkt zahlt aus, die davor schalten nur weiter.
	if art == Art.RENNEN and rennen_punkt + 1 < _strecke.size():
		rennen_punkt += 1
		_akt_ziel = _strecke[rennen_punkt]
		_zeige(false, true, _ziele[_akt_ziel]["ort"])
		_melde("Kontrollpunkt %d von %d – weiter zu %s" % [
				rennen_punkt, _strecke.size(), _ziele[_akt_ziel]["n"]])
		return
	# Streife: nach jedem gestellten Wagen ist die Fahndung erledigt, man ist
	# schliesslich im Dienst; solange noch Wagen fehlen, geht die Runde weiter.
	if art == Art.STREIFE:
		get_tree().call_group("polizei", "verwische")
		streife_rest = maxi(0, streife_rest - 1)
		if streife_rest > 0 and _suche_beute():
			lohn += 240
			_frist = _jetzt() + 90.0
			_melde("Einer steht – noch %d auf der Runde" % streife_rest)
			return
	var trinkgeld := 0
	# Krankenwagen: was der Wagen unterwegs abbekommen hat, zieht ab, bis zur
	# Haelfte des Lohns. Wer den Patienten durch die Stadt prellt, wird nicht
	# dafuer bezahlt.
	if art == Art.KRANKENWAGEN:
		var jetzt := _wagen_anfang
		if fokus is Auto:
			jetzt = 100.0 * (fokus as Auto).lebensanteil()
		var schaden := clampf(_wagen_anfang - jetzt, 0.0, 100.0)
		var abzug := roundi(lohn * 0.5 * (schaden / 100.0))
		if abzug > 0:
			lohn = maxi(0, lohn - abzug)
			_melde("Harte Fahrt – %d € weniger" % abzug)
	# Trinkgeld beim Taxi: wer die Haelfte der Frist noch uebrig hat, bekommt 40
	# Prozent obendrauf, linear abnehmend bis auf null.
	if art == Art.TAXI:
		var anteil := clampf(restzeit() / maxf(1.0, _gesamt_zeit) / 0.5, 0.0, 1.0)
		trinkgeld = roundi(lohn * 0.4 * anteil)
	Konto.gutschrift(lohn + trinkgeld)
	Konto.zaehle_auftrag()
	Konto.zaehle_art(art)
	# Ruf: ein erledigter Auftrag zaehlt, ein Rennen und eine Verfolgung doppelt.
	Konto.uebe(Konto.Wert.RUF, [14.0, 18.0, 32.0, 32.0, 26.0, 40.0][art])
	erledigt += 1
	unterwegs = false
	var hier: Dictionary = _ziele[_akt_ziel]
	_zeige(false, false, hier["ort"])
	# Naechste Saeule: ein anderer Ort in der Naehe, nicht genau hier, sonst
	# stuende man schon drin und der naechste Auftrag liefe ungefragt los.
	var nah: Array[int] = []
	for i in _ziele.size():
		var d := _waagerecht(_ziele[i]["ort"], hier["ort"])
		if i != _akt_ziel and d >= NAECHSTE_MIN and d <= NAECHSTE_MAX:
			nah.append(i)
	if not nah.is_empty():
		_start_ziel = nah[randi() % nah.size()]
		_start_ort = _ziele[_start_ziel]["ort"]
	angebot = true
	_zeige(true, true, _start_ort)
	var text := "Angekommen! +%d €" % (lohn + trinkgeld)
	if trinkgeld > 0:
		text += " (davon %d € Trinkgeld)" % trinkgeld
	_melde(text + " – nächster Auftrag an der blauen Säule" if art == Art.TAXI
			else "Geschafft! +%d € – nächster Auftrag an der blauen Säule" % lohn)
	print("LALABERG_AUFTRAG erledigt art=%s ziel=%s geld=%d" % [
			art_name().to_lower(), hier["n"], Konto.geld()])
	_akt_ziel = -1


func _scheitere() -> void:
	gescheitert += 1
	unterwegs = false
	_zeige(false, false, ziel_ort())
	angebot = true
	_zeige(true, true, _start_ort)
	# Wer noch in der Saeule steht, bekommt nicht gleich den naechsten Auftrag:
	# erst wieder hinaus und hinein.
	_erst_hinaus = true
	_melde("Zu spät – der Auftrag ist verfallen. Neuer Versuch an der blauen Säule")
	print("LALABERG_AUFTRAG gescheitert art=%s" % art_name().to_lower())
	_akt_ziel = -1


## Nach einer Festnahme: der laufende Auftrag faellt weg, die Saeule bleibt.
func abbrechen() -> void:
	if not unterwegs:
		return
	unterwegs = false
	_zeige(false, false, ziel_ort())
	print("LALABERG_AUFTRAG abgebrochen art=%s" % art_name().to_lower())
	_akt_ziel = -1
	angebot = true
	_erst_hinaus = true
	_zeige(true, true, _start_ort)


func test_erzwinge(neue_art: Art) -> void:
	_erzwungene_art = neue_art


func test_ablaufen() -> void:
	_frist = _jetzt() - 0.01


# --- Takt -------------------------------------------------------------------

func _physics_process(delta: float) -> void:
	_takt += delta
	if _takt < 0.1:
		return
	_takt = 0.0
	if fokus == null:
		return
	var wo := fokus.global_position
	if unterwegs and jagd_auf_wagen():
		# Gestellt ist, wer steht: ausgeschaltet durch Farbe oder Rammen.
		if not is_instance_valid(_beute):
			_scheitere()
		elif _beute.ist_ausgeschaltet():
			_erledige()
		elif _jetzt() > _frist:
			_scheitere()
	elif unterwegs:
		var ziel: Vector3 = _ziele[_akt_ziel]["ort"]
		if _waagerecht(wo, ziel) < ZIEL_RADIUS and absf(wo.y - ziel.y) < HOEHE_SPIEL:
			_erledige()
		elif _jetzt() > _frist:
			_scheitere()
	elif angebot:
		var drin := _waagerecht(wo, _start_ort) < ANNAHME_RADIUS \
				and absf(wo.y - _start_ort.y) < HOEHE_SPIEL
		if not drin:
			_erst_hinaus = false
		elif not _erst_hinaus:
			_nimm_an()
