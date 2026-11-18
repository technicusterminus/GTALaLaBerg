class_name Polizei
extends Node
## Fahndung und Streifenwagen. Taten sammeln Punkte, die Punkte ergeben 0 bis 5
## Sterne (1, 4, 9, 16, 25 Punkte; jeder weitere Stern verlangt mehr). Je Stern
## ein Streifenwagen, der ueber den Strassengraphen (netz.json) faehrt; die Route
## wird alle 1,5 s neu geplant. Sieht eine Streife den Spieler, fahren alle zu
## ihm; sonst zu dem Ort, an dem er zuletzt gesehen wurde, und suchen dort die
## Strassen ab. Wer ungesehen bleibt, ist die Fahndung nach einer Weile los,
## ausserhalb des Suchgebiets dreimal so schnell wie mittendrin. Wer neben einem
## Streifenwagen stehen bleibt, wird festgenommen. Ab drei Sternen baut die
## Polizei eine Strassensperre voraus.

const SICHT_NAH := 40.0     ## so nah sieht eine Streife auch ohne freie Sichtlinie
const SICHT_WEIT := 150.0   ## weiter weg sieht sie den Spieler gar nicht
const DIREKT := 35.0        ## ab hier geradewegs auf den Spieler zu, nicht ueber Knoten
const VERLOREN := 900.0     ## weiter weg: die Streife hat verloren und wird abgezogen
const FESTNAHME_ABSTAND := 8.0
const FESTNAHME_DAUER := 2.5
## Polizeiinspektion (orte.json "marken"): dort kommt man nach einer Festnahme frei.
const INSPEKTION := Vector3(-425.9, 0.0, 461.5)
## Gewicht der Taten, in der Reihenfolge von Schaden.Tat.
const GEWICHT := [1.0, 2.0, 2.0, 4.0]
const TAT_NAMEN := ["passant_beschossen", "passant_angefahren", "auto_gestohlen",
		"polizei_beschossen"]
const STREIFE_LACK := Color(0.85, 0.86, 0.9)

var fokus: Node3D           ## wer gerade gesteuert wird: Figur oder Wagen
var sterne := 0
var festnahmen := 0
var pausiert := false       ## im Film: keine Fahndungslogik
var sperre_steht := false
var sperr_ort := Vector3.ZERO
## Wo neue Streifen einsetzen (Meter vom Spieler); der Rauchtest verkuerzt das.
var einsatz_min_m := 250.0
var einsatz_max_m := 450.0

var _knoten := PackedVector3Array()
var _astar := AStar3D.new()
var _punkte := 0.0
var _gesehen := false
var _ungesehen := 0.0
var _letzter_ort := Vector3.ZERO
var _festnahme_uhr := 0.0
var _naechster_einsatz := 0.0
var _streifen: Array = []   # je Streife ein Dictionary (siehe _neue_streife)
var _sperrwagen: Array[Verkehrsauto] = []
var _baken: Array[MeshInstance3D] = []
var _sperre_seit := 0.0
var _letzte_tat := [-10.0, -10.0, -10.0, -10.0]
var _takt := 0.0


func _ready() -> void:
	add_to_group("polizei")
	_lade_netz()


func _lade_netz() -> void:
	var d = JSON.parse_string(FileAccess.get_file_as_string("res://daten/netz.json"))
	if d == null:
		push_warning("daten/netz.json fehlt: python3 Tools/konvertiere_sektoren.py ausfuehren")
		return
	var p: Array = d["p"]
	var e: Array = d["e"]
	for i in range(0, p.size() - 2, 3):
		var pos := Vector3(p[i], p[i + 2], p[i + 1]) / 100.0
		_knoten.append(pos)
		_astar.add_point(_knoten.size() - 1, pos)
	var kanten := 0
	for i in range(0, e.size() - 2, 3):
		# Gerichtete Kanten: Einbahnstrassen bleiben Einbahnstrassen.
		_astar.connect_points(int(e[i]), int(e[i + 1]), false)
		kanten += 1
	print("LALABERG_POLIZEI netz knoten=%d kanten=%d" % [_knoten.size(), kanten])


# --- Auskunft ---------------------------------------------------------------

func hole_sterne() -> int:
	return sterne


## Sieht gerade ein Streifenwagen den Spieler? Sonst laeuft die Suche ab.
func wird_gesehen() -> bool:
	return _gesehen


