class_name Verkehrsbremse
extends RefCounted
## Wann ein KI-Auto bremst. Jeder Faktor liegt zwischen 0 (Stillstand) und 1 (freie
## Fahrt); das Auto nimmt den kleinsten. Keine echte Verkehrssimulation, sondern
## kontinuierliches Vorrang-Bremsen: vor einem Auto voraus, vor Rot und Gelb, in
## der Kurve, vor dem Spieler und an Kreuzungen vor Wartepflichtigen.

const ABSTAND_AUTO := 9.0     ## so nah voraus bremst ein Wagen fuer den naechsten
const KURVEN_SICHT := 25.0    ## so weit voraus zaehlt die Richtungsaenderung


## Bremst, wenn ein anderer Verkehrswagen naeher als 9 m voraus steht; sonst
## faehrt jeder stur durch jeden hindurch.
static func vor_auto(auto: Verkehrsauto) -> float:
	var vor := auto.vorwaerts()
	var ort := auto.position_jetzt()
	var beste := ABSTAND_AUTO
	for a: Verkehrsauto in auto.get_tree().get_nodes_in_group("verkehrsauto"):
		if a == auto or a.geparkt:
			continue
		var diff := a.position_jetzt() - ort
		var dist := diff.length()
		if dist > ABSTAND_AUTO or dist < 0.01:
			continue
		if diff.dot(vor) / dist < 0.5:
			continue  # nicht voraus
		beste = minf(beste, dist)
	if beste >= ABSTAND_AUTO:
		return 1.0
	return clampf((beste - 2.6) / 6.4, 0.05, 1.0)


## Vor einer roten oder gelben Ampel bis zum Stillstand abbremsen. Keine
## Zuordnung zu einer Kreuzung: die naechste Ampel auf dem Fahrweg zaehlt.
static func vor_ampel(auto: Verkehrsauto) -> float:
	var vor := auto.vorwaerts()
	var ort := auto.position_jetzt()
	var bremse := 1.0
	for a: Ampel in auto.get_tree().get_nodes_in_group("ampel"):
		if not a.haelt_an():
			continue
		var diff := a.global_position - ort
		var dist := diff.length()
		if dist > 14.0 or dist < 0.01:
			continue
		if diff.dot(vor) / dist < 0.7:
			continue  # nicht voraus auf der Strecke
		bremse = minf(bremse, clampf((dist - 3.5) / 9.0, 0.0, 1.0))
	return bremse


## Vor einer Kurve langsamer werden: die Bewegung ist kinematisch, es gibt keine
## Fliehkraft, die von selbst bremste. Summiert die Richtungsaenderung der
## naechsten rund 25 m Route auf; eine volle Abbiegung (90 Grad) bremst auf 45
## Prozent.
static func in_kurve(weg: Wegfolger) -> float:
	if not weg.gueltig():
		return 1.0
	var anzahl := weg.route.size()
	var strecke := 0.0
	var summe := 0.0
	var letzte := Vector3.ZERO
	var i := weg.index
	# Schrittzahl begrenzt: auf einem Rundkurs liefe die Vorausschau sonst endlos
	# im Kreis, wenn der Ring kuerzer als die Vorausschau ist.
	for schritt in anzahl:
		if strecke >= KURVEN_SICHT:
			break
		var naechster := i + weg.richtung
		if weg.rund:
			naechster = (naechster + anzahl) % anzahl
		if i < 0 or i >= anzahl or naechster < 0 or naechster >= anzahl:
			break
		var segment := weg.route[naechster] - weg.route[i]
		i = naechster
		var laenge := Vector2(segment.x, segment.z).length()
		if laenge < 0.01:
			continue
		var richtung := segment / laenge
		if letzte != Vector3.ZERO:
			summe += rad_to_deg(acos(clampf(letzte.dot(richtung), -1.0, 1.0)))
		letzte = richtung
		strecke += laenge
	return clampf(1.0 - summe / 164.0, 0.45, 1.0)


## Bremst vor dem Spieler wie vor einem anderen Verkehrswagen, zu Fuss wie im Wagen.
static func vor_spieler(auto: Verkehrsauto) -> float:
	var fokus := Verkehrsauto.fokus
	if fokus == null:
		return 1.0
	var diff := fokus.global_position - auto.position_jetzt()
	var dist := diff.length()
	if dist > ABSTAND_AUTO or dist < 0.01:
		return 1.0
	if diff.dot(auto.vorwaerts()) / dist < 0.5:
		return 1.0
	return clampf((dist - 2.6) / 6.4, 0.05, 1.0)


## Vorfahrt an den Kreuzungen der eigenen Route: ein Auto bremst bis zum
## Stillstand, wenn ein anderes an derselben Kreuzung von der wichtigeren Strasse
## (niedrigere Klasse) naht. Bei gleicher Klasse zaehlt der Abstand zur Kreuzung
## als Naeherung fuer die Ankunft (wer naeher dran ist, war zuerst da); nur bei
## echtem Gleichstand (unter 3 m Unterschied) entscheidet Rechts-vor-Links.
static func vor_kreuzung(auto: Verkehrsauto) -> float:
	var bremse := 1.0
	var ort := auto.position_jetzt()
	var ort2 := Vector2(ort.x, ort.z)
	for k in auto.kreuzungen:
		if k >= Verkehrsauto.kreuzung_orte.size():
			continue
		var kreuz := Verkehrsauto.kreuzung_orte[k]
		var eigener := ort2.distance_to(kreuz)
		# Anhalteabstand nach der Kreuzungsbreite: halbe Breite bis zum eigenen
		# Fahrbahnrand plus die halbe Wagenlaenge (rund 2,1 m).
		var puffer := Verkehrsauto.kreuzung_breiten[k] * 0.5 + 2.1
		var relevanz := puffer + 11.0
		if eigener > relevanz:
			continue
		for andere: Verkehrsauto in Verkehrsauto.kreuzung_autos.get(k, []):
			if andere == auto or not is_instance_valid(andere) or andere.geparkt \
					or not andere.weg.gueltig():
				continue
			var ao := andere.position_jetzt()
			var andere2 := Vector2(ao.x, ao.z)
			var anderer := andere2.distance_to(kreuz)
			if anderer > relevanz:
				continue
			var meine_klasse := auto.klasse_jetzt()
			var klasse := andere.klasse_jetzt()
			var muss := klasse < meine_klasse
			if not muss and klasse == meine_klasse:
				if anderer < eigener - 3.0:
					muss = true
				elif absf(anderer - eigener) <= 3.0:
					muss = auto.rechts2().dot((andere2 - ort2).normalized()) > 0.3
			if muss:
				bremse = minf(bremse, clampf((eigener - puffer) / 11.0, 0.0, 1.0))
	return bremse
