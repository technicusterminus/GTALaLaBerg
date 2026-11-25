class_name Hartmannviertel
extends RefCounted
## Gebäude und Ausstattung auf den eingemessenen Außenflächen des Viertels.

var _putz := _material("#d8d1c3")
var _weiss := _material("#d5d9d6")
var _metall := _material("#55636b")
var _glas := _material("#365665", 0.34)
var _rot := _material("#b52327")
var _dach := _material("#777b6d")
var _holz := _material("#957457")
var _sand := _material("#ccb694")
var _solarmodul := _material("#273c55", 0.38)


func baue(sektor: Node3D, k: Vector2i, breslauer_objekte: Array) -> void:
	match k:
		Vector2i(-4, 0):
			_rewe(sektor)
			_pflegeschule(sektor)
			_kinderhaus(sektor)
			_haltestelle(sektor, Vector3(-1696.15, 28.60, 126.44), 1.88, "Klinikum")
			for ort in [Vector3(-1805.83, 27.25, 12.04), Vector3(-1780.20, 26.69, 84.49),
						Vector3(-1743.11, 27.75, 69.58)]:
				_leuchte(sektor, ort)
		Vector2i(-3, 0):
			_haltestelle(sektor, Vector3(-1448.35, 22.03, 22.17), 2.19, "Breslauer Straße")
			for ort in [Vector3(-1476.31, 21.00, 23.17), Vector3(-1462.34, 21.34, 50.39)]:
				_leuchte(sektor, ort)
	if DisplayServer.get_name() == "headless":
		return
	for eintrag: Array in breslauer_objekte:
		var ort := Vector3(eintrag[0], eintrag[1], eintrag[2])
		if eintrag[4] == "Leuchte":
			_leuchte(sektor, ort, eintrag[3])
		else:
			_haltestelle(sektor, ort, eintrag[3], eintrag[4])


func _rewe(sektor: Node3D) -> void:
	# Die Hauptfassade verbindet die beiden vorhandenen LoD2-Wandenden.
	var front := _gruppe(sektor, "REWEMarktBreslauerStrasse", Vector3(-1500.74, 20.96, 4.84), 0.655)
	_quader(front, Vector3(0, 1.93, 0.085), Vector3(54.91, 3.93, 0.16), _putz)
	_quader(front, Vector3(0, 0.18, 0.20), Vector3(54.82, 0.53, 0.30), _metall)
	_quader(front, Vector3(0, 3.43, 0.78), Vector3(55.1, 0.25, 1.67), _metall)
	_quader(front, Vector3(0, 3.43, 1.62), Vector3(55.1, 0.30, 0.12), _weiss)
	for i in 10:
		var x := -24.35 + i * 5.40
		_quader(front, Vector3(x, 1.70, 0.22), Vector3(4.58, 2.50, 0.16), _glas)
		for v in [-2.28, 0.0, 2.28]:
			_quader(front, Vector3(x + v, 1.70, 0.33), Vector3(0.072, 2.55, 0.14), _metall)
		_quader(front, Vector3(x, 2.83, 0.34), Vector3(4.60, 0.10, 0.16), _weiss)
		if i % 2 == 0:
			_quader(front, Vector3(x - 1.3, 1.60, 1.52), Vector3(0.15, 3.22, 0.15), _metall)
	var eingang := Vector3(7.55, 0.0, 0.0)
	_quader(front, eingang + Vector3(0, 1.49, 0.38), Vector3(6.40, 2.95, 0.20), _metall)
	for x in [-2.10, 0.0, 2.10]:
		_quader(front, eingang + Vector3(x, 1.44, 0.52), Vector3(1.99, 2.65, 0.16), _glas)
	_quader(front, eingang + Vector3(0, 3.85, 1.63), Vector3(8.5, 1.11, 0.22), _rot)
	_schrift(front, "REWE  Lischka", eingang + Vector3(0.0, 3.83, 1.78), 0.017, Color.WHITE)
	var wagen := _gruppe(front, "Wagenunterstand", Vector3(-16.1, 0.40, 6.2), 0)
	_quader(wagen, Vector3(0, 2.02, 0), Vector3(6.61, 0.16, 2.05), _glas)
	for x in [-3.16, 3.16]:
		for z in [-0.88, 0.88]:
			_quader(wagen, Vector3(x, 0.99, z), Vector3(0.12, 1.98, 0.12), _metall)
	for x in [-2.4, -1.65, -0.9, -0.15, 0.6]:
		_quader(wagen, Vector3(x, 0.68, 0.3), Vector3(0.64, 0.33, 0.61), _weiss)
		_quader(wagen, Vector3(x, 0.69, 0.69), Vector3(0.65, 0.07, 0.12), _rot)


