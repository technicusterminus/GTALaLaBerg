class_name Spieler
extends CharacterBody3D
## Der Spieler zu Fuss: laufen, springen, schiessen, Schaden nehmen. Wer bei
## null Leben ankommt, wacht im Klinikum wieder auf; das kostet Geld, nicht den
## Spielstand. Einsteigen und Aussteigen uebernimmt welt.gd.

const GEHEN := 4.5
const RENNEN := 8.5
const SPRUNG := 5.2
## Hier wacht man nach einem Zusammenbruch auf: der Spawn-Punkt am Klinikum. Der
## Punkt der Unreal-Version (-1590, 156) liegt unter einem Dach; der Bodenstrahl
## von oben traefe zuerst das Dach.
const KLINIKUM := Vector3(-1672.1, 0.0, 145.3)

var kamera: Kamera
var waffe := Waffe.new()
var gesperrt := false   ## im Film: keine Eingabe
var leben := 100.0
var krankenhausbesuche := 0

var _schwerkraft: float = ProjectSettings.get_setting("physics/3d/default_gravity")
var _koerper := Node3D.new()
var _figur: Figur
var _aktiv := true
var _letzter_anprall := -10.0
var _lande_schonzeit := 0.0
var _warte_boden := false
var _warte_ziel := Vector3.ZERO


func _ready() -> void:
	add_to_group("verletzbar")
	var kapsel := CollisionShape3D.new()
	var form := CapsuleShape3D.new()
	form.height = 1.8
	form.radius = 0.35
	kapsel.shape = form
	kapsel.position.y = 0.9
	add_child(kapsel)

	var mesh := MeshInstance3D.new()
	var kapselmesh := CapsuleMesh.new()
	kapselmesh.height = 1.8
	kapselmesh.radius = 0.35
	mesh.mesh = kapselmesh
	mesh.position.y = 0.9
	var farbe := StandardMaterial3D.new()
	farbe.albedo_color = Color(0.85, 0.35, 0.15)
	mesh.material_override = farbe
	_koerper.add_child(mesh)
	# Eine Nase zeigt, wohin die Figur schaut.
	var nase := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = Vector3(0.18, 0.18, 0.3)
	nase.mesh = box
	nase.position = Vector3(0, 1.5, -0.35)
	nase.material_override = farbe
	_koerper.add_child(nase)
	waffe.position = Vector3(0.32, 1.2, -0.3)
	_koerper.add_child(waffe)
	add_child(_koerper)
	if Figur.verfuegbar():
		_figur = Figur.neu(0)
		_koerper.add_child(_figur)
		_figur.passe_an()
		mesh.visible = false
		nase.visible = false


func aktiv(an: bool) -> void:
	_aktiv = an
	visible = an
	collision_layer = 1 if an else 0
	collision_mask = 1 if an else 0
	set_physics_process(an)
	velocity = Vector3.ZERO


# --- Schaden ----------------------------------------------------------------

func verletze(schaden: float, richtung: Vector3, art: Schaden.Art) -> void:
	if leben <= 0.0:
		return
	leben -= schaden
	# Ein Stoss wirft die Figur ein Stueck, ohne ihr die Kontrolle zu nehmen.
	if art == Schaden.Art.ANPRALL or art == Schaden.Art.SPRENGUNG:
		var r := Vector3(richtung.x, 0.0, richtung.z).normalized()
		velocity = r * 4.2 + Vector3(0, 2.6, 0)
	var text := ""
	if leben <= 0.0:
		text = "Umgehauen – ab ins Klinikum"
	elif art == Schaden.Art.STURZ:
		text = "Hart gelandet – %d Punkte übrig" % ceili(leben)
	else:
		text = "Getroffen – %d Punkte übrig" % ceili(leben)
	Schaden.rueckmeldung(get_tree(), text)
	print("LALABERG_SCHADEN spieler=%.0f art=%d" % [leben, art])
	if leben <= 0.0:
		ins_krankenhaus()


func geschwindigkeit_ms() -> Vector3:
	return velocity


func ist_ausgeschaltet() -> bool:
	return leben <= 0.0


func lebensanteil() -> float:
	return clampf(leben / 100.0, 0.0, 1.0)