## 0 bis 1: wie weit man die Verfolger schon abgeschuettelt hat.
func such_anteil() -> float:
	if sterne == 0 or _gesehen:
		return 0.0
	return clampf(_ungesehen / _suchdauer(), 0.0, 1.0)


## 0 bis 1: wie weit eine Festnahme schon ist.
func festnahme_anteil() -> float:
	return _festnahme_uhr / FESTNAHME_DAUER


## Aktive Streifenwagen, fuer die Karte.
func hole_streifen() -> PackedVector3Array:
	var orte := PackedVector3Array()
	for s in _streifen:
		var a: Verkehrsauto = s["auto"]
		if is_instance_valid(a) and not a.ist_geparkt():
			orte.append(a.position_jetzt())
	return orte


## Naechster Punkt des Strassengraphen, mit Hoehe: fuer Marken und Laeden, die
## auf einer Fahrbahn stehen sollen.
func naechster_punkt(ort: Vector3) -> Vector3:
	var k := _naechster_knoten(ort)
	return _knoten[k] if k >= 0 else ort


## Zustand der Streifen als Text, fuer Diagnose im Rauchtest.
func debug_streifen() -> String:
	var teile: Array[String] = []
	for s in _streifen:
		var a: Verkehrsauto = s["auto"]
		if not is_instance_valid(a):
			teile.append("(weg)")
			continue
		var d := _abstand2(a.position_jetzt(), fokus.global_position) if fokus != null else -1.0
		teile.append("[einsatz=%s geparkt=%s route=%d idx=%d ende=%s abstand=%.0fm tempo=%.1f]" % [
			s["im_einsatz"], a.ist_geparkt(), a.weg.route.size(), a.weg.index,
			a.weg.am_ende, d, a.geschwindigkeit_ms().length()])
	return " ".join(teile)


## Kuerzester Weg ueber denselben Strassengraphen, den die Streifen fahren, mit
## Einbahnregel; leer ohne Netz oder ohne Weg.
func route(von: Vector3, nach: Vector3) -> PackedVector3Array:
	var weg := PackedVector3Array()
	var pfad := _pfad(_naechster_knoten(von), _naechster_knoten(nach))
	if pfad.is_empty():
		return weg
	weg.append(von)
	for k in pfad:
		weg.append(_knoten[k])
	weg.append(nach)
	return weg


func _suchdauer() -> float:
	return 6.0 + 4.0 * sterne


func _suchradius() -> float:
	return 150.0 + 50.0 * sterne


# --- Taten ------------------------------------------------------------------

## Von ueberall gerufen, wo etwas passiert; dieselbe Tat zaehlt hoechstens einmal
## je Sekunde, sonst brachte ein Feuerstoss aus der MP gleich drei Sterne.
func melde(tat: int) -> void:
	var jetzt := _jetzt()
	if jetzt - _letzte_tat[tat] < 1.0:
		return
	_letzte_tat[tat] = jetzt
	print("LALABERG_TAT %s" % TAT_NAMEN[tat])
	_erhoehe(GEWICHT[tat])


func _erhoehe(gewicht: float) -> void:
	var vorher := sterne
	_punkte += gewicht
	# Stern n ab n*n Punkten: der erste kommt sofort, der fuenfte erst nach einer
	# ganzen Reihe von Taten.
	sterne = clampi(floori(sqrt(_punkte)), 1, 5)
	# Eine Tat ist immer auch gesehen worden: die Suche beginnt am Tatort von vorn.
	_gesehen = true
	_ungesehen = 0.0
	if fokus != null:
		_letzter_ort = fokus.global_position
	if sterne != vorher:
		print("LALABERG_FAHNDUNG sterne=%d punkte=%.0f" % [sterne, _punkte])
	if vorher == 0:
		_naechster_einsatz = _jetzt() + 1.0


## Neu lackiert und dabei ungesehen (Lackiererei) oder zusammengebrochen: die
## Fahndung endet, die Streifen fahren ab.
func verwische() -> void:
	if sterne > 0:
		_einstellen("lackiert")


func setze_sterne(anzahl: int) -> void:
	_punkte = float(anzahl * anzahl)
	var vorher := sterne
	sterne = clampi(anzahl, 0, 5)
	_gesehen = sterne > 0
	_ungesehen = 0.0
	if fokus != null:
		_letzter_ort = fokus.global_position
	if vorher == 0:
		_naechster_einsatz = _jetzt()
	print("LALABERG_FAHNDUNG sterne=%d punkte=%.0f" % [sterne, _punkte])


func _jetzt() -> float:
	return Time.get_ticks_msec() / 1000.0


