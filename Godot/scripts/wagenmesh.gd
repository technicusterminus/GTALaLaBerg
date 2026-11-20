class_name Wagenmesh
extends RefCounted
## Karosserie-Netz aus daten/wagen.json (Quelle der Unreal-Version), geteilt
## zwischen dem fahrbaren Auto, den KI-Autos und den geparkten Autos.
## Die Quelldaten schauen nach +X, das Netz behaelt diese Richtung.

const SHADER := preload("res://shader/flach.gdshader")
const SHADER_INSTANZ := preload("res://shader/flach_instanz.gdshader")

const AUTO_SHADER := preload("res://shader/auto.gdshader")
const RING := 16   ## Punkte je Querschnitt: Mitte unten, rechte Haelfte hinauf, linke herunter

static var _cache := {}
static var _wagen = null
static var _profil_daten = null


## lack: Farbe der lackierten Teile. instanziert: Lack kommt aus der
## Instanzfarbe eines MultiMesh (dann ist "lack" ohne Bedeutung).
static func netz(lack: Color, instanziert := false) -> ArrayMesh:
	var schluessel := "%s|%s" % [lack.to_html(false), instanziert]
	if _cache.has(schluessel):
		return _cache[schluessel]
	var profil := _profil()
	if not profil.is_empty():
		var neu := _karosserie(lack, instanziert, profil)
		_cache[schluessel] = neu
		return neu
	if _wagen == null:
		_wagen = JSON.parse_string(FileAccess.get_file_as_string("res://daten/wagen.json"))
	var netz := ArrayMesh.new()
	if _wagen == null:
		push_error("daten/wagen.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
		return netz
	for teil: Dictionary in _wagen["teile"]:
		var roh: Array = teil["positions"]
		var ecken := PackedVector3Array()
		ecken.resize(roh.size() / 3)
		for i in ecken.size():
			# Zentimeter, (x vorwaerts, y rechts, z oben) -> Meter, (x, y oben, z)
			ecken[i] = Vector3(roh[i * 3], roh[i * 3 + 2], roh[i * 3 + 1]) / 100.0
		var roh_idx: Array = teil["indices"]
		var indizes := PackedInt32Array()
		indizes.resize(roh_idx.size())
		for i in indizes.size():
			indizes[i] = int(roh_idx[i])
		var felder := []
		felder.resize(Mesh.ARRAY_MAX)
		felder[Mesh.ARRAY_VERTEX] = ecken
		felder[Mesh.ARRAY_INDEX] = indizes
		netz.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, felder)

		var lackiert: bool = teil.get("lack", false)
		var c: Array = teil["color"]
		var m := ShaderMaterial.new()
		m.shader = SHADER_INSTANZ if instanziert else SHADER
		m.set_shader_parameter("farbe", Vector3(c[0], c[1], c[2]))
		if instanziert:
			m.set_shader_parameter("lackiert", lackiert)
		elif lackiert:
			m.set_shader_parameter("farbe", Vector3(lack.r, lack.g, lack.b))
		var glatt: bool = teil["klasse"] == "Glas" or lackiert
		m.set_shader_parameter("rauheit", 0.25 if glatt else 0.8)
		m.set_shader_parameter("glanz", 1.0 if glatt else 0.0)
		netz.surface_set_material(netz.get_surface_count() - 1, m)
	_cache[schluessel] = netz
	return netz


# --- Neue Karosserie aus den Umrissen der Konstruktionszeichnung --------------------

static func _profil() -> Dictionary:
	if _profil_daten == null:
		var pfad := "res://daten/auto_profil.json"
		var d = JSON.parse_string(FileAccess.get_file_as_string(pfad)) \
				if FileAccess.file_exists(pfad) else null
		_profil_daten = d if d is Dictionary else {}
	return _profil_daten


static func _material(farbe: Color, lackiert := false, instanziert := false, glas := false,
		rauheit := 0.4, metall := 0.0, leuchten := Vector3.ZERO) -> ShaderMaterial:
	var m := ShaderMaterial.new()
	m.shader = AUTO_SHADER
	m.set_shader_parameter("farbe", Vector3(farbe.r, farbe.g, farbe.b))
	m.set_shader_parameter("lackiert", lackiert)
	m.set_shader_parameter("instanziert", instanziert)
	m.set_shader_parameter("glas", glas)
	m.set_shader_parameter("rauheit", rauheit)
	m.set_shader_parameter("metall", metall)
	m.set_shader_parameter("leuchten", leuchten)
	return m


