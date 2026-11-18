extends Node
## Was ueber das Spielende hinaus bleibt (Autoload "Konto"). Ort, Fahndung und
## laufender Auftrag fangen beim naechsten Start neu an. Gespeichert wird nach
## jeder Aenderung. Im Rauchtest beginnt das Konto leer und schreibt in eine
## eigene Datei, der echte Spielstand bleibt unberuehrt.

enum Wert { AUSDAUER, ZIELSICHERHEIT, FAHREN, RUF }
enum Fund { WERKSTATT, ROTOR, ZUENDUNG }

const WERT_SCHLUESSEL := ["ausdauer", "zielsicherheit", "fahren", "ruf"]
const ART_SCHLUESSEL := ["lieferungen", "taxifahrten", "rennen", "verfolgungen",
		"krankenfahrten", "streifen"]

var _stand := {}
var _pfad := "user://spielstand.json"
var _pruefung := false


func _ready() -> void:
	_pruefung = "--rauchtest" in OS.get_cmdline_user_args()
	if _pruefung:
		_pfad = "user://spielstand_test.json"
	_stand = _leer()
	if not _pruefung and FileAccess.file_exists(_pfad):
		var gelesen = JSON.parse_string(FileAccess.get_file_as_string(_pfad))
		if gelesen is Dictionary:
			for k in gelesen:
				_stand[k] = gelesen[k]
	print("LALABERG_KONTO datei=%s geld=%d waffen=%d" % [_pfad, geld(), _stand["waffen"]])


func _leer() -> Dictionary:
	return {
		"version": 1, "geld": 0, "waffen": 1, "hat_lack": false, "lack": [1.0, 1.0, 1.0],
		"erledigt": 0, "ausdauer": 0.0, "zielsicherheit": 0.0, "fahren": 0.0, "ruf": 0.0,
		"reviere": 0, "kapitel": 0, "funde": 0,
		"lieferungen": 0, "taxifahrten": 0, "rennen": 0, "verfolgungen": 0,
		"krankenfahrten": 0, "streifen": 0,
		"missionen": {}, "flags": {}, "dosen": [], "beste_bude": 0, "sender": 1,
	}


func speichere() -> void:
	var f := FileAccess.open(_pfad, FileAccess.WRITE)
	if f != null:
		f.store_string(JSON.stringify(_stand))


# --- Geld und Ausruestung ---------------------------------------------------

func geld() -> int:
	return int(_stand["geld"])


func gutschrift(betrag: int) -> void:
	_stand["geld"] = geld() + maxi(0, betrag)
	speichere()


## Nur wenn genug da ist; liefert, ob abgebucht wurde.
func bezahle(betrag: int) -> bool:
	if betrag > geld():
		return false
	_stand["geld"] = geld() - betrag
	speichere()
	return true


## Zieht hoechstens ab, was da ist, und liefert den tatsaechlichen Betrag.
func strafe(betrag: int) -> int:
	var ab := clampi(betrag, 0, geld())
	_stand["geld"] = geld() - ab
	speichere()
	return ab


func hat_waffe(art: int) -> bool:
	return art == 0 or (int(_stand["waffen"]) & (1 << art)) != 0


func gib_waffe(art: int) -> void:
	_stand["waffen"] = int(_stand["waffen"]) | (1 << art)
	speichere()


func hat_lack() -> bool:
	return _stand["hat_lack"]


func lack() -> Color:
	var c: Array = _stand["lack"]
	return Color(c[0], c[1], c[2])


func setze_lack(farbe: Color) -> void:
	_stand["hat_lack"] = true
	_stand["lack"] = [farbe.r, farbe.g, farbe.b]
	speichere()


func zaehle_auftrag() -> void:
	_stand["erledigt"] = int(_stand["erledigt"]) + 1
	speichere()


func erledigt() -> int:
	return int(_stand["erledigt"])


# --- Charakterwerte ---------------------------------------------------------

