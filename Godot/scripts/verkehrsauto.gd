class_name Verkehrsauto
extends AnimatableBody3D
## Ein KI-Auto auf einer festen Route (Rundkurs). Es wird nicht von der Physik
## bewegt, schiebt aber Spieler und Wagen beiseite (sync_to_physics). Es bremst
## vor Autos, Ampeln, Kurven, dem Spieler und wartepflichtigen Kreuzungen (siehe
## Verkehrsbremse), ueberholt Hindernisse und macht Platz fuer Streifen. Die Raeder drehen
## und lenken, der Aufbau nickt und neigt sich, Brems- und Blinklichter kommen aus der Textur.

const HALBE_LAENGE := 2.15
const HALBE_WAGENBREITE := 0.88
const MAX_SEITVERSATZ := 2.2   ## so weit weicht ein Wagen einem Hindernis nach rechts aus

const RUSS := Color(0.06, 0.055, 0.05)

## Kreuzungen aus dem Strassengraphen (verkehr.json): Ort in Meter (x, z), Klasse
## der Strasse, Breite; dazu je Kreuzung die Autos, deren Route sie kreuzt.
static var kreuzung_orte := PackedVector2Array()
static var kreuzung_klassen := PackedInt32Array()
static var kreuzung_breiten := PackedFloat32Array()
static var kreuzung_autos := {}
## Wer gesteuert wird (Figur oder Wagen): davor bremsen die KI-Autos.
static var fokus: Node3D

var weg := Wegfolger.new()
var tempo_ms := 10.0
var leben := 100.0
var polizei := false   ## Streifenwagen: Treffer melden sich bei der Polizei
var breiten := PackedFloat32Array()  ## Fahrbahnbreite je Wegpunkt in Metern
var klasse := 3                      ## Strassenklasse, niedriger heisst wichtiger
var klassen := PackedInt32Array()    ## Klasse je Wegpunkt (die Route verkettet Strassen)
var kreuzungen := PackedInt32Array() ## Kreuzungen auf der Route
var lack := Color(0.5, 0.5, 0.5)
var karosserie := Vector3.ONE   ## Laenge, Breite, Hoehe gegenueber dem Grundmodell
var modell := "klein"          ## Fahrzeugmodell (Wagenmesh.MODELLE)
var ueberholungen := 0   ## wie oft dieser Wagen schon ein Hindernis ueberholt hat

var geparkt := false   ## abgestellt: weit unter der Stadt, unsichtbar, ohne Route
var gesperrt := false  ## Sperrwagen: steht quer auf der Strasse, Blaulicht an

var _blick := Vector3(1, 0, 0)
var _rechts := Vector3(0, 0, 1)
var _netz := MeshInstance3D.new()
var _aufbau := Node3D.new()      ## traegt die Karosserie; nickt beim Bremsen, neigt sich in Kurven
var _raeder: Array[MeshInstance3D] = []
var _bremsen: Array[MeshInstance3D] = []   ## Scheibe und Sattel: lenken mit, drehen nicht
var _rad_radius := 0.33
var _radstand := 2.6
var _drehung := 0.0              ## Winkel der Raeder (rad)
var _lenkwinkel := 0.0
var _tempo_vorher := 0.0
var _beschl := 0.0               ## geglaettete Laengsbeschleunigung (m/s^2)
var _gierrate := 0.0
var _blinker := 0.0
var _licht_stand := Vector3(-9, -9, -9)   ## zuletzt gesetzte Bremse, Blinker, Licht
## Aufhaengung: der Aufbau schwingt als gedaempfte Feder (Eigenfrequenz FEDER_HZ, Daempfung
## FEDER_D) um Heben, Nicken und Wanken; die Raeder folgen dem Boden (Strahl je Rad).
const FEDER_HZ := 1.35
const FEDER_D := 0.32
const FEDERWEG := 0.09
var _feder := Vector3.ZERO      ## Heben (m), Nicken (rad), Wanken (rad)
var _feder_v := Vector3.ZERO
var _rad_hub := PackedFloat32Array([0.0, 0.0, 0.0, 0.0])   ## Einfederung je Rad (m)
var _boden_in := randf() * 0.1
var _lagen: Array = []          ## Radmitten des Modells (Wagenmesh.radlagen)
var _gier := 0.0
var _lage := Vector3.ZERO   ## zuletzt gesetzte Position, sofort gueltig (siehe position_jetzt)
var _geschwindigkeit := Vector3.ZERO
var _stoerung_bis := 0.0
var _faktor := 1.0
var _bremse := 1.0
var _bremse_objekt := 1.0
var _flucht_bis := 0.0      ## nach Schuessen in der Naehe: Gas geben
var _gasse := 0.0           ## > 0: Streife mit Blaulicht naht von hinten -> rechts ran, langsam
var _blockiert_spieler := 0.0
var _gehupt := -10.0
var _bremse_in := randf() * 0.12   ## Sekunden bis zur naechsten Berechnung, versetzt
var _seitversatz := 0.0
var _ueberholt: Node3D
var _ueberhol_bis := 0.0
var _blockiert := 0.0
var _leuchten: Array[MeshInstance3D] = []
var _blaulicht := OmniLight3D.new()
var _sirene: AudioStreamPlayer3D
## Zweiraeder (Motorrad, Fahrrad): alles haengt an einem Knoten, der sich in die Kurve legt.
var _einspurig := false
var _schraeg := Node3D.new()
var _traeger: Node3D = self
var _vorne: Array[bool] = []
var _schraeglage := 0.0