func _pflegeschule(sektor: Node3D) -> void:
	var bau := _gruppe(sektor, "BerufsfachschuleFuerPflege", Vector3(-1711.42, 28.02, 188.43), 0.337)
	var stein := _material("#c9c7ad")
	var sockel := _material("#b4ad9d")
	# Der Hof bleibt nach oben offen. Vier Riegel tragen das Dach und die Weltkollision.
	for s in [-1.0, 1.0]:
		_quader(bau, Vector3(0, 3.72, s * 10.35), Vector3(32.9, 7.60, 8.9), stein, true)
		_quader(bau, Vector3(s * 11.0, 3.72, 0), Vector3(10.9, 7.60, 11.86), stein, true)
		_quader(bau, Vector3(0, 7.62, s * 10.35), Vector3(33.15, 0.28, 9.16), _dach)
		_quader(bau, Vector3(s * 11.0, 7.62, 0), Vector3(11.16, 0.28, 12.0), _dach)
	for seite in range(4):
		var front := _gruppe(bau, "Fensterband", Vector3.ZERO, seite * PI * 0.5)
		var rand := 14.83 if seite % 2 == 0 else 16.48
		var breit := 32.9 if seite % 2 == 0 else 29.6
		_quader(front, Vector3(0, 0.35, rand), Vector3(breit, 0.72, 0.19), sockel)
		_quader(front, Vector3(0, 7.49, rand), Vector3(breit, 0.20, 0.20), _weiss)
		for i in range(10):
			var x := -breit * 0.44 + breit * 0.098 * i
			for stock in [2.02, 5.51]:
				_quader(front, Vector3(x, stock, rand + 0.06), Vector3(2.15, 2.41, 0.11), _metall)
				_quader(front, Vector3(x - 0.03, stock, rand + 0.14), Vector3(1.96, 2.24, 0.07), _glas)
				_quader(front, Vector3(x + 0.20, stock, rand + 0.18), Vector3(0.045, 2.25, 0.08), _weiss)
	var eingang := _gruppe(bau, "EingangPflegecampus", Vector3(16.6, 0, 0), PI * 0.5)
	_quader(eingang, Vector3(0, 1.73, 0.16), Vector3(6.0, 3.22, 0.16), _glas)
	_quader(eingang, Vector3(0, 3.39, 1.04), Vector3(6.55, 0.24, 2.15), _metall)
	_schrift(eingang, "BERUFSFACHSCHULE FÜR PFLEGE", Vector3(0, 4.29, 0.22),
			0.009, Color("#4e5150"))
	for z in [-10.9, 10.9]:
		_solaranlage(bau, Vector3(-12.0, 7.93, z), 12, 3)


func _kinderhaus(sektor: Node3D) -> void:
	var front := _gruppe(sektor, "KinderhausRoemerauterrasse", Vector3(-1742.0, 28.4, 162.3), 0.337)
	for i in range(9):
		var x := -16.2 + i * 4.03
		_quader(front, Vector3(x, 1.47, 0.14), Vector3(3.75, 2.66, 0.20), _glas)
		_quader(front, Vector3(x, 1.47, 0.27), Vector3(0.10, 2.70, 0.17), _weiss)
		_quader(front, Vector3(x + 1.78, 1.47, 1.68), Vector3(0.15, 2.98, 0.15), _weiss)
	_quader(front, Vector3(0, 2.93, 0.88), Vector3(36.9, 0.24, 2.14), _weiss)
	_schrift(front, "KINDERHAUS RÖMERAUTERRASSE", Vector3(-5.6, 3.38, 0.25),
			0.008, Color("#4b5d66"))
	_bank(sektor, Vector3(-1755.12, 28.68, 169.39), 0.35)
	_bank(sektor, Vector3(-1716.0, 28.67, 158.14), 0.40)
	var spiel := _gruppe(sektor, "Spielhaus", Vector3(-1776.20, 26.83, 171.53), 0.5)
	_quader(spiel, Vector3(0, 0.10, 0), Vector3(3.0, 0.22, 2.0), _holz)
	for x in [-1.28, 1.28]:
		for z in [-0.84, 0.84]:
			_quader(spiel, Vector3(x, 1.08, z), Vector3(0.13, 2.15, 0.13), _holz)
	for s in [-1.0, 1.0]:
		var dach := _quader(spiel, Vector3(s * 0.64, 2.23, 0), Vector3(1.51, 0.10, 2.28), _dach)
		dach.rotation.z = -s * 0.40
	_quader(spiel, Vector3(0, 0.31, 0.18), Vector3(2.4, 0.04, 1.53), _sand)


