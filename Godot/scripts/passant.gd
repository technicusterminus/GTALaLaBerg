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

static var _mesh: CapsuleMesh
static var _materialien: Array[StandardMaterial3D] = []

var weg := Wegfolger.new()
var tempo_ms := 1.3
var farbe := 0
var leben := 100.0

var _gier := 0.0
var _stolpert_bis := 0.0
var _liegt_bis := 0.0
var _instanz := MeshInstance3D.new()


func _ready() -> void:
	sync_to_physics = true
	add_to_group("verletzbar")
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
	weg.bewege(tempo_ms * faktor * delta)
	_setze_lage(weg.ort)


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