## Erfahrung dazurechnen. Wachsen darf nur langsam: gedeckelt bei 1000, eine
## Stufe kostet 200.
func uebe(wert: Wert, punkte: float) -> void:
	if punkte <= 0.0:
		return
	# Als Kommazahl gefuehrt: ein Schritt je Bild ist weit unter einem Punkt.
	var k: String = WERT_SCHLUESSEL[wert]
	var vorher := float(_stand[k])
	var nachher := clampf(vorher + punkte, 0.0, 1000.0)
	_stand[k] = nachher
	if int(vorher) / 200 != int(nachher) / 200:
		print("LALABERG_WERT %d stufe=%d" % [wert, int(nachher) / 200 + 1])
		speichere()


func hole_wert(wert: Wert) -> int:
	return int(_stand[WERT_SCHLUESSEL[wert]])


func _exit_tree() -> void:
	speichere()


## 0 bis 1: der Anteil am Hoechstwert, fuer Anzeigen und Wirkung.
func anteil(wert: Wert) -> float:
	return clampf(hole_wert(wert) / 1000.0, 0.0, 1.0)


## 1 bis 5: die Stufe, die im HUD steht.
func stufe(wert: Wert) -> int:
	return clampi(hole_wert(wert) / 200 + 1, 1, 5)


# --- Geschichte -------------------------------------------------------------

func hat_revier(nummer: int) -> bool:
	return (int(_stand["reviere"]) & (1 << nummer)) != 0


func nimm_revier(nummer: int) -> void:
	if hat_revier(nummer):
		return
	_stand["reviere"] = int(_stand["reviere"]) | (1 << nummer)
	print("LALABERG_REVIER genommen=%d alle=%d" % [nummer, _stand["reviere"]])
	speichere()


func reviere() -> int:
	return int(_stand["reviere"])


func kapitel() -> int:
	return int(_stand["kapitel"])


func setze_kapitel(neu: int) -> void:
	_stand["kapitel"] = neu
	speichere()


func hat_fund(fund: Fund) -> bool:
	return (int(_stand["funde"]) & (1 << fund)) != 0


func gib_fund(fund: Fund) -> void:
	_stand["funde"] = int(_stand["funde"]) | (1 << fund)
	speichere()


## Erledigte Auftraege nach Art: Lieferung, Taxi, Rennen, Verfolgung,
## Krankenwagen, Streife.
func zaehle_art(art: int) -> void:
	if art < 0 or art >= ART_SCHLUESSEL.size():
		return
	var k: String = ART_SCHLUESSEL[art]
	_stand[k] = int(_stand[k]) + 1
	speichere()


func hole_art_zahl(art: int) -> int:
	if art < 0 or art >= ART_SCHLUESSEL.size():
		return 0
	return int(_stand[ART_SCHLUESSEL[art]])


# --- Drehbuch: Missionen und freie Merker -----------------------------------

func hat_mission(id: String) -> bool:
	return _stand["missionen"].has(id)


func setze_mission(id: String) -> void:
	if hat_mission(id):
		return
	_stand["missionen"][id] = true
	speichere()


## Fuer den Rauchtest: die Hauptgeschichte von vorn.
func test_story_zuruecksetzen() -> void:
	_stand["missionen"] = {}
	_stand["flags"] = {}


func test_mission_loeschen(id: String) -> void:
	_stand["missionen"].erase(id)


func missionen_zahl() -> int:
	return _stand["missionen"].size()


## Freie Merker der Geschichte (Entscheidungen, Begegnungen, Zaehler).
func hole_flag(name: String, standard: Variant = 0) -> Variant:
	return _stand["flags"].get(name, standard)


func setze_flag(name: String, wert: Variant) -> void:
	_stand["flags"][name] = wert
	speichere()


# --- Sammelstuecke und Kleinkram --------------------------------------------

func hat_dose(nummer: int) -> bool:
	return nummer in _stand["dosen"]


func nimm_dose(nummer: int) -> void:
	if nummer < 0 or nummer >= 50 or hat_dose(nummer):
		return
	_stand["dosen"].append(nummer)
	speichere()


func dosen() -> int:
	return _stand["dosen"].size()


func beste_bude() -> int:
	return int(_stand["beste_bude"])


func setze_beste_bude(treffer: int) -> void:
	_stand["beste_bude"] = treffer
	speichere()


func sender() -> int:
	return int(_stand["sender"])


func setze_sender(nummer: int) -> void:
	if int(_stand["sender"]) != nummer:
		_stand["sender"] = nummer
		speichere()
