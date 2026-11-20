class_name Sonderfahrzeug
extends AnimatableBody3D
## Die beiden versteckten Fahrzeuge: ein Panzer und ein Hubschrauber. Beide fahren
## bzw. fliegen kinematisch, ohne Fahrwerk und Rotorphysik; sie sollen sich finden
## und benutzen lassen, nicht simuliert werden. Der Panzer steht abseits im
## Waldstueck, der Hubschrauber auf dem Dach des Klinikums; verschlossen sind sie,
## bis der passende Kopf gestellt ist (Zuendschluessel, Rotorschluessel).
##
## Der Panzer steckt Farbkugeln weg, eine Sprenggranate aber nicht; die duenne Zelle
## des Hubschraubers nimmt jeden Treffer krumm. Bei null Leben faellt die Maschine
## aus: der Panzer bleibt stehen, der Hubschrauber sinkt zu Boden, und wer drin
## sitzt, steigt aus. Das Modell schaut nach +X, wie bei den Autos.

enum Art { PANZER, HUBSCHRAUBER }

const SHADER := preload("res://shader/flach.gdshader")
const PANZER_TEMPO := 12.0
const PANZER_SCHUB := 5.0
const PANZER_DREHUNG := 0.66        ## rad/s
const HELI_TEMPO := 50.0
const HELI_SCHUB := 14.0
const HELI_DREHUNG := 0.96
const HELI_STEIGEN := 8.0
const HELI_BODENABSTAND := 0.4
## Kanone: alle drei Sekunden geladen, Muendung 7,8 m vor der Turmmitte. Geschossen
## wird Farbe: es ist ein Paintball-Spiel, und ein 40-m-Klecks ist die richtige
## Antwort auf ein Kaliber dieser Groesse.
const KANONE_LADEZEIT := 3.0
const KANONE_MUENDUNG := 7.8
const KANONE_TEMPO := 140.0
const TURM_HOEHE := 1.9
const OLIV := Color(0.10, 0.13, 0.07)
const HELI_LACK := Color(0.80, 0.82, 0.84)
const AUSGEBRANNT := Color(0.05, 0.045, 0.04)

var art := Art.PANZER
var fahrer: Node3D
var kamera: Kamera
var leben := 0.0
var voll := 0.0
var kaputt := false
var schuesse := 0
var gesperrt := false          ## im Film: keine Eingabe
var tempo := 0.0               ## m/s laengs

var _lage := Vector3.ZERO
var _gier := 0.0       ## Drehung um Y; das Modell schaut nach +X
var _roll := 0.0       ## Querlage (Drehung um X)
var _nick := 0.0       ## Nicken (Drehung um Z)
var _schub := 0.0
var _drehen := 0.0
var _steigen := 0.0
var _sinkflug := 0.0
var _drehphase := 0.0
var _dauerton: AudioStreamPlayer3D
var _letzter_schuss := -10.0
var _rueckstoss := 0.0
var _einstieg_zeit := -10.0
var _test := false
var _koerper := Node3D.new()
var _dreher := Node3D.new()      ## Turm bzw. Hauptrotor
var _heck := Node3D.new()
var _feuer := OmniLight3D.new()
var _modelle: Array[Node3D] = []


func _ready() -> void:
	sync_to_physics = true
	add_to_group("sonderfahrzeug")
	add_to_group("verletzbar")
	var panzer := art == Art.PANZER
	_dauerton = Ton.schleife(self, "SFX_Panzer" if panzer else "SFX_Rotor", 3.0)
	voll = 600.0 if panzer else 220.0
	leben = voll
	var form := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = Vector3(7.2, 2.2, 3.5) if panzer else Vector3(6.6, 3.0, 2.6)
	form.shape = box
	form.position = Vector3(0, 1.1 if panzer else 1.5, 0)
	add_child(form)

	add_child(_koerper)
	add_child(_dreher)
	add_child(_heck)
	if panzer:
		_modell(_koerper, "SM_Panzer_Wanne")
		_dreher.position = Vector3(0, TURM_HOEHE, 0)
		_modell(_dreher, "SM_Panzer_Turm")
		_feuer.position = Vector3(KANONE_MUENDUNG, 0, 0)
		_feuer.light_color = Color(1.0, 0.68, 0.25)
		_feuer.omni_range = 35.0
		_feuer.light_energy = 0.0
		_feuer.shadow_enabled = false
		_dreher.add_child(_feuer)
	else:
		_modell(_koerper, "SM_Heli_Rumpf")
		_dreher.position = Vector3(0.35, 2.72, 0)
		_modell(_dreher, "SM_Heli_Rotor")
		_heck.position = Vector3(-5.55, 1.88, 0)
		_modell(_heck, "SM_Heli_Heckrotor")
	_faerbe(OLIV if panzer else HELI_LACK)
	print("LALABERG_SONDER %s bei %s" % ["Panzer" if panzer else "Hubschrauber", global_position])