## Querschnitt an einer Stelle: 9 Punkte der rechten Haelfte (Seite, Hoehe), von der Mitte
## unten ueber Schwelle, Tuer, Schulter, Fensterband und Dach bis zur Dachmitte.
static func _halbring(dach: float, breite: float, schwelle: float, gurt: float) -> Array:
	var top := maxf(dach, schwelle + 0.3)
	var gurtlinie := minf(top, gurt)
	var kabine := clampf((top - gurtlinie - 0.02) / 0.12, 0.0, 1.0)
	var hp := []
	hp.append(Vector2(0.0, schwelle))
	hp.append(Vector2(breite * 0.9, schwelle))
	hp.append(Vector2(breite, schwelle + 0.14))
	hp.append(Vector2(breite, gurtlinie - 0.1))
	hp.append(Vector2(breite * 0.96, gurtlinie))
	# Ohne Kabine (Haube, Heck): flach gewoelbt; mit Kabine: einziehende Seitenwand.
	hp.append(Vector2(lerpf(breite * 0.88, breite * 0.86, kabine),
			lerpf(top, gurtlinie + 0.03, kabine)))
	hp.append(Vector2(lerpf(breite * 0.78, breite * 0.76, kabine),
			lerpf(top + 0.005, top - 0.07, kabine)))
	hp.append(Vector2(lerpf(breite * 0.68, breite * 0.68, kabine), top + 0.01 * (1.0 - kabine)))
	hp.append(Vector2(0.0, top + 0.025 * (1.0 - kabine)))
	return hp


static func _karosserie(lack: Color, instanziert: bool, p: Dictionary) -> ArrayMesh:
	var laenge: float = p["laenge"]
	var dach: Array = p["dach"]
	var breiten: Array = p["halbe_breite"]
	var n := dach.size()
	var schwelle: float = p["schwelle"]
	var gurt: float = p["gurt"]
	var seite: Array = p["seitenfenster"]
	var wind: Array = p["windschutz"]
	var heck: Array = p["heckscheibe"]
	# Gitter der Punkte: Schnitt i, Ringpunkt j. Vorne liegt u = 0 (Fahrtrichtung +X).
	var gitter: Array = []
	var halbringe: Array = []
	for i in n:
		var u := float(i) / float(n - 1)
		var x := (0.5 - u) * laenge
		var hp := _halbring(dach[i], maxf(breiten[i], 0.25), schwelle, gurt)
		halbringe.append(hp)
		var ring := PackedVector3Array()
		for k in 9:
			ring.append(Vector3(x, hp[k].y, hp[k].x))
		for k in range(7, 0, -1):
			ring.append(Vector3(x, hp[k].y, -hp[k].x))
		gitter.append(ring)
	var lackflaeche := SurfaceTool.new()
	var glasflaeche := SurfaceTool.new()
	var unterbau := SurfaceTool.new()
	for st in [lackflaeche, glasflaeche, unterbau]:
		(st as SurfaceTool).begin(Mesh.PRIMITIVE_TRIANGLES)
	for i in n - 1:
		var u := float(i) / float(n - 1)
		var kabine: bool = dach[i] > minf(dach[i], gurt) + 0.1
		var seitenglas: bool = kabine and u >= seite[0] and u <= seite[1]
		var frontglas: bool = kabine and ((u >= wind[0] and u <= wind[1]) \
				or (u >= heck[0] and u <= heck[1]))
		for j in RING:
			var halb := j if j <= 7 else 15 - j
			var ziel := lackflaeche
			if halb == 0:
				ziel = unterbau
			elif halb == 5 and seitenglas:
				ziel = glasflaeche
			elif (halb == 6 or halb == 7) and frontglas:
				ziel = glasflaeche
			var j2 := (j + 1) % RING
			var a0: Vector3 = gitter[i][j]
			var a1: Vector3 = gitter[i][j2]
			var b0: Vector3 = gitter[i + 1][j]
			var b1: Vector3 = gitter[i + 1][j2]
			for tri in [[a0, b0, a1], [a1, b0, b1]]:
				for v: Vector3 in tri:
					ziel.add_vertex(v)
	# Bug und Heck schliessen: Faecher zum Mittelpunkt.
	for ende in [0, n - 1]:
		var ring: PackedVector3Array = gitter[ende]
		var mitte := Vector3.ZERO
		for v in ring:
			mitte += v
		mitte /= ring.size()
		for j in RING:
			var a := ring[j]
			var b := ring[(j + 1) % RING]
			var tri := [a, b, mitte] if ende == 0 else [b, a, mitte]
			for v: Vector3 in tri:
				lackflaeche.add_vertex(v)
	var netz := ArrayMesh.new()
	_anfuegen(netz, lackflaeche, _material(lack, true, instanziert, false, 0.3))
	_anfuegen(netz, glasflaeche, _material(Color(0.02, 0.03, 0.04), false, false, true))
	_anfuegen(netz, unterbau, _material(Color(0.02, 0.02, 0.022), false, false, false, 0.9))
	_raeder_und_licht(netz, p, halbringe)
	return netz


