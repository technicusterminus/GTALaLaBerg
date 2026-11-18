class_name Schaden
extends RefCounted
## Woher der Schaden kommt. Die Art entscheidet nicht ueber die Hoehe (die steht
## im Aufruf), sondern ueber die Reaktion: ein Anprall wirft die Figur um, ein
## Treffer der Panzerkanone laesst nichts mehr stehen.
##
## Wer Lebenspunkte hat (Spieler, Passanten, Fahrzeuge), steht in der Gruppe
## "verletzbar" und kann:
##   verletze(schaden: float, richtung: Vector3, art: Schaden.Art)
##   ist_ausgeschaltet() -> bool
##   lebensanteil() -> float        # 0 bis 1
## Wer auf Paintball-Treffer reagiert, hat zusaetzlich
##   erhalte_farbe(farbe: Color, richtung: Vector3)
## Alles andere in der Stadt ist Geometrie und bleibt heil.

enum Art { ANPRALL, BESCHUSS, SPRENGUNG, STURZ }

## Was die Polizei auf den Plan ruft.
enum Tat { PASSANT_BESCHOSSEN, PASSANT_ANGEFAHREN, AUTO_GESTOHLEN, POLIZEI_BESCHOSSEN }


## Meldet eine Tat; ohne Polizei im Spiel passiert nichts.
static func melde(baum: SceneTree, tat: Tat) -> void:
	baum.call_group("polizei", "melde", tat)


## Kurze Meldung unten im Bild; ohne HUD passiert nichts.
static func rueckmeldung(baum: SceneTree, text: String) -> void:
	baum.call_group("hud", "zeige_rueckmeldung", text)
