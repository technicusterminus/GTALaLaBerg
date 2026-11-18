class_name Ampel
extends Node3D
## Eine Lichtsignalanlage an einem echten Standort (verkehr.json, aus OSM
## highway=traffic_signals). Wechselt selbststaendig Rot, Gruen, Gelb; KI-Autos
## fragen haelt_an() ab und bremsen vor Rot und Gelb. Nahe beieinander liegende
## Ampeln sind zu Kreuzungen gruppiert und in so viele Phasen geteilt, wie die
## Kreuzung Arme hat, damit sich kreuzende Strassen nie gleichzeitig Gruen
## zeigen. Der Zustand wird aus der Uhrzeit berechnet statt gezaehlt: alle
## Ampeln einer Kreuzung bleiben so im Takt, ohne einander zu kennen.

enum Zustand { ROT, GRUEN, GELB }

const GRUEN_S := 6.0
const GELB_S := 1.6
## Ein Zeitfenster genau einer Phase; der Zyklus ist so viele Fenster lang, wie
## die Kreuzung Phasen hat.
const PHASENFENSTER_S := GRUEN_S + GELB_S

const DUNKEL := Color(0.12, 0.12, 0.13)
const FARBEN := [Color(0.95, 0.08, 0.05), Color(0.10, 0.85, 0.20), Color(0.95, 0.75, 0.05)]

var gruppe := 0
var phase := 0
var anzahl_phasen := 2

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
	if rel < GRUEN_S:
		return Zustand.GRUEN
	if rel < PHASENFENSTER_S:
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
		var an := i == _zustand
		var f: Color = FARBEN[i]
		m.albedo_color = f if an else DUNKEL
		m.emission_enabled = an
		m.emission = f * 2.0
	_linsen_gesetzt = true


## Mast, Kasten und drei Linsen; die aktive Linse leuchtet.
func _baue_kopf() -> void:
	var mast := MeshInstance3D.new()
	var zyl := CylinderMesh.new()
	zyl.top_radius = 0.06
	zyl.bottom_radius = 0.06
	zyl.height = 3.0
	mast.mesh = zyl
	mast.position.y = 1.5
	var grau := StandardMaterial3D.new()
	grau.albedo_color = Color(0.30, 0.30, 0.32)
	mast.material_override = grau
	add_child(mast)
	var kasten := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = Vector3(0.18, 0.64, 0.18)
	kasten.mesh = box
	kasten.position.y = 3.3
	var dunkel := StandardMaterial3D.new()
	dunkel.albedo_color = Color(0.08, 0.08, 0.09)
	kasten.material_override = dunkel
	add_child(kasten)
	# Das Modell schaut nach +X wie die Autos; die Linsen sitzen auf dieser Seite.
	for i in 3:
		var linse := MeshInstance3D.new()
		var lb := BoxMesh.new()
		lb.size = Vector3(0.012, 0.12, 0.12)
		linse.mesh = lb
		linse.position = Vector3(0.092, [3.52, 3.30, 3.08][i], 0)
		linse.material_override = StandardMaterial3D.new()
		linse.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		add_child(linse)
		_linsen.append(linse)
