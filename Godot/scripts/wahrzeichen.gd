class_name Wahrzeichen
extends Node
## Nachgebaute Wahrzeichen nach dem Zielbild ziel-hauptplatz-real.png: der Marienbrunnen mitten
## auf dem Hauptplatz (achteckiges Becken, Saeule, Madonna, Gitter) und der Schmalzturm mit
## buntem, glasiertem Ziegeldach und Torbogen. Beide stehen, sobald ihr Sektor geladen ist,
## auf dem Boden, den ein Strahl von oben findet.

const STEIN := Color(0.62, 0.58, 0.52)
const TURMPUTZ := Color(0.86, 0.82, 0.72)

var stadt: Stadt
var polizei: Polizei
var gestellt: Array[String] = []

var _offen := {}   # Name -> Callable, die das Modell an einem Ort baut


func _ready() -> void:
	_offen["Marienbrunnen"] = _brunnen
	_offen["Schmalzturm"] = _turm
	_offen["Historisches Rathaus"] = Callable()   # Fassade, siehe _rathaus
	_offen["Cafe"] = Callable()                    # Tische am Platz, siehe _cafes
	if stadt != null:
		stadt.sektor_bereit.connect(func(_id: String) -> void: _versuche())


func _ort(name: String) -> Vector3:
	if name == "Marienbrunnen" or name == "Cafe":
		return _hauptplatz_mitte()
	return Orte.mitte(name)


func _hauptplatz_mitte() -> Vector3:
	var o = JSON.parse_string(FileAccess.get_file_as_string("res://daten/orte.json"))
	if o is Dictionary:
		for p: Dictionary in o["plaetze"]:
			if p["n"] == "Hauptplatz":
				var summe := Vector2.ZERO
				var punkte: Array = p["p"]
				for i in range(0, punkte.size() - 1, 2):
					summe += Vector2(punkte[i], punkte[i + 1])
				summe /= maxi(1, punkte.size() / 2)
				return Vector3(summe.x, 0.0, summe.y) / 100.0
	return Vector3.ZERO


func _versuche() -> void:
	for name: String in _offen.keys():
		var ort := _ort(name)
		if ort == Vector3.ZERO:
			continue
		# Bodenhoehe: von der Strassenhoehe der Umgebung aus nach unten tasten (nicht von hoch
		# oben, sonst trifft der Strahl ein Dach).
		var nah := polizei.naechster_punkt(ort) if polizei != null else ort
		var von := Vector3(ort.x, nah.y + 6.0, ort.z)
		var abfrage := PhysicsRayQueryParameters3D.create(von, von - Vector3(0, 40, 0))
		abfrage.collision_mask = 1
		var treffer := stadt.get_world_3d().direct_space_state.intersect_ray(abfrage)
		if treffer.is_empty():
			continue
		if name == "Historisches Rathaus":
			if _rathaus(ort, treffer["position"].y):
				_offen.erase(name)
				gestellt.append(name)
			continue
		if name == "Cafe":
			if _cafes(_hauptplatz_mitte(), treffer["position"].y):
				_offen.erase(name)
				gestellt.append(name)
			continue
		var bau: Callable = _offen[name]
		var modell: Node3D = bau.call()
		add_child(modell)
		modell.global_position = Vector3(ort.x, treffer["position"].y, ort.z)
		_offen.erase(name)
		gestellt.append(name)
		print("LALABERG_WAHRZEICHEN %s bei %s" % [name, modell.global_position])


static func _stoff(farbe: Color, rau := 0.85) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = farbe
	m.roughness = rau
	return m


static func _teil(eltern: Node3D, netz: Mesh, ort: Vector3, stoff: Material) -> MeshInstance3D:
	var mi := MeshInstance3D.new()
	mi.mesh = netz
	mi.position = ort
	mi.material_override = stoff
	eltern.add_child(mi)
	return mi


