class_name Wagenmesh
extends RefCounted
## Karosserie-Netz aus daten/wagen.json (Quelle der Unreal-Version), geteilt
## zwischen dem fahrbaren Auto, den KI-Autos und den geparkten Autos.
## Die Quelldaten schauen nach +X, das Netz behaelt diese Richtung.

const SHADER := preload("res://shader/flach.gdshader")
const SHADER_INSTANZ := preload("res://shader/flach_instanz.gdshader")

const AUTO_SHADER := preload("res://shader/auto.gdshader")
const RING := 18   ## Punkte je Querschnitt: Mitte unten, rechte Haelfte hinauf, linke herunter

static var _cache := {}
static var _wagen = null
static var _profil_daten = null


## Fahrzeugmodelle: "klein" ist der Kleinwagen (daten/auto_profil.json, texturen/auto_*),
## die anderen kommen aus Tools/baue_auto_modelle.py (daten/auto_profil_<modell>.json,
## texturen/auto_<modell>_*).
const MODELLE := ["klein", "limousine", "kombi", "bus", "sport", "renn", "super"]
const RAD_BREITE := 0.24

static var _rad_cache := {}


## lack: Farbe der lackierten Teile. instanziert: Lack kommt aus der Instanzfarbe eines
## MultiMesh (dann ist "lack" ohne Bedeutung). mit_raedern = false: die Raeder fehlen, der
## Aufrufer haengt sie einzeln an (rad_netz, radlagen) und dreht sie.
static func netz(lack: Color, instanziert := false, modell := "klein",
		mit_raedern := true) -> ArrayMesh:
	var schluessel := "%s|%s|%s|%s" % [lack.to_html(false), instanziert, modell, mit_raedern]
	if _cache.has(schluessel):
		return _cache[schluessel]
	var profil := _profil(modell)
	if not profil.is_empty():
		var neu := _karosserie(lack, instanziert, profil, modell, mit_raedern)
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

static func _profil(modell := "klein") -> Dictionary:
	if _profil_daten == null:
		_profil_daten = {}
	if not _profil_daten.has(modell):
		var pfad := "res://daten/auto_profil.json" if modell == "klein" \
				else "res://daten/auto_profil_%s.json" % modell
		var d = JSON.parse_string(FileAccess.get_file_as_string(pfad)) \
				if FileAccess.file_exists(pfad) else null
		_profil_daten[modell] = d if d is Dictionary else {}
		if modell != "klein" and (_profil_daten[modell] as Dictionary).is_empty():
			_profil_daten[modell] = _profil("klein")   # fehlt das Modell: Kleinwagen
	return _profil_daten[modell]


## Gibt es dieses Modell (Profil vorhanden)?
static func hat_modell(modell: String) -> bool:
	return modell == "klein" or FileAccess.file_exists("res://daten/auto_profil_%s.json" % modell)


## Laenge und Breite des Modells in Metern (fuer Kollision und Abstaende).
static func masse(modell := "klein") -> Vector2:
	var p := _profil(modell)
	if p.is_empty():
		return Vector2(4.3, 1.8)
	var b := 0.0
	for w in p["halbe_breite"]:
		b = maxf(b, float(w))
	return Vector2(float(p["laenge"]), minf(b * 2.0, 2.1))


## Radmitten im Netz (x vorwaerts, y oben, z rechts): vorn links, vorn rechts, hinten links,
## hinten rechts; w = Radius.
static func radlagen(modell := "klein") -> Array:
	var p := _profil(modell)
	var r: Array = []
	if p.is_empty():
		return r
	var n: int = (p["dach"] as Array).size()
	for rad: Dictionary in p["raeder"]:
		var u: float = rad["u"]
		var i := clampi(roundi(u * (n - 1)), 0, n - 1)
		var spur := _spur(p, i)
		for s in [-1.0, 1.0]:
			r.append(Vector4((0.5 - u) * float(p["laenge"]), float(rad["r"]), s * spur, float(rad["r"])))
	return r


## Abstand der Radmitte von der Mittellinie: die Aussenseite des Rads schliesst mit der
## Karosserie ab.
static func _spur(p: Dictionary, i: int) -> float:
	var b: float = maxf(float(p["halbe_breite"][i]), 0.25)
	return b * 0.97 - RAD_BREITE * 0.5


