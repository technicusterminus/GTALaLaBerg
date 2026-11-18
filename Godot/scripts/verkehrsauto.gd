class_name Verkehrsauto
extends AnimatableBody3D
## Ein KI-Auto auf einer festen Route (Rundkurs). Es wird nicht von der Physik
## bewegt, schiebt aber Spieler und Wagen beiseite (sync_to_physics).
## Noch ohne Ampeln und Kreuzungsbremsen.

const HALBE_LAENGE := 2.15

const RUSS := Color(0.06, 0.055, 0.05)

var weg := Wegfolger.new()
var tempo_ms := 10.0
var leben := 100.0
var polizei := false   ## Streifenwagen: Treffer melden sich bei der Polizei
var breiten := PackedFloat32Array()  ## Fahrbahnbreite je Wegpunkt in Metern
var lack := Color(0.5, 0.5, 0.5)

var _blick := Vector3(1, 0, 0)
var _rechts := Vector3(0, 0, 1)
var _netz := MeshInstance3D.new()
var _geschwindigkeit := Vector3.ZERO
var _stoerung_bis := 0.0
var _faktor := 1.0


func _ready() -> void:
	sync_to_physics = true
	add_to_group("verkehrsauto")
	add_to_group("verletzbar")
	var form := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = Vector3(4.3, 1.0, 1.8)
	form.shape = box
	form.position = Vector3(0, 0.55, 0)
	add_child(form)
	_netz.mesh = Wagenmesh.netz(lack)
	_netz.visibility_range_end = 500.0
	add_child(_netz)
	if weg.gueltig():
		global_position = weg.ort


func geschwindigkeit_ms() -> Vector3:
	return _geschwindigkeit


func ist_ausgeschaltet() -> bool:
	return leben <= 0.0


func lebensanteil() -> float:
	return clampf(leben / 100.0, 0.0, 1.0)


func setze_lack(farbe: Color) -> void:
	lack = farbe
	_netz.mesh = Wagenmesh.netz(lack)


## Ein Paintball beult nicht, aber er nimmt Sicht und Lack: zehn Treffer setzen
## einen Wagen ausser Gefecht. Sonst liesse sich eine Verfolgung nur durch Rammen
## beenden.
func erhalte_farbe(_farbe: Color, richtung: Vector3) -> void:
	verletze(11.0, richtung, Schaden.Art.BESCHUSS)
	if polizei:
		Schaden.melde(get_tree(), Schaden.Tat.POLIZEI_BESCHOSSEN)
		return
	_stoerung_bis = Time.get_ticks_msec() / 1000.0 + 1.4


func verletze(schaden: float, _richtung: Vector3, art: Schaden.Art) -> void:
	if ist_ausgeschaltet():
		return
	leben -= schaden
	if leben > 0.0:
		return
	# Stehenbleiben und russig werden: die Route endet hier, der Wagen bleibt als
	# Hindernis stehen, wo er getroffen wurde.
	leben = 0.0
	weg.route = PackedVector3Array()
	_geschwindigkeit = Vector3.ZERO
	setze_lack(RUSS)
	print("LALABERG_AUTO ausgeschaltet art=%d bei %s" % [art, global_position])


func _physics_process(delta: float) -> void:
	# Ohne Route (ausgeschaltet oder geparkt) bleibt der Wagen stehen, wo er ist.
	if ist_ausgeschaltet() or not weg.gueltig():
		_geschwindigkeit = Vector3.ZERO
		return
	# Nach einem Treffer kurz fast stehen bleiben, dann wieder auf Tempo; sonst
	# waere ein Treffer nur eine Farbe, kein Ereignis.
	var soll := 0.1 if Time.get_ticks_msec() / 1000.0 < _stoerung_bis else 1.0
	_faktor = move_toward(_faktor, soll, 3.0 * delta)
	var vorher := global_position
	weg.bewege(tempo_ms * _faktor * delta)
	if weg.blick != Vector3.ZERO:
		_blick = _blick.lerp(weg.blick, minf(8.0 * delta, 1.0)).normalized()
	var rechts_soll := Vector3(-_blick.z, 0.0, _blick.x)
	_rechts = _rechts.lerp(rechts_soll, minf(6.0 * delta, 1.0)).normalized()
	var breite := 6.0
	if weg.index < breiten.size():
		breite = breiten[weg.index]
	# Rechtsverkehr: eine Spur rechts von der Strassenmitte
	var versatz := clampf(breite / 4.0, 0.9, 2.2)
	global_position = weg.ort + _rechts * versatz
	_geschwindigkeit = (global_position - vorher) / delta
	# Das Netz schaut nach +X; Drehung um Y bringt +X auf die Blickrichtung.
	rotation.y = -atan2(_blick.z, _blick.x)