## Das Modell aus dem FBX unter einen Drehpunkt haengen.
func _modell(eltern: Node3D, name: String) -> void:
	var res = load("res://daten/modelle/%s.fbx" % name)
	if res == null:
		push_warning("Modell fehlt: %s (python3 Tools/konvertiere_sektoren.py, git lfs pull)" % name)
		return
	var m: Node3D = res.instantiate()
	eltern.add_child(m)
	_modelle.append(m)


func _faerbe(farbe: Color) -> void:
	var stoff := ShaderMaterial.new()
	stoff.shader = SHADER
	stoff.set_shader_parameter("farbe", Vector3(farbe.r, farbe.g, farbe.b))
	stoff.set_shader_parameter("rauheit", 0.7)
	for m in _modelle:
		for t: MeshInstance3D in m.find_children("*", "MeshInstance3D", true, false):
			t.material_override = stoff


# --- Auskunft ----------------------------------------------------------------

## Bei einem AnimatableBody3D mit sync_to_physics liest global_position nach dem
## Setzen noch bis zum naechsten Physikbild den alten Wert; deshalb eine eigene,
## sofort gueltige Position.
func position_jetzt() -> Vector3:
	return _lage


## Startlage nach dem Hinzufuegen: Ort und Blickrichtung (Drehung um Y).
func setze_lage(ort: Vector3, gier: float) -> void:
	_gier = gier
	_setze_position(ort)


## Vorwaerts und rechts in der Ebene, aus der eigenen Drehung (nicht aus dem Knoten,
## der bis zum naechsten Physikbild die alte Lage liefert).
func vorwaerts() -> Vector3:
	return Vector3(cos(_gier), 0.0, -sin(_gier))


func rechts() -> Vector3:
	return Vector3(sin(_gier), 0.0, cos(_gier))


func geschwindigkeit_ms() -> Vector3:
	return vorwaerts() * tempo


func tempo_kmh() -> float:
	return tempo * 3.6


func ist_ausgeschaltet() -> bool:
	return kaputt


func lebensanteil() -> float:
	return clampf(leben / maxf(1.0, voll), 0.0, 1.0)


func hat_fahrer() -> bool:
	return fahrer != null


func setze_fahrer(figur: Node3D) -> void:
	fahrer = figur
	_einstieg_zeit = Time.get_ticks_msec() / 1000.0
	_schub = 0.0
	_drehen = 0.0
	_steigen = 0.0


## Wer den Panzer fahren will, braucht den Zuendschluessel, wer den Hubschrauber
## fliegen will, den Rotorschluessel.
func gesperrt_fuer_spieler() -> bool:
	var fund := Konto.Fund.ZUENDUNG if art == Art.PANZER else Konto.Fund.ROTOR
	return not Konto.hat_fund(fund)


func sperrtext() -> String:
	return "Verschlossen – es fehlt der Zündschlüssel" if art == Art.PANZER \
			else "Verschlossen – es fehlt der Rotorschlüssel"


func steuerungstext() -> String:
	if art == Art.PANZER:
		return "Panzer – W/S fahren, A/D drehen, Turm folgt dem Blick, Maus links feuert, F aussteigen"
	return "Hubschrauber – Leertaste steigen, W/S fliegen, A/D drehen, F aussteigen"


## Aussteigen geht nur nahe am Boden und im Stand.
func aussteigen_hinweis() -> String:
	if Time.get_ticks_msec() / 1000.0 - _einstieg_zeit < 0.4:
		return "…"
	var boden := _boden_y()
	if is_nan(boden):
		return "Hier geht es nicht hinaus."
	if global_position.y - boden > 4.0:
		return "Zum Aussteigen zuerst anhalten." if art == Art.PANZER \
				else "Zum Aussteigen zuerst landen."
	return ""


func test_steuerung(schub: float, drehen: float, steigen: float) -> void:
	_test = true
	_schub = schub
	_drehen = drehen
	_steigen = steigen


# --- Schaden -------------------------------------------------------------------