func _ready() -> void:
	sync_to_physics = true
	add_to_group("verkehrsauto")
	add_to_group("verletzbar")
	var form := CollisionShape3D.new()
	var box := BoxShape3D.new()
	var masse := Wagenmesh.masse(modell)
	box.size = Vector3(masse.x, 1.0, masse.y)
	form.shape = box
	form.position = Vector3(0, 0.55, 0)
	add_child(form)
	_netz.mesh = Wagenmesh.netz(lack, false, modell, false)
	_aufbau.scale = karosserie
	_netz.visibility_range_end = 500.0
	# Beidseitig: der Innenraum zeigt nach innen und soll trotzdem Schatten werfen.
	_netz.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_DOUBLE_SIDED
	_aufbau.add_child(_netz)
	_einspurig = Wagenmesh.ist_einspurig(modell)
	if _einspurig:
		add_child(_schraeg)
		_traeger = _schraeg
		_setze_fahrer()
	_traeger.add_child(_aufbau)
	_baue_raeder()
	if weg.gueltig():
		_setze_position(weg.ort)
	if polizei:
		_baue_blaulicht()


## Zweirad: ein Fahrer in Fahrhaltung auf dem Sitz (Kleidung nach der Instanz).
func _setze_fahrer() -> void:
	var f := Figur.neu(get_instance_id() % 2)
	if f == null:
		return
	var p := Wagenmesh._profil(modell)
	var sitz: Dictionary = p.get("sitz", {"u": 0.6, "hoehe": 0.8})
	_aufbau.add_child(f)
	f.passe_an()
	f.einkleiden(get_instance_id() % 100003)
	# Die Figur schaut nach -Z, das Fahrzeug nach +X.
	f.rotation.y = -PI / 2.0
	f.position = Vector3((0.5 - float(sitz["u"])) * float(p["laenge"]) - 0.05,
			float(sitz["hoehe"]) - 0.92, 0.0)
	f.fahrhaltung(modell)
	for m: MeshInstance3D in f.find_children("*", "MeshInstance3D", true, false):
		m.visibility_range_end = 140.0


## Vier einzeln drehbare Raeder; die vorderen lenken. Sie haengen am Wagen, nicht am Aufbau,
## damit sie beim Nicken auf der Strasse bleiben.
func _baue_raeder() -> void:
	var lagen := Wagenmesh.radlagen(modell)
	_lagen = lagen
	for k in lagen.size():
		var l: Vector4 = lagen[k]
		var rad := MeshInstance3D.new()
		var nr := k if _einspurig else k / 2
		rad.mesh = Wagenmesh.rad_netz(modell, nr, k % 2 == 0 and not _einspurig)
		rad.position = Vector3(l.x, l.y, l.z) * karosserie
		rad.visibility_range_end = 160.0
		_traeger.add_child(rad)
		_raeder.append(rad)
		_vorne.append(l.x > 0.0)
		var bremse := MeshInstance3D.new()
		bremse.mesh = Wagenmesh.bremse_netz(modell, nr, k % 2 == 0 and not _einspurig)
		bremse.position = rad.position
		bremse.visibility_range_end = 60.0
		bremse.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		_traeger.add_child(bremse)
		_bremsen.append(bremse)
		_rad_radius = l.w
	if lagen.size() >= 4:
		_radstand = absf((lagen[0] as Vector4).x - (lagen[2] as Vector4).x)