## Ein Rad (Achse entlang Z, Mitte im Ursprung): Lauffläche dunkel, beide Flanken mit dem
## Rad aus der Seitenansicht der Zeichnung; dreht es sich, dreht sich die Felge sichtbar mit.
static func rad_netz(modell := "klein", nr := 0) -> ArrayMesh:
	var p := _profil(modell)
	var schluessel := "%s|%d" % [modell, nr]
	if _rad_cache.has(schluessel):
		return _rad_cache[schluessel]
	var rad: Dictionary = p["raeder"][mini(nr, (p["raeder"] as Array).size() - 1)]
	var r: float = rad["r"]
	var u0: float = rad["u"]
	var laenge: float = p["laenge"]
	var hoehe: float = p.get("seite_hoehe", 1.59)
	var v0 := 1.0 - r / hoehe
	var lauf := SurfaceTool.new()
	var flanke := SurfaceTool.new()
	lauf.begin(Mesh.PRIMITIVE_TRIANGLES)
	flanke.begin(Mesh.PRIMITIVE_TRIANGLES)
	var segmente := 18
	var h := RAD_BREITE * 0.5
	for k in segmente:
		var w0 := TAU * float(k) / segmente
		var w1 := TAU * float(k + 1) / segmente
		var a := Vector3(cos(w0) * r, sin(w0) * r, 0.0)
		var b := Vector3(cos(w1) * r, sin(w1) * r, 0.0)
		# Laufflaeche (leicht gerundet: aussen etwas kleiner)
		for tri in [[a + Vector3(0, 0, h), b + Vector3(0, 0, h), a - Vector3(0, 0, h)],
				[b + Vector3(0, 0, h), b - Vector3(0, 0, h), a - Vector3(0, 0, h)]]:
			for v: Vector3 in tri:
				lauf.add_vertex(v)
		# Flanken: Faecher zur Mitte, UV aus der Seitenansicht (x vorwaerts -> u kleiner).
		for seite in [1.0, -1.0]:
			var z: float = seite * h * 0.96
			var pts := [Vector3(0, 0, z), a * 0.97 + Vector3(0, 0, z), b * 0.97 + Vector3(0, 0, z)]
			if seite < 0.0:
				pts = [pts[0], pts[2], pts[1]]
			for v: Vector3 in pts:
				flanke.set_uv(Vector2(u0 - v.x / laenge, v0 - v.y / hoehe))
				flanke.add_vertex(v)
	var netz := ArrayMesh.new()
	_anfuegen(netz, lauf, _material(Color(0.02, 0.02, 0.022), false, false, false, 0.92))
	var m := StandardMaterial3D.new()
	var tex := _textur(modell, "seite")
	if tex != null:
		m.albedo_texture = tex
	else:
		m.albedo_color = Color(0.5, 0.5, 0.52)
	m.roughness = 0.45
	m.metallic = 0.3
	_anfuegen(netz, flanke, m)
	_rad_cache[schluessel] = netz
	return netz


static func _textur(modell: String, ansicht: String) -> Texture2D:
	var pfad := "res://texturen/auto_%s.png" % ansicht if modell == "klein" \
			else "res://texturen/auto_%s_%s.png" % [modell, ansicht]
	return load(pfad) if ResourceLoader.exists(pfad) else null


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


## Querschnitt an einer Stelle: 10 Punkte der rechten Haelfte (Seite, Hoehe), von der Mitte
## unten ueber Schwelle, Radlauf, Tuer, Schulter, Fensterband und Dach bis zur Dachmitte.
## bogen: Hoehe des Radlaufs an dieser Stelle (Punkt 3).
static func _halbring(dach: float, breite: float, schwelle: float, gurt: float,
		bogen: float) -> Array:
	var top := maxf(dach, schwelle + 0.3)
	var gurtlinie := minf(top, gurt)
	var kabine := clampf((top - gurtlinie - 0.02) / 0.12, 0.0, 1.0)
	var hp := []
	hp.append(Vector2(0.0, schwelle))
	hp.append(Vector2(breite * 0.9, schwelle))
	hp.append(Vector2(breite, schwelle + 0.14))
	hp.append(Vector2(breite, clampf(bogen, schwelle + 0.16, gurtlinie - 0.12)))
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