## Aufwachen im Klinikum: Leben voll, ein Viertel des Geldes weg (hoechstens
## 500), die Fahndung ist vorbei.
func ins_krankenhaus() -> void:
	leben = 100.0
	krankenhausbesuche += 1
	var kosten := mini(500, maxi(0, Konto.geld() / 4))
	if kosten > 0:
		Konto.bezahle(kosten)
	get_tree().call_group("polizei", "verwische")
	setze_auf_boden(KLINIKUM)
	Schaden.rueckmeldung(get_tree(), "Klinikum – wieder auf den Beinen, Behandlung %d €" % kosten
			if kosten > 0 else "Klinikum – wieder auf den Beinen")
	print("LALABERG_KRANKENHAUS besuch=%d kosten=%d" % [krankenhausbesuche, kosten])


## Setzt die Figur auf den Boden unter (x, z). Steht dort noch keine
## Kollision (der Sektor laedt erst), bleibt sie in der Luft stehen und wartet.
func setze_auf_boden(ziel: Vector3) -> void:
	_lande_schonzeit = Time.get_ticks_msec() / 1000.0 + 1.5
	velocity = Vector3.ZERO
	var boden := _boden_unter(ziel)
	if boden.is_empty():
		global_position = Vector3(ziel.x, 60.0, ziel.z)
		_warte_boden = true
		_warte_ziel = ziel
	else:
		global_position = boden["position"] + Vector3(0, 0.1, 0)
		_warte_boden = false


func _boden_unter(ziel: Vector3) -> Dictionary:
	var abfrage := PhysicsRayQueryParameters3D.create(
			Vector3(ziel.x, 300.0, ziel.z), Vector3(ziel.x, -300.0, ziel.z))
	abfrage.exclude = [get_rid()]
	abfrage.collision_mask = 1
	return get_world_3d().direct_space_state.intersect_ray(abfrage)


# --- Eingabe ----------------------------------------------------------------

func _unhandled_input(event: InputEvent) -> void:
	if not _aktiv:
		return
	for art in 4:
		if event.is_action_pressed("waffe%d" % (art + 1)):
			waehle_waffe(art)


func waehle_waffe(art: int) -> void:
	if not Konto.hat_waffe(art):
		Schaden.rueckmeldung(get_tree(), "%s gehört dir noch nicht – gibt es im Laden" % Waffe.NAMEN[art])
		return
	waffe.setze_art(art)
	Schaden.rueckmeldung(get_tree(), Waffe.NAMEN[art])


# --- Bewegung ---------------------------------------------------------------

func _physics_process(delta: float) -> void:
	if _warte_boden:
		var boden := _boden_unter(_warte_ziel)
		if boden.is_empty():
			return
		global_position = boden["position"] + Vector3(0, 0.1, 0)
		_warte_boden = false
		return
	var faktor := 1.0 + 0.33 * Konto.anteil(Konto.Wert.AUSDAUER)
	var eingabe := Vector2.ZERO
	if not gesperrt:
		eingabe = Input.get_vector("links", "rechts", "vorwaerts", "rueckwaerts")
	var richtung := Vector3.ZERO
	if kamera != null:
		richtung = kamera.rechts() * eingabe.x - kamera.vorwaerts() * eingabe.y
	var tempo := (RENNEN if Input.is_action_pressed("rennen") else GEHEN) * faktor
	velocity.x = move_toward(velocity.x, richtung.x * tempo, 40.0 * delta)
	velocity.z = move_toward(velocity.z, richtung.z * tempo, 40.0 * delta)
	var war_am_boden := is_on_floor()
	if war_am_boden:
		if Input.is_action_just_pressed("springen") and not gesperrt:
			velocity.y = SPRUNG
	else:
		velocity.y -= _schwerkraft * delta
	var fallgeschwindigkeit := -velocity.y
	move_and_slide()
	if is_on_floor() and not war_am_boden:
		_gelandet(fallgeschwindigkeit)
	if richtung.length_squared() > 0.01:
		var ziel := atan2(-richtung.x, -richtung.z)
		_koerper.rotation.y = lerp_angle(_koerper.rotation.y, ziel, minf(12.0 * delta, 1.0))
	# Ausdauer waechst vom Laufen am Boden.
	var lauf := Vector2(velocity.x, velocity.z).length()
	if is_on_floor() and lauf > 1.5:
		Konto.uebe(Konto.Wert.AUSDAUER, lauf * delta / 9.0)
	_animiere(lauf)
	_ziele_und_feuere()
	_pruefe_anprall()


