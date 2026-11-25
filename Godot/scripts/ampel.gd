class_name Ampel
extends Node3D
## Eine Lichtsignalanlage an einem echten Standort (verkehr.json, aus OSM
## highway=traffic_signals). Wechselt selbststaendig Rot, Gruen, Gelb; KI-Autos
## fragen haelt_an() ab und bremsen vor Rot und Gelb. Nahe beieinander liegende
## Ampeln sind zu Kreuzungen gruppiert und in so viele Phasen geteilt, wie die
## Kreuzung Arme hat, damit sich kreuzende Strassen nie gleichzeitig Gruen
## zeigen. Der Zustand wird aus der Uhrzeit berechnet statt gezaehlt: alle
## Ampeln einer Kreuzung bleiben so im Takt, ohne einander zu kennen.

enum Zustand { ROT, GRUEN, GELB, ROT_GELB }

const ROT_GELB_S := 0.8
const GRUEN_S := 10.0
const GELB_S := 2.0
const ALLE_ROT_S := 1.1
## Ein Zeitfenster genau einer Phase; der Zyklus ist so viele Fenster lang, wie
## die Kreuzung Phasen hat.
const PHASENFENSTER_S := ROT_GELB_S + GRUEN_S + GELB_S + ALLE_ROT_S

const DUNKEL := Color(0.12, 0.12, 0.13)
const FARBEN := [Color(0.95, 0.08, 0.05), Color(0.10, 0.85, 0.20), Color(0.95, 0.75, 0.05)]
const LINSEN_HOEHE := [3.59, 3.13, 3.36] # Rot oben, Gelb mittig, Grün unten.

var gruppe := 0
var phase := 0
var anzahl_phasen := 2
var mast_versatz := Vector2(4.25, 0.0)

var _zustand := Zustand.ROT
var _erzwungen := -1       ## Rauchtest: fester Zustand, -1 = nach der Uhr
var _linsen: Array[MeshInstance3D] = []
var _gruppenversatz := 0.0
var _linsen_gesetzt := false


## Zustand und Restzeit zu einem Zeitpunkt t innerhalb des Zyklus einer Kreuzung
## mit "phasen" Phasen, aus Sicht der Phase "meine". Jede Phase hat ihr eigenes,
## nicht ueberlappendes Zeitfenster (meine * PHASENFENSTER_S lang beginnend), so
## zeigt nie mehr als eine Phase gleichzeitig Gruen.
static func zustand_bei(t: float, meine: int, phasen: int) -> Zustand:
	var zyklus := PHASENFENSTER_S * phasen
	var rel := fposmod(fposmod(t, zyklus) - meine * PHASENFENSTER_S, zyklus)
	if rel < ROT_GELB_S:
		return Zustand.ROT_GELB
	if rel < ROT_GELB_S + GRUEN_S:
		return Zustand.GRUEN
	if rel < PHASENFENSTER_S - ALLE_ROT_S:
		return Zustand.GELB
	return Zustand.ROT


func _ready() -> void:
	add_to_group("ampel")
	anzahl_phasen = maxi(1, anzahl_phasen)
	# Kreuzungen schalten nicht alle im Gleichtakt: ein fester, aus der Gruppe
	# abgeleiteter Versatz verteilt sie ueber die Zykluszeit.
	_gruppenversatz = fmod(gruppe * 7.31, PHASENFENSTER_S * anzahl_phasen)
	_baue_kopf()
	_aktualisiere()


## Fuer KI-Autos: steht die Ampel gerade auf Rot oder Gelb, also: anhalten?
func haelt_an() -> bool:
	return _zustand != Zustand.GRUEN


func test_erzwinge(neu: int) -> void:
	_erzwungen = neu
	_aktualisiere()


func _process(_delta: float) -> void:
	_aktualisiere()


func _aktualisiere() -> void:
	var neu: Zustand
	if _erzwungen >= 0:
		neu = _erzwungen as Zustand
	else:
		neu = zustand_bei(Time.get_ticks_msec() / 1000.0 + _gruppenversatz, phase, anzahl_phasen)
	if neu == _zustand and _linsen_gesetzt:
		return
	_zustand = neu
	_setze_linsen()


func _setze_linsen() -> void:
	for i in 3:
		var m := _linsen[i].material_override as StandardMaterial3D
		var an := i == _zustand or (_zustand == Zustand.ROT_GELB and i != Zustand.GRUEN)
		var f: Color = FARBEN[i]
		m.albedo_color = f if an else DUNKEL.lerp(f, 0.08)
		m.emission_enabled = an
		m.emission = f * 2.0
	_linsen_gesetzt = true


## OSM liefert den Signalknoten auf der Fahrbahn. Der sichtbare Mast steht
## seitlich davon, während die Bremslogik weiter den OSM-Knoten verwendet.
func _baue_kopf() -> void:
	var mast := Node3D.new()
	mast.position = Vector3(mast_versatz.x, mast_versatz.y, 0)
	add_child(mast)
	var grau := StandardMaterial3D.new()
	grau.albedo_color = Color("#646c6b")
	grau.metallic = 0.45
	_modell(mast, _zylinder(0.062, 3.05), grau, Vector3(0, 1.51, 0))
	_modell(mast, _zylinder(0.115, 0.12), grau, Vector3(0, 0.035, 0))

	var kopf := Node3D.new()
	kopf.position.x = -signf(mast_versatz.x) * 0.11
	kopf.rotation.y = signf(mast_versatz.x) * PI * 0.5
	mast.add_child(kopf)
	var dunkel := StandardMaterial3D.new()
	dunkel.albedo_color = Color("#242926")
	dunkel.roughness = 0.82
	var box := BoxMesh.new()
	box.size = Vector3(0.22, 0.77, 0.29)
	_modell(kopf, box, dunkel, Vector3(0, 3.36, 0))
	for i in 3:
		var y: float = LINSEN_HOEHE[i]
		var rahmen := _modell(kopf, _zylinder(0.106, 0.078), dunkel, Vector3(0.12, y, 0))
		rahmen.rotation.z = PI * 0.5
		var blende := BoxMesh.new()
		blende.size = Vector3(0.19, 0.029, 0.215)
		_modell(kopf, blende, dunkel, Vector3(0.18, y + 0.107, 0))
		var linse := _modell(kopf, _zylinder(0.088, 0.015), StandardMaterial3D.new(),
				Vector3(0.163, y, 0))
		linse.rotation.z = PI * 0.5
		linse.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		_linsen.append(linse)


func _zylinder(radius: float, hoehe: float) -> CylinderMesh:
	var netz := CylinderMesh.new()
	netz.top_radius = radius
	netz.bottom_radius = radius
	netz.height = hoehe
	netz.radial_segments = 12
	netz.rings = 0
	return netz


func _modell(eltern: Node3D, mesh: Mesh, material: Material, ort: Vector3) -> MeshInstance3D:
	var instanz := MeshInstance3D.new()
	instanz.mesh = mesh
	instanz.material_override = material
	instanz.position = ort
	instanz.visibility_range_end = 320.0
	eltern.add_child(instanz)
	return instanz