func verletze(schaden: float, _richtung: Vector3, schadensart: Schaden.Art) -> void:
	if kaputt:
		return
	# Farbkugeln und Rempler an der Wanne: Lack ab, mehr nicht. Eine Sprenggranate
	# zaehlt dagegen voll, auch am Panzer.
	if art == Art.PANZER and schadensart != Schaden.Art.SPRENGUNG:
		schaden *= 0.12
	leben = maxf(0.0, leben - schaden)
	if fahrer != null:
		var text := "Treffer – %d %%" % ceili(100.0 * lebensanteil())
		if leben <= 0.0:
			text = "Panzer steht – Kette zerrissen" if art == Art.PANZER \
					else "Turbine aus – die Maschine sinkt"
		Schaden.rueckmeldung(get_tree(), text)
	if leben > 0.0:
		return
	kaputt = true
	_faerbe(AUSGEBRANNT)
	_feuer.light_energy = 0.0
	print("LALABERG_SONDER_KAPUTT art=%d schadensart=%d" % [art, schadensart])


## Die Kanone des Panzers: ein Schuss alle drei Sekunden, danach faehrt das Rohr
## zurueck. false, wenn noch geladen wird oder es kein Panzer ist.
func feuern() -> bool:
	if kaputt or art != Art.PANZER:
		return false
	var jetzt := Time.get_ticks_msec() / 1000.0
	if jetzt - _letzter_schuss < KANONE_LADEZEIT:
		return false
	_letzter_schuss = jetzt
	_rueckstoss = 1.0
	schuesse += 1
	var richtung := _dreher.global_basis.x
	var muendung := _dreher.global_position + richtung * KANONE_MUENDUNG
	var kugel := Farbkugel.new()
	kugel.halter = self
	# Grosses Kaliber: 20 cm Kugel, Klecks zwischen 12 und 22 m, flache Bahn, Wirkung
	# im Umkreis von 14 m: wer in der Mitte steht, ist erledigt, am Rand gibt es eine Beule.
	kugel.einrichten(Color(0.95, 0.55, 0.05), 0.2, 12.0, 22.0, 0.25)
	kugel.wucht = 130.0
	kugel.wucht_radius = 14.0
	get_tree().current_scene.add_child(kugel)
	kugel.global_position = muendung
	kugel.abschiessen(richtung, KANONE_TEMPO)
	Ton.einmal(get_tree(), "SFX_Panzer_Schuss", muendung, 3.0)
	print("LALABERG_KANONE schuss=%d bei %s" % [schuesse, muendung])
	return true


# --- Fahren --------------------------------------------------------------------

func _boden_y() -> float:
	var abfrage := PhysicsRayQueryParameters3D.create(
			global_position + Vector3(0, 6, 0), global_position - Vector3(0, 200, 0))
	var aus: Array[RID] = [get_rid()]
	if fahrer is CollisionObject3D:
		aus.append((fahrer as CollisionObject3D).get_rid())
	abfrage.exclude = aus
	abfrage.collision_mask = 1
	var treffer := get_world_3d().direct_space_state.intersect_ray(abfrage)
	return treffer["position"].y if not treffer.is_empty() else NAN


## Die GANZE Transformation auf einmal setzen: bei einem AnimatableBody3D mit
## sync_to_physics geht sonst das zuerst Gesetzte (Position oder Drehung) verloren.
func _setze_position(ort: Vector3) -> void:
	_lage = ort
	global_transform = Transform3D(Basis.from_euler(Vector3(_roll, _gier, _nick)), ort)


func _physics_process(delta: float) -> void:
	var faehrt := fahrer != null and not kaputt
	_klang(faehrt)
	if faehrt and not _test:
		_schub = 0.0 if gesperrt else Input.get_axis("rueckwaerts", "vorwaerts")
		_drehen = 0.0 if gesperrt else Input.get_axis("rechts", "links")
		_steigen = 1.0 if Input.is_action_pressed("springen") and not gesperrt else 0.0
		if Input.is_action_pressed("feuer") and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED \
				and not gesperrt:
			feuern()
	if art == Art.PANZER:
		_panzer(delta, faehrt)
	else:
		_hubschrauber(delta, faehrt)
	# Am Boden angekommen ist Schluss: der Fahrer klettert heraus.
	if kaputt and fahrer != null and absf(tempo) < 1.0:
		var boden := _boden_y()
		if not is_nan(boden) and global_position.y - boden < 4.0:
			get_tree().call_group("welt", "aussteigen_erzwingen")


