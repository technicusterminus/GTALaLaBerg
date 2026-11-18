class_name Passant
extends Node3D
## Ein Fussgaenger auf einer festen Route, hin und zurueck. Vorerst eine
## Kapsel als Platzhalter; die Figuren der Unreal-Version brauchen den
## FBX-Import im Godot-Editor.

const KLEIDER := [
	Color(0.62, 0.15, 0.12), Color(0.15, 0.30, 0.55), Color(0.20, 0.45, 0.25),
	Color(0.75, 0.62, 0.20), Color(0.30, 0.30, 0.34), Color(0.55, 0.35, 0.60),
]

static var _mesh: CapsuleMesh
static var _materialien: Array[StandardMaterial3D] = []

var weg := Wegfolger.new()
var tempo_ms := 1.3
var farbe := 0


func _ready() -> void:
	if _mesh == null:
		_mesh = CapsuleMesh.new()
		_mesh.height = 1.75
		_mesh.radius = 0.3
		for c: Color in KLEIDER:
			var m := StandardMaterial3D.new()
			m.albedo_color = c
			_materialien.append(m)
	var instanz := MeshInstance3D.new()
	instanz.mesh = _mesh
	instanz.material_override = _materialien[farbe % _materialien.size()]
	instanz.position.y = 0.875
	instanz.visibility_range_end = 250.0
	add_child(instanz)
	global_position = weg.ort


func _physics_process(delta: float) -> void:
	weg.bewege(tempo_ms * delta)
	global_position = weg.ort
