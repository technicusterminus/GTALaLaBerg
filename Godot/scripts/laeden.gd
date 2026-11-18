class_name Laeden
extends Node
## Wofuer das Geld aus den Auftraegen gut ist: ein Paintball-Laden (zu Fuss
## hineingehen, Waffen kaufen) und eine Lackiererei (mit dem Wagen hineinfahren,
## neu lackieren; wer dabei ungesehen ist, ist auch die Fahndung los). Gruene
## Marken. Im Laden waehlen Pfeil hoch und runter, Enter kauft, hinausgehen
## schliesst.

enum Ladenart { PAINTBALL, LACKIEREREI }

const RADIUS := 7.0
const HOEHE_SPIEL := 5.0
const GRUEN := Color(0.1, 0.85, 0.35)

var fokus: Node3D
var offen := -1
var auswahl := 0

var _laeden: Array[Dictionary] = []
var _erst_hinaus := false


func _ready() -> void:
	add_to_group("laeden")
	# Die Waffen sind Paintball-Waffen: der Laden also ein Paintball-Laden, kein
	# Waffengeschaeft.
	_laeden.append({"name": "Paintball-Laden", "im_wagen": false, "ort": Vector3.ZERO,
			"aufgestellt": false, "marke": null, "waren": [
		{"name": "Paintball-MP", "preis": 300, "art": "waffe", "waffe": 1},
		{"name": "Paintball-Schrotflinte", "preis": 600, "art": "waffe", "waffe": 2},
		{"name": "Paintball-Werfer", "preis": 1500, "art": "waffe", "waffe": 3},
	]})
	var lacke := [
		["Tiefschwarz", Color(0.02, 0.02, 0.025)], ["Alpinweiss", Color(0.85, 0.86, 0.86)],
		["Signalrot", Color(0.6, 0.03, 0.03)], ["Enzianblau", Color(0.03, 0.1, 0.5)],
		["Tannengrün", Color(0.03, 0.18, 0.07)], ["Rapsgelb", Color(0.8, 0.6, 0.03)],
	]
	var waren: Array = []
	for l in lacke:
		waren.append({"name": l[0], "preis": 150, "art": "lack", "farbe": l[1]})
	_laeden.append({"name": "Lackiererei", "im_wagen": true, "ort": Vector3.ZERO,
			"aufgestellt": false, "marke": null, "waren": waren})


## Von der Welt gerufen, sobald die Stadt steht.
func stelle(laden: int, ort: Vector3) -> void:
	var l := _laeden[laden]
	l["ort"] = ort
	l["aufgestellt"] = true
	# Niedriger als die Auftragssaeulen (20 m statt 60 m): ein Laden ist kein Ziel,
	# das man ueber die Stadt hinweg suchen muss. Am Rand des Rings, nicht in der
	# Mitte: im Laden steht man ja drin, und die Kamera steckte sonst in der Saeule.
	var marke := Saeule.neu(GRUEN, RADIUS * 2.0, 20.0, 1.2, Vector3(0, 0, RADIUS + 1.5))
	get_tree().current_scene.add_child(marke)
	marke.global_position = ort
	l["marke"] = marke
	print("LALABERG_LADEN %s bei %s" % [l["name"], ort])


func ort_von(laden: int) -> Vector3:
	return _laeden[laden]["ort"]


## Schon gekauft (Waffe) bzw. gerade der eigene Lack.
func hat_schon(ware: Dictionary) -> bool:
	if ware["art"] == "waffe":
		return Konto.hat_waffe(ware["waffe"])
	var f: Color = ware["farbe"]
	return Konto.hat_lack() and Konto.lack().is_equal_approx(f)


