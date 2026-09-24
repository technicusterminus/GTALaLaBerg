# Der Farbkrieg um Landsberg

Entwurf der Handlung. Mischung aus dem, was *Zelda* gut kann (eine Welt, die
sich in Abschnitten oeffnet; jeder Abschnitt endet mit einem Gegner, der ein
Werkzeug hinterlaesst, das den naechsten Abschnitt erst moeglich macht) und
dem, was *GTA San Andreas* gut kann (Reviere, Ruf, Werte, die durch Benutzung
wachsen, und eine Stadt, die auf beides reagiert).

Alles hier laesst sich mit dem bauen, was das Spiel schon hat: Farbkleckse,
Auftragsarten, Fahndung, Laeden, Spielstand, Charakterwerte. Es braucht keine
Dialoge, keine Zwischensequenzen und keine Figuren mit Gesichtern - die
Geschichte wird ueber Orte, Farben und Aufgaben erzaehlt.

## Ausgangslage

Landsberg ist unter vier Mannschaften aufgeteilt, seit der letzte Farbkrieg
unentschieden ausging. Jede haelt ein Revier und faerbt es in ihrer Farbe:

| Revier | Mannschaft | Farbe | Wahrzeichen (aus `orte.json`) |
|---|---|---|---|
| Altstadt | Die Weissen | weiss | Historisches Rathaus, Schmalzturm, Bayertor, Stadtpfarrkirche |

Genommen werden sie in dieser Reihenfolge: Vorstadt-Nord, Lechviertel,
Klinikum und Sued, Altstadt. Das eigene Viertel faellt spaet - man nimmt es
den Gruenen ab, nicht umgekehrt.
| Vorstadt-Nord | Die Gelben | gelb | Bahnhof, Polizeiinspektion, Sankt Katharina, Stadtverwaltung |
| Lechviertel | Die Blauen | blau | Mutterturm, Karolinenbruecke, Johanniskirche, Faerbertor |
| Klinikum und Sued | Die Gruenen | gruen | Klinikum, kbo-Lech-Mangfall-Klinik, Christuskirche, Friedhofskirche |

Der Spieler faengt ohne Farbe an: er wacht im Klinikum auf, ohne Geld, ohne
Ruf, mit einer geliehenen Paintball-Pistole. Genau dort, wo das Spiel ohnehin
beginnt - und wo man nach jedem Zusammenbruch wieder aufwacht.

## Aufbau

Vier Kapitel. Jedes hat dieselbe Form, und jedes veraendert die Stadt:

1. **Markieren.** Die vier Wahrzeichen des Reviers mit Farbe treffen. Sie
   stehen auf der Karte, sobald das Kapitel laeuft.
2. **Bewaehren.** Eine Auftragsart, die zum Revier passt, dreimal schaffen -
   das Revier gibt erst nach, wenn man dort gearbeitet hat.
3. **Der Kopf.** Ein besonderer Gegner: ein Fahrzeug in der Farbe der
   Mannschaft, das flieht, zurueckschiesst oder verteidigt wird.
4. **Der Fund.** Was der Kopf hinterlaesst, oeffnet das naechste Kapitel.

### Kapitel 1 - Die Gelben vom Bahnhof (Vorstadt-Nord)

Die Gelben fahren alles, was Raeder hat. Bewaehrung: drei Taxifahrten.
Der Kopf ist **der Dispatcher**, ein gelber Wagen, der durch die ganze
Vorstadt flieht und erst stehenbleibt, wenn er ausser Gefecht ist.
Fund: **der Werkstattschluessel** - die Lackiererei arbeitet ab jetzt
kostenlos, und der eigene Wagen haelt mehr aus (Leben 150 statt 100).

### Kapitel 2 - Die Blauen am Lech (Lechviertel)

Am Wasser wird gerast. Bewaehrung: drei Rennen. Der Kopf ist **die
Faehrfrau**, die auf der Rennstrecke vorausfaehrt und nicht gestellt, sondern
ueberholt werden muss - zweimal, auf demselben Kurs.
Fund: **der Rotorschluessel** - der Hubschrauber auf dem Klinikumsdach
laesst sich starten.

### Kapitel 3 - Die Gruenen ums Klinikum (Klinikum und Sued)

Die Gruenen halten sich draussen. Bewaehrung: drei Verfolgungen. Der Kopf
ist **der Foerster**: er sitzt im Wald, wo der Panzer steht, und schickt
Streifen. Man muss ihn markieren, ohne selbst umzufallen.
Fund: **der Zuendschluessel** - der Panzer faehrt.

### Kapitel 4 - Der Hauptplatz (Altstadt)

Alle drei geschlagenen Mannschaften stehen jetzt auf der Seite des Spielers,
die Weissen halten den Hauptplatz. Der letzte Auftrag ist eine Belagerung:
die vier Wahrzeichen der Altstadt markieren, waehrend die Polizei mit voller
Fahndungsstufe und weisse Wagen aus allen Richtungen kommen. Wer es schafft,
hat die Stadt - und die Karte faerbt sich in seiner Farbe.

## Was die Stadt davon merkt

- **Reviere**, die einem gehoeren, faerben ihre Wahrzeichen auf der Karte um.
- **Ruf** entscheidet, welches Kapitel offen ist, wie teuer die Laeden sind
  und ab wann die Mannschaften von selbst angreifen.
- **Werte** (Ausdauer, Zielsicherheit, Fahren) wachsen durch Benutzung und
  machen die spaeteren Kapitel erst machbar - wer nur liefert, kommt beim
  Foerster nicht weit.
- **Fundstuecke** sind keine Gegenstaende im Rucksack, sondern Schalter in
  der Welt: die Lackiererei, der Hubschrauber, der Panzer.

## Was davon steht

- Charakterwerte und Ruf: **gebaut** (siehe `LaLaBergKonto`).
- Reviere, Kapitel und Fundstuecke im Spielstand: **gebaut**.
- Markieren der Wahrzeichen, Bewaehrung ueber den Ruf: **gebaut**
  (`LaLaBergRevier`, Saeule plus Ring an jedem Wahrzeichen, Marken auf
  beiden Karten, Kapitelzeile im HUD).
- Die Koepfe: **gebaut** - je ein Wagen in der Farbe der Mannschaft, der
  flieht; zaeher von Kapitel zu Kapitel (100, 160, 220, 300 Lebenspunkte),
  und die letzten beiden rufen die Polizei dazu.
- Die Fundstuecke: **gebaut** - Werkstattschluessel (Lackiererei umsonst,
  der eigene Wagen nimmt nur zwei Drittel Schaden), Rotorschluessel und
  Zuendschluessel (Hubschrauber und Panzer sind bis dahin verschlossen).
- Offen: die Bewaehrungsaufgaben je Revier (drei Taxifahrten, drei Rennen,
  drei Verfolgungen) und die Belagerung des Hauptplatzes als eigener,
  gescripteter Abschnitt.
