class_name Ton
extends RefCounted
## Klaenge laden und abspielen. Die WAV-Dateien stammen aus Tools/Sounds (vom Converter
## nach daten/ton kopiert). Fehlt eine Datei, bleibt es einfach still.

static var _cache := {}


## Der Klang name (ohne .wav); mit schleife = true als Endlosschleife.
static func strom(name: String, schleife := false) -> AudioStreamWAV:
	var schluessel := "%s|%s" % [name, schleife]
	if _cache.has(schluessel):
		return _cache[schluessel]
	var pfad := "res://daten/ton/%s.wav" % name
	var s: AudioStreamWAV = null
	if ResourceLoader.exists(pfad):
		var roh := load(pfad) as AudioStreamWAV
		if roh != null and schleife:
			s = roh.duplicate() as AudioStreamWAV
			s.loop_mode = AudioStreamWAV.LOOP_FORWARD
			s.loop_begin = 0
			s.loop_end = roh.data.size() / 2   # 16 Bit, mono
		else:
			s = roh
	_cache[schluessel] = s
	return s


## Ein einzelner Knall oder Schritt an einem Ort der Welt.
static func einmal(baum: SceneTree, name: String, ort: Vector3, db := 0.0,
		tonhoehe := 1.0) -> void:
	var s := strom(name)
	if s == null or baum.current_scene == null:
		return
	var p := AudioStreamPlayer3D.new()
	p.stream = s
	p.volume_db = db
	p.pitch_scale = tonhoehe
	p.unit_size = 18.0
	p.max_distance = 300.0
	baum.current_scene.add_child(p)
	p.global_position = ort
	p.finished.connect(p.queue_free)
	p.play()


## Eine Klangschleife, die an eltern haengt (Motor, Sirene, Rotor). Der Spieler startet
## sie nicht selbst: der Aufrufer ruft play() und stop() wie noetig. null ohne Datei.
static func schleife(eltern: Node3D, name: String, db := 0.0) -> AudioStreamPlayer3D:
	var s := strom(name, true)
	if s == null:
		return null
	var p := AudioStreamPlayer3D.new()
	p.stream = s
	p.volume_db = db
	p.unit_size = 14.0
	p.max_distance = 260.0
	eltern.add_child(p)
	return p