static func _karosserie(lack: Color, instanziert: bool, p: Dictionary, modell: String,
		mit_raedern: bool) -> ArrayMesh:
	var laenge: float = p["laenge"]
	var dach: Array = p["dach"]
	var breiten: Array = p["halbe_breite"]
	# Geparkte Autos (Hunderte, instanziert) mit jedem zweiten Schnitt: halb so viele Dreiecke,
	# auf Abstand nicht zu unterscheiden.
	if instanziert:
		dach = _jeder_zweite(dach)
		breiten = _jeder_zweite(breiten)
	var n := dach.size()
	var schwelle: float = p["schwelle"]
	var gurt: float = p["gurt"]
	var seite: Array = p["seitenfenster"]
	var wind: Array = p["windschutz"]
	var heck: Array = p["heckscheibe"]
	# Radlaeufe: Mitte (x) und Radius je Rad; dort bleibt die Flanke offen, der Bogen folgt
	# dem Rad mit etwas Luft.
	var laeufe: Array = []
	for rad: Dictionary in p["raeder"]:
		laeufe.append(Vector2((0.5 - float(rad["u"])) * laenge, float(rad["r"]) * 1.12))
	# Gitter der Punkte: Schnitt i, Ringpunkt j. Vorne liegt u = 0 (Fahrtrichtung +X).
	var gitter: Array = []
	var halbringe: Array = []
	for i in n:
		var u := float(i) / float(n - 1)
		var x := (0.5 - u) * laenge
		var bogen := float(p["raeder"][0]["r"]) * 2.0 + 0.06
		for l: Vector2 in laeufe:
			var dx := absf(x - l.x)
			if dx < l.y:
				bogen = l.y / 1.12 + sqrt(l.y * l.y - dx * dx)
		var hp := _halbring(dach[i], maxf(breiten[i], 0.25), schwelle, gurt, bogen)
		halbringe.append(hp)
		var ring := PackedVector3Array()
		for k in 10:
			ring.append(Vector3(x, hp[k].y, hp[k].x))
		for k in range(8, 0, -1):
			ring.append(Vector3(x, hp[k].y, -hp[k].x))
		gitter.append(ring)
	# Mit Zeichnung kommen die Scheiben aus der Textur (der Shader macht sie glatt); eine
	# eigene Glasflaeche gibt es nur ohne.
	var mit_zeichnung := _textur(modell, "seite") != null
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
		var xm := (0.5 - (float(i) + 0.5) / float(n - 1)) * laenge
		var im_radlauf := false
		for l: Vector2 in laeufe:
			im_radlauf = im_radlauf or absf(xm - l.x) < l.y
		for j in RING:
			var halb := j if j <= 8 else 17 - j
			var ziel := lackflaeche
			if halb == 0:
				ziel = unterbau
			elif (halb == 1 or halb == 2) and im_radlauf:
				continue   # Radlauf: offen, dahinter das Rad und die dunkle Radhausschale
			elif halb == 6 and seitenglas and not mit_zeichnung:
				ziel = glasflaeche
			elif (halb == 7 or halb == 8) and frontglas and not mit_zeichnung:
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
	var lackmaterial := _material(lack, true, instanziert, false, 0.3)
	_zeichnung_zuweisen(lackmaterial, p, modell)
	_anfuegen(netz, lackflaeche, lackmaterial)
	if not mit_zeichnung:
		_anfuegen(netz, glasflaeche, _material(Color(0.02, 0.03, 0.04), false, false, true))
	_anfuegen(netz, unterbau, _material(Color(0.02, 0.02, 0.022), false, false, false, 0.9))
	_raeder_und_licht(netz, p, halbringe, modell, mit_raedern)
	return netz


## Hat die Linienzeichnung (Texturen aus Tools/baue_auto_profil.py), liegt sie auf dem Lack.
static func _zeichnung_zuweisen(m: ShaderMaterial, p: Dictionary, modell: String) -> void:
	for ansicht in ["seite", "oben", "front", "heck"]:
		if _textur(modell, ansicht) == null:
			return
	for ansicht in ["seite", "oben", "front", "heck"]:
		m.set_shader_parameter("tex_" + ansicht, _textur(modell, ansicht))
	m.set_shader_parameter("front_breite", p.get("front_breite", 2.08))
	m.set_shader_parameter("heck_breite", p.get("heck_breite", 2.08))
	m.set_shader_parameter("laenge", p["laenge"])
	m.set_shader_parameter("seite_hoehe", p.get("seite_hoehe", 1.59))
	m.set_shader_parameter("oben_breite", p.get("oben_breite", 1.94))
	m.set_shader_parameter("detail_an", true)


static func _jeder_zweite(a: Array) -> Array:
	var r: Array = []
	for i in range(0, a.size(), 2):
		r.append(a[i])
	if (a.size() - 1) % 2 != 0:
		r.append(a[a.size() - 1])
	return r


static func _anfuegen(netz: ArrayMesh, st: SurfaceTool, material: Material) -> void:
	st.generate_normals()
	st.commit(netz)
	netz.surface_set_material(netz.get_surface_count() - 1, material)