## Raeder drehen und lenken, Aufbau nicken und neigen, Lampen schalten.
func _bewege_teile(delta: float) -> void:
	var tempo := _geschwindigkeit.dot(_blick)
	_drehung = fmod(_drehung + tempo / maxf(_rad_radius, 0.2) * delta, TAU)
	var lenk_soll := 0.0
	if absf(tempo) > 0.5:
		lenk_soll = clampf(atan(_gierrate * _radstand / tempo), -0.6, 0.6)
	_lenkwinkel = lerpf(_lenkwinkel, lenk_soll, minf(6.0 * delta, 1.0))
	for k in _raeder.size():
		var lenk := _lenkwinkel if _vorne[k] else 0.0
		_raeder[k].rotation = Vector3(0.0, lenk, -_drehung)
		_bremsen[k].rotation = Vector3(0.0, lenk, 0.0)
	var a := (tempo - _tempo_vorher) / maxf(delta, 0.001)
	_tempo_vorher = tempo
	_beschl = lerpf(_beschl, clampf(a, -9.0, 6.0), minf(4.0 * delta, 1.0))
	_federung(delta, clampf(a, -10.0, 7.0), tempo)
	var bremst := 1.0 if (_beschl < -0.8 or (absf(tempo) < 0.3 and _bremse < 0.95)) else 0.0
	var stand := Vector3(bremst, _blinker, 0.0)
	if stand != _licht_stand:
		_licht_stand = stand
		_netz.set_instance_shader_parameter("bremse", bremst)
		_netz.set_instance_shader_parameter("blinker", _blinker)


## Aufbau auf Federn: Ziele aus Bremsen/Beschleunigen (Nicken), Kurve (Wanken) und Boden
## unter den Raedern (Heben, Nicken, Wanken); eine unterdaempfte Feder laeuft hin und schwingt
## nach (nach einer Vollbremsung nickt der Wagen zurueck).
func _federung(delta: float, a: float, tempo: float) -> void:
	_boden_in -= delta
	if _boden_in <= 0.0:
		_boden_in = 0.1
		_taste_boden()
	var vl := _rad_hub[0]
	var vr := _rad_hub[1]
	var hl := _rad_hub[2]
	var hr := _rad_hub[3]
	if _einspurig:
		vr = vl
		hl = _rad_hub[1]
		hr = hl
		# Schraeglage in die Kurve: tan(Neigung) = v * Gierrate / g (links positiv -> nach links).
		var soll := -atan(tempo * _gierrate / 9.81)
		_schraeglage = lerpf(_schraeglage, clampf(soll, -0.7, 0.7), minf(3.0 * delta, 1.0))
		_schraeg.rotation.x = _schraeglage
	var spur := 1.5
	var ziel := Vector3(
		(vl + vr + hl + hr) * 0.25,
		clampf(a * 0.0055, -0.045, 0.03) + ((vl + vr) - (hl + hr)) * 0.5 / maxf(_radstand, 1.0),
		# Rechts hoeher (Rad auf dem Bordstein) kippt den Aufbau nach links (-Drehung um X).
		0.0 if _einspurig else clampf(tempo * _gierrate * 0.0065, -0.05, 0.05)
				- ((vr + hr) - (vl + hl)) * 0.5 / spur)
	var w0 := TAU * FEDER_HZ
	var beschl := (ziel - _feder) * w0 * w0 - _feder_v * 2.0 * FEDER_D * w0
	_feder_v += beschl * delta
	_feder += _feder_v * delta
	_feder.x = clampf(_feder.x, -FEDERWEG, FEDERWEG)
	_aufbau.rotation = Vector3(_feder.z, 0.0, _feder.y)
	_aufbau.position.y = _feder.x
	# Raeder: auf dem Boden, gegen den Aufbau federnd.
	for k in mini(_raeder.size(), _lagen.size()):
		var l: Vector4 = _lagen[k]
		var y := l.y * karosserie.y + _rad_hub[k]
		_raeder[k].position.y = y
		_bremsen[k].position.y = y


