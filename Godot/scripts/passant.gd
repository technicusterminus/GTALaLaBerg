class_name Passant
extends AnimatableBody3D
## Ein Fussgaenger auf einer festen Route, hin und zurueck. Vorerst eine Kapsel
## als Platzhalter; echte Figuren folgen mit dem Modellimport. Treffer lassen ihn
## stolpern, zehn Paintballs oder ein Anprall legen ihn um, nach einer Weile
## steht er wieder auf. Gemeldet wird jede Tat an die Polizei.

const KLEIDER := [
	Color(0.62, 0.15, 0.12), Color(0.15, 0.30, 0.55), Color(0.20, 0.45, 0.25),
	Color(0.75, 0.62, 0.20), Color(0.30, 0.30, 0.34), Color(0.55, 0.35, 0.60),
]
## Wie viele Passanten gleichzeitig eine ausmodellierte, animierte Figur tragen (nahe
## am Betrachter); alle anderen bleiben Kapseln. Haelt die Zeichenaufrufe klein.
const FIGUR_NAH := 55.0
const FIGUR_FERN := 80.0
const FIGUR_MAX := 30

static var _figuren_zahl := 0
static var _mesh: CapsuleMesh
static var _materialien: Array[StandardMaterial3D] = []

var weg := Wegfolger.new()
var tempo_ms := 1.3
var farbe := 0
var leben := 100.0

var _gier := 0.0
var _panik_bis := 0.0       ## bis dahin rennt die Figur vor Schuessen davon
var _pause_bis := 0.0       ## bis dahin bleibt sie stehen (schaut, telefoniert ...)
var _naechste_pause := randf_range(15.0, 50.0)
var _versatz := Vector3.ZERO   ## seitliches Ausweichen vor Autos
var _versatz_ziel := Vector3.ZERO
var _stolpert_bis := 0.0
var _liegt_bis := 0.0
var _instanz := MeshInstance3D.new()
var _figur: Figur
var _lod_takt := randf() * 0.6


func _ready() -> void:
	sync_to_physics = true
	add_to_group("verletzbar")
	add_to_group("passanten")
	if _mesh == null:
		_mesh = CapsuleMesh.new()
		_mesh.height = 1.75
		_mesh.radius = 0.3
		for c: Color in KLEIDER:
			var m := StandardMaterial3D.new()
			m.albedo_color = c
			_materialien.append(m)
	var form := CollisionShape3D.new()
	var kapsel := CapsuleShape3D.new()
	kapsel.height = 1.75
	kapsel.radius = 0.3
	form.shape = kapsel
	form.position.y = 0.875
	add_child(form)
	_instanz.mesh = _mesh
	_instanz.material_override = _materialien[farbe % _materialien.size()]
	_instanz.position.y = 0.875
	_instanz.visibility_range_end = 250.0
	add_child(_instanz)
	if weg.gueltig():
		_setze_lage(weg.ort)


func hat_figur() -> bool:
	return _figur != null


## Nahe an der Kamera tritt die Kapsel zugunsten der animierten Figur zurueck.
func _pruefe_figur(delta: float) -> void:
	_lod_takt -= delta
	if _lod_takt > 0.0 or not Figur.verfuegbar():
		return
	_lod_takt = 0.6
	var kam := get_viewport().get_camera_3d()
	if kam == null:
		return
	var abstand := global_position.distance_to(kam.global_position)
	if _figur == null and abstand < FIGUR_NAH and _figuren_zahl < FIGUR_MAX:
		_figur = Figur.neu(farbe)
		add_child(_figur)
		_figur.passe_an()
		_figur.einkleiden(get_instance_id() % 100003)
		_figuren_zahl += 1
		_instanz.visible = false
	elif _figur != null and abstand > FIGUR_FERN:
		_figur.queue_free()
		_figur = null
		_figuren_zahl -= 1
		_instanz.visible = true


func _exit_tree() -> void:
	if _figur != null:
		_figuren_zahl -= 1
		_figur = null


func _animiere(jetzt: float) -> void:
	if _figur == null:
		return
	if ist_ausgeschaltet():
		_figur.spiele("Death")
	elif jetzt < _stolpert_bis:
		_figur.spiele("HitRecieve")
	elif in_panik():
		_figur.spiele("Run", 1.1)
	elif jetzt < _pause_bis:
		_figur.spiele("Idle")
	else:
		_figur.spiele("Walk", clampf(tempo_ms / 1.3, 0.6, 1.3))


## Ein Schuss bei ort: wer naeher als 32 m steht, rennt davon, weg vom Knall.
func hoere_schuss(ort: Vector3) -> void:
	if ist_ausgeschaltet():
		return
	var weg_vom_knall := global_position - ort
	if weg_vom_knall.length() > 32.0:
		return
	_panik_bis = _jetzt() + randf_range(6.0, 10.0)
	_pause_bis = 0.0
	# Auf der Route in die Richtung laufen, die vom Knall wegfuehrt.
	if weg.blick != Vector3.ZERO and weg.blick.dot(weg_vom_knall) < 0.0:
		weg.richtung = -weg.richtung


func in_panik() -> bool:
	return _jetzt() < _panik_bis


