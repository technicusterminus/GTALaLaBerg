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

var geparkt := false   ## abgestellt: weit unter der Stadt, unsichtbar, ohne Route
var gesperrt := false  ## Sperrwagen: steht quer auf der Strasse, Blaulicht an

var _blick := Vector3(1, 0, 0)
var _rechts := Vector3(0, 0, 1)
var _netz := MeshInstance3D.new()
var _lage := Vector3.ZERO   ## zuletzt gesetzte Position, sofort gueltig (siehe position_jetzt)
var _geschwindigkeit := Vector3.ZERO
var _stoerung_bis := 0.0
var _faktor := 1.0
var _leuchten: Array[MeshInstance3D] = []
var _blaulicht := OmniLight3D.new()


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
		_setze_position(weg.ort)
	if polizei:
		_baue_blaulicht()


## Dachbalken: zwei flache Leuchten links und rechts, darueber ein Punktlicht, das
## im selben Takt blau und rot blitzt und die Strasse anstrahlt.
func _baue_blaulicht() -> void:
	for i in 2:
		var m := MeshInstance3D.new()
		var b := BoxMesh.new()
		b.size = Vector3(0.35, 0.12, 0.3)
		m.mesh = b
		m.position = Vector3(0.1, 1.2, -0.5 if i == 0 else 0.5)
		m.material_override = StandardMaterial3D.new()
		m.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		add_child(m)
		_leuchten.append(m)
	_blaulicht.position = Vector3(0.1, 1.7, 0)
	_blaulicht.omni_range = 22.0
	_blaulicht.light_energy = 0.0
	_blaulicht.shadow_enabled = false
	add_child(_blaulicht)


func _blinke() -> void:
	var an := not geparkt and (weg.gueltig() or gesperrt) and not ist_ausgeschaltet()
	if not an:
		_blaulicht.light_energy = 0.0
		for m in _leuchten:
			(m.material_override as StandardMaterial3D).emission_enabled = false
		return
	var links := fmod(Time.get_ticks_msec() / 1000.0, 0.5) < 0.25
	var farben := [Color(0.1, 0.25, 1.0), Color(1.0, 0.1, 0.05)]
	for i in 2:
		var mat := _leuchten[i].material_override as StandardMaterial3D
		var heller := (i == 0) == links
		var f: Color = farben[i]
		mat.albedo_color = f
		mat.emission_enabled = true
		mat.emission = f * (2.0 if heller else 0.15)
	_blaulicht.light_color = farben[0] if links else farben[1]
	_blaulicht.light_energy = 3.0


## Streifenwagen: dem Weg folgen und am letzten Punkt halten.
func folge_weg(punkte: PackedVector3Array, tempo_kmh: float) -> void:
	weg.route = punkte
	weg.index = 0
	weg.richtung = 1
	weg.rund = false
	weg.halten = true
	weg.am_ende = false
	tempo_ms = tempo_kmh / 3.6
	breiten = PackedFloat32Array()
	geparkt = false
	visible = true


## Weit weg stellen und aus dem Spiel nehmen; die Polizei holt den Wagen wieder.
func parke() -> void:
	weg.route = PackedVector3Array()
	geparkt = true
	gesperrt = false
	_geschwindigkeit = Vector3.ZERO
	visible = false
	_setze_position(Vector3(0, -4000, 0))


func setze_sperre(an: bool) -> void:
	gesperrt = an


func am_ziel() -> bool:
	return weg.am_ende or not weg.gueltig()


func ist_geparkt() -> bool:
	return geparkt


## Ort auf der Mitte der Route (ohne den Spurversatz).
func route_ort() -> Vector3:
	return weg.ort


func weg_index() -> int:
	return weg.index


func vorwaerts() -> Vector3:
	return _blick


## Stellt den Wagen an einen Ort und dreht ihn in eine Richtung.
func setze_lage(ort: Vector3, blick: Vector3) -> void:
	weg.ort = ort
	_setze_position(ort)
	var b := Vector3(blick.x, 0, blick.z)
	if b.length_squared() > 0.0001:
		_blick = b.normalized()
	_rechts = Vector3(-_blick.z, 0.0, _blick.x)
	rotation.y = -atan2(_blick.z, _blick.x)
	geparkt = false
	visible = true


## Bei einem AnimatableBody3D mit sync_to_physics liest global_position nach dem
## Setzen noch bis zum naechsten Physikbild den alten Wert. Wer die Position
## gleich nach dem Setzen braucht (die Polizei), fragt deshalb hier.
func position_jetzt() -> Vector3:
	return _lage


func _setze_position(ort: Vector3) -> void:
	_lage = ort
	global_position = ort


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
	if polizei:
		_blinke()
	if geparkt:
		return
	# Ohne Route (ausgeschaltet oder geparkt) bleibt der Wagen stehen, wo er ist.
	if ist_ausgeschaltet() or not weg.gueltig():
		_geschwindigkeit = Vector3.ZERO
		return
	# Nach einem Treffer kurz fast stehen bleiben, dann wieder auf Tempo; sonst
	# waere ein Treffer nur eine Farbe, kein Ereignis.
	var soll := 0.1 if Time.get_ticks_msec() / 1000.0 < _stoerung_bis else 1.0
	_faktor = move_toward(_faktor, soll, 3.0 * delta)
	var vorher := _lage
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
	_setze_position(weg.ort + _rechts * versatz)
	_geschwindigkeit = (_lage - vorher) / delta
	# Das Netz schaut nach +X; Drehung um Y bringt +X auf die Blickrichtung.
	rotation.y = -atan2(_blick.z, _blick.x)