## Boden unter den vier Raedern (nur nah an der Kamera): wie weit jedes Rad gegenueber der
## Fahrbahnhoehe der Route ein- oder ausfedert (Bordstein, Buckel, Schlagloch).
func _taste_boden() -> void:
	var kamera := get_viewport().get_camera_3d() if is_inside_tree() else null
	if kamera == null or kamera.global_position.distance_to(_lage) > 70.0:
		for k in 4:
			_rad_hub[k] = lerpf(_rad_hub[k], 0.0, 0.5)
		return
	var raum := get_world_3d().direct_space_state
	for k in mini(4, _lagen.size()):
		var l: Vector4 = _lagen[k]
		var p := global_transform * Vector3(l.x, 0.0, l.z)
		var frage := PhysicsRayQueryParameters3D.create(p + Vector3.UP * 0.6, p - Vector3.UP * 0.6)
		frage.exclude = [get_rid()]
		var treffer := raum.intersect_ray(frage)
		var hub := 0.0
		if not treffer.is_empty():
			hub = clampf((treffer["position"] as Vector3).y - p.y, -FEDERWEG, FEDERWEG)
		_rad_hub[k] = lerpf(_rad_hub[k], hub, 0.6)


## Blinker: vor einer Abbiegung auf der Route, beim Ausscheren zum Ueberholen (links), beim
## Wiedereinordnen und beim Rechts-ran-Fahren fuer die Rettungsgasse (rechts).
func _waehle_blinker() -> void:
	if ist_ausgeschaltet():
		_blinker = 2.0   # Warnblinker
		return
	if _gasse > 0.0:
		_blinker = 1.0
		return
	if _ueberholt != null:
		_blinker = -1.0 if absf(_seitversatz) < 1.0 else 0.0
		return
	_blinker = 0.0
	var r := weg.route
	if r.size() < 3:
		return
	var i := weg.index
	var strecke := 0.0
	var vorher := _lage
	for k in 6:
		var j := i + k * weg.richtung
		if weg.rund:
			j = posmod(j, r.size())
		if j < 0 or j >= r.size():
			return
		strecke += Vector2(r[j].x - vorher.x, r[j].z - vorher.z).length()
		vorher = r[j]
		if strecke > 35.0:
			return
		var j2 := j + weg.richtung
		if weg.rund:
			j2 = posmod(j2, r.size())
		if j2 < 0 or j2 >= r.size():
			return
		var d := Vector3(r[j2].x - r[j].x, 0, r[j2].z - r[j].z)
		if d.length_squared() < 0.01:
			continue
		var winkel := _blick.signed_angle_to(d.normalized(), Vector3.UP)
		if absf(winkel) > 0.6:
			# Positiver Winkel um +Y: nach links (Richtung -Z, rechts ist +Z).
			_blinker = -1.0 if winkel > 0.0 else 1.0
			return


## Dachbalken: zwei flache Leuchten links und rechts, darueber ein Punktlicht, das
## im selben Takt blau und rot blitzt und die Strasse anstrahlt.
func _baue_blaulicht() -> void:
	for i in 2:
		var m := MeshInstance3D.new()
		var b := BoxMesh.new()
		b.size = Vector3(0.35, 0.12, 0.3)
		m.mesh = b
		m.position = Vector3(-0.6, 1.6, -0.45 if i == 0 else 0.45)
		m.material_override = StandardMaterial3D.new()
		m.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		add_child(m)
		_leuchten.append(m)
	_blaulicht.position = Vector3(-0.6, 2.0, 0)
	_blaulicht.omni_range = 22.0
	_blaulicht.light_energy = 0.0
	_blaulicht.shadow_enabled = false
	add_child(_blaulicht)
	_sirene = Ton.schleife(self, "SFX_Sirene", 2.0)


func blaulicht_an() -> bool:
	return polizei and not geparkt and (weg.gueltig() or gesperrt) and not ist_ausgeschaltet()


## Ein Schuss in der Naehe: normale Fahrer geben Gas und machen sich davon.
func hoere_schuss(ort: Vector3) -> void:
	if polizei or geparkt or ist_ausgeschaltet():
		return
	if _lage.distance_to(ort) < 40.0:
		_flucht_bis = Time.get_ticks_msec() / 1000.0 + 7.0


