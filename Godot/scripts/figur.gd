class_name Figur
extends Node3D
## Eine animierte Menschenfigur aus den Quaternius-Modellen (Casual, Worker; CC0, siehe
## Content/SourceData/People/LIZENZ.md). Die Modelle blicken nach +Z; hier dreht ein
## Innenknoten sie so, dass die Figur wie alles in Godot nach -Z schaut. Fehlt die
## Modelldatei (Converter nicht gelaufen), liefert neu() null und der Aufrufer nimmt
## seine Kapsel.

const ARTEN := ["Casual", "Worker"]
const HOEHE := 1.78

static var _szenen := {}

var _anim: AnimationPlayer
var _aktuell := ""
var _innen := Node3D.new()


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
			a.loop_mode = Animation.LOOP_NONE if k in ["Death", "Roll", "Gun_Shoot", "HitRecieve", "HitRecieve_2"] \
					else Animation.LOOP_LINEAR
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


func clip() -> String:
	return _aktuell


## Stellt das Modell so, dass die Figur HOEHE Meter hoch ist und mit den Fuessen auf y = 0
## steht. Wird einmal nach dem Einhaengen gerufen, wenn die Groesse bekannt ist.
func passe_an() -> void:
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
