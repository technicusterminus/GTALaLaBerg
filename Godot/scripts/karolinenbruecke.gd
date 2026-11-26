class_name Karolinenbruecke
extends Brueckenmodell
## Sichtbeton-Überbau, vier Pfeiler, Metallgeländer und Bogenleuchten.

var _beton := _material("#9b9990")
var _stein := _material("#a39b88")
var _wasserlinie := _material("#666e58")
var _stahl := _material("#788187", 0.54, 0.36)
var _lampenmetall := _material("#454d51", 0.58, 0.35)
var _milchglas := _material("#ded6b6", 0.32)
var _laenge: float
var _halb: float
var _hoehen: Array


func baue(sektor: Node3D, profil: Dictionary) -> void:
	var bruecke := Node3D.new()
	bruecke.name = "Karolinenbruecke"
	bruecke.position = Vector3(profil.west[0], 0, profil.west[1])
	bruecke.rotation.y = profil.winkel
	sektor.add_child(bruecke)
	_laenge = profil.laenge
	_halb = profil.halb
	_hoehen = profil.hoehen
	_gelaender_kollision(bruecke)
	if DisplayServer.get_name() == "headless":
		return
	_tragwerk(bruecke)
	_gelaender(bruecke)
	for i in 6:
		_laterne(bruecke, _laenge * (0.12 + 0.151 * i), -1.0 if i % 2 == 0 else 1.0)
	_uferdetail(sektor, profil.get("ufer", []))
	_wehr(sektor, profil.get("wehr", []))


func _deck(s: float) -> float:
	var index := clampf(s / _laenge, 0.0, 1.0) * (_hoehen.size() - 1)
	var i := mini(floori(index), _hoehen.size() - 2)
	return lerpf(_hoehen[i], _hoehen[i + 1], index - i)


func _tragwerk(bruecke: Node3D) -> void:
	var flaeche := SurfaceTool.new()
	flaeche.begin(Mesh.PRIMITIVE_TRIANGLES)
	for i in range(_hoehen.size() - 1):
		var x0 := _laenge * i / (_hoehen.size() - 1)
		var x1 := _laenge * (i + 1) / (_hoehen.size() - 1)
		var y0: float = _hoehen[i]
		var y1: float = _hoehen[i + 1]
		for q: float in [-_halb, _halb]:
			_viereck(flaeche, Vector3(x0, y0 + 0.10, q), Vector3(x1, y1 + 0.10, q),
					Vector3(x1, y1 - 0.92, q * 0.87), Vector3(x0, y0 - 0.92, q * 0.87))
		_viereck(flaeche, Vector3(x0, y0 - 0.92, -_halb * 0.87), Vector3(x1, y1 - 0.92, -_halb * 0.87),
				Vector3(x1, y1 - 0.92, _halb * 0.87), Vector3(x0, y0 - 0.92, _halb * 0.87))
	flaeche.generate_normals()
	_modell(bruecke, flaeche.commit(), _beton)
	for anteil in [0.191, 0.397, 0.603, 0.805]:
		_pfeiler(bruecke, _laenge * anteil)
	# Schmale helle Linie zwischen Asphalt und rotem Radstreifen.
	var markierungen: Array[Transform3D] = []
	for i in _hoehen.size() - 1:
		var a := _laenge * i / (_hoehen.size() - 1)
		var b := _laenge * (i + 1) / (_hoehen.size() - 1)
		markierungen.append(_stab(Vector3(a, _deck(a) + 0.014, 3.37),
				Vector3(b, _deck(b) + 0.014, 3.37), 0.085, 0.010))
	for i in range(5, int(_laenge) - 5, 9):
		var b := i + 3.20
		markierungen.append(_stab(Vector3(i, _deck(i) + 0.014, -0.13),
				Vector3(b, _deck(b) + 0.014, -0.13), 0.115, 0.010))
	_sammelnetz(bruecke, markierungen, _material("#d1cbb9"))


func _pfeiler(bruecke: Node3D, s: float) -> void:
	var oberkante := _deck(s) - 0.84
	var umriss := [Vector2(-1.27, -5.00), Vector2(0.0, -6.14), Vector2(1.27, -5.00),
			Vector2(1.27, 5.00), Vector2(0.0, 6.14), Vector2(-1.27, 5.00)]
	var netz := SurfaceTool.new()
	netz.begin(Mesh.PRIMITIVE_TRIANGLES)
	for i in umriss.size():
		var a: Vector2 = umriss[i]
		var b: Vector2 = umriss[(i + 1) % umriss.size()]
		_viereck(netz, Vector3(s + a.x, 1.50, a.y), Vector3(s + b.x, 1.50, b.y),
				Vector3(s + b.x, oberkante, b.y), Vector3(s + a.x, oberkante, a.y))
	netz.generate_normals()
	_modell(bruecke, netz.commit(), _stein)
	for h in [3.13, 3.81, 4.49]:
		var fugenton := _wasserlinie if h < 3.3 else _beton
		for d in [-1.0, 1.0]:
			_quader(bruecke, Vector3(s + d * 1.27, h, 0), Vector3(0.021, 0.055, 10.02), fugenton)
	_quader(bruecke, Vector3(s, oberkante - 0.08, 0), Vector3(2.80, 0.22, 11.95), _beton)


