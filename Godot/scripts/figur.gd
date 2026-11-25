class_name Figur
extends Node3D
## Eine animierte Menschenfigur aus den Quaternius-Modellen (Casual, Worker; CC0, siehe
## Content/SourceData/People/LIZENZ.md). Die Modelle blicken nach +Z; hier dreht ein
## Innenknoten sie so, dass die Figur wie alles in Godot nach -Z schaut. Fehlt die
## Modelldatei (Converter nicht gelaufen), liefert neu() null und der Aufrufer nimmt
## seine Kapsel.

const ARTEN := ["Casual", "Worker"]
const HOEHE := 1.78
## Clips, die einmal laufen und dann stehen bleiben.
const EINMAL := ["Death", "Roll", "Gun_Shoot", "HitRecieve", "HitRecieve_2"]

static var _szenen := {}

var _anim: AnimationPlayer
var _aktuell := ""
var _innen := Node3D.new()
var _skelett: Skeleton3D   ## gesetzt von fahrhaltung (fuer tritt)


static func verfuegbar() -> bool:
	return FileAccess.file_exists("res://daten/modelle/Casual.fbx.import")


## art: Index in ARTEN (wird umlaufend gewaehlt).
static func neu(art: int) -> Figur:
	var name: String = ARTEN[posmod(art, ARTEN.size())]
	if not _szenen.has(name):
		var pfad := "res://daten/modelle/%s.fbx" % name
		_szenen[name] = load(pfad) if ResourceLoader.exists(pfad) else null
	var szene: PackedScene = _szenen[name]
	if szene == null:
		return null
	var f := Figur.new()
	f.name = "Figur"
	f._innen.rotation.y = PI
	f.add_child(f._innen)
	var modell := szene.instantiate()
	f._innen.add_child(modell)
	f._anim = modell.find_child("AnimationPlayer", true, false) as AnimationPlayer
	if f._anim != null:
		for n in f._anim.get_animation_list():
			var a := f._anim.get_animation(n)
			var k := String(n).get_slice("|", 1)
			a.loop_mode = Animation.LOOP_NONE if k in EINMAL else Animation.LOOP_LINEAR
	f.spiele("Idle")
	return f


## Spielt "Idle", "Walk", "Run", "Death", "Wave" ... (ohne das Praefix des Rigs).
## tempo skaliert die Abspielgeschwindigkeit; ein schon laufender Clip bleibt.
func spiele(clip: String, tempo := 1.0) -> void:
	if _anim == null:
		return
	_anim.speed_scale = tempo
	if clip == _aktuell:
		return
	var voll := "CharacterArmature|" + clip
	if not _anim.has_animation(voll):
		return
	_aktuell = clip
	_anim.play(voll, 0.18)


## Sitzhaltung auf einem Zweirad: Animation aus, Oberschenkel nach vorn, Knie gebeugt, Arme
## nach vorn und unten zum Lenker, Oberkoerper vorgebeugt. Winkel in Grad (siehe HALTUNG).
## Die Achsen sind aus dem Rig ermittelt: Oberschenkel und Arme drehen um ihre lokale Z-Achse
## (rechts gespiegelt), Knie, Armsenken und Oberkoerper um X. Die Fuesse haengen im Rig an der
## Wurzel und werden an die Enden der Unterschenkel gesetzt.
const HALTUNG := {
	"fahrrad": {"bein": 62.0, "knie": 75.0, "arm": 62.0, "senken": 28.0, "unterarm": 12.0, "vor": 16.0},
	"motorrad": {"bein": 72.0, "knie": 95.0, "arm": 66.0, "senken": 34.0, "unterarm": 22.0, "vor": 30.0},
}