# --- Graph ------------------------------------------------------------------

## Naechster Knoten in der Ebene. Mit "vorwaerts" nur Knoten voraus: sonst dreht
## der Wagen bei jeder Neuplanung zu dem Knoten um, an dem er gerade vorbei ist.
func _naechster_knoten(ort: Vector3, vorwaerts := Vector3.ZERO) -> int:
	var beste := -1
	var beste_d := INF
	var filter := vorwaerts != Vector3.ZERO
	for i in _knoten.size():
		var d := _knoten[i] - ort
		var d2 := d.x * d.x + d.z * d.z
		if filter and d2 > 25.0 and d.dot(vorwaerts) < 0.0:
			continue
		if d2 < beste_d:
			beste_d = d2
			beste = i
	return beste


func _pfad(von: int, nach: int) -> PackedInt32Array:
	var pfad := PackedInt32Array()
	if von < 0 or nach < 0:
		return pfad
	for id in _astar.get_id_path(von, nach):
		pfad.append(int(id))
	return pfad


func _abstand2(a: Vector3, b: Vector3) -> float:
	return Vector2(a.x - b.x, a.z - b.z).length()


# --- Streifen ---------------------------------------------------------------

func _sieht(auto: Verkehrsauto) -> bool:
	var von := auto.position_jetzt() + Vector3(0, 1.8, 0)
	var nach := fokus.global_position
	var d := von.distance_to(nach)
	if d < SICHT_NAH:
		return true
	if d > SICHT_WEIT:
		return false
	var abfrage := PhysicsRayQueryParameters3D.create(von, nach + Vector3(0, 1.0, 0))
	abfrage.exclude = _ausnahmen(auto)
	abfrage.collision_mask = 1
	return fokus.get_world_3d().direct_space_state.intersect_ray(abfrage).is_empty()


## Streifenwagen und Spieler stehen einem Sichtstrahl nicht im Weg.
func _ausnahmen(auto: Verkehrsauto) -> Array[RID]:
	var liste: Array[RID] = [auto.get_rid()]
	if fokus is CollisionObject3D:
		liste.append((fokus as CollisionObject3D).get_rid())
	return liste


func _neue_streife() -> Dictionary:
	return {"auto": null, "naechste_planung": 0.0, "im_einsatz": false,
			"suchziel": Vector3.ZERO, "sucht": false, "pfad": PackedInt32Array()}


func _entsende(s: Dictionary, ziel: Vector3) -> bool:
	var auto: Verkehrsauto = s["auto"]
	# Faehrt der Wagen noch vom letzten Einsatz davon, dreht er einfach um.
	if is_instance_valid(auto) and not auto.ist_geparkt():
		s["im_einsatz"] = true
		_plane(s, ziel)
		return true
	var kandidaten := PackedInt32Array()
	for i in _knoten.size():
		var d := _abstand2(_knoten[i], ziel)
		if d >= einsatz_min_m and d <= einsatz_max_m:
			kandidaten.append(i)
	if kandidaten.is_empty():
		return false
	var k := kandidaten[randi() % kandidaten.size()]
	var ort := _knoten[k]
	# Blick auf den ersten Nachbarn, sonst faehrt der Wagen die erste Neuplanung
	# quer aus dem Stand an.
	var blick := ziel - ort
	var nachbarn := _astar.get_point_connections(k)
	if nachbarn.size() > 0:
		blick = _knoten[nachbarn[0]] - ort
	if not is_instance_valid(auto):
		auto = Verkehrsauto.new()
		auto.polizei = true
		auto.lack = STREIFE_LACK
		get_tree().current_scene.add_child(auto)
		s["auto"] = auto
	auto.setze_lage(ort, blick)
	s["im_einsatz"] = true
	s["naechste_planung"] = 0.0
	_plane(s, ziel)
	print("LALABERG_STREIFE einsatz bei %s abstand=%.0fm" % [ort, _abstand2(ort, ziel)])
	return true


