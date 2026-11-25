class_name Schiessbude
extends Node
## Die Schiessbude auf dem Weg in die Stadt: wer zu Fuss in den orangen Ring tritt, startet
## eine Runde von dreissig Sekunden. Sechs Scheiben klappen nach einer Weile hoch und
## wieder weg; jede Paintball-Kugel, die eine hochstehende trifft, zaehlt. Am Ende gibt es
## 20 € je Treffer, und die Bestleistung wird gemerkt.

const RUNDE_S := 30.0
const SCHEIBEN := 6
const ENTFERNUNG := 16.0
const OBEN_S := 2.6
const PREIS := 20
const ORANGE := Color(1.0, 0.5, 0.1)

var fokus: Node3D
var laeuft := false
var test_oben := false   ## nur Tests: hochgeklappte Scheiben bleiben stehen (langsame Laeufe)
var treffer_runde := 0
var runden := 0

var _stand := Vector3.ZERO
var _aufgestellt := false
var _scheiben: Array[Dictionary] = []   # {knoten, oben_bis, naechste}
var _rest := 0.0
var _takt := 0.0
var _sperre := 0.0


func _ready() -> void:
	add_to_group("schiessbude")


func ist_aufgestellt() -> bool:
	return _aufgestellt


func stand() -> Vector3:
	return _stand


## Stellt Marke und Scheiben auf; die Scheiben stehen ENTFERNUNG Meter vom Stand weg in
## Blickrichtung blick (waagerecht), nebeneinander, mit dem Gesicht zum Stand.
func stelle(ort: Vector3, blick: Vector3) -> void:
	_stand = ort
	_aufgestellt = true
	var marke := Saeule.neu(ORANGE, 8.0, 20.0, 1.2)
	get_tree().current_scene.add_child(marke)
	marke.global_position = ort
	var vor := Vector3(blick.x, 0, blick.z).normalized()
	var quer := vor.cross(Vector3.UP)
	for i in SCHEIBEN:
		var versatz := (i - (SCHEIBEN - 1) / 2.0) * 2.4
		var ziel := ort + vor * ENTFERNUNG + quer * versatz
		var koerper := StaticBody3D.new()
		var scheibe := MeshInstance3D.new()
		var zyl := CylinderMesh.new()
		zyl.top_radius = 0.55
		zyl.bottom_radius = 0.55
		zyl.height = 0.08
		scheibe.mesh = zyl
		scheibe.rotation.x = PI / 2.0
		var m := StandardMaterial3D.new()
		m.albedo_color = ORANGE if i % 2 == 0 else Color(0.95, 0.95, 0.9)
		scheibe.material_override = m
		koerper.add_child(scheibe)
		var form := CollisionShape3D.new()
		var zs := CylinderShape3D.new()
		zs.radius = 0.55
		zs.height = 0.12
		form.shape = zs
		form.rotation.x = PI / 2.0
		koerper.add_child(form)
		get_tree().current_scene.add_child(koerper)
		koerper.global_position = ziel + Vector3(0, 1.4, 0)
		koerper.look_at(ort + Vector3(0, 1.4, 0))
		koerper.visible = false
		form.disabled = true
		_scheiben.append({"knoten": koerper, "form": form, "oben_bis": 0.0, "naechste": 0.0})
	print("LALABERG_BUDE bei %s scheiben=%d" % [ort, _scheiben.size()])


func _jetzt() -> float:
	return Time.get_ticks_msec() / 1000.0


func starte() -> bool:
	if laeuft or not _aufgestellt:
		return false
	laeuft = true
	treffer_runde = 0
	_rest = RUNDE_S
	for s in _scheiben:
		s["naechste"] = _jetzt() + randf_range(0.2, 1.5)
	Schaden.rueckmeldung(get_tree(), "Schießbude – %d Sekunden, triff die Scheiben!" % int(RUNDE_S))
	print("LALABERG_BUDE start")
	return true


func hochstehende() -> Array[Node3D]:
	var liste: Array[Node3D] = []
	for s in _scheiben:
		if (s["knoten"] as Node3D).visible:
			liste.append(s["knoten"])
	return liste


## Von der Farbkugel gerufen: ziel ist, was sie getroffen hat.
func treffer(ziel: Object) -> void:
	if not laeuft:
		return
	for s in _scheiben:
		if s["knoten"] == ziel and (s["knoten"] as Node3D).visible:
			_klappe(s, false)
			treffer_runde += 1
			s["naechste"] = _jetzt() + randf_range(0.6, 1.8)
			return


func beende() -> void:
	if not laeuft:
		return
	laeuft = false
	_sperre = _jetzt() + 8.0
	for s in _scheiben:
		_klappe(s, false)
	runden += 1
	var lohn := treffer_runde * PREIS
	var text := "Schießbude: %d Treffer – %d €" % [treffer_runde, lohn]
	if treffer_runde > Konto.beste_bude():
		Konto.setze_beste_bude(treffer_runde)
		text += "  (neue Bestleistung!)"
	if lohn > 0:
		Konto.gutschrift(lohn)
	Schaden.rueckmeldung(get_tree(), text)
	print("LALABERG_BUDE ende treffer=%d lohn=%d beste=%d" % [
			treffer_runde, lohn, Konto.beste_bude()])


func _klappe(s: Dictionary, hoch: bool) -> void:
	(s["knoten"] as Node3D).visible = hoch
	(s["form"] as CollisionShape3D).set_deferred("disabled", not hoch)
	if hoch:
		s["oben_bis"] = _jetzt() + OBEN_S


func hud_text() -> String:
	if not laeuft:
		return ""
	return "SCHIESSBUDE  %d s   Treffer %d   (Bestleistung %d)" % [
			ceili(_rest), treffer_runde, Konto.beste_bude()]


func _process(delta: float) -> void:
	if not _aufgestellt:
		return
	var jetzt := _jetzt()
	if laeuft:
		_rest -= delta
		var oben := hochstehende().size()
		for s in _scheiben:
			var k := s["knoten"] as Node3D
			if k.visible and jetzt > s["oben_bis"] and not test_oben:
				_klappe(s, false)
				s["naechste"] = jetzt + randf_range(0.5, 1.6)
			elif not k.visible and jetzt > s["naechste"] and oben < 3:
				_klappe(s, true)
				oben += 1
		if _rest <= 0.0:
			beende()
		return
	_takt -= delta
	if _takt > 0.0 or fokus == null or jetzt < _sperre:
		return
	_takt = 0.25
	var ab := fokus.global_position - _stand
	if fokus is Spieler and Vector2(ab.x, ab.z).length() < 4.0 and absf(ab.y) < 3.0:
		starte()