func _gelaender(bruecke: Node3D) -> void:
	var stangen: Array[Transform3D] = []
	var schuss := _laenge / 33.0
	for seite in [-1.0, 1.0]:
		var q: float = seite * (_halb - 0.12)
		for i in 33:
			var x := schuss * i
			var ende := x + schuss
			for h in [0.17, 0.88, 1.10]:
				var hoehe := 0.075 if h > 1.0 else 0.042
				stangen.append(_stab(Vector3(x, _deck(x) + h, q),
						Vector3(ende, _deck(ende) + h, q), hoehe, hoehe))
			stangen.append(_stab(Vector3(x, _deck(x) + 0.10, q),
					Vector3(x, _deck(x) + 1.17, q), 0.087, 0.087))
			for j in range(1, 12):
				var pos := x + schuss * j / 12.0
				var naechster := pos + schuss / 12.0
				stangen.append(_stab(Vector3(pos, _deck(pos) + 0.18, q),
						Vector3(pos, _deck(pos) + 0.87, q), 0.023, 0.028))
				var links := 0.90 if j % 2 == 0 else 1.07
				var rechts := 1.07 if j % 2 == 0 else 0.90
				stangen.append(_stab(Vector3(pos, _deck(pos) + links, q),
						Vector3(naechster, _deck(naechster) + rechts, q), 0.021, 0.024))
		stangen.append(_stab(Vector3(_laenge, _deck(_laenge) + 0.10, q),
				Vector3(_laenge, _deck(_laenge) + 1.17, q), 0.087, 0.087))
	_sammelnetz(bruecke, stangen, _stahl)


func _gelaender_kollision(bruecke: Node3D) -> void:
	var koerper := StaticBody3D.new()
	koerper.name = "Brueckengelaender"
	bruecke.add_child(koerper)
	for seite in [-1.0, 1.0]:
		for i in 6:
			var s := _laenge * (i + 0.5) / 6.0
			var form := BoxShape3D.new()
			form.size = Vector3(_laenge / 6.0 + 0.02, 1.04, 0.13)
			var kollision := CollisionShape3D.new()
			kollision.shape = form
			kollision.position = Vector3(s, _deck(s) + 0.61, seite * (_halb - 0.12))
			koerper.add_child(kollision)


func _laterne(bruecke: Node3D, s: float, seite: float) -> void:
	var y := _deck(s) + 0.10
	var q := seite * (_halb - 0.36)
	var teile: Array[Transform3D] = []
	teile.append(_stab(Vector3(s, y, q), Vector3(s, y + 4.38, q), 0.085, 0.085))
	var alt := Vector3(s, y + 4.38, q)
	for i in range(1, 14):
		var winkel := PI * i / 13.0
		var neu := Vector3(s, y + 4.38 + 0.48 * sin(winkel), q - seite * 0.48 * (1.0 - cos(winkel)))
		teile.append(_stab(alt, neu, 0.046, 0.046))
		alt = neu
	_sammelnetz(bruecke, teile, _lampenmetall)
	var haube := CylinderMesh.new()
	haube.top_radius = 0.073
	haube.bottom_radius = 0.23
	haube.height = 0.17
	haube.radial_segments = 16
	_modell(bruecke, haube, _lampenmetall).position = alt + Vector3.DOWN * 0.08
	var glas := SphereMesh.new()
	glas.radius = 0.18
	glas.height = 0.20
	glas.radial_segments = 14
	glas.rings = 7
	_modell(bruecke, glas, _milchglas).position = alt + Vector3.DOWN * 0.20