## Dauerton: Panzerdiesel oder Rotor, solange jemand fahrt; Tonhoehe nach Tempo.
func _klang(faehrt: bool) -> void:
	if _dauerton == null:
		return
	if faehrt != _dauerton.playing:
		if faehrt:
			_dauerton.play()
		else:
			_dauerton.stop()
	var anteil := absf(tempo) / (PANZER_TEMPO if art == Art.PANZER else HELI_TEMPO)
	_dauerton.pitch_scale = 0.85 + 0.35 * clampf(anteil, 0.0, 1.0)


func _panzer(delta: float, faehrt: bool) -> void:
	var ziel := clampf(_schub, -0.6, 1.0) * PANZER_TEMPO
	tempo = move_toward(tempo, ziel if faehrt else 0.0, PANZER_SCHUB * delta)
	# Kettenfahrzeug: dreht auch im Stand, nur langsamer als in Fahrt.
	var rate := PANZER_DREHUNG * (0.45 + 0.55 * absf(tempo) / PANZER_TEMPO)
	_gier += _drehen * rate * delta * (1.0 if faehrt else 0.0)
	var ort := _lage + vorwaerts() * tempo * delta
	var boden := _boden_y()
	if not is_nan(boden):
		# Sanft nachfuehren: ueber einen Bordstein rollt der Panzer, er hupft nicht.
		ort.y = lerpf(ort.y, boden, minf(9.0 * delta, 1.0))
	_setze_position(ort)
	# Der Turm folgt der Blickrichtung der Kamera; das Rohr faehrt nach dem Schuss zurueck.
	if kamera != null and fahrer != null:
		var ziel_winkel := kamera.yaw + PI / 2.0
		_dreher.global_rotation.y = lerp_angle(_dreher.global_rotation.y, ziel_winkel,
				minf(3.0 * delta, 1.0))
	_rueckstoss = lerpf(_rueckstoss, 0.0, minf(4.5 * delta, 1.0))
	_dreher.position = Vector3(0, TURM_HOEHE, 0) \
			+ Basis(Vector3.UP, _gier).inverse() * (_dreher.global_basis.x * -0.7 * _rueckstoss)
	_feuer.light_energy = 6.0 * _rueckstoss if _rueckstoss > 0.55 else 0.0
	if _rueckstoss > 0.01:
		tempo -= 2.6 * _rueckstoss * delta   # der Schuss schiebt den Panzer zurueck


func _hubschrauber(delta: float, faehrt: bool) -> void:
	var ziel := clampf(_schub, -0.5, 1.0) * HELI_TEMPO
	tempo = move_toward(tempo, ziel if faehrt else 0.0, HELI_SCHUB * delta)
	_gier += _drehen * HELI_DREHUNG * delta * (1.0 if faehrt else 0.0)
	# Steigen auf Leertaste, sonst sinkt die Maschine langsam von selbst; ohne Fahrer
	# oder bei Ausfall faellt sie schneller.
	var steig_ziel := -9.0
	if faehrt:
		steig_ziel = HELI_STEIGEN if _steigen > 0.5 else -2.0
	_sinkflug = lerpf(_sinkflug, steig_ziel, minf(2.5 * delta, 1.0))
	var ort := _lage + vorwaerts() * tempo * delta + Vector3(0, _sinkflug * delta, 0)
	var boden := _boden_y()
	if not is_nan(boden) and ort.y < boden + HELI_BODENABSTAND:
		ort.y = boden + HELI_BODENABSTAND
		_sinkflug = maxf(_sinkflug, 0.0)
	elif is_nan(boden) and fahrer == null:
		# Der Sektor unter der Maschine ist (noch) nicht geladen: stehen bleiben statt
		# ins Leere zu fallen.
		ort.y = _lage.y
		_sinkflug = 0.0
	# Nase leicht in Fahrtrichtung, Querlage in die Kurve; ohne Neigung schwebt die
	# Zelle waagerecht wie ein Aufzug.
	var nicken := -0.14 * (tempo / HELI_TEMPO)
	var rollen := -0.21 * clampf(_drehen, -1.0, 1.0) * (0.3 + 0.7 * absf(tempo) / HELI_TEMPO)
	_nick = lerpf(_nick, nicken, minf(2.0 * delta, 1.0))
	_roll = lerpf(_roll, rollen, minf(2.0 * delta, 1.0))
	_setze_position(ort)
	# Rotoren: im Stand langsam, im Flug schnell; der Heckrotor dreht schneller.
	_drehphase = fposmod(_drehphase + (15.7 if faehrt else 1.6) * delta, TAU)
	_dreher.rotation.y = _drehphase
	_heck.rotation.x = _drehphase * 3.8
