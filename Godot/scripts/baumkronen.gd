class_name Baumkronen
extends RefCounted
## Texturierte Zweige bilden die Kronen. Das Netz lässt den Himmel
## zwischen den Zweigen durchscheinen und wird pro Sektor instanziert.

const SHADER := preload("res://shader/baumlaub.gdshader")
const TEXEL := 128


static func formen() -> Array[Mesh]:
	var laub := _material(false)
	var nadeln := _material(true)
	return [_laubbaum(laub, 78, 1.0), _laubbaum(laub, 91, 0.86), _laubbaum(laub, 128, 1.13),
			_nadelbaum(nadeln)]


static func _material(nadeln: bool) -> ShaderMaterial:
	var bild := Image.create(TEXEL, TEXEL, false, Image.FORMAT_RGBA8)
	var zufall := RandomNumberGenerator.new()
	zufall.seed = 7162 if nadeln else 511
	var wurzel := Vector2(64, 123)
	var spitze := Vector2(60, 11)
	_strich(bild, wurzel, spitze, 1.55, 0.42)
	for i in 9:
		var t := (i + 0.8) / 10.2
		var ort := wurzel.lerp(spitze, t)
		for seite in [-1.0, 1.0]:
			var breite := (1.0 - t * 0.79) * zufall.randf_range(33.0, 51.0)
			var ende := ort + Vector2(seite * breite, -14.0 - (1.0 - t) * 7.0)
			var achse := (ende - ort).normalized()
			_strich(bild, ort, ende, 1.06 if nadeln else 0.72, 0.47)
			for j in (9 if nadeln else 5):
				var p := ort.lerp(ende, (j + 1.0) / (10.0 if nadeln else 5.6))
				if nadeln:
					for rand in [-1.0, 1.0]:
						var d: Vector2 = (achse + achse.orthogonal() * float(rand) * 0.74).normalized()
						_strich(bild, p, p + d * zufall.randf_range(6.2, 13.5), 0.65,
								zufall.randf_range(0.59, 0.94))
				else:
					var quer := achse.orthogonal() * (1.0 if j % 2 == 0 else -1.0)
					var d := (achse * 0.49 + quer).normalized()
					var l := zufall.randf_range(7.0, 13.5) * (1.0 - t * 0.30)
					_blatt(bild, p + d * l * 0.42, d, l, zufall.randf_range(0.60, 0.97))
			if not nadeln:
				_blatt(bild, ende, achse, zufall.randf_range(8.0, 12.0), 0.88)
	bild.generate_mipmaps()
	var m := ShaderMaterial.new()
	m.shader = SHADER
	m.set_shader_parameter("zweig", ImageTexture.create_from_image(bild))
	return m


static func _strich(bild: Image, von: Vector2, bis: Vector2, breit: float, ton: float) -> void:
	var richtung := bis - von
	var min_x := maxi(0, floori(minf(von.x, bis.x) - breit - 1.0))
	var min_y := maxi(0, floori(minf(von.y, bis.y) - breit - 1.0))
	var max_x := mini(TEXEL - 1, ceili(maxf(von.x, bis.x) + breit + 1.0))
	var max_y := mini(TEXEL - 1, ceili(maxf(von.y, bis.y) + breit + 1.0))
	for y in range(min_y, max_y + 1):
		for x in range(min_x, max_x + 1):
			var p := Vector2(x, y) - von
			var t := clampf(p.dot(richtung) / maxf(0.01, richtung.length_squared()), 0.0, 1.0)
			var a := clampf(breit + 0.48 - (p - richtung * t).length(), 0.0, 1.0)
			if a > bild.get_pixel(x, y).a:
				bild.set_pixel(x, y, Color(ton, ton, ton, a))


