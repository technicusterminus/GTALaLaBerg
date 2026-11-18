extends Node3D
## Wurzel des Spiels: baut Licht, Stadt, Spieler, Auto, Kamera und HUD zusammen
## und kuemmert sich um Ein- und Aussteigen.
##
## Rauchtest ohne Fenster:  godot --path Godot --headless -- --rauchtest

const EINSTEIGEN_M := 4.0

var stadt := Stadt.new()
var spieler := Spieler.new()
var kamera := Kamera.new()
var hud := Hud.new()
var verkehr := Verkehr.new()
var auto: Auto
var aktuelles_auto: Auto

var _rauchtest := false
var _rauch_zeit := 0.0


func _ready() -> void:
	_eingaben()
	_umgebung()
	_rauchtest = "--rauchtest" in OS.get_cmdline_user_args()

	add_child(stadt)
	add_child(kamera)
	add_child(spieler)
	add_child(hud)
	add_child(verkehr)
	spieler.kamera = kamera
	spieler.aktiv(false)  # bis unter dem Spawn der Boden steht
	kamera.ziel = spieler
	kamera.ausnehmen(spieler)
	stadt.fokus = spieler
	hud.ziel = spieler

	var s: Dictionary = stadt.spawn
	if s.is_empty():
		return  # Daten fehlen, Fehlermeldung kam schon aus stadt.gd
	var start := Vector3(s["x"], s["y"], s["z"])
	spieler.global_position = start + Vector3(0, 3, 0)
	kamera.yaw = deg_to_rad(float(s["blick"]))
	stadt.sektor_bereit.connect(_sektor_bereit.bind(start), CONNECT_ONE_SHOT)
	if not _rauchtest:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func _sektor_bereit(id: String, start: Vector3) -> void:
	# Der erste fertige Sektor kann ein anderer sein als der unter dem Spawn.
	if stadt.eintrag_fuer(start).get("id", "") != id:
		stadt.sektor_bereit.connect(_sektor_bereit.bind(start), CONNECT_ONE_SHOT)
		return
	# Die Kollisionsform steht erst nach einem Physikschritt im Raum.
	await get_tree().physics_frame
	await get_tree().physics_frame
	var boden := _boden(start)
	spieler.global_position = boden + Vector3(0, 0.1, 0)
	spieler.aktiv(true)
	auto = Auto.new()
	add_child(auto)
	var voraus := kamera.vorwaerts() * 7.0
	auto.global_position = _boden(boden + voraus) + Vector3(0, 0.9, 0)
	auto.rotation.y = kamera.yaw
	print("LALABERG_START boden=%s auto=%s" % [boden, auto.global_position])


## Hoehe des Bodens unter einer Stelle; faellt auf die Stelle selbst zurueck.
func _boden(pos: Vector3) -> Vector3:
	var von := pos + Vector3(0, 300, 0)
	var abfrage := PhysicsRayQueryParameters3D.create(von, pos + Vector3(0, -300, 0))
	abfrage.collision_mask = 1
	var treffer := get_world_3d().direct_space_state.intersect_ray(abfrage)
	return treffer["position"] if not treffer.is_empty() else pos


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("menue"):
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE \
				if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED else Input.MOUSE_MODE_CAPTURED
	elif event.is_action_pressed("einsteigen") and auto != null:
		if aktuelles_auto == null:
			_einsteigen()
		else:
			_aussteigen()
	elif event is InputEventMouseButton and event.pressed:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func _einsteigen() -> void:
	var nah: Auto = null
	var bester := EINSTEIGEN_M
	for a: Auto in get_tree().get_nodes_in_group("auto"):
		var d := a.global_position.distance_to(spieler.global_position)
		if d < bester:
			bester = d
			nah = a
	if nah == null:
		return
	aktuelles_auto = nah
	nah.fahrer = spieler
	spieler.aktiv(false)
	kamera.ziel = nah
	kamera.hoehe = 1.2
	kamera.abstand_setzen(7.0)
	kamera.ausnehmen(nah)
	stadt.fokus = nah
	hud.ziel = nah


func _aussteigen() -> void:
	var a := aktuelles_auto
	aktuelles_auto = null
	a.fahrer = null
	spieler.global_position = a.global_position - a.global_basis.x * 2.2 + Vector3(0, 0.6, 0)
	spieler.aktiv(true)
	kamera.ziel = spieler
	kamera.hoehe = 1.5
	kamera.abstand_setzen(4.5)
	kamera.ausnehmen(spieler)
	kamera.folge_yaw = NAN
	stadt.fokus = spieler
	hud.ziel = spieler
	hud.tempo_ms = 0.0


func _process(delta: float) -> void:
	if aktuelles_auto != null:
		var vor := -aktuelles_auto.global_basis.z
		kamera.folge_yaw = atan2(-vor.x, -vor.z) if aktuelles_auto.tempo_ms > 2.0 else NAN
		hud.tempo_ms = aktuelles_auto.tempo_ms
		hud.hinweis = "F  aussteigen"
	elif auto != null and spieler.global_position.distance_to(auto.global_position) < EINSTEIGEN_M:
		hud.hinweis = "F  einsteigen"
	else:
		hud.hinweis = ""
	if _rauchtest:
		_rauch_zeit += delta
		if _rauch_zeit > 25.0:
			var p := spieler.global_position
			var ki := verkehr.bewegung()
			print("LALABERG_RAUCHTEST ende pos=%s auto=%s ki_bewegt=%.1f fps=%d"
					% [p, auto.global_position if auto != null else "-", ki,
					Engine.get_frames_per_second()])
			get_tree().quit(0 if auto != null and ki > 1.0 else 1)


# --- Aufbau -----------------------------------------------------------------

func _umgebung() -> void:
	var himmel := ProceduralSkyMaterial.new()
	himmel.sky_top_color = Color(0.25, 0.45, 0.78)
	himmel.sky_horizon_color = Color(0.65, 0.76, 0.88)
	himmel.ground_horizon_color = Color(0.6, 0.64, 0.68)
	var sky := Sky.new()
	sky.sky_material = himmel
	var env := Environment.new()
	env.background_mode = Environment.BG_SKY
	env.sky = sky
	env.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	env.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	env.fog_enabled = true
	env.fog_light_color = Color(0.7, 0.78, 0.88)
	env.fog_density = 0.0004
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)

	var sonne := DirectionalLight3D.new()
	sonne.rotation_degrees = Vector3(-50, 152, 0)
	sonne.shadow_enabled = true
	sonne.directional_shadow_max_distance = 250.0
	sonne.light_energy = 1.2
	add_child(sonne)


func _eingaben() -> void:
	var tasten := {
		"vorwaerts": [KEY_W, KEY_UP], "rueckwaerts": [KEY_S, KEY_DOWN],
		"links": [KEY_A, KEY_LEFT], "rechts": [KEY_D, KEY_RIGHT],
		"rennen": [KEY_SHIFT], "springen": [KEY_SPACE], "handbremse": [KEY_SPACE],
		"einsteigen": [KEY_F, KEY_ENTER], "menue": [KEY_ESCAPE],
	}
	for aktion: String in tasten:
		if not InputMap.has_action(aktion):
			InputMap.add_action(aktion)
		for taste: int in tasten[aktion]:
			var e := InputEventKey.new()
			e.physical_keycode = taste as Key
			InputMap.action_add_event(aktion, e)
