class_name Radio
extends Node
## Das Autoradio: drei Sender (Blasmusik, Klassik, Lech), R schaltet weiter, Aus, Blasmusik,
## Klassik, Lech. Es spielt nur, solange man in einem Wagen sitzt. Die Titel liegen als
## MP3 in daten/radio (Tools/erzeuge_radio.py erzeugt sie mit Lyria ueber OpenRouter);
## fehlen sie, laeuft die gerechnete Schleife aus Tools/Sounds.

const SENDER := ["Blasmusik", "Klassik", "Lech"]

var nummer := 0   ## 0 = aus, 1 bis 3 = Sender
var im_wagen := false

var _player := AudioStreamPlayer.new()
var _titel := {}        ## Sender -> Array[AudioStream]
var _laeuft: Array = [] ## die Titel des laufenden Senders
var _index := 0


func _ready() -> void:
	add_to_group("radio")
	process_mode = Node.PROCESS_MODE_ALWAYS
	_player.volume_db = -6.0
	_player.finished.connect(_naechster_titel)
	add_child(_player)
	nummer = clampi(Konto.sender(), 0, SENDER.size())


func sender_name() -> String:
	return "Aus" if nummer == 0 else SENDER[nummer - 1]


func laeuft() -> bool:
	return _player.playing


## Ein- und Aussteigen: nur im Wagen spielt das Radio.
func fahrt(an: bool) -> void:
	im_wagen = an
	_aktualisiere()


func weiter() -> void:
	nummer = (nummer + 1) % (SENDER.size() + 1)
	Konto.setze_sender(nummer)
	_aktualisiere()
	if im_wagen:
		Schaden.rueckmeldung(get_tree(), "Radio: " + sender_name())


func _unhandled_input(event: InputEvent) -> void:
	if im_wagen and event.is_action_pressed("radio"):
		weiter()


func _aktualisiere() -> void:
	_player.stop()
	if not im_wagen or nummer == 0:
		return
	_laeuft = _lade_titel(SENDER[nummer - 1])
	if _laeuft.is_empty():
		return
	_index = randi() % _laeuft.size()
	_spiele()


func _naechster_titel() -> void:
	if not im_wagen or nummer == 0 or _laeuft.is_empty():
		return
	_index = (_index + 1) % _laeuft.size()
	_spiele()


func _spiele() -> void:
	_player.stream = _laeuft[_index]
	_player.play()


func _lade_titel(sender: String) -> Array:
	if _titel.has(sender):
		return _titel[sender]
	var liste: Array = []
	var ordner := "res://daten/radio"
	if DirAccess.dir_exists_absolute(ProjectSettings.globalize_path(ordner)):
		var namen := DirAccess.get_files_at(ProjectSettings.globalize_path(ordner))
		for n in namen:
			if n.begins_with(sender + "_") and n.ends_with(".mp3"):
				var mp3 := AudioStreamMP3.load_from_file(
						ProjectSettings.globalize_path(ordner) + "/" + n)
				if mp3 != null:
					liste.append(mp3)
	if liste.is_empty():
		var schleife := Ton.strom("SFX_Radio_" + sender, true)
		if schleife != null:
			liste.append(schleife)
	_titel[sender] = liste
	return liste