func fahrhaltung(art: String) -> void:
	var h: Dictionary = HALTUNG.get(art, HALTUNG["fahrrad"])
	if _anim != null:
		_anim.stop()
		_anim.active = false
	var sk: Skeleton3D = null
	for k in _innen.find_children("*", "Skeleton3D", true, false):
		sk = k
	if sk == null:
		return
	var setze := func(knochen: String, q: Quaternion) -> void:
		var i := sk.find_bone(knochen)
		if i >= 0:
			sk.set_bone_pose_rotation(i, sk.get_bone_rest(i).basis.get_rotation_quaternion() * q)
	for seite in [[".L", 1.0], [".R", -1.0]]:
		var n: String = seite[0]
		var v: float = seite[1]
		setze.call("UpperLeg" + n, Quaternion(Vector3.BACK, deg_to_rad(v * float(h["bein"]))))
		setze.call("LowerLeg" + n, Quaternion(Vector3.RIGHT, deg_to_rad(float(h["knie"]))))
		setze.call("UpperArm" + n, Quaternion(Vector3.BACK, deg_to_rad(-v * float(h["arm"])))
				* Quaternion(Vector3.RIGHT, deg_to_rad(-float(h["senken"]))))
		setze.call("LowerArm" + n, Quaternion(Vector3.BACK, deg_to_rad(-v * float(h["unterarm"]))))
	setze.call("Torso", Quaternion(Vector3.RIGHT, deg_to_rad(float(h["vor"]))))
	_skelett = sk
	_fuesse_setzen(sk)


## Treten: jedes Bein mit zwei Gelenken auf sein Pedal (Weltkoordinaten, erst links, dann
## rechts) gestellt. Hueftpunkt und Beinlaengen aus der Ruhelage des Rigs; in der Ebene aus
## vorwaerts und oben ergeben sich Oberschenkelwinkel und Kniebeugung (Kosinussatz).
func tritt(pedale: Array, vorwaerts: Vector3, oben: Vector3) -> void:
	var sk := _skelett
	if sk == null or not sk.is_inside_tree():
		return
	var seiten := [[".L", 1.0], [".R", -1.0]]
	for k in 2:
		var n: String = seiten[k][0]
		var v: float = seiten[k][1]
		var ober := sk.find_bone("UpperLeg" + n)
		var knie := sk.find_bone("LowerLeg" + n)
		var ende := sk.find_bone("LowerLeg" + n + "_end")
		if ober < 0 or knie < 0 or ende < 0:
			continue
		var g := sk.global_transform
		var huefte := g * sk.get_bone_global_rest(ober).origin
		var l1 := huefte.distance_to(g * sk.get_bone_global_rest(knie).origin)
		var l2 := (g * sk.get_bone_global_rest(knie).origin).distance_to(g * sk.get_bone_global_rest(ende).origin)
		var t: Vector3 = (pedale[k] as Vector3) - huefte
		var vor := t.dot(vorwaerts)
		var hoch := t.dot(oben)
		var d := clampf(sqrt(vor * vor + hoch * hoch), absf(l1 - l2) + 0.01, (l1 + l2) * 0.995)
		var beuge := PI - acos(clampf((l1 * l1 + l2 * l2 - d * d) / (2.0 * l1 * l2), -1.0, 1.0))
		var winkel := atan2(vor, -hoch) + acos(clampf((l1 * l1 + d * d - l2 * l2) / (2.0 * l1 * d), -1.0, 1.0))
		sk.set_bone_pose_rotation(ober, sk.get_bone_rest(ober).basis.get_rotation_quaternion()
				* Quaternion(Vector3.BACK, v * winkel))
		sk.set_bone_pose_rotation(knie, sk.get_bone_rest(knie).basis.get_rotation_quaternion()
				* Quaternion(Vector3.RIGHT, beuge))
	_fuesse_setzen(sk)


## Die Fuesse haengen im Rig an der Wurzel: an die Enden der Unterschenkel setzen.
func _fuesse_setzen(sk: Skeleton3D) -> void:
	for n in [".L", ".R"]:
		var fuss := sk.find_bone("Foot" + n)
		var ende := sk.find_bone("LowerLeg" + n + "_end")
		if fuss < 0 or ende < 0:
			continue
		var ziel := _knochen_global(sk, ende).origin
		var eltern := sk.get_bone_parent(fuss)
		var basis := _knochen_global(sk, eltern) if eltern >= 0 else Transform3D()
		sk.set_bone_pose_position(fuss, basis.affine_inverse() * ziel)


## Lage eines Knochens im Skelettraum aus den aktuellen Posen (ohne auf die Engine zu warten).
func _knochen_global(sk: Skeleton3D, i: int) -> Transform3D:
	var t := Transform3D()
	while i >= 0:
		t = sk.get_bone_pose(i) * t
		i = sk.get_bone_parent(i)
	return t


func clip() -> String:
	return _aktuell