## Raeder (oder nur die Radhausschalen), Spiegel und ein Heckfluegel, wenn das Profil einen
## hat; Lichter, Grill und Kennzeichen kommen aus der Textur.
static func _raeder_und_licht(netz: ArrayMesh, p: Dictionary, halbringe: Array, modell: String,
		mit_raedern: bool) -> void:
	var laenge: float = p["laenge"]
	var schalen := SurfaceTool.new()
	var anbau := SurfaceTool.new()
	for st in [schalen, anbau]:
		(st as SurfaceTool).begin(Mesh.PRIMITIVE_TRIANGLES)
	schalen.set_smooth_group(-1)   # flache Normalen: die Gegenwicklung hebt sie sonst auf
	var n: int = halbringe.size()
	# Radhausschale: obere Haelfte eines Zylinders um das Rad, innen sichtbar (dunkel).
	for rad: Dictionary in p["raeder"]:
		var x: float = (0.5 - float(rad["u"])) * laenge
		var r: float = float(rad["r"]) * 1.12
		var i := clampi(roundi(float(rad["u"]) * (n - 1)), 0, n - 1)
		var b: float = (halbringe[i][2] as Vector2).x
		for k in 10:
			var w0 := PI * float(k) / 10.0
			var w1 := PI * float(k + 1) / 10.0
			var a0 := Vector3(x + cos(w0) * r, float(rad["r"]) + sin(w0) * r, 0.0)
			var a1 := Vector3(x + cos(w1) * r, float(rad["r"]) + sin(w1) * r, 0.0)
			var z := Vector3(0, 0, b)
			# Beide Wicklungen: von aussen durch den Radlauf sieht man die Innenseite.
			for tri in [[a0 - z, a1 - z, a0 + z], [a1 - z, a1 + z, a0 + z],
					[a0 - z, a0 + z, a1 - z], [a1 - z, a0 + z, a1 + z]]:
				for v: Vector3 in tri:
					schalen.add_vertex(v)
	for s in [1.0, -1.0]:
		var w: float = halbringe[roundi(0.34 * (n - 1))][2].x
		schalen.append_from(_kasten(Vector3(0.10, 0.09, 0.16)), 0, Transform3D(Basis(),
				Vector3(laenge * 0.16, p["gurt"] + 0.02, s * (w + 0.06))))
	if p.has("fluegel"):
		var f: Dictionary = p["fluegel"]
		var x0: float = (0.5 - float(f["u"][0])) * laenge
		var x1: float = (0.5 - float(f["u"][1])) * laenge
		var hoch: float = f["hoehe"]
		var unter: float = f["unter"]
		var halb_b := 0.0
		for wb in p["halbe_breite"]:
			halb_b = maxf(halb_b, float(wb))
		var tiefe := absf(x0 - x1) * 0.55
		var xm := (x0 + x1) * 0.5 - absf(x0 - x1) * 0.15
		anbau.append_from(_kasten(Vector3(tiefe, 0.03, halb_b * 1.9)), 0,
				Transform3D(Basis(Vector3.BACK, 0.08), Vector3(xm, hoch - 0.02, 0)))
		for s in [1.0, -1.0]:
			anbau.append_from(_kasten(Vector3(tiefe * 1.1, 0.16, 0.01)), 0,
					Transform3D(Basis(), Vector3(xm, hoch - 0.06, s * halb_b * 0.95)))
			anbau.append_from(_kasten(Vector3(0.06, hoch - unter + 0.04, 0.015)), 0,
					Transform3D(Basis(), Vector3(xm + 0.05, (hoch + unter) * 0.5, s * halb_b * 0.4)))
	# Radhausschalen und Spiegel teilen sich eine dunkle Flaeche (ein Zeichenaufruf weniger).
	_anfuegen(netz, schalen, _material(Color(0.014, 0.014, 0.016), false, false, false, 0.8))
	if p.has("fluegel"):
		_anfuegen(netz, anbau, _material(Color(0.03, 0.03, 0.035), false, false, false, 0.35, 0.2))
	if not mit_raedern:
		return
	# Fest eingebaute Raeder (geparkte Autos): dieselben Netze wie die drehbaren.
	# Alle vier in zwei Flaechen (Laufflaechen, Flanken).
	var lagen := radlagen(modell)
	var teile: Array[SurfaceTool] = [SurfaceTool.new(), SurfaceTool.new()]
	for k in lagen.size():
		var l: Vector4 = lagen[k]
		var rad := rad_netz(modell, k / 2)
		for f in 2:
			teile[f].append_from(rad, f, Transform3D(Basis(), Vector3(l.x, l.y, l.z)))
	var vorlage := rad_netz(modell, 0)
	for f in 2:
		teile[f].commit(netz)
		netz.surface_set_material(netz.get_surface_count() - 1, vorlage.surface_get_material(f))


static func _kasten(groesse: Vector3) -> BoxMesh:
	var b := BoxMesh.new()
	b.size = groesse
	return b