static func _zylinder(r_unten: float, r_oben: float, h: float, seg := 8) -> CylinderMesh:
	var c := CylinderMesh.new()
	c.bottom_radius = r_unten
	c.top_radius = r_oben
	c.height = h
	c.radial_segments = seg
	c.rings = 1
	return c


static func _kasten(g: Vector3) -> BoxMesh:
	var b := BoxMesh.new()
	b.size = g
	return b


## Marienbrunnen: Stufe, achteckiges Becken mit Wasser, Saeule, Sockel und goldene Madonna,
## schmiedeeisernes Gitter um das Becken.
func _brunnen() -> Node3D:
	var w := Node3D.new()
	w.name = "Marienbrunnen"
	var stein := _stoff(STEIN)
	_teil(w, _zylinder(4.6, 4.6, 0.25, 8), Vector3(0, 0.12, 0), stein)
	_teil(w, _zylinder(4.0, 4.0, 0.7, 8), Vector3(0, 0.6, 0), stein)
	var rand := TorusMesh.new()
	rand.inner_radius = 3.65
	rand.outer_radius = 4.05
	rand.rings = 8
	rand.ring_segments = 8
	var rr := _teil(w, rand, Vector3(0, 1.0, 0), stein)
	rr.scale = Vector3(1.0, 0.6, 1.0)
	var wasser := StandardMaterial3D.new()
	wasser.albedo_color = Color(0.10, 0.32, 0.34)
	wasser.roughness = 0.05
	wasser.metallic = 0.2
	# Becken als Ring (Rand), darin das Wasser knapp unter der Kante.
	_teil(w, _zylinder(3.7, 3.7, 0.05, 8), Vector3(0, 1.08, 0), wasser)
	_teil(w, _zylinder(0.55, 0.45, 1.2, 8), Vector3(0, 1.5, 0), stein)
	_teil(w, _zylinder(0.28, 0.24, 4.2, 12), Vector3(0, 4.2, 0), _stoff(Color(0.70, 0.66, 0.60)))
	_teil(w, _kasten(Vector3(0.75, 0.35, 0.75)), Vector3(0, 6.45, 0), stein)
	var gold := StandardMaterial3D.new()
	gold.albedo_color = Color(0.85, 0.66, 0.25)
	gold.metallic = 0.85
	gold.roughness = 0.3
	# Madonna: Gewand (Kegel), Koerper, Kopf, Strahlenkranz.
	_teil(w, _zylinder(0.38, 0.16, 1.2, 10), Vector3(0, 7.2, 0), gold)
	var kopf := SphereMesh.new()
	kopf.radius = 0.15
	kopf.height = 0.3
	_teil(w, kopf, Vector3(0, 7.95, 0), gold)
	var kranz := TorusMesh.new()
	kranz.inner_radius = 0.2
	kranz.outer_radius = 0.26
	var k := _teil(w, kranz, Vector3(0, 8.0, -0.05), gold)
	k.rotation.x = PI / 2.0
	# Plaetschern (gerechnet, Tools/Sounds/SFX_Brunnen.wav), hoerbar bis gut 40 m.
	var klang := Ton.schleife(w, "SFX_Brunnen", -4.0)
	if klang != null:
		klang.position = Vector3(0, 1.2, 0)
		klang.max_distance = 45.0
		klang.autoplay = true
	# Gitter: duenne Staebe rund um das Becken.
	var eisen := _stoff(Color(0.08, 0.08, 0.09), 0.5)
	var stab := _kasten(Vector3(0.04, 1.1, 0.04))
	for i in 48:
		var a := TAU * float(i) / 48.0
		_teil(w, stab, Vector3(cos(a) * 4.3, 0.8, sin(a) * 4.3), eisen)
	var ring := TorusMesh.new()
	ring.inner_radius = 4.27
	ring.outer_radius = 4.33
	_teil(w, ring, Vector3(0, 1.35, 0), eisen)
	return w