## Rettungsgasse: naht eine Streife mit Blaulicht von hinten, faehrt der Wagen rechts ran.
func _pruefe_gasse() -> void:
	_gasse = 0.0
	if polizei:
		return
	for s: Verkehrsauto in get_tree().get_nodes_in_group("verkehrsauto"):
		if s == self or not s.blaulicht_an():
			continue
		var hinter := _lage - s.position_jetzt()
		if hinter.length() < 45.0 and hinter.dot(_blick) > 0.0 and s.vorwaerts().dot(_blick) > 0.3:
			_gasse = 1.0
			return


## Steht der Spieler im Weg, wird nach gut zwei Sekunden gehupt.
func _pruefe_hupe(delta_s: float) -> void:
	if polizei or Verkehrsbremse.vor_spieler(self) > 0.3:
		_blockiert_spieler = 0.0
		return
	_blockiert_spieler += delta_s
	var jetzt := Time.get_ticks_msec() / 1000.0
	if _blockiert_spieler > 2.2 and jetzt - _gehupt > 6.0:
		_gehupt = jetzt
		Ton.einmal(get_tree(), "SFX_Hupe", _lage, 2.0, randf_range(0.95, 1.08))


func _blinke() -> void:
	var an := not geparkt and (weg.gueltig() or gesperrt) and not ist_ausgeschaltet()
	if _sirene != null and an != _sirene.playing:
		if an:
			_sirene.play()
		else:
			_sirene.stop()
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


## Strassenklasse an der aktuellen Stelle der Route.
func klasse_jetzt() -> int:
	if weg.index < klassen.size():
		return klassen[weg.index]
	return klasse


## Rechte Seite in der Ebene, fuer Rechts-vor-Links.
func rechts2() -> Vector2:
	return Vector2(_rechts.x, _rechts.z)


func ueberholt_gerade() -> bool:
	return _ueberholt != null


## Steht ein Hindernis laenger als zwei Sekunden vor dem Wagen, die Gegenspur ist frei und
## keine rote Ampel in Sicht, schert er aus, faehrt vorbei und ordnet sich wieder ein.
func _pruefe_ueberholen() -> void:
	var jetzt := Time.get_ticks_msec() / 1000.0
	if _ueberholt != null:
		if not is_instance_valid(_ueberholt):
			_ueberholt = null
			return
		var p: Vector3 = _ueberholt.position_jetzt() if _ueberholt is Verkehrsauto \
				else _ueberholt.global_position
		if (p - _lage).dot(_blick) < -6.0 or jetzt > _ueberhol_bis \
				or Verkehrsbremse.gegenverkehr(self, 30.0):
			_ueberholt = null
		return
	var hindernis := Verkehrsbremse.hindernis_voraus(self)
	if hindernis == null:
		_blockiert = 0.0
		return
	_blockiert += 0.12
	var breite := breiten[weg.index] if weg.index < breiten.size() else 6.0
	if _blockiert > 2.0 and breite >= 5.5 and not Verkehrsbremse.gegenverkehr(self, 45.0) \
			and (polizei or not Verkehrsbremse.ampel_haelt_voraus(self, 45.0)):
		_ueberholt = hindernis
		_ueberhol_bis = jetzt + 9.0
		_blockiert = 0.0
		ueberholungen += 1


func _berechne_bremse() -> void:
	_pruefe_ueberholen()
	_bremse_objekt = minf(Verkehrsbremse.vor_auto(self, _ueberholt),
			Verkehrsbremse.vor_spieler(self, _ueberholt))
	# Mit Blaulicht gelten weder Rot noch Vorfahrt.
	var ampel := 1.0 if polizei else Verkehrsbremse.vor_ampel(self)
	var kreuzung := 1.0 if polizei else Verkehrsbremse.vor_kreuzung(self)
	_bremse = minf(minf(_bremse_objekt, ampel), minf(kreuzung, Verkehrsbremse.in_kurve(weg)))
	_pruefe_gasse()
	_pruefe_hupe(0.12)
	if _gasse > 0.0:
		_bremse = minf(_bremse, 0.3)
	_waehle_blinker()


## Stellt den Wagen an einen Ort und dreht ihn in eine Richtung.
func setze_lage(ort: Vector3, blick: Vector3) -> void:
	weg.ort = ort
	var b := Vector3(blick.x, 0, blick.z)
	if b.length_squared() > 0.0001:
		_blick = b.normalized()
	_rechts = Vector3(-_blick.z, 0.0, _blick.x)
	_setze_transform(ort, -atan2(_blick.z, _blick.x))
	geparkt = false
	visible = true