func _plane(s: Dictionary, ziel: Vector3) -> void:
	var auto: Verkehrsauto = s["auto"]
	var ort := auto.route_ort()
	var vor := auto.vorwaerts()
	# Mit jedem Stern etwas schneller: 58 km/h bei einem, 90 km/h bei fuenf.
	var tempo := 50.0 + 8.0 * maxi(1, sterne)
	var route_punkte := PackedVector3Array([ort])
	var alter_pfad: PackedInt32Array = s["pfad"]
	s["pfad"] = PackedInt32Array()
	if _abstand2(ort, ziel) >= DIREKT:
		# Am Knoten weiter, den der Wagen gerade ansteuert. Nach der Blickrichtung
		# allein gewaehlt, drehte ein Wagen mitten in der Kurve bei jeder
		# Neuplanung zu einem anderen Knoten und kreiste auf der Stelle.
		var von := -1
		var ziel_index := auto.weg_index() - 1
		if ziel_index >= 0 and ziel_index < alter_pfad.size() \
				and _abstand2(_knoten[alter_pfad[ziel_index]], ort) < 60.0:
			von = alter_pfad[ziel_index]
		if von < 0:
			von = _naechster_knoten(ort + vor * 4.0, vor)
		if von < 0:
			von = _naechster_knoten(ort)
		var pfad := _pfad(von, _naechster_knoten(ziel))
		if pfad.is_empty():
			print("LALABERG_STREIFE parkt: kein Weg von=%d nach=%d" % [von, _naechster_knoten(ziel)])
			auto.parke()
			s["im_einsatz"] = false
			return
		for k in pfad:
			route_punkte.append(_knoten[k])
		s["pfad"] = pfad
	# Querfeldein nur auf den letzten 150 m; weiter weg vom Netz bleibt die Streife
	# am letzten Knoten stehen.
	if _abstand2(route_punkte[route_punkte.size() - 1], ziel) < 150.0:
		route_punkte.append(_haltepunkt(route_punkte[route_punkte.size() - 1], ziel, auto))
	auto.folge_weg(route_punkte, tempo)


## 2,5 m vor dem Ziel anhalten: wer neben der Strasse steht, soll trotzdem
## erreicht werden.
func _haltepunkt(von: Vector3, ziel: Vector3, auto: Verkehrsauto) -> Vector3:
	var richtung := Vector3(ziel.x - von.x, 0, ziel.z - von.z).normalized()
	var halt := ziel - richtung * 2.5
	var abfrage := PhysicsRayQueryParameters3D.create(
			halt + Vector3(0, 3, 0), halt - Vector3(0, 15, 0))
	abfrage.exclude = _ausnahmen(auto)
	abfrage.collision_mask = 1
	var treffer := auto.get_world_3d().direct_space_state.intersect_ray(abfrage)
	halt.y = treffer["position"].y if not treffer.is_empty() else von.y
	return halt


func _einstellen(grund: String) -> void:
	print("LALABERG_FAHNDUNG sterne=0 grund=%s" % grund)
	sterne = 0
	_punkte = 0.0
	_festnahme_uhr = 0.0
	_gesehen = false
	# Die Streifen fahren ab, irgendwohin weit weg, und verschwinden ausser Sicht.
	var wo := fokus.global_position if fokus != null else Vector3.ZERO
	for s in _streifen:
		s["im_einsatz"] = false
		var auto: Verkehrsauto = s["auto"]
		if not is_instance_valid(auto) or auto.ist_geparkt():
			continue
		var weit := -1
		for versuch in 40:
			var k := randi() % _knoten.size()
			if _abstand2(_knoten[k], wo) > 600.0:
				weit = k
				break
		var pfad := _pfad(_naechster_knoten(auto.position_jetzt(), auto.vorwaerts()), weit)
		if weit < 0 or pfad.is_empty():
			auto.parke()
			continue
		var r := PackedVector3Array([auto.route_ort()])
		for k in pfad:
			r.append(_knoten[k])
		auto.folge_weg(r, 50.0)


func _festnahme() -> void:
	festnahmen += 1
	# Laufender Auftrag ist hinfaellig, dazu eine Strafe.
	get_tree().call_group("auftraege", "abbrechen")
	var strafe := Konto.strafe(100 + Konto.geld() / 10)
	# Aus dem Wagen und zur Polizeiinspektion: dort geht es weiter.
	get_tree().call_group("welt", "aussteigen_erzwingen")
	var k := _naechster_knoten(INSPEKTION)
	if k >= 0:
		get_tree().call_group("welt", "versetze_spieler", _knoten[k])
	for s in _streifen:
		s["im_einsatz"] = false
		if is_instance_valid(s["auto"]):
			s["auto"].parke()
	sterne = 0
	_punkte = 0.0
	_festnahme_uhr = 0.0
	_gesehen = false
	Schaden.rueckmeldung(get_tree(),
			"Festgenommen – %d € Strafe. Wieder frei an der Polizeiinspektion" % strafe)
	print("LALABERG_FESTNAHME strafe=%d ort=%s" % [strafe, _knoten[k] if k >= 0 else Vector3.ZERO])