## Schmalzturm: Torturm mit hellem Putz, Torbogen, Gesimsen, kleinen Fenstern und steilem
## Walmdach mit glasierten Ziegeln im Rautenmuster (grün, gelb, rot), Turmknauf.
func _turm() -> Node3D:
	var t := Node3D.new()
	t.name = "Schmalzturm"
	var putz := _stoff(TURMPUTZ)
	var hoehe := 24.0
	_teil(t, _kasten(Vector3(8.2, hoehe, 8.2)), Vector3(0, hoehe / 2.0, 0), putz)
	var stein := _stoff(Color(0.60, 0.56, 0.50))
	_teil(t, _kasten(Vector3(8.6, 2.5, 8.6)), Vector3(0, 1.25, 0), stein)
	for y in [8.0, 16.0, hoehe]:
		_teil(t, _kasten(Vector3(8.6, 0.35, 8.6)), Vector3(0, y, 0), stein)
	# Torbogen auf zwei Seiten (dunkle Durchfahrt).
	var dunkel := _stoff(Color(0.10, 0.09, 0.08), 0.95)
	for z in [4.12, -4.12]:
		_teil(t, _kasten(Vector3(3.6, 5.0, 0.1)), Vector3(0, 2.5, z), dunkel)
		var bogen := _teil(t, _zylinder(1.8, 1.8, 0.1, 16), Vector3(0, 5.0, z), dunkel)
		bogen.rotation.x = PI / 2.0
	# Fenster
	var glas := _stoff(Color(0.06, 0.08, 0.12), 0.2)
	for y in [11.0, 13.5, 19.5]:
		for seite in 4:
			var a := seite * PI / 2.0
			var f := _teil(t, _kasten(Vector3(0.9, 1.5, 0.1)),
					Vector3(sin(a) * 4.13, y, cos(a) * 4.13), glas)
			f.rotation.y = a
	# Dach: steile Pyramide mit glasierten Ziegeln.
	var dach := CylinderMesh.new()
	dach.top_radius = 0.0
	dach.bottom_radius = 6.4
	dach.height = 13.0
	dach.radial_segments = 4
	dach.rings = 1
	var d := _teil(t, dach, Vector3(0, hoehe + 6.5, 0), _glasur())
	d.rotation.y = PI / 4.0
	var knauf := SphereMesh.new()
	knauf.radius = 0.35
	knauf.height = 0.7
	var gold := StandardMaterial3D.new()
	gold.albedo_color = Color(0.85, 0.66, 0.25)
	gold.metallic = 0.85
	gold.roughness = 0.3
	_teil(t, knauf, Vector3(0, hoehe + 13.3, 0), gold)
	_teil(t, _zylinder(0.04, 0.04, 2.0, 6), Vector3(0, hoehe + 14.5, 0), gold)
	return t


## Glasierte Dachziegel im Rautenmuster, wie auf dem echten Schmalzturm.
static func _glasur() -> ShaderMaterial:
	var sh := Shader.new()
	sh.code = """
shader_type spatial;
varying vec3 w;
void vertex() { w = VERTEX; }
void fragment() {
	vec2 uv = vec2(atan(w.x, w.z) * 3.0, w.y * 2.2);
	vec2 r = vec2(uv.x + uv.y, uv.x - uv.y);
	vec2 zelle = floor(r);
	float m = mod(zelle.x + zelle.y * 2.0, 3.0);
	vec3 c = vec3(0.55, 0.14, 0.08);
	if (m < 1.0) { c = vec3(0.10, 0.36, 0.16); }
	else if (m < 2.0) { c = vec3(0.78, 0.60, 0.10); }
	vec2 f = fract(r);
	float fuge = smoothstep(0.0, 0.06, f.x) * smoothstep(0.0, 0.06, f.y);
	ALBEDO = c * (0.55 + 0.45 * fuge);
	ROUGHNESS = 0.25;
	SPECULAR = 0.8;
}
"""
	var m := ShaderMaterial.new()
	m.shader = sh
	return m