func _animiere(lauf: float) -> void:
	if _figur == null or not is_on_floor():
		return
	if lauf < 0.4:
		_figur.spiele("Idle_Gun")
	elif lauf < 6.0:
		_figur.spiele("Walk", clampf(lauf / GEHEN, 0.5, 1.4))
	else:
		_figur.spiele("Run", clampf(lauf / RENNEN, 0.7, 1.3))


## Aus der Hoehe gefallen. Gemessen wird die Geschwindigkeit beim Aufsetzen: bis
## 8 m/s (gut drei Meter) faengt man sich ab, ab 22 m/s (rund 25 Meter) ist es
## vorbei, dazwischen steigt der Schaden gleichmaessig.
func _gelandet(aufprall: float) -> void:
	if Time.get_ticks_msec() / 1000.0 < _lande_schonzeit or aufprall <= 8.0:
		return
	var schaden := remap(clampf(aufprall, 8.0, 22.0), 8.0, 22.0, 6.0, 100.0)
	print("LALABERG_STURZ tempo=%.1f schaden=%.0f" % [aufprall, schaden])
	verletze(schaden, Vector3.UP, Schaden.Art.STURZ)


## Die Waffe zeigt dorthin, wo die Bildmitte hintrifft; bei gedrueckter
## Feuertaste (und eingefangener Maus) schiesst sie.
func _ziele_und_feuere() -> void:
	if kamera == null:
		return
	var strahl := kamera.blick_strahl()
	var von: Vector3 = strahl["von"]
	var r: Vector3 = strahl["richtung"]
	var abfrage := PhysicsRayQueryParameters3D.create(von + r * 1.0, von + r * 200.0)
	abfrage.exclude = [get_rid()]
	var treffer := get_world_3d().direct_space_state.intersect_ray(abfrage)
	var ziel: Vector3 = treffer["position"] if not treffer.is_empty() else von + r * 200.0
	var brust := global_position + Vector3(0, 1.35, 0)
	var dir := (ziel - brust).normalized()
	waffe.global_basis = Basis.looking_at(dir)
	if Input.is_action_pressed("feuer") and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED \
			and not gesperrt:
		_koerper.rotation.y = atan2(-dir.x, -dir.z)
		feuere_auf(ziel)


## Ein Schuss auf einen Punkt; true, wenn die Feuerrate ihn zugelassen hat.
func feuere_auf(ziel: Vector3) -> bool:
	var brust := global_position + Vector3(0, 1.35, 0)
	var dir := (ziel - brust).normalized()
	var muendung := brust + dir * (0.45 + waffe.muendung_abstand())
	return waffe.feuern(muendung, (ziel - muendung).normalized(), self)


## Angefahren werden: die KI-Autos bewegen sich nicht ueber die Physik, ein
## Stossimpuls kaeme bei der Figur nie an. Deshalb jedes Bild nachsehen, was nah
## ist, wie schnell es ist und ob es auf mich zukommt.
func _pruefe_anprall() -> void:
	var jetzt := Time.get_ticks_msec() / 1000.0
	if leben <= 0.0 or jetzt - _letzter_anprall < 1.2:
		return
	for gruppe in ["verkehrsauto", "auto"]:
		for n: Node3D in get_tree().get_nodes_in_group(gruppe):
			var weg := global_position - n.global_position
			if Vector2(weg.x, weg.z).length_squared() > 3.2 * 3.2 or absf(weg.y) > 2.6:
				continue
			var fahrt: Vector3 = n.geschwindigkeit_ms()
			var kmh := fahrt.length() * 3.6
			if kmh < 12.0:
				continue
			var f2 := Vector3(fahrt.x, 0, fahrt.z).normalized()
			var w2 := Vector3(weg.x, 0, weg.z).normalized()
			if f2.dot(w2) < 0.2:
				continue
			_letzter_anprall = jetzt
			verletze(remap(clampf(kmh, 12.0, 80.0), 12.0, 80.0, 8.0, 95.0), f2, Schaden.Art.ANPRALL)
			return
