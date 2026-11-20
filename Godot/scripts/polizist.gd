class_name Polizist
extends CharacterBody3D
## Ein Beamter zu Fuss: ab drei Sternen steigen zwei aus einer Streife, die in der Naehe
## des Gesuchten steht, und laufen auf ihn zu. Wer erreicht wird, ist festgenommen. Mit dem
## Wagen davonfahren geht (sie laufen nur 5,5 m/s), Paintball-Treffer werfen sie kurz aus
## dem Tritt. Nach einer halben Minute geben sie auf.

const TEMPO := 5.5
const KONTAKT_M := 1.7
const LEBENSDAUER := 30.0
const UNIFORM := Color(0.10, 0.18, 0.42)

var polizei: Polizei
var kontakte := 0

var _figur: Figur
var _schwerkraft: float = ProjectSettings.get_setting("physics/3d/default_gravity")
var _stolpert_bis := 0.0
var _bis := 0.0


func _ready() -> void:
	add_to_group("verletzbar")
	add_to_group("fusspolizei")
	_bis = _jetzt() + LEBENSDAUER
	var kapsel := CollisionShape3D.new()
	var form := CapsuleShape3D.new()
	form.height = 1.8
	form.radius = 0.35
	kapsel.shape = form
	kapsel.position.y = 0.9
	add_child(kapsel)
	if Figur.verfuegbar():
		_figur = Figur.neu(1)
		add_child(_figur)
		_figur.passe_an()
	else:
		var m := MeshInstance3D.new()
		var k := CapsuleMesh.new()
		k.height = 1.75
		k.radius = 0.32
		m.mesh = k
		m.position.y = 0.875
		var mat := StandardMaterial3D.new()
		mat.albedo_color = UNIFORM
		m.material_override = mat
		add_child(m)


func _jetzt() -> float:
	return Time.get_ticks_msec() / 1000.0


func erhalte_farbe(_farbe: Color, _richtung: Vector3) -> void:
	_stolpert_bis = _jetzt() + 1.6
	Schaden.melde(get_tree(), Schaden.Tat.POLIZEI_BESCHOSSEN)


func verletze(_schaden: float, _richtung: Vector3, _art: Schaden.Art) -> void:
	_stolpert_bis = _jetzt() + 1.6


func _physics_process(delta: float) -> void:
	var jetzt := _jetzt()
	if jetzt > _bis or polizei == null or polizei.fokus == null:
		queue_free()
		return
	var ziel := polizei.fokus.global_position
	var weg := ziel - global_position
	var flach := Vector3(weg.x, 0, weg.z)
	var strauchelt := jetzt < _stolpert_bis
	var richtung := flach.normalized() if flach.length() > 0.2 and not strauchelt else Vector3.ZERO
	velocity.x = richtung.x * TEMPO
	velocity.z = richtung.z * TEMPO
	velocity.y = 0.0 if is_on_floor() else velocity.y - _schwerkraft * delta
	move_and_slide()
	if richtung != Vector3.ZERO:
		rotation.y = lerp_angle(rotation.y, atan2(-richtung.x, -richtung.z), minf(10.0 * delta, 1.0))
	if _figur != null:
		_figur.spiele("Run" if richtung != Vector3.ZERO else "HitRecieve")
	if flach.length() < KONTAKT_M and absf(weg.y) < 2.5 and not strauchelt:
		kontakte += 1
		polizei.kontakt()
