class_name Gauben
extends RefCounted
## Dachgauben als Instanzen: je Sektor ein MultiMesh. Zeilen aus Tools/konvertiere_sektoren.py
## (*.gauben): x, y, z auf der Dachflaeche und die waagerechte Richtung, in die die Dachflaeche
## abfaellt (dort schaut die Gaube hin). Das Modell: Front mit Fenster, zwei Wangen und ein
## kleines Satteldach, vorn ueber die Front hinaus.

const SPALTEN := 5
const SICHT := 240.0

static var _netz: ArrayMesh


static func stelle(wurzel: Node3D, zeilen: PackedFloat32Array) -> int:
	var n := zeilen.size() / SPALTEN
	if n == 0:
		return 0
	_vorbereiten()
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.mesh = _netz
	mm.instance_count = n
	for i in n:
		var o := i * SPALTEN
		var richtung := Vector3(zeilen[o + 3], 0.0, zeilen[o + 4])
		var basis := Basis.looking_at(richtung, Vector3.UP)
		mm.set_instance_transform(i, Transform3D(basis, Vector3(zeilen[o], zeilen[o + 1] + 0.02,
				zeilen[o + 2])))
	var inst := MultiMeshInstance3D.new()
	inst.multimesh = mm
	inst.visibility_range_end = SICHT
	wurzel.add_child(inst)
	return n


static func _flaeche(netz: ArrayMesh, teile: Array, material: Material) -> void:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	for t: Array in teile:
		var b := BoxMesh.new()
		b.size = t[0]
		st.append_from(b, 0, t[1])
	st.generate_normals()
	st.commit(netz)
	netz.surface_set_material(netz.get_surface_count() - 1, material)


static func _stoff(farbe: Color, rauheit := 0.85) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = farbe
	m.roughness = rauheit
	return m


## Vorn ist -Z (Basis.looking_at); der Ursprung liegt auf der Dachflaeche. Front und Wangen
## reichen nach unten ueber die Dachflaeche, damit sie nicht schweben.
static func _vorbereiten() -> void:
	if _netz != null:
		return
	_netz = ArrayMesh.new()
	var wand := _stoff(Color(0.78, 0.72, 0.60))
	var rahmen := _stoff(Color(0.92, 0.91, 0.86))
	var glas := StandardMaterial3D.new()
	glas.albedo_color = Color(0.03, 0.05, 0.08)
	glas.roughness = 0.1
	glas.metallic = 0.3
	var ziegel := _stoff(Color(0.52, 0.2, 0.09), 0.9)
	# Wangen und Front.
	_flaeche(_netz, [
		[Vector3(1.3, 1.7, 0.1), Transform3D(Basis(), Vector3(0, 0.15, -0.55))],
		[Vector3(0.1, 1.7, 1.5), Transform3D(Basis(), Vector3(-0.6, 0.15, 0.2))],
		[Vector3(0.1, 1.7, 1.5), Transform3D(Basis(), Vector3(0.6, 0.15, 0.2))],
	], wand)
	_flaeche(_netz, [[Vector3(0.82, 0.72, 0.03), Transform3D(Basis(), Vector3(0, 0.42, -0.61))]],
			rahmen)
	_flaeche(_netz, [[Vector3(0.66, 0.56, 0.03), Transform3D(Basis(), Vector3(0, 0.42, -0.63))]],
			glas)
	# Satteldach: zwei Platten, vorn ueberstehend.
	var links := Basis(Vector3(0, 0, 1), deg_to_rad(38.0))
	var rechts := Basis(Vector3(0, 0, 1), deg_to_rad(-38.0))
	_flaeche(_netz, [
		[Vector3(0.95, 0.05, 2.0), Transform3D(links, Vector3(-0.4, 1.3, 0.25))],
		[Vector3(0.95, 0.05, 2.0), Transform3D(rechts, Vector3(0.4, 1.3, 0.25))],
	], ziegel)