func _solaranlage(eltern: Node3D, start: Vector3, spalten: int, reihen: int) -> void:
	for z in reihen:
		for x in spalten:
			var platte := _quader(eltern, start + Vector3(x * 1.19, 0, z * 1.73),
					Vector3(1.12, 0.065, 1.56), _solarmodul)
			platte.rotation.x = 0.10


func _haltestelle(eltern: Node3D, ort: Vector3, winkel: float, name: String) -> void:
	var halt := _gruppe(eltern, name + "Haltestelle", ort, winkel)
	_quader(halt, Vector3(0, 2.27, 0), Vector3(4.15, 0.14, 1.52), _metall)
	_quader(halt, Vector3(0, 1.21, -0.67), Vector3(3.91, 1.81, 0.09), _glas)
	for x in [-1.94, 1.94]:
		_quader(halt, Vector3(x, 1.13, -0.62), Vector3(0.07, 2.26, 0.09), _metall)
	_schrift(halt, name, Vector3(0, 2.24, 0.78), 0.0065, Color.WHITE)
	_bank(halt, Vector3(0, 0, -0.25), 0)


func _leuchte(eltern: Node3D, ort: Vector3, winkel := 0.0) -> void:
	var mast := _gruppe(eltern, "Straßenleuchte", ort, winkel)
	_quader(mast, Vector3(0, 2.63, 0), Vector3(0.11, 5.26, 0.11), _metall)
	_quader(mast, Vector3(0, 5.22, 0.46), Vector3(0.18, 0.13, 1.04), _metall)
	_quader(mast, Vector3(0, 5.16, 0.92), Vector3(0.24, 0.10, 0.28), _weiss)


func _bank(eltern: Node3D, ort: Vector3, winkel: float) -> void:
	var bank := _gruppe(eltern, "Sitzbank", ort, winkel)
	for x in [-0.67, 0.67]:
		_quader(bank, Vector3(x, 0.21, 0), Vector3(0.08, 0.42, 0.46), _metall)
	_quader(bank, Vector3(0, 0.44, 0), Vector3(1.67, 0.10, 0.46), _holz)
	_quader(bank, Vector3(0, 0.74, -0.21), Vector3(1.67, 0.19, 0.07), _holz)


func _gruppe(eltern: Node3D, name: String, ort: Vector3, winkel: float) -> Node3D:
	var gruppe := Node3D.new()
	gruppe.name = name
	gruppe.position = ort
	gruppe.rotation.y = winkel
	eltern.add_child(gruppe)
	return gruppe


func _quader(eltern: Node3D, ort: Vector3, masse: Vector3, material: Material,
		fest := false) -> MeshInstance3D:
	var netz := BoxMesh.new()
	netz.size = masse
	var instanz := MeshInstance3D.new()
	instanz.mesh = netz
	instanz.material_override = material
	instanz.position = ort
	eltern.add_child(instanz)
	if fest:
		var koerper := StaticBody3D.new()
		var kollision := CollisionShape3D.new()
		var form := BoxShape3D.new()
		form.size = masse
		kollision.shape = form
		koerper.add_child(kollision)
		instanz.add_child(koerper)
	return instanz


func _schrift(eltern: Node3D, text: String, ort: Vector3, pixel: float, farbe: Color) -> void:
	var schild := Label3D.new()
	schild.text = text
	schild.font_size = 72
	schild.pixel_size = pixel
	schild.outline_size = 0
	schild.modulate = farbe
	schild.position = ort
	eltern.add_child(schild)


static func _material(farbe: String, rauheit := 0.82) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = Color(farbe)
	material.roughness = rauheit
	return material
