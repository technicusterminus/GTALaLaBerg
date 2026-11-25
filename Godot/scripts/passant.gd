class_name Passant
extends AnimatableBody3D
## Ein Fussgaenger auf einer festen Route, hin und zurueck. Nahe der Kamera
## läuft eine animierte Figur, in der Ferne eine einfachere Darstellung. Treffer lassen ihn
## stolpern, zehn Paintballs oder ein Anprall legen ihn um, nach einer Weile
## steht er wieder auf. Gemeldet wird jede Tat an die Polizei.

const KLEIDER := [
	Color("#a45049"), Color("#465c7b"), Color("#3f664e"), Color("#bd9a53"),
	Color("#53545e"), Color("#806288"), Color("#6e7f77"), Color("#ad7053"),
	Color("#435f66"), Color("#b69286"), Color("#5c5844"), Color("#7f504e"),
]
const HOSEN := [Color("#3c424a"), Color("#53566a"), Color("#665d50"), Color("#3f5759")]
const HAAR := [Color("#473527"), Color("#332a29"), Color("#795d40"), Color("#9a8070")]
## Wie viele Passanten gleichzeitig eine ausmodellierte, animierte Figur tragen (nahe
## am Betrachter); alle anderen bleiben eine Silhouette in einem Zeichenaufruf.
const FIGUR_NAH := 55.0
const FIGUR_FERN := 80.0
const FIGUR_MAX := 30

static var _figuren_zahl := 0
static var _mesh: ArrayMesh
static var _materialien: Array[StandardMaterial3D] = []

var weg := Wegfolger.new()
var tempo_ms := 1.3
var farbe := 0
var leben := 100.0

var _gier := 0.0
var _stolpert_bis := 0.0
var _liegt_bis := 0.0
var _instanz := MeshInstance3D.new()
var _figur: Figur
var _lod_takt := randf() * 0.6


func _ready() -> void:
	sync_to_physics = true
	add_to_group("verletzbar")
	if _mesh == null:
		_mesh = _baue_silhouette()
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


func _baue_silhouette() -> ArrayMesh:
	var kugel := SphereMesh.new()
	kugel.radial_segments = 6
	kugel.rings = 2
	kugel.radius = 0.5
	kugel.height = 1.0
	var teile := [
		[Vector3(0, 0.676, 0), Vector3(0.25, 0.295, 0.25)],
		[Vector3(0, 0.24, 0), Vector3(0.45, 0.65, 0.27)],
		[Vector3(-0.24, 0.13, 0.015), Vector3(0.18, 0.59, 0.17)],
		[Vector3(0.24, 0.13, -0.015), Vector3(0.18, 0.59, 0.17)],
		[Vector3(-0.103, -0.41, -0.042), Vector3(0.197, 0.94, 0.21)],
		[Vector3(0.103, -0.41, 0.042), Vector3(0.197, 0.94, 0.21)],
	]
	var netz := SurfaceTool.new()
	netz.begin(Mesh.PRIMITIVE_TRIANGLES)
	for teil: Array in teile:
		netz.append_from(kugel, 0, Transform3D(Basis.from_scale(teil[1]), teil[0]))
	netz.set_material(StandardMaterial3D.new())
	return netz.commit()


## Nahe an der Kamera tritt die Silhouette zugunsten der animierten Figur zurueck.
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
		_figur = Figur.neu(1 if farbe % 7 == 5 else 0)
		if _figur == null:
			return
		add_child(_figur)
		_figur.passe_an()
		_figur.scale = Vector3.ONE * (0.92 + float(farbe * 13 % 17) * 0.011)
		var jacke: Color = KLEIDER[farbe % KLEIDER.size()]
		_figur.faerbe({
			"LightBrown": jacke,
			"LightBlue": HOSEN[farbe * 3 % HOSEN.size()],
			"Worker_Vest": jacke.lightened(0.15),
			"Hair": HAAR[floori(farbe / 3.0) % HAAR.size()],
		})
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
	else:
		_figur.spiele("Walk", clampf(tempo_ms / 1.3, 0.6, 1.3))


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
	weg.bewege(tempo_ms * faktor * delta)
	if weg.blick.length_squared() > 0.0:
		var richtung := atan2(-weg.blick.x, -weg.blick.z)
		_gier = lerp_angle(_gier, richtung, 1.0 - exp(-9.0 * delta))
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
