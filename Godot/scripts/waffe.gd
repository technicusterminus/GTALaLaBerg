class_name Waffe
extends Node3D
## Der Paintball-Marker: ein einfaches Ansichtsmodell am Koerper der Figur und
## das Abfeuern der Farbkugeln. Keine Munition, die zur Neige geht; die
## Feuerrate begrenzt, wie schnell hintereinander geschossen wird.

enum Art { PISTOLE, MASCHINE, SCHROTFLINTE, RAKETENWERFER }

const NAMEN := ["Paintball-Pistole", "Paintball-MP", "Paintball-Schrotflinte", "Paintball-Werfer"]

## Die kraeftigen Farben eines Paintballs, je Kugel zufaellig.
const KUGELFARBEN := [
	Color(0.92, 0.05, 0.35), Color(0.95, 0.75, 0.05), Color(0.10, 0.70, 0.25),
	Color(0.10, 0.45, 0.90), Color(0.95, 0.35, 0.05), Color(0.70, 0.10, 0.85),
]

## Eine Kennzahl je Waffenart, so bleibt die Feuerlogik fuer alle vier gleich.
## feuerrate: Sekunden zwischen zwei Schuessen; tempo: m/s; streuung: Grad;
## kleck: halbe Kantenlaenge in Metern; muendung: Abstand vor der Wurzel.
const KENNZAHLEN := [
	{"feuerrate": 0.28, "tempo": 32.0, "g": 0.55, "kugeln": 1, "streuung": 0.6,
			"radius": 0.014, "kleck": [0.09, 0.15], "muendung": 0.35},
	{"feuerrate": 0.09, "tempo": 36.0, "g": 0.55, "kugeln": 1, "streuung": 2.2,
			"radius": 0.012, "kleck": [0.07, 0.12], "muendung": 0.5},
	{"feuerrate": 0.75, "tempo": 24.0, "g": 0.60, "kugeln": 7, "streuung": 8.5,
			"radius": 0.009, "kleck": [0.05, 0.09], "muendung": 0.55},
	{"feuerrate": 1.40, "tempo": 15.0, "g": 0.35, "kugeln": 1, "streuung": 0.0,
			"radius": 0.06, "kleck": [0.42, 0.58], "muendung": 0.5},
]

## Die Modelle (Quaternius, CC0) liegen mit dem Lauf entlang X; laenge ist die Soll-Laenge in
## Metern, drehung dreht den Lauf nach -Z (-90 Grad: Muendung bei +X, +90: bei -X).
const MODELLE := {
	Art.PISTOLE: {"datei": "Pistol", "laenge": 0.26, "drehung": -90.0},
	Art.MASCHINE: {"datei": "Smg", "laenge": 0.50, "drehung": -90.0},
	Art.SCHROTFLINTE: {"datei": "Shotgun", "laenge": 0.85, "drehung": -90.0},
	Art.RAKETENWERFER: {"datei": "RocketLauncher", "laenge": 0.8, "drehung": -90.0},
}


const SCHUSSKLANG := {
	Art.PISTOLE: "SFX_Schuss_Pistole", Art.MASCHINE: "SFX_Schuss_Maschine",
	Art.SCHROTFLINTE: "SFX_Schuss_Schrot", Art.RAKETENWERFER: "SFX_Schuss_Rakete",
}

var art := Art.PISTOLE
var schuesse := 0

var _letzter_schuss := -10.0
var _modell := Node3D.new()


func _ready() -> void:
	add_child(_modell)
	_baue_modell()


func art_name() -> String:
	return NAMEN[art]


func setze_art(neu: int) -> void:
	if art == neu:
		return
	art = neu as Art
	_baue_modell()


## Loest einen Schuss aus, wenn die Feuerrate es zulaesst; true bei Schuss.
## ort: Muendung in Weltkoordinaten, richtung: Zielrichtung.
func feuern(ort: Vector3, richtung: Vector3, halter: Node3D) -> bool:
	var k: Dictionary = KENNZAHLEN[art]
	var jetzt := Time.get_ticks_msec() / 1000.0
	if jetzt - _letzter_schuss < k["feuerrate"]:
		return false
	_letzter_schuss = jetzt
	Ton.einmal(get_tree(), SCHUSSKLANG[art], ort, -3.0, randf_range(0.96, 1.04))
	get_tree().call_group("passanten", "hoere_schuss", ort)
	get_tree().call_group("verkehrsauto", "hoere_schuss", ort)
	for i in int(k["kugeln"]):
		# Zielsicherheit halbiert die Streuung, wenn sie ausgebaut ist.
		var streuung: float = k["streuung"] * (1.0 - 0.5 * Konto.anteil(Konto.Wert.ZIELSICHERHEIT))
		var kegel := _kegel(richtung, deg_to_rad(streuung)) if streuung > 0.0 else richtung
		var kugel := Farbkugel.new()
		kugel.halter = halter
		var kleck: Array = k["kleck"]
		kugel.einrichten(KUGELFARBEN[randi() % KUGELFARBEN.size()], k["radius"],
				kleck[0], kleck[1], k["g"])
		get_tree().current_scene.add_child(kugel)
		kugel.global_position = ort
		kugel.abschiessen(kegel, k["tempo"])
	schuesse += 1
	return true