func _uferdetail(sektor: Node3D, ostufer: Array) -> void:
	# Abdeckplatten an den steinernen Widerlagern auf beiden Seiten.
	for anker in [Vector3(-203.1, 5.17, 260.3), Vector3(-97.0, 5.10, 215.2)]:
		var kopf := _quader(sektor, anker, Vector3(2.16, 1.88, 13.22), _stein)
		kopf.rotation.y = 0.402
	if ostufer.size() < 2:
		return
	var wand := Node3D.new()
	wand.name = "Befestigtes Altstadtufer"
	sektor.add_child(wand)
	var steine: Array[Array] = [[], [], []]
	var decksteine: Array[Transform3D] = []
	var sockel: Array[Transform3D] = []
	var rng := RandomNumberGenerator.new()
	rng.seed = 4891
	for i in range(ostufer.size() - 1):
		var a := Vector3(ostufer[i][0], ostufer[i][2], ostufer[i][1])
		var b := Vector3(ostufer[i + 1][0], ostufer[i + 1][2], ostufer[i + 1][1])
		if a.z < 223.0 and b.z > 209.0:
			continue
		var n := (b - a).cross(Vector3.UP).normalized()
		var boden := 2.59 if i > 7 else 2.32
		var darauf := Vector3.UP * 0.07
		decksteine.append(_stab(a + darauf, b + darauf, 0.48, 0.20))
		var tief_a := Vector3(a.x, boden, a.z)
		var tief_b := Vector3(b.x, boden, b.z)
		var mitte_a := a.lerp(tief_a, 0.5)
		var mitte_b := b.lerp(tief_b, 0.5)
		sockel.append(_stab(mitte_a - n * 0.08, mitte_b - n * 0.08, 0.31, maxf(a.y, b.y) - boden))
		var laenge := a.distance_to(b)
		for reihe in ceili((maxf(a.y, b.y) - boden) / 0.36):
			var s := 0.0
			while s < laenge - 0.04:
				var ende := minf(s + rng.randf_range(0.60, 1.17), laenge)
				var startstein := a.lerp(b, s / laenge) + n * 0.08
				var endstein := a.lerp(b, ende / laenge) + n * 0.08
				startstein.y -= 0.18 + reihe * 0.36
				endstein.y -= 0.18 + reihe * 0.36
				if minf(startstein.y, endstein.y) > boden + 0.14:
					steine[rng.randi_range(0, 2)].append(_stab(startstein, endstein, 0.18, 0.327))
				s = ende + 0.028
	_sammelnetz(wand, sockel, _material("#716b5f"))
	_sammelnetz(wand, decksteine, _material("#a7a091"))
	for i in steine.size():
		var reihe: Array[Transform3D] = []
		reihe.assign(steine[i])
		_sammelnetz(wand, reihe, _material(["#96917f", "#a19a86", "#837e6c"][i]))


func _wehr(sektor: Node3D, trasse: Array) -> void:
	if trasse.size() < 2:
		return
	var mauer := SurfaceTool.new()
	mauer.begin(Mesh.PRIMITIVE_TRIANGLES)
	var schaum := SurfaceTool.new()
	schaum.begin(Mesh.PRIMITIVE_TRIANGLES)
	var punkte := PackedVector3Array()
	for p in trasse:
		punkte.append(Vector3(p[0], 3.22, p[1]))
	# Alle Segmente teilen die Schnittkante ihrer versetzten Stufen im Bogen.
	var quer := PackedVector3Array()
	for i in punkte.size():
		var davor := punkte[maxi(i - 1, 0)].direction_to(punkte[i]).cross(Vector3.UP)
		var danach := punkte[i].direction_to(punkte[mini(i + 1, punkte.size() - 1)]).cross(Vector3.UP)
		var halbierende := (davor + danach).normalized()
		quer.append(halbierende / maxf(halbierende.dot(davor), halbierende.dot(danach)))
	var station := 0.0
	for i in range(punkte.size() - 1):
		var a := punkte[i]
		var b := punkte[i + 1]
		var laenge := a.distance_to(b)
		for stufe in 4:
			var ebene := Vector3.DOWN * stufe * 0.25
			var v := a - quer[i] * (stufe * 2.03) + ebene
			var h := b - quer[i + 1] * (stufe * 2.03) + ebene
			var kante_a := a - quer[i] * ((stufe + 1) * 2.03) + ebene
			var kante_b := b - quer[i + 1] * ((stufe + 1) * 2.03) + ebene
			if i == 0 or i == trasse.size() - 2:
				var rand := v if i == 0 else h
				var kante := kante_a if i == 0 else kante_b
				var sohle := Vector3.DOWN * (1.25 - stufe * 0.25)
				_viereck(mauer, rand, kante, kante + sohle, rand + sohle)
			var profil := [v, h, kante_b, kante_a, kante_a + Vector3.DOWN * 0.25,
					kante_b + Vector3.DOWN * 0.25]
			for j in [0, 3, 2, 0, 2, 1, 3, 4, 5, 3, 5, 2]:
				var u := station + (laenge if j in [1, 2, 5] else 0.0)
				var v_uv := 1.14 if j >= 4 else (1.0 if j >= 2 else 0.0)
				schaum.set_uv(Vector2(u, v_uv))
				schaum.set_uv2(Vector2(stufe, 0))
				schaum.set_smooth_group(-1)
				schaum.add_vertex(profil[j])
		station += laenge
	mauer.generate_normals()
	schaum.generate_normals()
	var wasser := ShaderMaterial.new()
	wasser.shader = preload("res://shader/lechwehr.gdshader")
	var wehr := _modell(sektor, mauer.commit(), _stein)
	wehr.name = "Karolinenwehr"
	wehr.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	_modell(sektor, schaum.commit(), wasser).cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
