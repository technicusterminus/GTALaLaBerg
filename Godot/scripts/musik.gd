class_name Musik
extends Node
## Die Spielmusik: im Menue das Menuestueck, in der Verfolgung (Fahndung) das Verfolgungsstueck,
## im Hubschrauber das Flugstueck, im Wagen das Fahrstueck, sonst Stille. Laeuft das Autoradio,
## schweigt die Musik. Die Stuecke erzeugt Tools/erzeuge_musik.py mit Lyria ueber OpenRouter
## nach daten/musik/<Name>.mp3; fehlen sie, bleibt es still. Wechsel werden ueberblendet.

const NAMEN := ["menue", "fahrt", "heli", "verfolgung"]
const LAUT_DB := -9.0
const UEBERBLENDUNG := 2.0

var welt: Node
var karte: Karte
var polizei: Polizei
var radio: Radio

var aktuell := ""

var _strom := {}
var _spieler := AudioStreamPlayer.new()
var _takt := 0.0


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	_spieler.volume_db = -60.0
	add_child(_spieler)
	for n: String in NAMEN:
		var pfad := ProjectSettings.globalize_path("res://daten/musik/%s.mp3" % n)
		if FileAccess.file_exists(pfad):
			var mp3 := AudioStreamMP3.load_from_file(pfad)
			if mp3 != null:
				mp3.loop = true
				_strom[n] = mp3


## Welches Stueck gerade laufen soll ("" = keines).
func wunsch() -> String:
	if karte != null and karte.menue_offen:
		return "menue"
	if radio != null and radio.laeuft():
		return ""
	if polizei != null and polizei.hole_sterne() > 0:
		return "verfolgung"
	if welt != null and welt.get("aktuelles_sonder") != null:
		var s: Object = welt.get("aktuelles_sonder")
		return "heli" if s.get("art") == Sonderfahrzeug.Art.HUBSCHRAUBER else "verfolgung"
	if welt != null and welt.get("aktuelles_auto") != null:
		return "fahrt"
	return ""


func _process(delta: float) -> void:
	_takt -= delta
	if _takt <= 0.0:
		_takt = 0.4
		var neu := wunsch()
		if neu != aktuell:
			aktuell = neu
	# Lautstaerke zum Ziel: das laufende Stueck leiser machen, bis es still ist, dann wechseln.
	var soll_db := LAUT_DB if aktuell != "" and _strom.has(aktuell) else -60.0
	var spielt: String = _spieler.get_meta("stueck", "")
	if spielt != aktuell and spielt != "":
		_spieler.volume_db = move_toward(_spieler.volume_db, -60.0,
				(LAUT_DB + 60.0) / UEBERBLENDUNG * delta)
		if _spieler.volume_db <= -59.0:
			_spieler.stop()
			_spieler.set_meta("stueck", "")
		return
	if spielt == "" and aktuell != "" and _strom.has(aktuell):
		_spieler.stream = _strom[aktuell]
		_spieler.set_meta("stueck", aktuell)
		_spieler.volume_db = -60.0
		_spieler.play()
	_spieler.volume_db = move_toward(_spieler.volume_db, soll_db,
			(LAUT_DB + 60.0) / UEBERBLENDUNG * delta)