## Naht ein Auto schnell, tritt die Figur zur Seite (weg von seiner Fahrlinie).
func _weiche_aus() -> void:
	_versatz_ziel = Vector3.ZERO
	for gruppe in ["verkehrsauto", "auto"]:
		for a: Node3D in get_tree().get_nodes_in_group(gruppe):
			var v: Vector3 = a.geschwindigkeit_ms()
			if v.length() < 3.0:
				continue
			var zu_mir := global_position - a.global_position
			var flach := Vector3(zu_mir.x, 0, zu_mir.z)
			if flach.length() > 9.0 or absf(zu_mir.y) > 3.0:
				continue
			var fahrt := Vector3(v.x, 0, v.z).normalized()
			if fahrt.dot(flach.normalized()) < 0.3:
				continue   # faehrt nicht auf mich zu
			var quer := Vector3(-fahrt.z, 0, fahrt.x)
			_versatz_ziel = quer * (1.8 if quer.dot(flach) >= 0.0 else -1.8)
			return


func ist_ausgeschaltet() -> bool:
	return leben <= 0.0


func lebensanteil() -> float:
	return clampf(leben / 100.0, 0.0, 1.0)


## Paintball-Treffer: kurz stolpern, die Polizei erfaehrt davon. Ein Paintball
## tut weh, mehr nicht; erst der zehnte legt jemanden um.
func erhalte_farbe(_farbe: Color, richtung: Vector3) -> void:
	_stolpert_bis = _jetzt() + 1.6
	Schaden.melde(get_tree(), Schaden.Tat.PASSANT_BESCHOSSEN)
	verletze(11.0, richtung, Schaden.Art.BESCHUSS)


func verletze(schaden: float, richtung: Vector3, art: Schaden.Art) -> void:
	if ist_ausgeschaltet():
		return
	leben -= schaden
	var dauer := 2.5 if art == Schaden.Art.ANPRALL else 1.6
	_stolpert_bis = maxf(_stolpert_bis, _jetzt() + dauer)
	if leben > 0.0:
		return
	# Umfallen: zur Seite legen, in Stossrichtung ein Stueck rutschen und eine
	# Weile liegen bleiben. Kein Ragdoll, die Figur haengt an einer Route.
	leben = 0.0
	_liegt_bis = _jetzt() + 14.0
	var stoss := Vector3(richtung.x, 0.0, richtung.z)
	stoss = stoss.normalized() if stoss.length_squared() > 0.0001 else Vector3.FORWARD
	_gier = atan2(-stoss.x, -stoss.z)
	_instanz.rotation = Vector3(0, 0, deg_to_rad(86.0))
	_instanz.position.y = 0.3
	weg.ort += stoss * 0.7
	_setze_lage(weg.ort)
	print("LALABERG_PASSANT umgefallen art=%d" % art)


## Bei einem AnimatableBody3D mit sync_to_physics die ganze Transformation auf
## einmal setzen (siehe Verkehrsauto._setze_transform).
func _setze_lage(ort: Vector3) -> void:
	global_transform = Transform3D(Basis(Vector3.UP, _gier), ort)


func _jetzt() -> float:
	return Time.get_ticks_msec() / 1000.0


func _physics_process(delta: float) -> void:
	var jetzt := _jetzt()
	_pruefe_figur(delta)
	_animiere(jetzt)
	_pruefe_anprall(jetzt)
	if ist_ausgeschaltet():
		if jetzt < _liegt_bis:
			return
		leben = 100.0
		_instanz.rotation = Vector3.ZERO
		_instanz.position.y = 0.875
	if not weg.gueltig():
		return
	var faktor := 0.15 if jetzt < _stolpert_bis else 1.0
	if in_panik():
		faktor = 3.2   # rennen
	elif jetzt < _pause_bis:
		faktor = 0.0
	else:
		_naechste_pause -= delta
		if _naechste_pause <= 0.0:
			_naechste_pause = randf_range(20.0, 60.0)
			_pause_bis = jetzt + randf_range(2.5, 7.0)
	_weiche_aus()
	_versatz = _versatz.lerp(_versatz_ziel, minf(4.0 * delta, 1.0))
	weg.bewege(tempo_ms * faktor * delta)
	_setze_lage(weg.ort + _versatz)


## Angefahren: der Spieler sitzt in einem Wagen und ist schneller als Schritttempo
## bis auf Stossstangenbreite heran. Schaden nach Tempo: ab 30 km/h wird es
## ernst, ab 70 legt es jeden um.
func _pruefe_anprall(jetzt: float) -> void:
	if jetzt < _stolpert_bis:
		return
	for a: Node3D in get_tree().get_nodes_in_group("spielerauto"):
		var v: Vector3 = a.geschwindigkeit_ms()
		var weg_zum := global_position - a.global_position
		if v.length() <= 4.0 or Vector2(weg_zum.x, weg_zum.z).length() >= 2.6 \
				or absf(weg_zum.y) >= 2.5:
			continue
		Schaden.melde(get_tree(), Schaden.Tat.PASSANT_ANGEFAHREN)
		var kmh := v.length() * 3.6
		verletze(remap(clampf(kmh, 15.0, 70.0), 15.0, 70.0, 12.0, 110.0),
				Vector3(weg_zum.x, 0, weg_zum.z).normalized(), Schaden.Art.ANPRALL)
		return
