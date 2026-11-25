class_name Auto
extends VehicleBody3D
## Ein fahrbares Auto. Die Karosserie kommt aus Fahrzeug/wagen.json (Quelle der
## Unreal-Version); die Fahrphysik ist Godots VehicleBody3D.
## Vorwaerts ist in Godot -Z, die Quelldaten schauen nach +X: die Huelle ist
## deshalb um 90 Grad gedreht.

const MOTOR := 7500.0
const RUECKWAERTS := 4000.0
const BREMSE := 45.0
const HOECHST_MS := 48.0
## Fahrwerk je Modell: Federhaerte, Druck- und Zugstufe, Federweg, Rollneigung. Sportler hart
## und tief, Bus weich mit viel Weg; der Kleinwagen behaelt die erprobten Werte.
const FAHRWERK := {
	"klein": [45.0, 2.2, 2.6, 0.25, 0.35],
	"limousine": [40.0, 2.4, 2.8, 0.22, 0.4],
	"kombi": [38.0, 2.3, 2.7, 0.24, 0.4],
	"bus": [28.0, 1.8, 2.2, 0.28, 0.5],
	"sport": [60.0, 3.0, 3.4, 0.16, 0.25],
	"super": [70.0, 3.2, 3.6, 0.14, 0.2],
	"renn": [90.0, 3.6, 4.0, 0.10, 0.15],
	"motorrad": [55.0, 2.8, 3.2, 0.14, 0.1],
	"fahrrad": [40.0, 2.4, 2.8, 0.05, 0.1],
}

@export var lack := Color(0.55, 0.06, 0.05)
## Fahrzeugmodell (Wagenmesh.MODELLE); bestimmt Karosserie, Raeder und Radstand.
@export var modell := "klein"
## Ein gestohlenes KI-Auto behaelt seinen Lack statt den des eigenen Wagens.
var uebernommen := false

## Wer gerade faehrt; solange jemand am Steuer sitzt, zaehlt der Wagen fuer
## Anprall und Polizei als "spielerauto".
var fahrer: Node3D:
	set(neu):
		fahrer = neu
		if neu != null:
			add_to_group("spielerauto")
		else:
			remove_from_group("spielerauto")
var tempo_ms := 0.0
var gesperrt := false   ## im Film: keine Eingabe, der Wagen bremst
var leben := 100.0
var _letzter_stoss := -10.0
var _lenkung := 0.0
var _motor: AudioStreamPlayer3D
var _karosserie: MeshInstance3D
var _bremse_an := -1.0
var _bremsen: Array = []   ## [VehicleWheel3D, Bremse]: die Bremse dreht nicht mit dem Rad