func muendung_abstand() -> float:
	return KENNZAHLEN[art]["muendung"]


## Zufaellige Richtung in einem Kegel um "richtung".
func _kegel(richtung: Vector3, halbwinkel: float) -> Vector3:
	var r := richtung.normalized()
	var hoch := Vector3.UP if absf(r.y) < 0.99 else Vector3.RIGHT
	var basis := Basis.looking_at(r, hoch)
	var winkel := acos(lerpf(1.0, cos(halbwinkel), randf()))
	var phi := randf() * TAU
	return basis * Vector3(sin(winkel) * cos(phi), sin(winkel) * sin(phi), -cos(winkel))


## Einfaches Ansichtsmodell aus Kasten und Zylinder; zeigt nach -Z.
## Setzt das Modell aus der GLB-Datei ein; false, wenn sie fehlt (dann bleiben die Kasten).
func _lade_glb() -> bool:
	var k: Dictionary = MODELLE[art]
	var pfad := "res://daten/modelle/%s.glb" % k["datei"]
	if not ResourceLoader.exists(pfad) or not is_inside_tree():
		return false
	var szene := load(pfad) as PackedScene
	if szene == null:
		return false
	var n := szene.instantiate() as Node3D
	_modell.add_child(n)
	var box := _huelle(n)
	if box.size.x < 0.000001:
		n.queue_free()
		return false
	n.scale = Vector3.ONE * (float(k["laenge"]) / box.size.x)
	n.rotation.y = deg_to_rad(float(k["drehung"]))
	# Hinten (Griff) knapp hinter dem Ursprung, mittig in X und auf Handhoehe.
	var neu := _huelle(n)
	n.position += Vector3(-(neu.position.x + neu.size.x / 2.0), -(neu.position.y + neu.size.y * 0.7),
			0.1 - neu.end.z)
	for m: MeshInstance3D in n.find_children("*", "MeshInstance3D", true, false):
		m.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	return true


func _huelle(wurzel: Node3D) -> AABB:
	var summe := AABB()
	var erste := true
	for m: MeshInstance3D in wurzel.find_children("*", "MeshInstance3D", true, false):
		var b: AABB = _modell.global_transform.affine_inverse() * m.global_transform \
				* m.get_aabb()
		summe = b if erste else summe.merge(b)
		erste = false
	return summe


func _baue_modell() -> void:
	for c in _modell.get_children():
		c.queue_free()
	if _lade_glb():
		return
	var metall := StandardMaterial3D.new()
	metall.albedo_color = Color(0.09, 0.09, 0.10)
	metall.metallic = 0.6
	metall.roughness = 0.4
	var oliv := StandardMaterial3D.new()
	oliv.albedo_color = Color(0.20, 0.22, 0.14)
	match art:
		Art.PISTOLE:
			_kasten(Vector3(0.05, 0.07, 0.22), Vector3(0, 0, -0.08), metall)
			_kasten(Vector3(0.04, 0.10, 0.05), Vector3(0, -0.08, 0.0), metall)
		Art.MASCHINE:
			_kasten(Vector3(0.06, 0.09, 0.40), Vector3(0, 0, -0.15), oliv)
			_kasten(Vector3(0.04, 0.12, 0.05), Vector3(0, -0.10, -0.05), metall)
			_kasten(Vector3(0.03, 0.03, 0.22), Vector3(0, 0.0, -0.45), metall)
		Art.SCHROTFLINTE:
			_kasten(Vector3(0.06, 0.08, 0.50), Vector3(0, 0, -0.22), oliv)
			_kasten(Vector3(0.05, 0.05, 0.40), Vector3(0, -0.04, -0.40), metall)
		Art.RAKETENWERFER:
			_kasten(Vector3(0.16, 0.16, 0.70), Vector3(0.05, 0.06, -0.2), oliv)
			_kasten(Vector3(0.05, 0.09, 0.05), Vector3(0.05, -0.05, 0.0), metall)


func _kasten(groesse: Vector3, ort: Vector3, material: Material) -> void:
	var m := MeshInstance3D.new()
	var b := BoxMesh.new()
	b.size = groesse
	m.mesh = b
	m.position = ort
	m.material_override = material
	m.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	_modell.add_child(m)
