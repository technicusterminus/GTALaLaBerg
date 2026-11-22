class_name Giebel
extends RefCounted
## Geschwungene Barockgiebel (Schweifgiebel) wie am Landsberger Hauptplatz, als Instanzen auf den
## Giebelenden der Satteldaecher in der Altstadt. Zeilen aus Tools/konvertiere_sektoren.py
## (*.giebel): x, y_traufe, z, normale_x, normale_z, breite, hoehe. Das Netz ist auf Breite 1
## und Hoehe 1 normiert und steigt ueber die Dachlinie hinaus; vorn eine helle Stuckkante,
## ein Ochsenauge und zwei Fenster. Die Putzfarbe kommt aus der Instanzfarbe.

const SPALTEN := 7
const SICHT := 380.0
const PUTZ := [
	Color(0.86, 0.62, 0.62), Color(0.84, 0.66, 0.38), Color(0.90, 0.82, 0.52),
	Color(0.62, 0.78, 0.64), Color(0.92, 0.90, 0.86), Color(0.86, 0.64, 0.54),
]

static var _netz: ArrayMesh


static func stelle(wurzel: Node3D, zeilen: PackedFloat32Array) -> int:
	var n := zeilen.size() / SPALTEN
	if n == 0:
		return 0
	_vorbereiten()
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.use_colors = true
	mm.mesh = _netz
	mm.instance_count = n
	for i in n:
		var o := i * SPALTEN
		var normale := Vector3(zeilen[o + 3], 0.0, zeilen[o + 4]).normalized()
		var quer := Vector3.UP.cross(normale).normalized()
		# x: quer zur Giebelflaeche, y: nach oben, z: nach aussen (Normale).
		var basis := Basis(quer * zeilen[o + 5], Vector3.UP * (zeilen[o + 6] * 1.25),
				normale * 0.35)
		var fuss := Vector3(zeilen[o], zeilen[o + 1], zeilen[o + 2]) + normale * 0.08
		mm.set_instance_transform(i, Transform3D(basis, fuss))
		var z := fposmod(sin(zeilen[o] * 1.7 + zeilen[o + 2] * 2.3) * 43758.5, 1.0)
		mm.set_instance_color(i, PUTZ[int(z * PUTZ.size()) % PUTZ.size()])
	var inst := MultiMeshInstance3D.new()
	inst.multimesh = mm
	inst.visibility_range_end = SICHT
	wurzel.add_child(inst)
	return n


## Umriss der rechten Haelfte (x von 0 bis 0,5, y von 0 bis 1): steile Schultern mit
## Voluten (S-Schwuenge), oben ein kleiner Aufsatz.
static func _umriss() -> PackedVector2Array:
	var r := PackedVector2Array()
	r.append(Vector2(0.5, 0.0))
	r.append(Vector2(0.5, 0.28))
	# erste Volute
	for i in 9:
		var t := float(i) / 8.0
		r.append(Vector2(0.5 - 0.14 * t, 0.28 + 0.16 * t - 0.05 * sin(t * PI)))
	r.append(Vector2(0.33, 0.46))
	# zweite Volute
	for i in 9:
		var t := float(i) / 8.0
		r.append(Vector2(0.33 - 0.13 * t, 0.46 + 0.22 * t - 0.05 * sin(t * PI)))
	r.append(Vector2(0.18, 0.72))
	# Aufsatz mit Bogen
	for i in 9:
		var t := float(i) / 8.0
		r.append(Vector2(0.11 * cos(t * PI * 0.5) + 0.0, 0.80 + 0.20 * sin(t * PI * 0.5)))
	return r


