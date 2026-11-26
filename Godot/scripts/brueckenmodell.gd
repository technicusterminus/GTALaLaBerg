class_name Brueckenmodell
extends RefCounted
## Gemeinsame Geometrie für Stahlprofile, Geländer und Widerlager.

var _schiene := BoxMesh.new()


func _stab(a: Vector3, b: Vector3, breite: float, hoehe: float) -> Transform3D:
	var laengs := b - a
	var rechts := laengs.normalized().cross(Vector3.UP)
	if rechts.length_squared() < 0.0001:
		rechts = Vector3.RIGHT
	else:
		rechts = rechts.normalized()
	var oben := rechts.cross(laengs.normalized())
	return Transform3D(Basis(laengs, oben * hoehe, rechts * breite), (a + b) * 0.5)


func _sammelnetz(eltern: Node3D, teile: Array[Transform3D], material: Material) -> void:
	var netz := MultiMesh.new()
	netz.transform_format = MultiMesh.TRANSFORM_3D
	netz.mesh = _schiene
	netz.instance_count = teile.size()
	for i in teile.size():
		netz.set_instance_transform(i, teile[i])
	var instanz := MultiMeshInstance3D.new()
	instanz.multimesh = netz
	instanz.material_override = material
	eltern.add_child(instanz)


func _viereck(netz: SurfaceTool, a: Vector3, b: Vector3, c: Vector3, d: Vector3) -> void:
	for p in [a, b, c, a, c, d]:
		netz.set_smooth_group(-1)
		netz.add_vertex(p)


func _quader(eltern: Node3D, pos: Vector3, masse: Vector3, material: Material) -> MeshInstance3D:
	var n := BoxMesh.new()
	n.size = masse
	var instanz := _modell(eltern, n, material)
	instanz.position = pos
	return instanz


func _modell(eltern: Node3D, mesh: Mesh, material: Material) -> MeshInstance3D:
	var instanz := MeshInstance3D.new()
	instanz.mesh = mesh
	instanz.material_override = material
	eltern.add_child(instanz)
	return instanz


static func _material(farbcode: String, rau := 0.84, metall := 0.0) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = Color(farbcode)
	material.roughness = rau
	material.metallic = metall
	material.cull_mode = BaseMaterial3D.CULL_DISABLED
	return material