# --- Strassensperre ---------------------------------------------------------

## Knoten ein Stueck voraus in Fahrtrichtung plus die Strassenrichtung dort.
## Nicht Kante fuer Kante nach der Fahrtrichtung tasten (das scheitert an Einbahn,
## Kurve und Sackgasse), sondern derselbe A*: vom Knoten beim Spieler zu dem
## Knoten, der dort liegt, wo er in dieser Richtung hinkaeme.
func _knoten_voraus(von: Vector3, richtung: Vector3, weite: float) -> Dictionary:
	var start := _naechster_knoten(von, richtung)
	var ende := _naechster_knoten(von + richtung * weite)
	var pfad := _pfad(start, ende)
	if start < 0 or ende < 0 or pfad.size() < 2:
		return {}
	for i in range(1, pfad.size()):
		if _abstand2(_knoten[pfad[i]], von) < weite * 0.6:
			continue
		var strasse := _knoten[pfad[i]] - _knoten[pfad[i - 1]]
		strasse.y = 0.0
		return {"knoten": pfad[i], "strasse": strasse.normalized()}
	return {}


func _raeume_sperre() -> void:
	for w in _sperrwagen:
		if is_instance_valid(w):
			w.setze_sperre(false)
			w.parke()
	_sperrwagen.clear()
	for b in _baken:
		if is_instance_valid(b):
			b.queue_free()
	_baken.clear()
	sperre_steht = false


## Ein Stueck voraus stellen sich zwei Streifenwagen quer, dazwischen stehen
## rot-weisse Baken. Wer trotzdem durch die Mitte faehrt, kommt durch: es ist
## eine Sperre, keine Wand.
func _stelle_sperre(wo: Vector3, fahrt: Vector3) -> void:
	var treffer := _knoten_voraus(wo, fahrt, 280.0)
	if treffer.is_empty():
		return
	_raeume_sperre()
	var ort: Vector3 = _knoten[treffer["knoten"]]
	var strasse: Vector3 = treffer["strasse"]
	var quer := Vector3.UP.cross(strasse).normalized()
	for i in 2:
		var w := Verkehrsauto.new()
		w.polizei = true
		w.lack = STREIFE_LACK
		get_tree().current_scene.add_child(w)
		w.setze_lage(ort + quer * (-2.6 if i == 0 else 2.6), quer)
		w.setze_sperre(true)
		_sperrwagen.append(w)
	for i in 5:
		var b := MeshInstance3D.new()
		var box := BoxMesh.new()
		box.size = Vector3(0.12, 0.9, 1.0)
		b.mesh = box
		var m := StandardMaterial3D.new()
		m.albedo_color = Color(0.85, 0.85, 0.82) if i % 2 else Color(0.75, 0.06, 0.05)
		b.material_override = m
		get_tree().current_scene.add_child(b)
		b.global_position = ort + quer * (i - 2) * 1.1 + Vector3(0, 0.45, 0)
		b.rotation.y = -atan2(strasse.z, strasse.x)
		_baken.append(b)
	sperr_ort = ort
	sperre_steht = true
	_sperre_seit = _jetzt()
	Schaden.rueckmeldung(get_tree(), "Straßensperre voraus")
	print("LALABERG_SPERRE bei %s wagen=%d baken=%d" % [ort, _sperrwagen.size(), _baken.size()])


# --- Takt -------------------------------------------------------------------

func _physics_process(delta: float) -> void:
	if pausiert:
		return
	_takt += delta
	if _takt < 0.1:
		return
	var dt := _takt
	_takt = 0.0
	if fokus == null or _knoten.is_empty():
		return
	var jetzt := _jetzt()
	var wo := fokus.global_position
	# Gestohlene Streifenwagen sind weg.
	_streifen = _streifen.filter(func(s: Dictionary) -> bool: return is_instance_valid(s["auto"]))
	if sterne == 0:
		if sperre_steht:
			_raeume_sperre()
		# Nach der Fahndung verschwinden abziehende Wagen ausser Sicht.
		for s in _streifen:
			var a: Verkehrsauto = s["auto"]
			if not a.ist_geparkt() and (a.am_ziel() or _abstand2(a.position_jetzt(), wo) > 250.0):
				a.parke()
		return
	_gesehen = false
	for s in _streifen:
		if s["im_einsatz"] and _sieht(s["auto"]):
			_gesehen = true
			break
	if _gesehen:
		_letzter_ort = wo
		_ungesehen = 0.0
	else:
		var im_gebiet := _abstand2(wo, _letzter_ort) < _suchradius()
		_ungesehen += dt * (1.0 / 3.0 if im_gebiet else 1.0)
		if _ungesehen > _suchdauer():
			_einstellen("abgehaengt")
			Schaden.rueckmeldung(get_tree(), "Abgehängt – die Fahndung ist eingestellt")
			return
	_pflege_sperre(wo, jetzt)
	_pflege_streifen(wo, jetzt)
	_pruefe_festnahme(wo, dt)