## Kauft die Ware im offenen Laden.
func kaufe(index: int) -> bool:
	if offen < 0 or index < 0 or index >= _laeden[offen]["waren"].size():
		return false
	var ware: Dictionary = _laeden[offen]["waren"][index]
	# Werkstattschluessel: die Lackiererei arbeitet umsonst.
	var umsonst: bool = ware["art"] == "lack" and Konto.hat_fund(Konto.Fund.WERKSTATT)
	if ware["art"] == "waffe" and hat_schon(ware):
		_melde("%s hast du schon" % ware["name"])
		return false
	if not umsonst and not Konto.bezahle(ware["preis"]):
		_melde("Zu wenig Geld – %s kostet %d €, du hast %d €" % [
				ware["name"], ware["preis"], Konto.geld()])
		return false
	if ware["art"] == "waffe":
		Konto.gib_waffe(ware["waffe"])
		_melde("%s gekauft – Taste %d" % [ware["name"], int(ware["waffe"]) + 1])
	else:
		var farbe: Color = ware["farbe"]
		if fokus is Auto:
			(fokus as Auto).setze_lack(farbe)
		Konto.setze_lack(farbe)
		_nach_lackierung(ware["name"])
	print("LALABERG_KAUF %s preis=%d geld=%d" % [ware["name"], ware["preis"], Konto.geld()])
	return true


## Neu lackiert erkennt einen die Polizei nicht wieder, wenn sie gerade nicht
## zusieht.
func _nach_lackierung(name: String) -> void:
	var polizei := get_tree().get_first_node_in_group("polizei") as Polizei
	if polizei != null and polizei.hole_sterne() > 0:
		if polizei.wird_gesehen():
			_melde("Neu lackiert in %s – aber die Polizei hat zugesehen" % name)
		else:
			polizei.verwische()
			_melde("Neu lackiert in %s – die Polizei sucht jetzt einen anderen Wagen" % name)
	else:
		_melde("Neu lackiert in %s" % name)


func _melde(text: String) -> void:
	Schaden.rueckmeldung(get_tree(), text)


## Die Speisekarte fuers HUD; leer, wenn man in keinem Laden steht.
func menue_text() -> String:
	if offen < 0:
		return ""
	var zeilen: Array[String] = [_laeden[offen]["name"].to_upper()]
	var waren: Array = _laeden[offen]["waren"]
	for i in waren.size():
		var w: Dictionary = waren[i]
		var preis := "%d €" % w["preis"]
		if w["art"] == "lack" and Konto.hat_fund(Konto.Fund.WERKSTATT):
			preis = "umsonst"
		var zusatz := "  (hast du)" if hat_schon(w) else ""
		zeilen.append("%s %s   %s%s" % ["▶" if i == auswahl else "  ", w["name"], preis, zusatz])
	zeilen.append("↑ ↓ wählen   Enter kaufen   hinausgehen schließt")
	return "\n".join(zeilen)


func _unhandled_input(event: InputEvent) -> void:
	if offen < 0 or not (event is InputEventKey) or not event.pressed or event.echo:
		return
	var anzahl: int = _laeden[offen]["waren"].size()
	match (event as InputEventKey).keycode:
		KEY_UP:
			auswahl = (auswahl + anzahl - 1) % anzahl
		KEY_DOWN:
			auswahl = (auswahl + 1) % anzahl
		KEY_ENTER, KEY_KP_ENTER:
			kaufe(auswahl)


func _physics_process(_delta: float) -> void:
	if fokus == null:
		return
	var wo := fokus.global_position
	var drin := -1
	for i in _laeden.size():
		var l := _laeden[i]
		var d := Vector2(wo.x - l["ort"].x, wo.z - l["ort"].z).length()
		if l["aufgestellt"] and d < RADIUS and absf(wo.y - l["ort"].y) < HOEHE_SPIEL:
			drin = i
	if drin < 0:
		offen = -1
		_erst_hinaus = false
		return
	if offen < 0:
		if _erst_hinaus:
			return
		# Die Lackiererei nur mit dem Wagen, den Laden nur zu Fuss.
		if _laeden[drin]["im_wagen"] != (fokus is Auto):
			_melde("In die Lackiererei mit dem Wagen hineinfahren"
					if _laeden[drin]["im_wagen"] else "Den Paintball-Laden zu Fuß betreten")
			_erst_hinaus = true
			return
		offen = drin
		auswahl = 0
		print("LALABERG_LADEN offen %s" % _laeden[offen]["name"])
