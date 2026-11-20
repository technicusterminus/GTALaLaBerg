class_name Farbdosen
extends Node3D
## Fuenfzig versteckte Farbdosen im Strassennetz der Stadt. Wer eine einsammelt (zu Fuss
## oder im Wagen), bekommt Geld; jede zehnte bringt einen Staffelpreis obendrauf. Welche
## schon eingesammelt sind, merkt sich das Konto.

const ANZAHL := 50
const MIN_ABSTAND := 90.0
const PREIS := 25
const STAFFEL := {10: 150, 20: 300, 30: 450, 40: 600, 50: 1000}
const FARBEN := [
	Color(0.95, 0.25, 0.20), Color(0.98, 0.70, 0.10), Color(0.25, 0.75, 0.35),
	Color(0.20, 0.55, 0.95), Color(0.80, 0.30, 0.85), Color(0.15, 0.85, 0.85),
]

var fokus: Node3D
var polizei: Polizei
var gestellt := 0

var _orte: Array[Vector3] = []
var _knoten: Array[Node3D] = []
var _takt := 0.0
var _zeit := 0.0


func _ready() -> void:
	add_to_group("farbdosen")
	if polizei == null:
		return
	_waehle_orte(polizei.netz_knoten())
	var koerper := CylinderMesh.new()
	koerper.top_radius = 0.16
	koerper.bottom_radius = 0.16
	koerper.height = 0.42
	var deckel := CylinderMesh.new()
	deckel.top_radius = 0.07
	deckel.bottom_radius = 0.14
	deckel.height = 0.1
	for i in _orte.size():
		var wurzel := Node3D.new()
		var m := StandardMaterial3D.new()
		m.albedo_color = FARBEN[i % FARBEN.size()]
		m.emission_enabled = true
		m.emission = m.albedo_color
		m.emission_energy_multiplier = 0.6
		var dose := MeshInstance3D.new()
		dose.mesh = koerper
		dose.material_override = m
		dose.visibility_range_end = 400.0
		wurzel.add_child(dose)
		var kopf := MeshInstance3D.new()
		kopf.mesh = deckel
		kopf.position.y = 0.26
		var grau := StandardMaterial3D.new()
		grau.albedo_color = Color(0.7, 0.7, 0.72)
		kopf.material_override = grau
		kopf.visibility_range_end = 400.0
		wurzel.add_child(kopf)
		wurzel.visible = not Konto.hat_dose(i)
		add_child(wurzel)
		wurzel.global_position = _orte[i] + Vector3(0, 1.0, 0)
		_knoten.append(wurzel)
	gestellt = _orte.size()
	print("LALABERG_DOSEN gestellt=%d schon=%d" % [gestellt, Konto.dosen()])


## Verteilt die Dosen mit festem Zufall (jeder Spielstand sieht dieselbe Karte) so,
## dass keine zwei naeher als MIN_ABSTAND beieinander liegen.
func _waehle_orte(knoten: PackedVector3Array) -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 4711
	var versuche := 0
	while _orte.size() < ANZAHL and versuche < 20000 and knoten.size() > 0:
		versuche += 1
		var k := knoten[rng.randi() % knoten.size()]
		var frei := true
		for o in _orte:
			if Vector2(o.x - k.x, o.z - k.z).length() < MIN_ABSTAND:
				frei = false
				break
		if frei:
			_orte.append(k)


func ort(nummer: int) -> Vector3:
	return _orte[nummer]


## Sammelt Dose nummer ein; false, wenn es sie nicht gibt oder sie schon fehlt.
func sammle(nummer: int) -> bool:
	if nummer < 0 or nummer >= _knoten.size() or Konto.hat_dose(nummer):
		return false
	Konto.nimm_dose(nummer)
	_knoten[nummer].visible = false
	var summe := PREIS
	var n := Konto.dosen()
	var text := "Farbdose %d/%d  +%d €" % [n, ANZAHL, PREIS]
	if STAFFEL.has(n):
		summe += STAFFEL[n]
		text = "Farbdose %d/%d  +%d € Staffelpreis!" % [n, ANZAHL, summe]
	Konto.gutschrift(summe)
	Schaden.rueckmeldung(get_tree(), text)
	print("LALABERG_DOSE nummer=%d gesammelt=%d summe=%d" % [nummer, n, summe])
	return true


func _process(delta: float) -> void:
	_zeit += delta
	for i in _knoten.size():
		var d := _knoten[i]
		if d.visible:
			d.rotation.y = _zeit * 1.6 + i
			d.position.y = _orte[i].y + 1.0 + 0.12 * sin(_zeit * 2.0 + i)
	_takt -= delta
	if _takt > 0.0 or fokus == null:
		return
	_takt = 0.2
	for i in _knoten.size():
		if not _knoten[i].visible:
			continue
		var ab := fokus.global_position - _orte[i]
		if Vector2(ab.x, ab.z).length() < 2.4 and absf(ab.y) < 3.5:
			sammle(i)