## Ab drei Sternen wird der Weg gesperrt. Eine neue Sperre erst, wenn die alte
## passiert ist (naeher als 60 m) oder eine Minute ungenutzt stand.
func _pflege_sperre(wo: Vector3, jetzt: float) -> void:
	if sterne < 3:
		if sperre_steht:
			_raeume_sperre()
		return
	var tempo: Vector3 = fokus.geschwindigkeit_ms()
	var fahrt := Vector3(tempo.x, 0, tempo.z)
	if fahrt.length_squared() > 4.0:
		fahrt = fahrt.normalized()
	else:
		fahrt = Vector3(-fokus.global_basis.z.x, 0, -fokus.global_basis.z.z).normalized()
	var verbraucht := sperre_steht and (
			_abstand2(wo, sperr_ort) < 60.0 or jetzt - _sperre_seit > 60.0)
	if not sperre_steht or verbraucht:
		_stelle_sperre(wo, fahrt)


## Je Stern eine Streife; neue kommen nicht alle auf einmal.
func _pflege_streifen(wo: Vector3, jetzt: float) -> void:
	var aktiv := 0
	for s in _streifen:
		if s["im_einsatz"]:
			aktiv += 1
	if aktiv < sterne and jetzt >= _naechster_einsatz:
		var frei := {}
		for s in _streifen:
			if not s["im_einsatz"]:
				frei = s
				break
		if frei.is_empty():
			frei = _neue_streife()
			_streifen.append(frei)
		_entsende(frei, wo if _gesehen else _letzter_ort)
		_naechster_einsatz = jetzt + 3.0
	for s in _streifen:
		if not s["im_einsatz"]:
			continue
		var a: Verkehrsauto = s["auto"]
		if _abstand2(a.position_jetzt(), wo) > VERLOREN:
			print("LALABERG_STREIFE parkt: verloren bei %s, spieler %s" % [a.position_jetzt(), wo])
			a.parke()
			s["im_einsatz"] = false
			continue
		if jetzt < s["naechste_planung"]:
			continue
		s["naechste_planung"] = jetzt + 1.5
		if _gesehen:
			s["sucht"] = false
			_plane(s, wo)
			continue
		_suche(s, a)


## Ungesehen: erst zum letzten bekannten Ort, dort dann von Knoten zu Knoten im
## Suchgebiet.
func _suche(s: Dictionary, a: Verkehrsauto) -> void:
	var hier := a.position_jetzt()
	if not s["sucht"] and _abstand2(hier, _letzter_ort) > 40.0:
		_plane(s, _letzter_ort)
		return
	if not s["sucht"] or _abstand2(hier, s["suchziel"]) < 15.0 or a.am_ziel():
		for versuch in 30:
			var k := _knoten[randi() % _knoten.size()]
			if _abstand2(k, _letzter_ort) < _suchradius() and _abstand2(k, hier) > 50.0:
				s["suchziel"] = k
				s["sucht"] = true
				break
		if not s["sucht"]:
			s["suchziel"] = _letzter_ort
		s["sucht"] = true
	_plane(s, s["suchziel"])


## Festnahme: neben einem Streifenwagen stehen bleiben (zu Fuss wie im Wagen).
## Wer wieder losfaehrt oder -rennt, baut den Balken ab.
func _pruefe_festnahme(wo: Vector3, dt: float) -> void:
	var naechste := INF
	for s in _streifen:
		if s["im_einsatz"]:
			naechste = minf(naechste, _abstand2(s["auto"].position_jetzt(), wo))
	var grenze := 2.5 if fokus is Spieler else 3.0
	if naechste < FESTNAHME_ABSTAND and fokus.geschwindigkeit_ms().length() < grenze:
		_festnahme_uhr += dt
		if _festnahme_uhr >= FESTNAHME_DAUER:
			_festnahme()
	else:
		_festnahme_uhr = maxf(0.0, _festnahme_uhr - dt * 2.0)