func _ready() -> void:
	add_to_group("auto")
	add_to_group("verletzbar")
	if Konto.hat_lack() and not uebernommen:
		lack = Konto.lack()
	mass = 1300.0
	if modell != "fahrrad":
		_motor = Ton.schleife(self, "SFX_Motor", -4.0)
	center_of_mass_mode = RigidBody3D.CENTER_OF_MASS_MODE_CUSTOM
	center_of_mass = Vector3(0, -0.15, 0)
	linear_damp = 0.05
	angular_damp = 1.0

	var form := CollisionShape3D.new()
	var box := BoxShape3D.new()
	var masse := Wagenmesh.masse(modell)
	var einspurig := Wagenmesh.ist_einspurig(modell)
	box.size = Vector3(maxf(masse.y, 0.5), 0.9, masse.x) if modell != "klein" else Vector3(1.8, 0.9, 4.3)
	form.shape = box
	form.position = Vector3(0, 0.45, 0)
	add_child(form)

	# Das Netz schaut nach +X, rechts ist +Z; um 90 Grad gedreht schaut es nach -Z (vorn)
	# und +Z wird +X (rechts). Die Radmitten liegen auf y = 0 des Wagens.
	var lagen := Wagenmesh.radlagen(modell)
	var radius := 0.315 if lagen.is_empty() else (lagen[0] as Vector4).w
	var huelle := Node3D.new()
	huelle.rotation.y = PI / 2.0
	huelle.position.y = -radius
	huelle.add_child(_baue_karosserie())
	add_child(huelle)

	# Radstand und Spur aus dem Profil des Modells (sonst die alten Werte: +-131 cm, 76 cm).
	for k in 4:
		var vorne := k < 2
		var links := k % 2 == 0
		var ort := Vector3(-0.76 if links else 0.76, 0.0, -1.31 if vorne else 1.31)
		# Zweirad: zwei Radlagen; die Physik faehrt auf vier eng stehenden Raedern (stabil),
		# zu sehen ist je Achse ein Rad in der Mitte.
		var lage_nr := k / 2 if einspurig else k
		var sichtbar := lage_nr < lagen.size() and (not einspurig or links)
		if lage_nr < lagen.size():
			var l: Vector4 = lagen[lage_nr]
			ort = Vector3(l.z, 0.0, -l.x)
			if einspurig:
				ort.x = -0.14 if links else 0.14
		var rad := VehicleWheel3D.new()
		rad.position = ort
		var fw: Array = FAHRWERK.get(modell, FAHRWERK["klein"])
		rad.wheel_radius = radius
		rad.suspension_travel = fw[3]
		rad.suspension_stiffness = fw[0]
		rad.suspension_max_force = 9000.0
		rad.wheel_rest_length = 0.12
		rad.damping_compression = fw[1]
		rad.damping_relaxation = fw[2]
		rad.wheel_friction_slip = 3.2
		rad.wheel_roll_influence = fw[4]
		rad.use_as_steering = vorne
		rad.use_as_traction = true
		add_child(rad)
		# Das Radnetz (Achse entlang Z) dreht sich mit dem Rad; Godot dreht und lenkt
		# den Radknoten selbst.
		if sichtbar:
			var nr := lage_nr if einspurig else k / 2
			var netz := MeshInstance3D.new()
			netz.mesh = Wagenmesh.rad_netz(modell, nr, links and not einspurig)
			netz.rotation.y = PI / 2.0
			netz.position.x = -ort.x if einspurig else 0.0
			rad.add_child(netz)
			var bremse := MeshInstance3D.new()
			bremse.mesh = Wagenmesh.bremse_netz(modell, nr, links and not einspurig)
			bremse.position.x = netz.position.x
			rad.add_child(bremse)
			_bremsen.append([rad, bremse])


func ist_ausgeschaltet() -> bool:
	return leben <= 0.0


func lebensanteil() -> float:
	return clampf(leben / 100.0, 0.0, 1.0)


## Wer den Werkstattschluessel hat, faehrt einen verstaerkten Wagen: der Schaden
## kommt nur zu zwei Dritteln an. Bei null Leben ist der Motor hin.
func verletze(schaden: float, _richtung: Vector3, art: Schaden.Art) -> void:
	if ist_ausgeschaltet():
		return
	if Konto.hat_fund(Konto.Fund.WERKSTATT):
		schaden *= 0.66
	leben -= schaden
	if fahrer != null:
		Schaden.rueckmeldung(get_tree(), "Wagen beschädigt – %d %%" % ceili(leben)
				if leben > 0.0 else "Motor hin – aussteigen mit F")
	if leben > 0.0:
		return
	leben = 0.0
	engine_force = 0.0
	setze_lack(Color(0.06, 0.055, 0.05))
	print("LALABERG_WAGEN ausgeschaltet art=%d" % art)


## Auffahren, Abdraengen, frontal ineinander: was zaehlt, ist die Annaeherungs-
## geschwindigkeit, nicht das eigene Tempo. Beide nehmen denselben Schaden.
## Gemessen ueber Abstand und Richtung statt ueber Kollisionsereignisse: die
## KI-Autos bewegen sich kinematisch und melden keinen physikalischen Stoss.
func _pruefe_zusammenstoss() -> void:
	var jetzt := Time.get_ticks_msec() / 1000.0
	if ist_ausgeschaltet() or jetzt - _letzter_stoss < 1.0:
		return
	var fahrt := linear_velocity
	for gruppe in ["verkehrsauto", "auto"]:
		for a: Node3D in get_tree().get_nodes_in_group(gruppe):
			if a == self:
				continue
			var weg := a.global_position - global_position
			# Zwei Wagen sind je rund 4,5 m lang: naeher als 4,2 m zwischen den
			# Mittelpunkten stehen sie nur, wenn es gekracht hat.
			if Vector2(weg.x, weg.z).length_squared() > 4.2 * 4.2 or absf(weg.y) > 3.0:
				continue
			var richtung := Vector3(weg.x, 0, weg.z).normalized()
			var andere_fahrt: Vector3 = a.geschwindigkeit_ms()
			var annaeherung: float = (fahrt - andere_fahrt).dot(richtung) * 3.6
			if annaeherung < 15.0:
				continue
			_letzter_stoss = jetzt
			# 15 km/h kosten ein paar Prozent, ab 90 km/h ist der Wagen hin.
			var schaden := remap(clampf(annaeherung, 15.0, 90.0), 15.0, 90.0, 5.0, 100.0)
			verletze(schaden, -richtung, Schaden.Art.ANPRALL)
			a.verletze(schaden, richtung, Schaden.Art.ANPRALL)
			print("LALABERG_STOSS tempo=%.0f schaden=%.0f gegen=%s" % [annaeherung, schaden, a.name])
			return