static func _anfuegen(netz: ArrayMesh, st: SurfaceTool, material: Material) -> void:
	st.generate_normals()
	st.commit(netz)
	netz.surface_set_material(netz.get_surface_count() - 1, material)


## Reifen, Felgen, Scheinwerfer, Rueckleuchten, Kuehlergrill und Spiegel.
static func _raeder_und_licht(netz: ArrayMesh, p: Dictionary, halbringe: Array) -> void:
	var laenge: float = p["laenge"]
	var reifen := SurfaceTool.new()
	var felgen := SurfaceTool.new()
	var licht_vorn := SurfaceTool.new()
	var licht_hinten := SurfaceTool.new()
	var schwarz := SurfaceTool.new()
	var spiegel := SurfaceTool.new()
	for st in [reifen, felgen, licht_vorn, licht_hinten, schwarz, spiegel]:
		(st as SurfaceTool).begin(Mesh.PRIMITIVE_TRIANGLES)
	var n: int = halbringe.size()
	for rad: Dictionary in p["raeder"]:
		var x: float = (0.5 - float(rad["u"])) * laenge
		var r: float = rad["r"]
		var i := clampi(roundi(float(rad["u"]) * (n - 1)), 0, n - 1)
		var spur: float = (halbringe[i][2] as Vector2).x * 0.9
		for seite_z in [spur, -spur]:
			var t := Transform3D(Basis(Vector3.RIGHT, PI / 2.0), Vector3(x, r, seite_z))
			var reif := CylinderMesh.new()
			reif.top_radius = r
			reif.bottom_radius = r
			reif.height = 0.22
			reif.radial_segments = 16
			reifen.append_from(reif, 0, t)
			var felge := CylinderMesh.new()
			felge.top_radius = r * 0.62
			felge.bottom_radius = r * 0.62
			felge.height = 0.23
			felge.radial_segments = 12
			felgen.append_from(felge, 0, t)
	var vorn := _kasten(Vector3(0.05, 0.10, 0.30))
	var hinten := _kasten(Vector3(0.05, 0.14, 0.30))
	var breite0: float = (halbringe[0][2] as Vector2).x
	var breite1: float = (halbringe[n - 1][2] as Vector2).x
	var schwelle: float = p["schwelle"]
	for s in [1.0, -1.0]:
		licht_vorn.append_from(vorn, 0, Transform3D(Basis(), Vector3(
				laenge / 2.0 - 0.03, schwelle + 0.52, s * breite0 * 0.62)))
		licht_hinten.append_from(hinten, 0, Transform3D(Basis(), Vector3(
				-laenge / 2.0 + 0.03, schwelle + 0.62, s * breite1 * 0.66)))
		var w: float = halbringe[roundi(0.34 * (n - 1))][2].x
		spiegel.append_from(_kasten(Vector3(0.10, 0.09, 0.16)), 0, Transform3D(Basis(),
				Vector3(laenge * 0.16, p["gurt"] + 0.02, s * (w + 0.06))))
	schwarz.append_from(_kasten(Vector3(0.04, 0.13, 0.78)), 0, Transform3D(Basis(),
			Vector3(laenge / 2.0 - 0.02, schwelle + 0.38, 0.0)))
	for st in [reifen, felgen, licht_vorn, licht_hinten, schwarz, spiegel]:
		(st as SurfaceTool).generate_normals()
	_anfuegen(netz, reifen, _material(Color(0.015, 0.015, 0.016), false, false, false, 0.95))
	_anfuegen(netz, felgen, _material(Color(0.55, 0.56, 0.58), false, false, false, 0.35, 0.8))
	_anfuegen(netz, licht_vorn, _material(Color(1.0, 0.97, 0.88), false, false, false, 0.2,
			0.0, Vector3(0.9, 0.85, 0.6)))
	_anfuegen(netz, licht_hinten, _material(Color(0.5, 0.02, 0.02), false, false, false, 0.3,
			0.0, Vector3(0.5, 0.02, 0.02)))
	_anfuegen(netz, schwarz, _material(Color(0.02, 0.02, 0.02), false, false, false, 0.8))
	_anfuegen(netz, spiegel, _material(Color(0.02, 0.02, 0.02), false, false, false, 0.5))


static func _kasten(groesse: Vector3) -> BoxMesh:
	var b := BoxMesh.new()
	b.size = groesse
	return b