## Wand vor sich: waagerechter Strahl von von in Richtung richtung, bis 60 m. {} ohne Treffer.
func _wand(von: Vector3, richtung: Vector3) -> Dictionary:
	var abfrage := PhysicsRayQueryParameters3D.create(von, von + richtung.normalized() * 90.0)
	abfrage.collision_mask = 1
	var t := stadt.get_world_3d().direct_space_state.intersect_ray(abfrage)
	if t.is_empty() or absf(t["normal"].y) > 0.3:
		return {}
	return t


## Flaeche mit Textur (Alpha), Unterkante bei ort, Gesicht in Richtung normale.
static func _bildflaeche(textur: Texture2D, breite: float, normale: Vector3,
		beidseitig := false) -> MeshInstance3D:
	var q := QuadMesh.new()
	var hoehe := breite * float(textur.get_height()) / float(textur.get_width())
	q.size = Vector2(breite, hoehe)
	q.center_offset = Vector3(0, hoehe / 2.0, 0)
	var m := StandardMaterial3D.new()
	m.albedo_texture = textur
	m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA_SCISSOR
	m.alpha_scissor_threshold = 0.5
	m.roughness = 0.85
	if beidseitig:
		m.cull_mode = BaseMaterial3D.CULL_DISABLED
	var mi := MeshInstance3D.new()
	mi.mesh = q
	mi.material_override = m
	mi.basis = Basis.looking_at(-normale, Vector3.UP)
	return mi


## Die Stuckfassade des Historischen Rathauses (vom Bildmodell ueber OpenRouter gemalt) vor die
## Hauswand zum Hauptplatz.
func _rathaus(ort: Vector3, boden_y: float) -> bool:
	var pfad := "res://texturen/rathaus_fassade.png"
	if not ResourceLoader.exists(pfad):
		return true
	var platz := _hauptplatz_mitte()
	var richtung := Vector3(platz.x - ort.x, 0, platz.z - ort.z)
	# Vom Platz aus auf das Rathaus schauen und die Wand suchen.
	var von := Vector3(platz.x, boden_y + 4.0, platz.z)
	var t := _wand(von, -richtung)
	if t.is_empty():
		return false
	var normale := Vector3(t["normal"].x, 0, t["normal"].z).normalized()
	var flaeche := _bildflaeche(load(pfad), 13.0, normale)
	add_child(flaeche)
	flaeche.global_position = Vector3(t["position"].x, boden_y - 0.2, t["position"].z) \
			+ normale * 0.12
	print("LALABERG_WAHRZEICHEN Rathausfassade bei %s" % flaeche.global_position)
	return true


## Cafe-Tische mit Schirm (vom Bildmodell gemalt, je zwei gekreuzte Flaechen) vor den Hauswaenden
## rund um den Hauptplatz.
func _cafes(platz: Vector3, boden_y: float) -> bool:
	var pfad := "res://texturen/cafe.png"
	if not ResourceLoader.exists(pfad):
		return true
	var tex: Texture2D = load(pfad)
	var zahl := 0
	for i in 16:
		var w := TAU * float(i) / 16.0 + 0.2
		var richtung := Vector3(cos(w), 0, sin(w))
		var t := _wand(Vector3(platz.x, boden_y + 1.5, platz.z), richtung)
		if t.is_empty():
			continue
		var normale := Vector3(t["normal"].x, 0, t["normal"].z).normalized()
		for k in 2:
			var seitlich := normale.cross(Vector3.UP) * (k * 4.5 - 2.25)
			var ort := Vector3(t["position"].x, boden_y, t["position"].z) + normale * 3.5 + seitlich
			var gruppe := Node3D.new()
			for drehung in [0.0, PI / 2.0]:
				var f := _bildflaeche(tex, 3.0, normale.rotated(Vector3.UP, drehung), true)
				gruppe.add_child(f)
			add_child(gruppe)
			gruppe.global_position = ort
			if zahl == 0:
				print("LALABERG_WAHRZEICHEN erstes Cafe bei %s" % ort)
			zahl += 1
	print("LALABERG_WAHRZEICHEN Cafes=%d" % zahl)
	return zahl > 0