## Der Motor laeuft mit Fahrer; Tonhoehe und Lautstaerke folgen dem Tempo.
func _motorklang() -> void:
	if _motor == null:
		return
	if fahrer == null:
		if _motor.playing:
			_motor.stop()
		return
	if not _motor.playing:
		_motor.play()
	_motor.pitch_scale = 0.75 + clampf(tempo_ms / 28.0, 0.0, 1.0) * 1.1
	_motor.volume_db = -9.0 + clampf(tempo_ms / 28.0, 0.0, 1.0) * 7.0


## Der Radknoten dreht sich um seine Achse; Scheibe und Sattel sollen nur lenken. Ihre Lage
## im Radknoten ist deshalb dessen Drehung rueckgaengig gemacht und nur die Lenkung behalten.
func _process(_delta: float) -> void:
	for paar: Array in _bremsen:
		var rad: VehicleWheel3D = paar[0]
		var bremse: MeshInstance3D = paar[1]
		var soll := Basis(Vector3.UP, rad.steering) * Basis(Vector3.UP, PI / 2.0)
		bremse.transform.basis = rad.transform.basis.inverse() * soll


func _physics_process(delta: float) -> void:
	tempo_ms = linear_velocity.length()
	_motorklang()
	_pruefe_zusammenstoss()
	var vorwaerts_ms := -global_basis.z.dot(linear_velocity)
	if fahrer == null:
		engine_force = 0.0
		brake = BREMSE * 0.2
		_bremslicht(0.0)
		steering = move_toward(steering, 0.0, delta)
		return
	var gas := 0.0 if gesperrt else Input.get_axis("rueckwaerts", "vorwaerts")
	var lenk := 0.0 if gesperrt else Input.get_axis("rechts", "links")
	var bremse := 0.0
	var kraft := 0.0
	if gas > 0.0:
		kraft = gas * MOTOR
	elif gas < 0.0:
		if vorwaerts_ms > 1.0:
			bremse = BREMSE * -gas
		else:
			kraft = gas * RUECKWAERTS
	if tempo_ms > HOECHST_MS:
		kraft = 0.0
	if Input.is_action_pressed("handbremse") or gesperrt:
		bremse = BREMSE * 1.5
	if ist_ausgeschaltet():
		kraft = 0.0
	engine_force = kraft
	brake = bremse
	_bremslicht(1.0 if bremse > 0.0 else 0.0)
	var maximal := lerpf(0.5, 0.1, clampf(tempo_ms / 30.0, 0.0, 1.0))
	_lenkung = move_toward(_lenkung, lenk * maximal, 2.5 * delta)
	steering = _lenkung


func _bremslicht(an: float) -> void:
	if _karosserie != null and an != _bremse_an:
		_bremse_an = an
		_karosserie.set_instance_shader_parameter("bremse", an)


func geschwindigkeit_ms() -> Vector3:
	return linear_velocity


## Ein Paintball-Treffer faerbt den fahrbaren Wagen um.
func erhalte_farbe(farbe: Color, _richtung: Vector3) -> void:
	setze_lack(farbe)


func setze_lack(farbe: Color) -> void:
	lack = farbe
	if _karosserie != null:
		_karosserie.mesh = Wagenmesh.netz(lack, false, modell, false)


func _baue_karosserie() -> MeshInstance3D:
	_karosserie = MeshInstance3D.new()
	_karosserie.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_DOUBLE_SIDED
	_karosserie.mesh = Wagenmesh.netz(lack, false, modell, false)
	return _karosserie
