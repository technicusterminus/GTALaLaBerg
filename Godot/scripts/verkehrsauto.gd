class_name Verkehrsauto
extends AnimatableBody3D
## Ein KI-Auto auf einer festen Route (Rundkurs). Es wird nicht von der Physik
## bewegt, schiebt aber Spieler und Wagen beiseite (sync_to_physics).
## Noch ohne Ampeln und Kreuzungsbremsen.

const HALBE_LAENGE := 2.15

var weg := Wegfolger.new()
var tempo_ms := 10.0
var breiten := PackedFloat32Array()  ## Fahrbahnbreite je Wegpunkt in Metern
var lack := Color(0.5, 0.5, 0.5)

var _blick := Vector3(1, 0, 0)
var _rechts := Vector3(0, 0, 1)


func _ready() -> void:
	sync_to_physics = true
	add_to_group("verkehrsauto")
	var form := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = Vector3(4.3, 1.0, 1.8)
	form.shape = box
	form.position = Vector3(0, 0.55, 0)
	add_child(form)
	var netz := MeshInstance3D.new()
	netz.mesh = Wagenmesh.netz(lack)
	netz.visibility_range_end = 500.0
	add_child(netz)
	global_position = weg.ort


func _physics_process(delta: float) -> void:
	weg.bewege(tempo_ms * delta)
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
	# Das Netz schaut nach +X; Drehung um Y bringt +X auf die Blickrichtung.
	rotation.y = -atan2(_blick.z, _blick.x)