## Stellt das Modell so, dass die Figur HOEHE Meter hoch ist und mit den Fuessen auf y = 0
## steht. Wird einmal nach dem Einhaengen gerufen, wenn die Groesse bekannt ist.
func passe_an() -> void:
	if not is_inside_tree():
		return   # noch nicht im Baum: ohne Weltlage laesst sich nichts messen
	var box := _huelle(_innen)
	if box.size.y < 0.01:
		return
	var s := HOEHE / box.size.y
	_innen.scale = Vector3.ONE * s
	_innen.position.y = -box.position.y * s


func _huelle(wurzel: Node) -> AABB:
	var summe := AABB()
	var erste := true
	for m: MeshInstance3D in wurzel.find_children("*", "MeshInstance3D", true, false):
		var b := _welt_box(m)
		summe = b if erste else summe.merge(b)
		erste = false
	return summe


func _welt_box(m: MeshInstance3D) -> AABB:
	# Relativ zu diesem Knoten messen, damit die Lage der Figur keine Rolle spielt.
	var zu_mir := global_transform.affine_inverse() * m.global_transform
	return zu_mir * m.get_aabb()


## Polizeiuniform aus dem Arbeiter-Modell: Helm und Weste werden Marineblau, die Hose dunkel,
## das Hemd hellblau. Greift ueber die Materialnamen des Modells.
func uniformiere() -> void:
	var farben := {
		"Worker_Yellow": Color(0.05, 0.08, 0.20), "Worker_Vest": Color(0.06, 0.10, 0.28),
		"Brown": Color(0.04, 0.06, 0.14), "Brown2": Color(0.04, 0.06, 0.14),
		"LightBrown": Color(0.45, 0.58, 0.78), "Grey": Color(0.05, 0.05, 0.06),
	}
	for m: MeshInstance3D in _innen.find_children("*", "MeshInstance3D", true, false):
		for i in m.mesh.get_surface_count():
			var mat := m.mesh.surface_get_material(i)
			if mat is BaseMaterial3D and farben.has(mat.resource_name):
				var neu := (mat as BaseMaterial3D).duplicate() as BaseMaterial3D
				neu.albedo_color = farben[mat.resource_name]
				m.set_surface_override_material(i, neu)


## Abwechslungsreiche Kleidung fuer Passanten (Oberteil, Hose, Schuhe, Haare) je nach samen;
## greift ueber die Materialnamen der Quaternius-Modelle.
func einkleiden(samen: int) -> void:
	var oben := [Color(0.80, 0.20, 0.18), Color(0.18, 0.32, 0.62), Color(0.92, 0.90, 0.84),
			Color(0.24, 0.48, 0.30), Color(0.86, 0.68, 0.22), Color(0.50, 0.30, 0.56),
			Color(0.20, 0.20, 0.22), Color(0.88, 0.58, 0.62)]
	var unten := [Color(0.16, 0.22, 0.36), Color(0.12, 0.12, 0.13), Color(0.52, 0.44, 0.32),
			Color(0.30, 0.32, 0.36), Color(0.62, 0.58, 0.50)]
	var haare := [Color(0.18, 0.11, 0.06), Color(0.55, 0.40, 0.20), Color(0.08, 0.07, 0.06),
			Color(0.70, 0.58, 0.36), Color(0.45, 0.45, 0.45)]
	var rng := RandomNumberGenerator.new()
	rng.seed = samen * 7919 + 13
	var wahl := {
		"LightBrown": oben[rng.randi() % oben.size()],
		"Worker_Yellow": oben[rng.randi() % oben.size()],
		"Worker_Vest": oben[rng.randi() % oben.size()],
		"LightBlue": unten[rng.randi() % unten.size()],
		"Brown": unten[rng.randi() % unten.size()],
		"Brown2": unten[rng.randi() % unten.size()],
		"Red_Dark": unten[rng.randi() % unten.size()].darkened(0.3),
		"Grey": unten[rng.randi() % unten.size()].darkened(0.3),
		"Hair": haare[rng.randi() % haare.size()],
		"Moustache": haare[rng.randi() % haare.size()],
	}
	for m: MeshInstance3D in _innen.find_children("*", "MeshInstance3D", true, false):
		for i in m.mesh.get_surface_count():
			var mat := m.mesh.surface_get_material(i)
			if mat is BaseMaterial3D and wahl.has(mat.resource_name):
				var neu := (mat as BaseMaterial3D).duplicate() as BaseMaterial3D
				neu.albedo_color = wahl[mat.resource_name]
				m.set_surface_override_material(i, neu)
