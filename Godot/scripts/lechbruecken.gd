class_name Lechbruecken
extends Brueckenmodell
## Die Querprofile kommen aus dem Stadtkonverter. Überbau und Geländer folgen ihnen.

const SEILNETZ := preload("res://shader/brueckennetz.gdshader")

var _beton := _material("#a6a49a")
var _stein := _material("#969283")
var _sandauer_blau := _material("#294676", 0.49, 0.36)
var _stahl := _material("#858e91", 0.48, 0.48)
var _corten := _material("#854628", 0.76, 0.15)
var _dunkel := _material("#333738", 0.53, 0.45)
var _holz := _material("#b0946a")


func baue(sektor: Node3D, profil: Dictionary) -> void:
	var bruecke := Node3D.new()
	bruecke.name = profil.name
	sektor.add_child(bruecke)
	var stationen: Array = profil.stationen
	var bauart: String = profil.bauart
	_gelaender_kollision(bruecke, stationen)
	if DisplayServer.get_name() == "headless":
		return
	_tragwerk(bruecke, stationen, bauart, profil.laenge)
	_gelaender(bruecke, stationen, bauart)
	for anteil: float in profil.pfeiler:
		_aufleger(bruecke, _station_beim(stationen, profil.laenge * anteil), bauart, false)
	for i in [0, stationen.size() - 1]:
		_aufleger(bruecke, stationen[i], bauart, true)


func _x_y_z(station: Array, quer: float = 0, dy: float = 0) -> Vector3:
	return Vector3(station[0] + station[3] * quer, station[1] + dy, station[2] + station[4] * quer)


func _station_beim(stationen: Array, s: float) -> Array:
	for i in range(1, stationen.size()):
		var b: Array = stationen[i]
		if b[7] < s:
			continue
		var a: Array = stationen[i - 1]
		var anteil: float = (s - a[7]) / (b[7] - a[7])
		var punkt := []
		for j in 8:
			punkt.append(lerpf(a[j], b[j], anteil))
		return punkt
	return stationen.back()


func _tiefe(station: Array, laenge: float, bauart: String) -> float:
	if bauart == "corten":
		return 0.70 + 0.76 * pow(absf(cos(TAU * station[7] / laenge)), 3.4)
	if bauart == "holz":
		return 0.24
	return 0.48 if bauart in ["verbund", "bahnstrasse"] else 0.39


func _tragwerk(bruecke: Node3D, stationen: Array, bauart: String, laenge: float) -> void:
	var netz := SurfaceTool.new()
	netz.begin(Mesh.PRIMITIVE_TRIANGLES)
	var traeger: Array[Transform3D] = []
	var flansche: Array[Transform3D] = []
	var markierung: Array[Transform3D] = []
	var baenke: Array[Transform3D] = []
	var bankfuesse: Array[Transform3D] = []
	for i in range(stationen.size() - 1):
		var a: Array = stationen[i]
		var b: Array = stationen[i + 1]
		var ta := _tiefe(a, laenge, bauart)
		var tb := _tiefe(b, laenge, bauart)
		var faktor := 0.58 if bauart == "corten" else 0.86
		var ay := 0.10 if bauart == "verbund" else -0.02
		var la := _x_y_z(a, -a[5] * faktor, -ta)
		var lb := _x_y_z(b, -b[5] * faktor, -tb)
		var ra := _x_y_z(a, a[6] * faktor, -ta)
		var rb := _x_y_z(b, b[6] * faktor, -tb)
		_viereck(netz, _x_y_z(a, -a[5], ay), _x_y_z(b, -b[5], ay), lb, la)
		_viereck(netz, _x_y_z(b, b[6], ay), _x_y_z(a, a[6], ay), ra, rb)
		_viereck(netz, la, lb, rb, ra)
		if bauart == "verbund":
			for q in [-4.45, -1.47, 1.47, 4.45]:
				traeger.append(_stab(_x_y_z(a, q, -0.86), _x_y_z(b, q, -0.86), 0.26, 0.83))
				flansche.append(_stab(_x_y_z(a, q, -1.28), _x_y_z(b, q, -1.28), 0.56, 0.095))
			for q in [-3.53, 3.53]:
				markierung.append(_stab(_x_y_z(a, q, 0.015), _x_y_z(b, q, 0.015), 0.10, 0.015))
		elif bauart == "corten" and a[7] > laenge * 0.39 and b[7] < laenge * 0.62:
			for reihe in range(5):
				var q := 0.39 + reihe * 0.105
				baenke.append(_stab(_x_y_z(a, a[6] - q, 0.47), _x_y_z(b, b[6] - q, 0.47), 0.099, 0.058))
			bankfuesse.append(_stab(_x_y_z(a, a[6] - 0.59, 0.02), _x_y_z(a, a[6] - 0.59, 0.44), 0.14, 0.10))
	netz.generate_normals()
	var stoff := _corten if bauart == "corten" else (_holz if bauart == "holz" else _beton)
	_modell(bruecke, netz.commit(), stoff)
	if bauart == "verbund":
		for x in range(4, int(laenge) - 4, 8):
			var a := _station_beim(stationen, x)
			var b := _station_beim(stationen, x + 2.9)
			markierung.append(_stab(_x_y_z(a, 0, 0.014), _x_y_z(b, 0, 0.014), 0.13, 0.012))
		_sammelnetz(bruecke, traeger, _sandauer_blau)
		_sammelnetz(bruecke, flansche, _sandauer_blau)
		_sammelnetz(bruecke, markierung, _material("#ccc8b8"))
	elif bauart == "corten":
		_sammelnetz(bruecke, baenke, _holz)
		_sammelnetz(bruecke, bankfuesse, _dunkel)
	elif bauart == "holz":
		var raster: Array[Transform3D] = []
		for s in range(1, ceili(laenge / 0.17)):
			var p := _station_beim(stationen, s * 0.17)
			raster.append(_stab(_x_y_z(p, -p[5], 0.005), _x_y_z(p, p[6], 0.005), 0.019, 0.010))
		_sammelnetz(bruecke, raster, _material("#75644b"))


