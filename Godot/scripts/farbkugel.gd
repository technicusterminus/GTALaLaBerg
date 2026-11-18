class_name Farbkugel
extends Node3D
## Eine Paintball-Kugel: fliegt mit Schwerkraft, hinterlaesst beim Aufprall einen
## Farbklecks (Decal) auf jeder getroffenen Flaeche. Trifft sie eine KI-Figur
## oder einen Wagen, loest sie dessen Reaktion aus; alles andere in der Stadt ist
## Geometrie, dort bleibt der Klecks die einzige Spur. Geflogen wird per Strahl
## von Bild zu Bild, so tunnelt auch eine schnelle Kugel nicht durch duenne Waende.

const LEBENSDAUER := 8.0
const KLECK_DAUER := 45.0
const SCHWERKRAFT := 9.81

static var _klecktextur: ImageTexture

var farbe := Color(0.8, 0.1, 0.5)
var radius := 0.014
var kleck_min := 0.09     ## halbe Kantenlaenge des Kleckses in Metern
var kleck_max := 0.15
var schwerkraft := 0.55   ## Faktor auf die Erdbeschleunigung
var wucht := 0.0          ## Panzerkanone: Schaden in der Mitte des Einschlags
var wucht_radius := 0.0
var halter: Node3D
var geschwindigkeit := Vector3.ZERO

var _alter := 0.0
var _ignoriere: Array[RID] = []


func einrichten(neue_farbe: Color, kugel_radius: float, k_min: float, k_max: float,
		g: float) -> void:
	farbe = neue_farbe
	radius = kugel_radius
	kleck_min = k_min
	kleck_max = k_max
	schwerkraft = g


func abschiessen(richtung: Vector3, tempo: float) -> void:
	geschwindigkeit = richtung.normalized() * tempo


## Die Kugeln einer Salve entstehen dicht nebeneinander und sollen sich nicht treffen.
func ignoriere(koerper: CollisionObject3D) -> void:
	_ignoriere.append(koerper.get_rid())


func _ready() -> void:
	var kugel := MeshInstance3D.new()
	var mesh := SphereMesh.new()
	mesh.radius = radius
	mesh.height = radius * 2.0
	mesh.radial_segments = 8
	mesh.rings = 4
	kugel.mesh = mesh
	var m := StandardMaterial3D.new()
	m.albedo_color = farbe
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	kugel.material_override = m
	kugel.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(kugel)
	if halter is CollisionObject3D:
		_ignoriere.append((halter as CollisionObject3D).get_rid())


func _physics_process(delta: float) -> void:
	_alter += delta
	if _alter > LEBENSDAUER:
		queue_free()
		return
	geschwindigkeit.y -= SCHWERKRAFT * schwerkraft * delta
	var von := global_position
	var nach := von + geschwindigkeit * delta
	var abfrage := PhysicsRayQueryParameters3D.create(von, nach)
	abfrage.exclude = _ignoriere
	var treffer := get_world_3d().direct_space_state.intersect_ray(abfrage)
	if treffer.is_empty():
		global_position = nach
		return
	_aufprall(treffer)
	queue_free()


func _aufprall(treffer: Dictionary) -> void:
	var ort: Vector3 = treffer["position"]
	var normale: Vector3 = treffer["normal"]
	var ziel: Object = treffer["collider"]
	_setze_klecks(ort, normale, ziel)
	var baum := get_tree()
	# Wahrzeichen des Farbkriegs und Scheiben der Schiessbude.
	baum.call_group("revier", "markiere", ziel, farbe)
	baum.call_group("schiessbude", "treffer", ziel)
	if ziel != null and ziel.has_method("erhalte_farbe"):
		ziel.erhalte_farbe(farbe, geschwindigkeit.normalized())
		# Getroffen zu haben uebt das Zielen - eine Hauswand zaehlt nicht.
		Konto.uebe(Konto.Wert.ZIELSICHERHEIT, 6.0)
	if wucht > 0.0 and wucht_radius > 0.0:
		_wirkung_im_umkreis(ort)
	baum.call_group("welt", "zaehle_farbtreffer")


## Panzerkanone: Wirkung im Umkreis, nach aussen linear abnehmend.
func _wirkung_im_umkreis(ort: Vector3) -> void:
	var getroffen := 0
	for n: Node in get_tree().get_nodes_in_group("verletzbar"):
		if n == halter or not (n is Node3D):
			continue
		var weg: Vector3 = (n as Node3D).global_position - ort
		var abstand := weg.length()
		if abstand > wucht_radius:
			continue
		n.verletze(wucht * (1.0 - abstand / wucht_radius),
				weg / abstand if abstand > 0.01 else Vector3.UP, Schaden.Art.SPRENGUNG)
		getroffen += 1
	print("LALABERG_WUCHT radius=%.0fm getroffen=%d" % [wucht_radius, getroffen])


func _setze_klecks(ort: Vector3, normale: Vector3, ziel: Object) -> void:
	var halbe := randf_range(kleck_min, kleck_max)
	var decal := Decal.new()
	decal.size = Vector3(halbe * 2.0, 0.1, halbe * 2.0)
	decal.texture_albedo = _klecktextur_holen()
	decal.modulate = farbe
	decal.cull_mask = 1
	var eltern: Node = get_tree().current_scene
	if ziel is Node:
		eltern = ziel as Node
	# An die getroffene Flaeche gehaengt, damit der Klecks mit einem fahrenden
	# Wagen oder einem gehenden Passanten mitwandert.
	eltern.add_child(decal)
	var hoch := normale.normalized()
	var quer := Vector3.RIGHT if absf(hoch.dot(Vector3.RIGHT)) < 0.9 else Vector3.FORWARD
	var x := hoch.cross(quer).normalized()
	var z := x.cross(hoch).normalized()
	# Der Decal projiziert entlang seiner lokalen -Y; +Y zeigt aus der Flaeche heraus.
	decal.global_transform = Transform3D(Basis(x, hoch, z), ort + hoch * 0.02)
	decal.rotate(hoch, randf() * TAU)
	var uhr := get_tree().create_timer(KLECK_DAUER)
	uhr.timeout.connect(decal.queue_free)


static func _klecktextur_holen() -> ImageTexture:
	if _klecktextur != null:
		return _klecktextur
	const GROESSE := 64
	var bild := Image.create(GROESSE, GROESSE, false, Image.FORMAT_RGBA8)
	var rauschen := FastNoiseLite.new()
	rauschen.frequency = 0.08
	for y in GROESSE:
		for x in GROESSE:
			var d := Vector2(x - GROESSE / 2.0, y - GROESSE / 2.0).length() / (GROESSE / 2.0)
			var rand := 0.72 + 0.28 * rauschen.get_noise_2d(x * 2.0, y * 2.0) * 1.5
			var a := clampf((rand - d) * 6.0, 0.0, 1.0)
			bild.set_pixel(x, y, Color(1, 1, 1, a))
	_klecktextur = ImageTexture.create_from_image(bild)
	return _klecktextur