static func _vorbereiten() -> void:
	if _netz != null:
		return
	var halb := _umriss()
	var ganz := PackedVector2Array()
	for p in halb:
		ganz.append(p)
	for i in range(halb.size() - 1, -1, -1):
		ganz.append(Vector2(-halb[i].x, halb[i].y))
	_netz = ArrayMesh.new()
	# Wand: Vorderseite (z = 0) und Ruecken (z = -1, also 0,35 m tief), Kanten dazwischen.
	var wand := SurfaceTool.new()
	wand.begin(Mesh.PRIMITIVE_TRIANGLES)
	var flaeche := Geometry2D.triangulate_polygon(ganz)
	for tiefe in [0.0, -1.0]:
		for k in range(0, flaeche.size(), 3):
			var reihen := [flaeche[k], flaeche[k + 1], flaeche[k + 2]]
			if tiefe < 0.0:
				reihen.reverse()
			for j in reihen:
				wand.add_vertex(Vector3(ganz[j].x, ganz[j].y, tiefe))
	for i in ganz.size():
		var a := ganz[i]
		var b := ganz[(i + 1) % ganz.size()]
		for v in [Vector3(a.x, a.y, 0), Vector3(b.x, b.y, 0), Vector3(b.x, b.y, -1),
				Vector3(a.x, a.y, 0), Vector3(b.x, b.y, -1), Vector3(a.x, a.y, -1)]:
			wand.add_vertex(v)
	wand.generate_normals()
	wand.commit(_netz)
	var putz := StandardMaterial3D.new()
	putz.vertex_color_use_as_albedo = true
	putz.roughness = 0.9
	_netz.surface_set_material(0, putz)
	# Stuckkante entlang des Umrisses (vorn), Ochsenauge und Fenster.
	var stuck := SurfaceTool.new()
	stuck.begin(Mesh.PRIMITIVE_TRIANGLES)
	for i in ganz.size() - 1:
		var a := ganz[i]
		var b := ganz[i + 1]
		var innen_a := a * Vector2(0.94, 0.97)
		var innen_b := b * Vector2(0.94, 0.97)
		for v in [a, b, innen_b, a, innen_b, innen_a]:
			stuck.add_vertex(Vector3(v.x, v.y, 0.04))
	_kreis(stuck, Vector2(0.0, 0.62), 0.07, 0.04)
	stuck.generate_normals()
	stuck.commit(_netz)
	var weiss := StandardMaterial3D.new()
	weiss.albedo_color = Color(0.94, 0.92, 0.86)
	weiss.roughness = 0.8
	weiss.cull_mode = BaseMaterial3D.CULL_DISABLED
	_netz.surface_set_material(1, weiss)
	var glas := SurfaceTool.new()
	glas.begin(Mesh.PRIMITIVE_TRIANGLES)
	_kreis(glas, Vector2(0.0, 0.62), 0.045, 0.06)
	for x in [-0.14, 0.14]:
		_rechteck(glas, Vector2(x, 0.27), Vector2(0.075, 0.12), 0.06)
	glas.generate_normals()
	glas.commit(_netz)
	var dunkel := StandardMaterial3D.new()
	dunkel.albedo_color = Color(0.05, 0.07, 0.10)
	dunkel.roughness = 0.15
	dunkel.cull_mode = BaseMaterial3D.CULL_DISABLED
	_netz.surface_set_material(2, dunkel)


static func _kreis(st: SurfaceTool, m: Vector2, r: float, z: float) -> void:
	for i in 16:
		var a := TAU * float(i) / 16.0
		var b := TAU * float(i + 1) / 16.0
		st.add_vertex(Vector3(m.x, m.y, z))
		st.add_vertex(Vector3(m.x + cos(a) * r, m.y + sin(a) * r * 1.6, z))
		st.add_vertex(Vector3(m.x + cos(b) * r, m.y + sin(b) * r * 1.6, z))


static func _rechteck(st: SurfaceTool, m: Vector2, halb: Vector2, z: float) -> void:
	var a := Vector3(m.x - halb.x, m.y - halb.y, z)
	var b := Vector3(m.x + halb.x, m.y - halb.y, z)
	var c := Vector3(m.x + halb.x, m.y + halb.y, z)
	var d := Vector3(m.x - halb.x, m.y + halb.y, z)
	for v in [a, b, c, a, c, d]:
		st.add_vertex(v)