func _aufleger(bruecke: Node3D, station: Array, bauart: String, widerlager: bool) -> void:
	var ort := _x_y_z(station)
	var oben := ort.y - (0.46 if widerlager else (1.26 if bauart == "verbund" else 1.05))
	var unten := -0.35 if bauart == "verbund" else 0.75
	if bauart in ["bahnsteg", "bahnstrasse"]:
		unten = ort.y - 3.8
	elif bauart in ["kanalstrasse", "kleinsteg", "holz"]:
		unten = ort.y - (1.32 if bauart == "kanalstrasse" else 0.78)
	var breite: float = (station[5] + station[6]) * (0.88 if widerlager else 0.72)
	var winkel := atan2(-float(station[3]), float(station[4]))
	ort.y = (oben + unten) / 2.0
	if widerlager or bauart == "verbund":
		var staerke := 1.68 if widerlager else 1.92
		var auflager := _quader(bruecke, ort, Vector3(staerke, oben - unten, breite), _stein)
		auflager.rotation.y = -winkel
		if not widerlager:
			for dy in [0.41, 0.83, 1.24]:
				var fuge := _quader(bruecke, Vector3(ort.x, unten + dy, ort.z),
						Vector3(staerke + 0.015, 0.025, breite + 0.02), _beton)
				fuge.rotation.y = -winkel
		return
	var schaft := CylinderMesh.new()
	schaft.top_radius = 0.64 if bauart == "corten" else 0.53
	schaft.bottom_radius = 0.49 if bauart == "corten" else 0.69
	schaft.height = oben - unten
	schaft.radial_segments = 8
	var pfeiler := _modell(bruecke, schaft, _beton)
	pfeiler.position = ort
	pfeiler.rotation.y = -winkel
	pfeiler.scale.z = 1.4 if bauart == "corten" else 1.8


func _gelaender(bruecke: Node3D, stationen: Array, bauart: String) -> void:
	var stangen: Array[Transform3D] = []
	var bruestungen: Array[Transform3D] = []
	var netz := SurfaceTool.new()
	netz.begin(Mesh.PRIMITIVE_TRIANGLES)
	var ist_seilnetz := bauart == "corten"
	for seite: int in [-1, 1]:
		for i in range(stationen.size() - 1):
			var a: Array = stationen[i]
			var b: Array = stationen[i + 1]
			var unten := 0.16 if bauart == "steg" else 0.19
			var oben := 1.08 if bauart == "steg" else 1.20
			if bauart == "holz":
				unten = 0.53
				oben = 1.00
			elif bauart == "kanalstrasse":
				unten = 0.45
				oben = 1.03
			var q := 5 if seite < 0 else 6
			var p := _x_y_z(a, seite * (a[q] - 0.11))
			var ende := _x_y_z(b, seite * (b[q] - 0.11))
			if bauart == "kanalstrasse":
				bruestungen.append(_stab(p + Vector3.UP * 0.22, ende + Vector3.UP * 0.22, 0.30, 0.42))
			for y in [unten, oben]:
				stangen.append(_stab(p + Vector3.UP * y, ende + Vector3.UP * y,
						0.074 if y == oben else 0.034, 0.048 if y == oben else 0.025))
			stangen.append(_stab(p, p + Vector3.UP * oben, 0.067, 0.056))
			if ist_seilnetz:
				var r := Vector3.UP * unten
				var o := Vector3.UP * oben
				var guertel := [p + r, ende + r, ende + o, p + o]
				for j in [0, 1, 2, 0, 2, 3]:
					netz.set_uv(Vector2(a[7] if j in [0, 3] else b[7], unten if j < 2 else oben))
					netz.add_vertex(guertel[j])
			elif bauart != "holz":
				var staebe := maxi(1, floori(p.distance_to(ende) / 0.20))
				for j in range(1, staebe):
					var d := p.lerp(ende, j / float(staebe))
					stangen.append(_stab(d + Vector3.UP * unten, d + Vector3.UP * oben, 0.026, 0.022))
	_sammelnetz(bruecke, stangen, _dunkel if ist_seilnetz else (_holz if bauart == "holz" else _stahl))
	if not bruestungen.is_empty():
		_sammelnetz(bruecke, bruestungen, _stein)
	if ist_seilnetz:
		netz.generate_normals()
		var material := ShaderMaterial.new()
		material.shader = SEILNETZ
		var gitter := _modell(bruecke, netz.commit(), material)
		gitter.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF


func _gelaender_kollision(bruecke: Node3D, stationen: Array) -> void:
	var koerper := StaticBody3D.new()
	koerper.name = "Seitenschutz"
	bruecke.add_child(koerper)
	for seite: int in [-1, 1]:
		for i in range(stationen.size() - 1):
			var a: Array = stationen[i]
			var b: Array = stationen[i + 1]
			var q := 5 if seite < 0 else 6
			var p := _x_y_z(a, seite * (a[q] - 0.10), 0.60)
			var ende := _x_y_z(b, seite * (b[q] - 0.10), 0.60)
			var form := BoxShape3D.new()
			form.size = Vector3(p.distance_to(ende) + 0.02, 1.10, 0.12)
			var kollision := CollisionShape3D.new()
			kollision.shape = form
			kollision.position = (p + ende) / 2.0
			kollision.rotation.y = atan2(p.z - ende.z, ende.x - p.x)
			koerper.add_child(kollision)