static func _blatt(bild: Image, ort: Vector2, achse: Vector2, l: float, ton: float) -> void:
	var auf := achse.orthogonal()
	for y in range(maxi(0, floori(ort.y - l)), mini(TEXEL, ceili(ort.y + l))):
		for x in range(maxi(0, floori(ort.x - l)), mini(TEXEL, ceili(ort.x + l))):
			var p := Vector2(x, y) - ort
			var u := p.dot(achse) / (l * 0.60)
			var v := p.dot(auf) / (l * 0.30)
			var r := absf(u) ** 1.38 + absf(v) ** 1.82
			var a := clampf((1.02 - r) * 4.2, 0.0, 1.0)
			if a > bild.get_pixel(x, y).a:
				var schatten := ton + (0.045 if v < 0 else -0.035) + (0.07 if absf(v) < 0.13 else 0.0)
				bild.set_pixel(x, y, Color(schatten, schatten, schatten, a))


static func _laubbaum(material: Material, seed_value: int, breite: float) -> ArrayMesh:
	var zufall := RandomNumberGenerator.new()
	zufall.seed = seed_value
	var netz := _beginne(material)
	for i in 80:
		var a := TAU * i / 2.6180339
		var y := zufall.randf_range(-0.41, 0.43)
		var r := sqrt(1.0 - minf(0.97, y * y * 4.5)) * breite
		var nach_aussen := Vector3(cos(a), 0, sin(a))
		var quer := Vector3(-sin(a), 0, cos(a))
		var von := nach_aussen * r * zufall.randf_range(0.03, 0.27) + Vector3.UP * y
		var bis := nach_aussen * r * zufall.randf_range(0.34, 0.52)
		bis.y = y + zufall.randf_range(-0.09, 0.14)
		var halbe := zufall.randf_range(0.11, 0.185)
		_karte(netz, von, bis, (quer * 0.65 + Vector3.UP * zufall.randf_range(-0.81, 0.81)).normalized() * halbe)
		if i % 2 == 0:
			_karte(netz, von.lerp(bis, 0.27), bis + Vector3.UP * 0.025, Vector3.UP * halbe * 0.68)
	netz.generate_normals()
	return netz.commit()


static func _nadelbaum(material: Material) -> ArrayMesh:
	var zufall := RandomNumberGenerator.new()
	zufall.seed = 702619
	var netz := _beginne(material)
	for i in 9:
		var fortschritt := i / 8.0
		var y := -0.44 + fortschritt * 0.88
		var radius := lerpf(0.54, 0.067, pow(fortschritt, 1.25))
		for j in 7:
			var a := j * TAU / 7.0 + i * 0.48 + zufall.randf_range(-0.23, 0.23)
			var nach_aussen := Vector3(cos(a), 0, sin(a))
			var quer := Vector3(-sin(a), 0, cos(a))
			var von := Vector3.UP * (y + zufall.randf_range(-0.027, 0.027))
			var bis := von + nach_aussen * radius * zufall.randf_range(0.76, 1.09)
			bis.y += lerpf(-0.067, 0.092, fortschritt)
			_karte(netz, von, bis, quer * radius * 0.36 + Vector3.UP * 0.024)
			_karte(netz, von.lerp(bis, 0.24), bis + Vector3.UP * 0.034, Vector3.UP * radius * 0.16)
	netz.generate_normals()
	return netz.commit()


static func _beginne(material: Material) -> SurfaceTool:
	var netz := SurfaceTool.new()
	netz.begin(Mesh.PRIMITIVE_TRIANGLES)
	netz.set_material(material)
	return netz


static func _karte(netz: SurfaceTool, von: Vector3, bis: Vector3, quer: Vector3) -> void:
	var ecken := [von - quer, von + quer, bis + quer, bis - quer]
	var uv := [Vector2(0, 1), Vector2(1, 1), Vector2(1, 0), Vector2(0, 0)]
	for j in [0, 1, 2, 0, 2, 3]:
		netz.set_uv(uv[j])
		netz.add_vertex(ecken[j])