## Bei einem AnimatableBody3D mit sync_to_physics liest global_position nach dem
## Setzen noch bis zum naechsten Physikbild den alten Wert. Wer die Position
## gleich nach dem Setzen braucht (die Polizei), fragt deshalb hier.
func position_jetzt() -> Vector3:
	return _lage


## Bei einem AnimatableBody3D mit sync_to_physics gilt: die GANZE Transformation auf
## einmal setzen. Wer erst die Position und danach rotation setzt (oder umgekehrt),
## verliert das zuerst Gesetzte, der Koerper bewegt sich dann gar nicht.
func _setze_position(ort: Vector3) -> void:
	_setze_transform(ort, _gier)


func _setze_transform(ort: Vector3, gier: float) -> void:
	_lage = ort
	_gier = gier
	global_transform = Transform3D(Basis(Vector3.UP, gier), ort)


## Drehung um Y; das Netz schaut nach +X, die Drehung bringt +X auf die Blickrichtung.
func gier() -> float:
	return _gier


func geschwindigkeit_ms() -> Vector3:
	return _geschwindigkeit


func ist_ausgeschaltet() -> bool:
	return leben <= 0.0


func lebensanteil() -> float:
	return clampf(leben / 100.0, 0.0, 1.0)


func setze_leben(neu: float) -> void:
	leben = neu


func setze_lack(farbe: Color) -> void:
	lack = farbe
	_netz.mesh = Wagenmesh.netz(lack, false, modell, false)


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
		if ist_ausgeschaltet() and _blinker != 2.0:
			_blinker = 2.0
			_bewege_teile(delta)
		return
	# Nach einem Treffer kurz fast stehen bleiben, dann wieder auf Tempo; sonst
	# waere ein Treffer nur eine Farbe, kein Ereignis.
	var soll := 0.1 if Time.get_ticks_msec() / 1000.0 < _stoerung_bis else 1.0
	_faktor = move_toward(_faktor, soll, 3.0 * delta)
	_bremse_in -= delta
	if _bremse_in <= 0.0:
		_bremse_in = 0.12
		_berechne_bremse()
	var vorher := _lage
	var flucht := 1.5 if Time.get_ticks_msec() / 1000.0 < _flucht_bis else 1.0
	weg.bewege(tempo_ms * _faktor * _bremse * flucht * (1.25 if _ueberholt != null else 1.0) * delta)
	if weg.blick != Vector3.ZERO:
		_blick = _blick.lerp(weg.blick, minf(8.0 * delta, 1.0)).normalized()
	var rechts_soll := Vector3(-_blick.z, 0.0, _blick.x)
	_rechts = _rechts.lerp(rechts_soll, minf(6.0 * delta, 1.0)).normalized()
	var breite := 6.0
	if weg.index < breiten.size():
		breite = breiten[weg.index]
	# Rechtsverkehr: eine Spur rechts von der Strassenmitte (bei zwei Spuren ein
	# Viertel der Breite). Vor einem Hindernis weicht der Wagen zusaetzlich nach
	# rechts aus, nie ueber den Fahrbahnrand hinaus.
	var spur := breite * 0.25
	var rand := maxf(0.0, breite * 0.5 - HALBE_WAGENBREITE)
	var ziel := minf(spur + (MAX_SEITVERSATZ if _bremse_objekt < 0.9 else 0.0), rand)
	if modell == "fahrrad":
		ziel = maxf(rand - 0.4, spur)   # Radfahrer am rechten Rand
	if _ueberholt != null:
		ziel = -minf(spur, rand)   # auf die Gegenspur
	elif _gasse > 0.0:
		ziel = rand   # ganz nach rechts, Platz fuer die Streife
	_seitversatz = lerpf(_seitversatz, ziel, minf((1.6 if _ueberholt != null else 0.7) * delta, 1.0))
	# Das Netz schaut nach +X; Drehung um Y bringt +X auf die Blickrichtung.
	var gier_vorher := _gier
	_setze_transform(weg.ort + _rechts * _seitversatz, -atan2(_blick.z, _blick.x))
	_geschwindigkeit = (_lage - vorher) / delta
	_gierrate = lerpf(_gierrate, wrapf(_gier - gier_vorher, -PI, PI) / delta, minf(5.0 * delta, 1.0))
	_bewege_teile(delta)
