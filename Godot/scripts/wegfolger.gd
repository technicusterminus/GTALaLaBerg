class_name Wegfolger
extends RefCounted
## Eine Route abfahren: vorwaerts bis zum letzten Wegpunkt, dann entweder
## rueckwaerts zurueck oder, auf einem Rundkurs, wieder vorn beginnen. So
## springt am Ende nichts zu einem neuen Startpunkt. Geteilt zwischen
## Verkehrswagen und Passanten.

var route := PackedVector3Array()
var ort := Vector3.ZERO
var index := 0
var richtung := 1
var rund := false
var wenden := 0       ## wie oft umgekehrt wurde; auf einem Rundkurs bleibt es 0
var blick := Vector3.ZERO  ## letzte waagerechte Bewegungsrichtung, Null wenn nichts bewegt


func gueltig() -> bool:
	return route.size() >= 2


func setze(neue_route: PackedVector3Array, ist_rund: bool) -> void:
	route = neue_route
	rund = ist_rund
	index = 0
	richtung = 1
	wenden = 0
	if gueltig():
		ort = route[0]


## Bewegt den Ort bis zu "strecke" Meter entlang der Route.
func bewege(strecke: float) -> void:
	blick = Vector3.ZERO
	if not gueltig():
		return
	var letzte := Vector3.ZERO
	# Mehrere Wegpunkte je Bild sind moeglich; die Obergrenze verhindert eine
	# Endlosschleife bei einer Route aus zwei identischen Punkten.
	for versuch in 8:
		if strecke <= 0.0001:
			break
		index = clampi(index, 0, route.size() - 1)
		var ziel := route[index]
		var delta := ziel - ort
		var abstand := delta.length()
		if abstand < 0.01:
			if index + richtung < 0 or index + richtung >= route.size():
				if rund:
					index = (index + richtung + route.size()) % route.size()
				else:
					richtung = -richtung
					wenden += 1
			else:
				index += richtung
			continue
		letzte = delta / abstand
		if abstand <= strecke:
			ort = ziel
			strecke -= abstand
		else:
			ort += letzte * strecke
			strecke = 0.0
	letzte.y = 0.0
	if letzte.length_squared() > 0.0001:
		blick = letzte.normalized()
