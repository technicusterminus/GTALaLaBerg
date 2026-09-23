# GTALaLaBerg – Unreal Engine 5.8

> **Landsberg am Lech als begehbare und befahrbare Open World im Maßstab 1:1.**
> Native Unreal-Engine-5.8-Umsetzung mit amtlichen LoD2-Gebäudedaten, Laserscan-Gelände, OpenStreetMap-Straßennetz, Nanite-Sektorstreaming, KI-Verkehr, interaktiven Fahrzeugen, Paintball-Waffen und Tageszeiten.

***

## Projektüberblick

**GTALaLaBerg** ist kein klassisches GTA-Remake, sondern ein technisch orientiertes Unreal-Engine-Projekt: Landsberg am Lech wird aus öffentlichen Geodaten, Gebäudemodellen, Straßeninformationen und Höhendaten als begeh- und befahrbare Stadt rekonstruiert.

Der Schwerpunkt liegt auf einer skalierbaren Pipeline für große Stadtmodelle in Unreal Engine:

- automatisierter Import und Sektorbildung
- Nanite-Static-Meshes für die gesamte Stadt
- amtliche LoD2-Gebäudegeometrie
- KI-Verkehr und Fußgänger
- interaktive, übernehmbare Fahrzeuge
- Open-World-Streaming in Stadtsektoren
- ortsbezogene HUD-Anzeigen
- automatisierte Smoke-, Fahr-, Bild- und Verkehrstests
- Cook-/Package-fähiger Shipping-Build

Das Projekt ist als **natives C++-Unreal-Projekt** aufgebaut. Die Projektdatei heißt `GTALaLaBerg.uproject`; das interne C++-Modul heißt bewusst `LaLaBerg`.

| Bezeichnung | Wert |
|---|---|
| Unreal-Projekt | `GTALaLaBerg.uproject` |
| C++-Modul | `LaLaBerg` |
| Editor-Target | `GTALaLaBergEditor` |
| Game-Target | `GTALaLaBerg` |
| Engine | Unreal Engine 5.8 |
| Zielplattform | Windows 64-bit |
| Lizenz für Code/Tools | MIT |
| Stadt- und Geodaten | ODbL / CC BY 4.0, siehe Lizenzabschnitt |

***

## Features

### Stadtmodell

Die Stadt wird als sektorisiertes Nanite-Stadtmodell geladen und besteht aus Gebäuden, Straßen, Freiflächen, Gewässern, Vegetation und Landmarken.

| Inhalt | Umfang |
|---|---:|
| Gebäude | 7.207 |
| Bäume | 13.320 |
| Statische Nanite-Meshes | 2.060 |
| Stadtsektoren | 101 |
| Sektorgröße | 500 × 500 m |
| Ladezeit | ca. 6 Sekunden |
| Straßen | 263 |
| Plätze | 12 |
| Wahrzeichen | 40 |

Enthalten sind unter anderem:

- amtliche LoD2-Gebäudegeometrie mit Gebäudehöhen und Dachformen
- laserbasiertes Gelände
- Straßen, Plätze und Freiflächen aus OpenStreetMap-Daten
- Lech, Wehr und Uferbereiche
- Stadtvegetation mit mehr als 13.000 Bäumen
- Altstadtaufwertung mit Putzfarben, Dachdeckung, Gesimsen, Sockeln, Brandmauern, Ladenzonen, Fenstern, Türen und Gauben
- Hauptplatz, Rathaus, Marienbrunnen, Schmalzturm und Klinikum-Campus
- in etwa 11,5-m-Abschnitte aufgeteilte Häuserzeilen für eine glaubwürdigere Fassadenstruktur

Farbe und Oberflächen (überarbeitet 2026-09-21, weil die Stadt im Spiel blass und wie aus Pappe wirkte): Die Farbe jedes Gebäudes kommt weiter aus den Scheitelfarben (amtliche Daten plus Altstadtpalette), die Detailtexturen aus `Tools/baue_texturen.py` modulieren sie. Deren Ausschläge lagen früher bei ±7–10 % und waren im Spiel unsichtbar; jetzt: Biberschwanzdeckung mit einzeln gebrannten Ziegeln (wärmer und dunkler – Terrakotta statt Lachsrosa), sichtbarer Kalkputz mit Schmutzfahnen unter den Fensterbänken und Gesimsschatten, weiße Fensterstöcke, Wiese mit trockenen Stellen, auf Plätzen Kleinpflaster in Segmentbögen (`T_Pflaster_D`, jeder Granitstein etwas anders) statt einfarbigem Grau. Seit dem 2026-09-21 abends kommen Dach, Pflaster, Wiese und Putzkorn aus CC0-Fotomaterialien (siehe Lizenzen, `Tools/baue_fototexturen.py`); die Fotos bringen ihren Farbton nur anteilig ein (Dach 25 %, Pflaster 35 %, Wiese 30 %), weil die Scheitelfarbe ihn schon mitbringt – mit halbem Anteil wurden die Dächer orange und die Wiese giftgrün. `Tools/baue_materialien.py` hebt die Sättigung von Fassaden (×1,5) und Dächern (×1,15) und baut die Materialien an Ort und Stelle neu, damit die Stadt-Meshes ihre Verweise behalten. Dazu Himmelslicht 1,6 → 1,2, Sättigung im Post-Process 1,04 → rund 1,1, Kontrast 1,05 → 1,10, Umgebungsverdeckung 0,55 → 0,70. Offen: Die Putzpalette der Ausleitung selbst ist pastellig; kräftigere Grundtöne (Ocker, Altrosa, Türkis wie am Hauptplatz) bräuchten eine neue Ausleitung aller Sektoren.

### Verkehrs- und Stadtleben

| Element | Anzahl |
|---|---:|
| Statische Passanten als Stadtgeometrie | 371 |
| Geparkte Fahrzeuge | 1.078 |
| Fahrende KI-Autos | 70 |
| KI-Passanten | 90 |
| Ampeln | 44 |

Die Verkehrslogik basiert auf vorbereiteten Wegen, OpenStreetMap-Straßendaten und Kreuzungsinformationen:

- KI-Autos fahren geschlossene Rundkurse über das Straßennetz, biegen dabei an Kreuzungen ab und befahren auch Einbahnstraßen in Fahrtrichtung.
- Fahrzeuge fahren grundsätzlich rechts versetzt zur Straßenachse.
- Gegenverkehr entsteht automatisch durch den fahrtrichtungsabhängigen Spurversatz.
- Fahrzeuge bremsen vor Autos, Spielern, roten Ampeln und bevorrechtigtem Verkehr.
- Bei Hindernissen weichen Fahrzeuge seitlich aus, sofern es sich nicht um eine Ampel oder Vorrangregelung handelt.
- KI-Passanten laufen Gehwegsegmente ab und reagieren auf andere Passanten und den Spieler.
- Ampeln sind aus OSM-Punkten mit `highway=traffic_signals` erzeugt.
- Kreuzungsgruppen werden bevorzugt über reale Graphknoten des Straßennetzes gebildet.
- Die Ampelsteuerung verhindert gleichzeitig grüne, geometrisch konfliktierende Fahrtrichtungen.

### Fahrzeuge

Jeder Wagen in der Stadt ist grundsätzlich übernehmbar.

- Der Spielstart platziert ein fahrbares Fahrzeug auf einer freien Straße am Klinikum.
- Falls kein fahrbarer Wagen in Reichweite steht, kann das nächstgelegene KI-Auto übernommen werden.
- Das betroffene KI-Fahrzeug verschwindet dabei und wird an derselben Position zum Spielerfahrzeug.
- Das gilt für fahrende und geparkte KI-Fahrzeuge.
- Geparkte Fahrzeuge stehen an ihren aus den Quelldaten übernommenen Standorten und benötigen keine Route.

Es stehen bis zu **14 Fahrzeugtypen** zur Verfügung:

| Quelle | Fahrzeuge |
|---|---:|
| CarConcept, CC BY 4.0 | 1 |
| Epic City Sample Vehicles via Fab | 13 |
| Gesamt | 14 |

Die Fahrzeugdarstellung nutzt einen bewusst aggressiven Sichtweiten-Kompromiss:

- Detaillierte Fahrzeuge werden nur innerhalb von 80 m um den Spieler angezeigt.
- Entfernte Fahrzeuge verwenden ein vereinfachtes Fernmodell.
- Detailfahrzeuge werden über `ALaLaBergAutoPool` als Instanced Static Meshes gepoolt.
- Geparkte Kasten-Fallbacks werden über `ALaLaBergKastenPool` pro Lackfarbe instanziiert.

Diese Pooling-Strategie reduziert Draw Calls massiv. Ohne Pooling sank die Bildrate bei 1.078 geparkten Fahrzeugen im Fahrtest von 26 auf 17 fps; alle Detailfahrzeuge gleichzeitig führten zu rund 2 fps.

Der fahrbare Wagen läuft über echte Chaos-Vehicle-Physik (`UChaosWheeledVehicleMovementComponent`) statt vier Federstrahlen - Motor-/Getriebekennfeld, Vorderradlenkung, Hinterradantrieb, echte Reifenreibung je Rad. Das Motormoment muss dafür hundertfach überhöht werden, sonst beschleunigt die Karosserie nicht; die Ursache ist offen, die Bremse arbeitet mit realistischen Werten - siehe „Bekannte Grenzen".

### Spielfigur

Die Spielfigur wird aus einer Third-Person-Kamera gesteuert.

- Kamera über Federarm hinter und über der Figur
- Bevorzugt echte CC0-Skeletal-Meshes von Quaternius
- Drei mögliche Figurenvarianten: `Farmer`, `Casual` und `Worker`
- Zufällige Farbvarianten für Kleidung und Körpergröße
- KI-Passanten verwenden dieselben Varianten
- Fallback auf ein selbstgebautes, animiertes Gelenk-Rig, falls die Skeletal Assets nicht verfügbar sind
- Steuerung wie in GTA: Beim Laufen dreht sich die Figur in die Bewegungsrichtung und rennt vorwärts; beim Schießen dreht sie sich zur Kamera und geht - wenn sie sich dabei bewegt - mit eigenen Richtungsanimationen seitlich oder rückwärts (`Run_Left`/`Run_Right`/`Run_Back`). Zuvor war die Blickrichtung fest an die Kamera gebunden, und mit A/D rutschte die Figur mit der Vorwärts-Laufanimation seitwärts.
- Die importierten Figuren schauen nach +Y, gelaufen wird entlang +X - Spielfigur und KI-Passanten sind deshalb um -90° gedreht. Bei den Passanten fehlte das zunächst, sie liefen alle seitwärts.
- Die Spielfigur hält die Waffe in der rechten Hand und zielt damit nach vorn (`Idle_Gun_Pointing` im Stand, `Run_Shoot` in Bewegung). Das Rig endet beim Unterarm (`LowerArm_R`), einen Hand-Knochen gibt es nicht - die Handfläche liegt deshalb 8 cm hinter dem Unterarmende. Jede Waffe hat einen eigenen Griffpunkt (`ALaLaBergWaffe::GriffOrt`), der in diese Handfläche gesetzt wird, und wird jedes Bild aufrecht entlang des Unterarms ausgerichtet. Geprüft über `-LaLaBergKoerperFoto`: je Waffe `LALABERG_WAFFE_GEHALTEN` samt Seitenansicht, dazu ein Bild im Laufen. Mit `-LaLaBergPose=<Name>` lassen sich die Standposen des Figurenpakets im Bild vergleichen.
- Der Weg dorthin, weil jeder Schritt einzeln sichtbar falsch war: Zuerst hing die Waffe am unsichtbaren Kasten-Arm und schwebte neben der Figur. Am Unterarm erbte sie dann den Maßstab 100 der Knochen und hing 2 m daneben. `Idle_Gun` ließ den Arm hängen, die Waffe zeigte zu Boden. Die Waffenmodelle lagen durch ihren alten Kameraversatz 23 bis 54 cm vor der Hand. Und nur einmal in der Ruhepose ausgerichtet, drehte die Zeige-Animation den Unterarm mit - der Griff zeigte zur Seite, der Werfer umschloss den Arm.

- Zielen nach oben und unten: Der Oberkörper beugt sich um die Kameraneigung (bis ±60°), verteilt auf die Wirbel Abdomen/Torso/Chest (30/30/40 %) – Arme, Waffe und Kopf folgen der Brust, die Beine bleiben, wie die Animation sie stellt. Im Stand immer, im Laufen nur beim Schießen. Umgesetzt als eigene C++-Animationsinstanz (`ULaLaBergZielAnim`), die dieselbe Animation abspielt und danach die Knochen dreht – ohne Control Rig und ohne Animations-Blueprint. Gemessen: Waffe bei −35°/0°/+35° Neigung auf −41°/−6°/+29° (`-LaLaBergZielFoto`).

### Paintball-Waffen

Das Projekt verwendet absichtlich keine tödlichen Projektile, sondern Paintball-Mechaniken.

| Taste | Waffe | Verhalten |
|---|---|---|
| `1` | Pistole | Einzelschuss |
| `2` | MP | Dauerfeuer |
| `3` | Schrotflinte | Sieben Projektile im Streukegel |
| `4` | Werfer | Langsames Projektil, hohe Wucht, großer Farbklecks |

Treffer erzeugen einen Decal-Farbklecks auf der getroffenen Fläche.

- Der Farbklecks bewegt sich mit getroffenen Flächen, Fahrzeugen oder Figuren mit.
- Hauswände und statische Objekte behalten die aufgebrachte Farbe.
- Treffer auf Fahrzeuge bremsen diese kurz ab.
- Treffer auf KI-Passanten lösen ein kurzes Stolpern aus.
- Die Waffenmodelle stammen aus einem CC0-lizenzierten Quaternius-Paket.

### Lieferaufträge

Die erste Aufgabe im Spiel. Neben dem Startpunkt am Klinikum steht eine **blaue Säule** (60 m hoch, mit Ring am Boden). Wer hineinläuft oder -fährt, nimmt eine Lieferfahrt an:

- Ziel ist ein echter Ort in Landsberg: die 39 Wahrzeichen aus `orte.json` (Bahnhof, Schmalzturm, Bayertor, Heilig-Kreuz-Kirche …), jeweils auf den nächsten Punkt einer Straße davor gesetzt, damit man mit dem Wagen hinkommt. Gewählt wird zufällig unter den Orten 400 m bis 2,5 km Luftlinie entfernt.
- Das Ziel markiert eine **gelbe Säule**. Zeit: Luftlinie × 1,5 bei 10 m/s plus 30 s (1,8 km ≈ 5 Minuten). Lohn: 100 € plus 1 € je 5 m Luftlinie, auf 10 € gerundet.
- Angekommen (8 m Umkreis): Geld aufs Konto, die nächste blaue Säule steht an einem anderen Ort 150–900 m weiter.
- Zu spät: der Auftrag verfällt, die blaue Säule bleibt, wo sie war. Wer noch darin steht, muss einmal hinaus und wieder hinein.
- Oben rechts zeigt eine Tafel Ziel, Entfernung, Restzeit (die letzten 20 s rot), Lohn und Kontostand, dazu einen Pfeil in Blickrichtung der Kamera.

Bewusst noch nicht dabei: Speicherstand (das Konto gilt für eine Sitzung), Fracht oder Fahrgäste als Figuren, Schadenabzug, eine Karte mit Route.

### Spielstand, Paintball-Laden und Lackiererei

- **Spielstand**: Kontostand, gekaufte Waffen, Lack des eigenen Wagens und die Zahl erledigter Aufträge überdauern das Spielende (Unreal-SaveGame, Slot `LaLaBerg`, in `Saved/SaveGames`). Gespeichert wird nach jeder Änderung – Auftrag, Strafe, Kauf – und beim Beenden. Läuft das Spiel mit einem `-LaLaBerg…`-Schalter (Prüfläufe, Fotomodus), beginnt das Konto leer und schreibt in den Slot `LaLaBergTest`; der echte Spielstand bleibt unberührt.
- **Paintball-Laden** (grüne Marke beim Bahnhof, zu Fuß hineingehen): Am Anfang hat man nur die Paintball-Pistole. MP 300 €, Schrotflinte 600 €, Werfer 1500 €. Die Tasten 2–4 sagen bis zum Kauf, wo es die Waffe gibt.
- **Lackiererei** (grüne Marke nahe dem Start, mit dem Wagen hineinfahren): sechs Farben zu je 150 €; der eigene Wagen behält die Farbe auch beim nächsten Spielstart. Wie in GTA: Wer gesucht wird und dabei von keiner Streife gesehen wird, ist die Fahndung los.
- Im Laden: Pfeil hoch/runter wählen, Enter kauft, hinausgehen schließt. Die Liste zeigt Preis, „gekauft“/„aktuell“ und in Rot, was zu teuer ist.
- Umlackieren der echten Fahrzeugmodelle: Die Lackslots stammen aus einem Editor-Skript über alle Modelle (City Sample `veh_carPaint`/`veh_paint`, CarConcept `Paint_1/2_Carmine`). Der City-Sample-Lack holt seine Farbe je Instanz aus einer Palettentextur (statischer Schalter „Paint Variation“) – deshalb gibt es `Content/Art/Materials/MI_Lack_Einfarbig`, denselben Lack ohne Variation, dessen `BaseColor` die Lackiererei setzt. Beim allerersten Umlackieren kompiliert der Editor diese Shader-Variante; bis dahin erscheint der Lack kurz grau.

### Karte

- **Minikarte** unten links, Norden oben, der Spieler als Pfeil in der Mitte. Zu Fuß 300 m im Blick, im Wagen 500 m. Die Auftragssäule (blau bzw. gelb) bleibt am Rand stehen, wenn sie außerhalb liegt; Streifenwagen blinken rot-weiß.
- **Vollkarte** mit `M`: die ganze Stadt, Auftrag mit Namen, alle Streifenwagen, der eigene Standort.
- **Route**: Minikarte und Vollkarte zeigen den Weg zum Auftragsziel (gelb) bzw. zur blauen Säule (blau) als Linie über die Straßen – kürzester Weg über denselben Straßengraphen, den die Streifenwagen fahren, mit Einbahnregel, also eine Autoroute. Einmal je Sekunde neu berechnet, auf der Minikarte am Rand abgeschnitten.
- Beide zeigen einen einmal vorgerenderten Stadtplan (`Tools/Export/prepare-karte.py`, 1 Pixel je Meter, dieselben Quelldaten und Farben wie der Stadtplan im Webprojekt): Straßen nach Klasse, Gebäude, Lech, Parks, Wald, Plätze. Die Minikarte dreht sich nicht mit – eine drehende Karte bräuchte ein eigenes Maskenmaterial, das Bild auf dem Canvas würde sonst über den Rand ragen.

### Polizei und Fahndung

- **Taten** geben Punkte: Passant beschossen 1, Passant angefahren (schneller als Schritttempo im Wagen) 2, fahrendes oder geparktes Auto übernommen 2, Streifenwagen beschossen 4. Dieselbe Tat zählt höchstens einmal je Sekunde.
- **Sterne** oben rechts: der n-te Stern ab n² Punkten, höchstens fünf. Je Stern ein Streifenwagen, mit jedem Stern schneller (58 bis 90 km/h).
- **Streifenwagen** sind KI-Autos mit Blaulicht (zwei Leuchten auf der Karosserie im Wechsel und ein blau blitzendes Licht), die bei Rot und ohne Vorfahrt fahren. Sie tragen eigene CarConcept-Teile statt einer Instanz im gemeinsamen Autopool: erst während des Spiels hinzugefügte Instanzen zeichnete der hierarchische Pool nicht (es blieb ein schwebender Blaulichtbalken). Die Karosserie hat noch die rote Grundlackierung des CarConcept, keine Polizeifarben. Sie setzen 250–450 m entfernt ein und fahren über den Straßengraphen (`Tools/Export/prepare-netz.cjs`, 6023 Knoten, Einbahnregel, A*-Suche), die Route wird alle 1,5 s neu geplant – ab dem Knoten, den der Wagen gerade ansteuert (nach der Blickrichtung gewählt, kreiste ein Wagen in der Kurve im Test 50 s auf der Stelle). Das letzte Stück zum Spieler fahren sie querfeldein, wenn er abseits der Straße steht.
- **Sehen**: auf 40 m immer, bis 150 m bei freier Sichtlinie. Wer gesehen wird, wird verfolgt; sonst fahren alle Streifen zum Ort, an dem der Spieler zuletzt gesehen wurde, und suchen dort die Straßen ab.
- **Abhängen**: Solange keine Streife den Spieler sieht, blinken die Sterne und ein Balken läuft (6 s + 4 s je Stern). Im Suchgebiet um den letzten bekannten Ort (150 m + 50 m je Stern) läuft er dreimal langsamer.
- **Festnahme**: Neben einem Streifenwagen (8 m) stehen bleiben, 2,5 s lang – ein roter Balken zeigt, wie lange noch; wer losfährt oder wegrennt, baut ihn wieder ab. Folgen: laufender Auftrag verfällt, 100 € plus 10 % des Kontos Strafe, weiter geht es zu Fuß an der Polizeiinspektion.

Bewusst noch nicht dabei: Polizisten zu Fuß, Straßensperren, Rammen oder Schießen durch die Polizei, Sirene (es gibt noch keinen passenden Klang im Projekt), Hubschrauber ab vier Sternen.

### HUD und Menüs

Das HUD enthält:

- Tacho im Fahrzeug
- kontextuelle Einsteigeaufforderung `E – Einsteigen`
- Fadenkreuz
- aktiver Waffenname
- Schussanzahl
- Steuerungshinweise
- Ortsanzeige mit Straße, Platz oder Wahrzeichen
- Auftragstafel mit Richtungspfeil, Restzeit und Kontostand
- Ladenliste mit Preisen und Farbfeldern
- Minikarte, Vollkarte (`M`)
- Fahndungssterne, Suchbalken beim Abhängen, Festnahmebalken
- Ortsteil und Stadt

Das Menü umfasst:

- Startbildschirm
- Pause-Menü
- Auflösungswahl
- Fenstermodus
- Grafikstufe 0–3
- Lautstärkeregelung

***

## Steuerung

| Zu Fuß | Im Fahrzeug |
|---|---|
| `WASD` – bewegen | `W` / `S` – Gas / Rückwärts |
| Maus – umsehen | `A` / `D` – lenken |
| `Leertaste` – springen | `Leertaste` halten – bremsen |
| Linke Maustaste halten – feuern | – |
| `1`–`4` – Waffe wechseln | – |
| `E` – einsteigen, bis 8 m Abstand | `E` oder `R` – aussteigen |
| `M` – Karte | `M` – Karte |
| `Esc` – Menü | `Esc` – Menü |

***

## Architektur

```text
GTALaLaBerg/
├── Config/
│   ├── DefaultEngine.ini
│   └── DefaultGame.ini
├── Content/
│   ├── Art/
│   │   ├── People/
│   │   ├── Vehicles/
│   │   └── Waffen/
│   ├── City/
│   │   └── Sectors/
│   └── SourceData/
│       ├── Fahrzeug/
│       ├── Orte/
│       ├── Sectors/
│       └── Verkehr/
├── Docs/
│   └── ChaosVehicle-ForumAnfrage.md
├── Source/
│   └── LaLaBerg/
├── Tools/
│   ├── Export/
│   └── SectorPipeline/
├── GTALaLaBerg.uproject
└── README.md
```

### Kernprinzipien

- **Native C++-Logik:** Gameplay, Streaming, Verkehr, UI, Fahrzeugübernahme, Waffen und Tageszeit sind im Modul `LaLaBerg` implementiert (`Source/LaLaBerg/`, keine Private/Public-Unterteilung).
- **Asset-basierte Stadt:** Die Stadt wird regulär aus Unreal-Assets geladen; `stadt.json` dient als langsamer Laufzeit-Fallback.
- **Sektorisierung:** Stadtgeometrie ist in 500-m-Sektoren aufgeteilt.
- **Nanite:** Stadtgeometrie wird als Nanite-Static-Mesh geladen.
- **Automatisierte Pipeline:** Die Geodatenaufbereitung wird mit Node.js- und Python-Tools reproduzierbar ausgeführt.
- **Cook-Sicherheit:** Stadt- und Kunst-Assets werden über `DefaultGame.ini` explizit in den Cook aufgenommen (`DirectoriesToAlwaysCook`); die JSON-Quelldaten (Sektoren, Orte, Fahrzeug, Verkehr) werden zusätzlich unverändert mitgepackt (`DirectoriesToAlwaysStageAsUFS`), damit der Laufzeit-Fallback auch im Shipping-Build funktioniert.
- **Diagnostik:** Funktions- und Bildtests laufen automatisiert über Kommandozeilenparameter.

***

## Datenpipeline

```text
Tools/Export/prepare-stadt.cjs
    → Content/SourceData/stadt.json

Tools/SectorPipeline/sectorize-city.cjs
    → Content/SourceData/Sectors/*.json
    → Content/SourceData/Sectors/manifest.json

UnrealEditor-Cmd.exe
    -run=LaLaBergImport
    -Sector=alle
    → /Game/City/Sectors/<id>/SM_<id>_<Klasse>

Tools/baue_texturen.py
Tools/baue_materialien.py
    → /Game/Art

Tools/Export/prepare-orte.cjs
    → Content/SourceData/Orte/orte.json

Tools/Export/prepare-wagen.cjs
    → Content/SourceData/Fahrzeug/wagen.json

Tools/Export/prepare-verkehr.cjs
    → Content/SourceData/Verkehr/verkehr.json

Tools/Export/prepare-netz.cjs
    → Content/SourceData/Verkehr/netz.json      (Straßengraph für die Polizei)

Tools/Export/prepare-karte.py
    → Content/SourceData/Karte/karte.png, karte-klein.png, karte.json

Tools/baue_farbklecks.py
    → M_Farbklecks

Tools/baue_waffenmetall.py
Tools/erzeuge_waffentextur.py
    → M_Waffenmetall

Tools/importiere_waffen.py
    → /Game/Art/Waffen

Tools/importiere_npc.py
    → /Game/Art/People

Tools/erzeuge_sounds.py
Tools/importiere_sounds.py
    → /Game/Audio
```

### Datenquellen außerhalb des Repositories

Die Exporter benötigen die Rohdaten des separaten Projekts **GTA LaLaBerg**. Diese Daten sind nicht Teil dieses Repositories.

Die Umgebungsvariable `LALABERG_QUELLE` muss auf das Wurzelverzeichnis dieses Quellprojekts zeigen. Ohne gesetzte Variable wird ein benachbarter Ordner erwartet.

```powershell
$env:LALABERG_QUELLE = "D:\Projekte\GTA-LaLaBerg"
```

Danach müssen die Node-Abhängigkeiten der Export-Tools einmalig installiert werden:

```powershell
cd Tools\Export
npm install
```

***

## Schnellstart

### Voraussetzungen

- Unreal Engine **5.8**
- Visual Studio 2022 mit C++ Desktop Development und Unreal-Unterstützung
- Windows 10 oder Windows 11
- Git und Git LFS
- Node.js für die Export-Tools
- Python für Textur-, Material- und Import-Tools
- Optional: Epic Games Launcher bzw. Fab-Integration für **City Sample Vehicles**

### Repository klonen

```powershell
git clone <repository-url>
cd GTALaLaBerg
git lfs pull
```

> Ohne `git lfs pull` fehlen große JSON-Dateien sowie Unreal-Assets wie `.uasset` und `.umap`.

### City Sample Vehicles installieren

Die 13 zusätzlichen Fahrzeugtypen sind nicht im Repository enthalten. Sie unterliegen der Fab Content License und dürfen nicht frei als Rohdaten weitergegeben werden.

1. Projekt in Unreal Engine öffnen.
2. Im Content Browser den Bereich **Fab** öffnen.
3. Nach **City Sample Vehicles** suchen.
4. Das kostenlose Paket zum Projekt hinzufügen.
5. Alle Assets speichern.

Falls `Content/CitySampleVehicles` nicht vorhanden ist, bleibt das Projekt funktionsfähig. Alle Fahrzeuge fallen automatisch auf das CarConcept-Modell zurück.

### Projekt generieren und bauen

```powershell
UnrealVersionSelector.exe /projectfiles GTALaLaBerg.uproject
```

Danach die erzeugte Solution in Visual Studio öffnen und bauen:

```text
Configuration: Development Editor
Platform: Win64
Target: GTALaLaBergEditor
```

Alternativ über die Unreal-Engine-Buildwerkzeuge:

```powershell
<UE_ROOT>\Engine\Build\BatchFiles\Build.bat `
  GTALaLaBergEditor Win64 Development `
  -project="<PROJECT_ROOT>\GTALaLaBerg.uproject"
```

### Editor starten

```powershell
<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor.exe `
  GTALaLaBerg.uproject
```

***

## Import- und Laufzeitregeln

> **Wichtig: Import und Spiellauf niemals gleichzeitig ausführen.**

Ein laufender Spielprozess mit `-game` hält Stadt-Assets geöffnet. Ein parallel laufender Import kann diese Assets nicht zuverlässig ersetzen und meldet in diesem Fall:

```text
LALABERG_STATIC_SAVE_FAILED
```

Am 11.09.2026 betraf dies drei Meshes im Sektor `S_N2_N3`.

### Vor einem Spiellauf

Sicherstellen, dass kein Importprozess läuft:

```powershell
Get-Process UnrealEditor-Cmd -ErrorAction SilentlyContinue
```

### Vor einem Import

Sicherstellen, dass kein laufender Spielprozess aktiv ist:

```powershell
Get-CimInstance Win32_Process |
    Where-Object {
        $_.Name -eq "UnrealEditor.exe" -and
        $_.CommandLine -match "-game"
    } |
    Select-Object ProcessId, CommandLine
```

### Stadtimport ausführen

```powershell
<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe `
  GTALaLaBerg.uproject `
  -run=LaLaBergImport `
  -Sector=alle `
  -unattended `
  -nop4 `
  -log
```

Der Import erzeugt sektorisierte Static Meshes im folgenden Assetpfad:

```text
/Game/City/Sectors/<Sektor-ID>/SM_<Sektor-ID>_<Klasse>
```

***

## Fahrzeuggenerierung

Die Fahrzeugform ist zentral definiert:

```text
Tools/Export/wagen-form.cjs
```

Diese Definition wird verwendet für:

- den fahrbaren Startwagen
- fahrende KI-Autos
- geparkte KI-Autos
- die Laufzeitdaten in `wagen.json`

Nach Änderungen an der Fahrzeugform muss der Export neu ausgeführt werden:

```powershell
node Tools\Export\prepare-wagen.cjs
```

Die generierten Fahrzeugdaten liegen anschließend hier:

```text
Content/SourceData/Fahrzeug/wagen.json
```

***

## Automatisierte Prüfläufe

Die Prüfläufe starten im Game-Modus, beenden sich selbst und schreiben strukturierte `LALABERG_…`-Zeilen in das Log.

```powershell
<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor.exe `
  GTALaLaBerg.uproject `
  -game `
  -windowed `
  -ResX=1600 `
  -ResY=900 `
  -log `
  <Schalter>
```

Logs:

```text
Saved/Logs/GTALaLaBerg.log
```

Screenshots:

```text
Saved/Screenshots/WindowsEditor
```

| Schalter | Zweck | Erwartetes Ergebnis |
|---|---|---|
| `-LaLaBergSmoke` | Grundlegender Welttest | `LALABERG_SMOKE PASS` |
| `-LaLaBergFahrtest` | Einsteigen, vier Sekunden Vollgas, bis zum Stillstand bremsen, aussteigen | `LALABERG_FAHRTEST PASS` ab 8 m Weg auf allen Rädern, `LALABERG_BREMS_TEST PASS` (Stillstand unter 3 km/h, mit Bremszeit und Bremsweg), `LALABERG_AUSSTIEG_TEST PASS` |
| `-LaLaBergFoto` | Hauptplatz, Straße, Fahrzeug, Luftbild | Vier Screenshots |
| `-LaLaBergHimmelEchtzeit` | Test der Echtzeit-Himmelsaufnahme | Diagnose im Log/Bild |
| `-LaLaBergNacht` | Setzt die Tageszeit auf Mitternacht | Für Nachtbildtests |
| `-LaLaBergGpu` | GPU-Zeiten pro Renderphase | Profilingdaten im Log |
| `-LaLaBergWaffentest` | Alle Waffen plus Treffer auf Fahrzeug und Wand | `LALABERG_WAFFENTEST PASS` |
| `-LaLaBergVerkehrFoto` | Screenshot von erstem KI-Auto und KI-Passanten, dazu Bodenprobe: steht jede Figur auf dem Boden statt darin? | `LALABERG_VERKEHRFOTO PASS autos=70 passanten=94 boden_geprueft=94 versenkt=0 tiefste=0cm` |
| `-LaLaBergLechFoto` | Luftaufnahme über dem Lech | Prüft `M_Lech` |
| `-LaLaBergKoerperFoto` | Prüft Figur, Arm und Waffenhaltung: Bild von hinten, je Waffe ein Seitenbild, ein Bild im Laufen | `LALABERG_WAFFE_GEHALTEN PASS` je Waffe (Griff unter 25 cm von der Handfläche) |
| `-LaLaBergAuftragTest` | Ganzer Lieferauftrag ohne Tastatur: vor die blaue Säule (Bild), hinein (Bild Richtung Ziel), vor das Ziel (Bild), hinein, dann in die nächste blaue Säule und die Frist ablaufen lassen | `LALABERG_AUFTRAGTEST PASS` mit `erledigt=1 gescheitert=1`, Geld > 0 und mindestens 10 Zielen |
| `-LaLaBergPolizeiTest` | Zwei echte Taten (Passant, Streifenwagen) ergeben zwei Sterne; die Figur bleibt stehen, die Streifen fahren heran und nehmen fest (Bild beim Eintreffen, Bild der Vollkarte). Danach ein Stern, die Figur wird 1,1 km weit versetzt und muss die Fahndung abschütteln | `LALABERG_POLIZEITEST PASS festnahmen=1` mit `abgehaengt_nach` |
| `-LaLaBergLadenTest` | 2000 € aufs Konto, im Paintball-Laden die MP kaufen, in den Wagen, in der Lackiererei zwei Sterne bekommen und ungesehen umlackieren; danach den Spielstand frisch von der Platte lesen | `LALABERG_LADENTEST PASS mp=1 lack=1 gespeichert=1` |
| `-LaLaBergAntriebTest` | Befund statt Prüfung: einsteigen, Vollgas, halbsekündlich Bodenabstand des Rumpfs, Masse, Motordrehzahl, Gang und je Rad Federweg, Federkraft, Antriebsmoment, Schlupf. Mit `-LaLaBergDrehmoment=<Nm>` ein anderes Motormoment, mit `-LaLaBergSchub` statt Motor eine wachsende Schubkraft | `LALABERG_ANTRIEB …`-Zeilen |
| `-LaLaBergZielFoto` | Oberkörper folgt der Kameraneigung: von der Seite je ein Bild bei −35°, 0° und +35°, dazu die gemessene Waffenneigung; zuletzt ein Bild von hinten mit echter Kameraneigung | `LALABERG_ZIELTEST PASS` bei mehr als 30° Spanne der Waffe |
| `-LaLaBergAmpelTest` | Prüft Ampelkonflikte über vollen Zyklus | `LALABERG_AMPELTEST PASS verstoesse=0 ampeln=44` |

### Beispiel: Fahrtest mit GPU-Profiling

```powershell
<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor.exe `
  GTALaLaBerg.uproject `
  -game `
  -windowed `
  -ResX=1600 `
  -ResY=900 `
  -LaLaBergFahrtest `
  -LaLaBergGpu `
  -log
```

***

## Packaging

Für einen Shipping-Build wird `BuildCookRun` verwendet.

```powershell
<UE_ROOT>\Engine\Build\BatchFiles\RunUAT.bat BuildCookRun `
  -project="<PROJECT_ROOT>\GTALaLaBerg.uproject" `
  -noP4 `
  -platform=Win64 `
  -clientconfig=Shipping `
  -build `
  -cook `
  -stage `
  -pak `
  -iostore `
  -archive `
  -archivedirectory="<OUTPUT_DIRECTORY>" `
  -prereqs
```

Das Zielverzeichnis sollte außerhalb von OneDrive, Dropbox oder anderen Cloud-Sync-Ordnern liegen. Der Build umfasst mehrere hundert Megabyte.

Der gepackte Build unterstützt dieselben Testschalter:

```powershell
GTALaLaBerg.exe `
  -windowed `
  -ResX=1600 `
  -ResY=900 `
  -LaLaBergFahrtest
```

Da Shipping-Builds nicht zwingend ein Unreal-Log schreiben, werden Ergebnis, Bildrate und Laufzeit zusätzlich hier protokolliert:

```text
Saved/Logs/LaLaBerg-Test.txt
```

Beim ausgepackten Build steht er im Paket selbst:

```text
<OUTPUT_DIRECTORY>\Windows\GTALaLaBerg\Saved\Logs\LaLaBerg-Test.txt
```

**Erster vollständiger Pakettest am 2026-09-22** (Development statt Shipping, damit Log und Konsole erhalten bleiben; 4 GB, Bauzeit 21 min, danach 4 min je Neubau). Dabei gefunden und behoben:

- `/Game/Audio`, das Ersatzmaterial und die 13 verwendeten City-Sample-Fahrzeugtypen werden nur über ihren Pfad geladen und fehlten in der Cook-Liste – im Paket hätte es keine Klänge gegeben und fast nur Kastenautos (der ganze Fahrzeugordner hat 7,2 GB, deshalb nur die Mesh-Ordner der benutzten Typen).
- Absturz im Fahrtest, nur im gepackten Spiel: Die Bremsprüfung löschte ihren eigenen Timer und griff danach weiter über `this` auf die Spielart zu – in dessen Timerdaten die Lambda mitsamt Kopien liegt. Im Editor blieb der Speicher gültig, im Paket nicht. Jetzt liegen Zeiger und Werte vor dem Löschen auf dem Stapel.
- Das Wegkriterium des Fahrtests (8 m in 4,5 s) hing an der Bildrate; es gilt jetzt Weg **oder** Tempo (25 km/h).

Bestanden im Paket: Fahrtest mit Bremsen/Aussteigen/Krankenhaus, Auftrag, Laden (Spielstand, Kauf, Lackierung), Polizei, Zielen, Ampel. Die Bildrate liegt im Paket am Startpunkt bei 13–16 fps gegenüber 25 im Editor – dieselbe offene Frage wie unter „Bildrate am Startpunkt".

### macOS

Eine Mac-Fassung lässt sich **nicht auf Windows bauen**: Xcode, der Metal-Shadercompiler und die Signatur laufen nur unter macOS. Der Bau muss also auf einem Mac stattfinden, das Projekt selbst ist dafür vorbereitet – kein plattformabhängiger Code, die Direct3D-Einstellungen stehen in einem reinen Windows-Abschnitt, und die beiden Sonderplugins (`ModelContextProtocol`, `MCPClientToolset`) liefert die Engine selbst mit.

Voraussetzungen und Ablauf stehen im Kopf von `Tools/baue_mac.sh`; kurz:

```bash
git clone <repo> && cd GTALaLaBerg
git lfs install && git lfs pull
# City Sample Vehicles im Editor über den Fab-Reiter nach
# Content/CitySampleVehicles holen (nicht im Repo, Epic-Lizenz)
Tools/baue_mac.sh ~/GTALaLaBerg-Paket
```

Erwartbare Stolpersteine, ungetestet mangels Mac:

- **Apple Silicon nötig.** Die Stadt ist Nanite-Geometrie und braucht Metal 3; auf Intel-Macs fehlt das.
- Ohne die Fab-Fahrzeuge fährt der Verkehr als Kastenform – das Spiel läuft, sieht aber ärmer aus.
- Der erste Bau dauert deutlich länger als die 21 Minuten unter Windows, weil alle Shader für Metal neu entstehen.
- Die Prüfschalter funktionieren wie unter Windows; das Protokoll landet im Paket unter `GTALaLaBerg.app/Contents/UE/GTALaLaBerg/Saved/Logs/LaLaBerg-Test.txt` bzw. `~/Library/Logs`.

***

## Performance

Messung vom **11.09.2026** im Fahrtest bei 1600 × 900 Pixeln auf der AMD Radeon PRO Graphics des Entwicklungsrechners.

| Schattenmodus | GPU-Zeit pro Bild | Schattenanteil |
|---|---:|---:|
| Cascaded Shadow Maps | 36,2 ms | 19,9 ms |
| Virtual Shadow Maps | 19,3 ms | 1,2 ms |

Der Wechsel auf Virtual Shadow Maps reduzierte die GPU-Zeit deutlich. Klassische Kaskadenschatten rasterten die gesamte Nanite-Stadt pro Kaskade erneut. VSM ist für große, dynamisch beleuchtete Nanite-Szenen wesentlich besser geeignet.

Für eine RTX 3060 liegen derzeit noch keine Vergleichsmessungen vor.

***

## Rendering und Tageszeit

Das Projekt nutzt Lumen für Global Illumination und Reflexionen:

```ini
r.DynamicGlobalIlluminationMethod=1
r.ReflectionMethod=1
```

Die Tageszeit wird in `ALaLaBergGameMode::AktualisiereTageszeit` gesteuert.

- Ein kompletter Tag-Nacht-Zyklus dauert zehn Minuten.
- Die Sonne folgt einer künstlichen, aber visuell nachvollziehbaren Bahn.
- Elevation wird über eine Kosinuskurve um die Mittagszeit modelliert.
- Azimut ist bewusst vereinfacht und nicht astronomisch exakt.
- Sonnenintensität, Sonnenfarbe, Skylight-Intensität und Skylight-Farbe ändern sich dynamisch.
- Die Auto Exposure folgt einem absichtlich engen Helligkeitsfenster.
- Nachts wandert dieses Fenster auf ein dunkleres Zielniveau, damit Innenhöfe, Torbögen und Straßenzüge nicht unnatürlich hell aufgeblendet werden.

Die Skylight-Cubemap bleibt aktuell eine feste Tagesaufnahme. Die Echtzeitaufnahme liefert in dieser Welt bislang kein brauchbares Bild. Die Ausgabe wird deshalb dynamisch abgedunkelt und blau eingefärbt, jedoch bleibt die zugrundeliegende Cubemap sichtbar tagesbasiert.

***

## Bekannte Grenzen

### Chaos Vehicles

Der Umbau auf echte Chaos-Vehicle-Physik ist abgeschlossen und aktiv (`UChaosWheeledVehicleMovementComponent`, siehe `LaLaBergWagen`/`LaLaBergWagenBewegung`/`LaLaBergWagenRad`).

Zwei Engine-eigene Probleme im UE-5.8-`ChaosVehiclesPlugin` (Experimental) mussten dafür umgangen werden:

- `CanCreateVehicle()` verlangt einen Bone-Namen pro Rad, selbst wenn kein Skeletal Mesh verwendet wird, obwohl `UChaosVehicleMovementComponent::LocateBoneOffset()` `BoneName=NAME_None` ausdrücklich unterstützt - eine Unterklasse überspringt nur genau diese eine Prüfung.
- **Gelöst 2026-09-21:** Der Antrieb brauchte lange ein hundertfach überhöhtes Motormoment (`MaxTorque` 32.000 statt 320 Nm). Ursache, gemessen mit `-LaLaBergAntriebTest`: Chaos' constraintbasierte Federung blieb in diesem Aufbau voll eingefedert (Federweg 0,00 an allen Rädern), die unsichtbare Rumpf-Kollisionsbox lag 3 cm in der Fahrbahn, und der Wagen rutschte wie ein Schlitten – ein Schubtest ohne Motor brauchte rund 12.000 N, bis er sich bewegte. Nur mit durchdrehenden Rädern kam der Antrieb gegen diese Gleitreibung an. Mit kraftbasierter Federung (`p.Vehicle.DisableConstraintSuspension=1` in `Config/DefaultEngine.ini`) hängt die Box 18–22 cm über der Straße, und realistische 320 Nm bringen den Wagen in 5 s auf 67 km/h. Die frühere Vermutung einer Einheitenverwechslung war falsch – die Rechnung in `WheelSystem.cpp` ist in sich stimmig (`TorqueMToCm` ist ×10.000, geteilt durch den Radius in cm).
- Bremse: Das harte Anhalten (>1 g) kam zum Teil ebenfalls vom schleifenden Rumpf. Ohne ihn und mit dem von Codex abgestimmten Bremsmoment (500 Nm) hält der Wagen von 50 km/h in 3,5 s auf 17,5 m an, rund 4 m/s² (`-LaLaBergFahrtest`, `LALABERG_BREMS_TEST`).
- `bReverseAsBrake` ist abgeschaltet: Chaos deutet eine im Stand gehaltene Bremse sonst als Rückwärtsgas - der Wagen hielt kurz an und fuhr dann mit über 30 km/h rückwärts. Vorwärts und rückwärts wählt stattdessen die Gangschaltung (`SetTargetGear`), die Leertaste bremst nur.

Die ausführliche Diagnose (ursprünglich als Community-Anfrage vorbereitet, mittlerweile mit der Lösung ergänzt) steht unter:

```text
Docs/ChaosVehicle-ForumAnfrage.md
```

### Fahrzeug-LOD

Der Fahrzeug-Detailgrad ist bewusst auf Echtzeit-Performance optimiert.

- Detailfahrzeuge sind nur innerhalb von 80 m sichtbar.
- Beim Über- oder Unterschreiten dieser Distanz ist ein sichtbarer Wechsel zwischen Detail- und Fernmodell möglich.
- Es gibt derzeit keine weich überblendeten Fahrzeug-LODs.
- Die Fahrzeugmodelle sind funktional und lizenzkonform, aber nicht auf Film- oder Rennspielniveau texturiert.

### Verkehrsmodell

Die Verkehrslogik priorisiert skalierbares, glaubwürdiges Verhalten gegenüber vollständiger Verkehrssimulation.

- Behoben am 2026-09-18, gefunden beim Bau der Polizei: Die KI-Autos folgten ihrer Route bis dahin kaum. Das Tempo stand in m/s, wurde aber als cm/s gefahren (100-fach zu langsam), und der Spurversatz wurde jedes Bild erneut auf die schon versetzte Lage addiert - die Autos kamen vor allem durch dieses seitliche Wandern voran. Jetzt fährt jedes Auto sein Tempo in cm/s entlang der Route, der Spurversatz wird nur angezeigt (wie bei den Passanten). Die Bremsabstände, das Überholen und die Ampel-/Vorfahrtsregeln waren unter dem alten Verhalten kalibriert; die Prüfläufe dazu sind nach der Korrektur neu gelaufen: Ampeltest `verstoesse=0`, Abbiegetest `abgebogen=60 von=70 wenden=0` (vorher 70 von 70 - gemessen am alten, wandernden Verhalten), Überholtest nach Anpassung der Prüfung bestanden.

- Der Spurversatz der KI-Autos richtet sich nach der echten Fahrbahnbreite an der jeweiligen Stelle (`verkehr.json`, Feld `bp`) statt nach einem für jede Straße gleichen Festwert - auf schmalen Straßen bleibt entsprechend weniger Platz zum Ausweichen, ohne über den Fahrbahnrand hinauszufahren. Die Breite wird je Wegpunkt über ein Raster aus der nächstgelegenen echten Straße übernommen (Median-Abstand 0,11 m, maximal 2,43 m, kein Wegpunkt ohne Treffer). Zwischenzeitlich stand dort nur der Mittelwert je Straßenklasse, weil der Straßengraph keine Breite trägt - eine schmale Hauptstraße (3,1 m) galt damit als 7,5 m breit.
- Keine mehrspurige Fahrspurwahl - und dafür fehlt in Landsberg schlicht die Geometrie: Von 307 km Straßennetz sind **378 m** (0,12 %) mindestens 10 m breit, also überhaupt breit genug für zwei Spuren je Richtung, und das ist ein einzelner 16-m-Ausreißer. Die breiteste echte Hauptstraße misst 9,3 m, das sind zwei Spuren plus Parkstreifen. Ein Spurwahlmodell hätte hier fast keine Straße, auf der es wirken könnte.
- Ein Auto überholt jetzt ein deutlich langsameres oder stehendes Auto direkt voraus, wenn die Straße breit genug ist (≥ 5,5 m), keine Ampel oder Vorfahrt unmittelbar ansteht und die Gegenspur über rund 18 m frei ist - danach schert es wieder ein. Da bei nur rund 70 verteilten Autos in der ganzen Stadt diese Konstellation im echten Spiel selten zusammentrifft, ist die Logik zusätzlich über einen synthetischen Testschalter (`-LaLaBergUeberholTest`) isoliert nachgewiesen, unabhängig vom Zufall im organischen Verkehr: seit der Tempokorrektur (siehe oben) muss der hintere Wagen überholt haben und nach 10 s vorn liegen (`LALABERG_UEBERHOLTEST PASS ueberholt=1 vorsprung=68.3m`). Weiterhin keine echte mehrspurige Fahrspurwahl, kein gleichzeitiges Überholen mehrerer Autos an derselben Stelle, und der seitliche Schwenk ist ein einfacher Versatz statt einer echten Kurve.
- Kein explizites Queue-System pro Kreuzung als eigene Datenstruktur - Warteschlangen entstehen stattdessen implizit aus zwei bereits vorhandenen, jeden Frame neu ausgewerteten Mechanismen: Auto-folgt-Auto-Bremsen (siehe Überholen oben) sorgt für Einordnung hintereinander auf derselben Fahrspur, die Vorfahrtslogik an Kreuzungen (Distanz plus Rechts-vor-Links bei gleicher Straßenklasse) für die Reihenfolge zwischen den Armen. Für den ganz überwiegenden Regelfall (einzelne Autos, keine langen Rückstaus) ausreichend; bei mehreren gleichzeitig wartenden Autos an mehreren Armen ist die Reihenfolge nicht so austariert wie bei einer echten FIFO-Queue.
- Der Anhalteabstand an einer Kreuzung richtet sich jetzt nach deren geschätzter tatsächlicher Breite (aus der Straßenklasse der wichtigsten angeschlossenen Straße, kalibriert an echten Breitendaten) statt nach einem für jede Kreuzung gleichen Festwert - an einer breiten oder mehrarmigen Kreuzung steht ein wartendes Auto entsprechend weiter vom Mittelpunkt entfernt.
- KI-Autos fahren geschlossene Rundkurse über den Straßengraphen statt wie zuvor eine einzige zugewiesene Straße vor und zurück. Eine Zufallsfahrt (geradeaus wahrscheinlicher als Abbiegen, Hauptstraßen bevorzugt, Sackgassen gemieden) wird über den kürzesten gerichteten Rückweg zum Startknoten zu einem Ring geschlossen; im Median rund 1460 m mit 7 Abbiegungen, 35 berührten Kreuzungen und 3 verschiedenen Straßenklassen je Runde. Dadurch dreht kein Auto mehr am Routenende auf der Stelle um - der Wegfolger hängt hinter dem letzten Wegpunkt wieder den ersten an. Nachgewiesen über den Testschalter `-LaLaBergAbbiegeTest`, der im laufenden Spiel sowohl die Abbiegevorgänge als auch die Wendemanöver zählt (letztere müssen 0 sein).
- Jede Abbiegung ist ein geglätteter Bézier-Bogen statt einer rechtwinkligen Ecke; sein Radius wächst mit der Fahrbahnbreite (eine Hauptstraße bekommt einen weiteren Bogen als eine Gasse) und verbraucht nie mehr als knapp die Hälfte der angrenzenden Segmente. 99,4 % aller Wegpunkte knicken dadurch um weniger als 20°. Vor einer Kurve bremst das Auto zusätzlich ab - die Bewegung ist kinematisch, es gibt also keine Fliehkraft, die das von selbst täte.
- Fahrbahnbreite und Vorfahrtsrang gelten jetzt je Wegpunkt statt je Route (`verkehr.json`, Felder `bp`/`kp`): Wer von der Hauptstraße in eine Nebenstraße abbiegt, fährt ab dort auf schmalerer Fahrbahn und hat an der nächsten Kreuzung auch keine Vorfahrt mehr.
- Einbahnstraßen werden mitbefahren, seit die Routen Rundkurse sind: Eine Route wird nie gegen ihre Fahrtrichtung durchlaufen, deshalb liest der Export den Graphen gerichtet (Einbahnregel wie im Webprojekt) und beschränkt Routen auf die größte stark zusammenhängende Komponente - nur dort ist ein gerichteter Rückweg garantiert. Die umfasst 6023 der 8192 Knoten, 100,6 km Straße und 406 der 574 Einbahnkanten.
- Jedes Auto wiederholt seinen eigenen Rundkurs, statt sich unterwegs eine neue Strecke auszudenken. Eine fortlaufende Routenneuplanung zur Laufzeit hätte die komplette Wegplanung (Graph, Zufallsfahrt, Kurvenglättung, Resampling) ein zweites Mal in C++ gebraucht - die Rundkurse erreichen dasselbe sichtbare Verhalten mit der Planung allein im Export.
- Die Ampel-Phasenlogik teilt eine Kreuzung jetzt in so viele nicht überlappende Zeitfenster wie sie tatsächlich unterschiedliche Fahrbahnachsen hat (statt fest zwei Phasen für vier Arme) - eine breite Kreuzung mit mehr als vier Armen bekommt entsprechend mehr Phasen, statt eine ihrer Achsen fälschlich mit einer anderen zu teilen.
- Einzelfälle können trotz Graph-Gruppierung in getrennten Ampelgruppen landen.
- Der fahrbare Wagen hat jetzt eine einfache Spurhalteassistenz: Zwei Seitensonden knapp vor dem Wagen prüfen per Bodenstrahl, ob dort noch ein Sektor-Actor mit "Road"-Tag liegt (siehe `ALaLaBergGameMode::LadeAusAssets`) - erkennt nur eine Seite die Fahrbahn nicht mehr, schiebt eine sanfte Lenkkorrektur zur anderen Seite zurück, skaliert mit der Gegenlenkung des Fahrers (übersteuert keine absichtliche scharfe Kurve). Keine echte Fahrspurbindung (z. B. Zentrierung auf eine bestimmte Fahrspur bei mehrspurigen Straßen), nur eine Fahrbahnrand-Erkennung.

### Figuren

- Der rechte (Waffen-)Arm des selbstgebauten Fallback-Rigs (nur bei fehlendem Skeletal Asset genutzt) folgt jetzt dem Kamera-Pitch - rein kosmetisch, die Treffer-Ermittlung selbst nutzt weiterhin direkt die Kamerarichtung.
- Fehlt ein zufällig ausgewähltes Skeletal Asset, wird jetzt der Reihe nach jede andere verfügbare Figur versucht (Spielfigur wie KI-Passanten) - das selbstgebaute Fallback-Rig bleibt nur noch reserviert für den Fall, dass wirklich keine der Figuren lädt.
- Das Fallback-Rig trägt nicht zu klassischen Mesh-Distanzfeldern bei - eine echte Engine-Grenze: `UProceduralMeshComponent` unterstützt grundsätzlich keine gebackenen Distanzfelder (keine `bAffectDistanceFieldLighting`-Eigenschaft, kein SDF-Asset wie bei Static Meshes), ohne die prozedurale Natur des Fallback-Rigs komplett aufzugeben nicht behebbar.

### Aufgaben

Es gibt eine Art Aufgabe: Lieferaufträge, dazu Polizei und Fahndung (siehe oben). Das Geld wird gespeichert und lässt sich für Waffen und Lack ausgeben; Fahrzeuge kaufen, Garagen oder weitere Läden gibt es noch nicht. Ort, Fahndung und laufender Auftrag beginnen bei jedem Start neu.

### Bildrate am Startpunkt

Zu Fuß am Startpunkt beim Klinikum fällt die Bildrate auf dem Entwicklungsrechner (integrierte AMD-Radeon-PRO-Grafik) auf 2–3 fps; im Wagen und an anderen Orten der Stadt sind es 25–30 fps. Gemessen beim Polizeitest, unabhängig von Polizei und Karte (beides abgeschaltet: gleiche Bildrate). Die Ursache ist nicht untersucht; die Blickrichtung über das offene Feld auf die Stadt liegt nahe.

***

## Zusammenarbeit mit KI-Assistenten

Codex und Claude arbeiten beide im selben Projekt, verfügen aber über **keine automatische Nachrichtenverbindung**.

Daher gilt:

1. Vor jeder Änderung an einer bestehenden Datei den aktuellen Inhalt lesen.
2. Änderungen anderer Assistenten nicht überschreiben oder stillschweigend zurücksetzen.
3. Im Arbeitsbericht klar dokumentieren:
   - welche Dateien geändert wurden,
   - welche bestehende Logik berücksichtigt wurde,
   - welche Auswirkungen oder offenen Punkte bestehen.
4. Importläufe und Spielläufe koordinieren.
5. Niemals `UnrealEditor-Cmd.exe -run=LaLaBergImport` parallel zu einem laufenden `UnrealEditor.exe -game` starten.

Empfohlene Berichtsvorlage:

```markdown
## Änderung

- Geänderte Datei: `Source/LaLaBerg/LaLaBergVerkehrsauto.cpp`
- Vorheriger Stand gelesen: Ja
- Bestehende Änderung anderer Assistenten berücksichtigt: Ja
- Änderung: Bremsdistanz vor Ampeln angepasst.
- Auswirkungen: Nur KI-Autos betroffen; Fahrtest und Ampeltest erneut ausführen.
- Offene Punkte: Verhalten an mehrarmigen Kreuzungen prüfen.
```

***

## Lizenzen und Datenquellen

Der Quellcode, die Konfiguration, Werkzeuge und selbst erzeugten Materialien stehen unter der [MIT-Lizenz](LICENSE).

### OpenStreetMap

Straßen, Plätze, Namen, Gebäudeumrisse und Bäume stammen aus OpenStreetMap.

- Copyright: © OpenStreetMap-Mitwirkende
- Lizenz: [ODbL 1.0](https://opendatacommons.org/licenses/odbl/)
- Abgeleitete Daten in `Content/SourceData` und Stadt-Assets in `Content/City` stehen ebenfalls unter ODbL.

### Amtliche LoD2-Gebäude

Gebäudehöhen und Dachformen basieren auf LoD2-Daten der Bayerischen Vermessungsverwaltung.

- Lizenz: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)
- Die Daten wurden für GTALaLaBerg bearbeitet.

### Schmalzturm

Die Beschreibung orientiert sich an der [Wikipedia-Seite zum Schmalzturm](https://de.wikipedia.org/wiki/Schmalzturm_(Landsberg_am_Lech)).

- Die farbigen Helm-Bänder sind eine visuelle Annäherung.
- Sie stellen keine verbindliche historische Rekonstruktion dar.

### CarConcept

Das Fahrzeugmodell `Car Concept` liegt unter:

```text
Content/SourceData/Vehicles
Content/Art/Vehicles
```

- Lizenz: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)
- Zusätzliche Hinweise: `Content/SourceData/Vehicles/CarConcept-LICENSE.md`

### City Sample Vehicles

Die 13 zusätzlichen Fahrzeugtypen stammen aus Epics kostenlosem Fab-Paket **City Sample Vehicles**.

- Lizenz: Fab Content License
- Nur innerhalb eigener Unreal-Engine-Projekte nutzbar
- Nicht Bestandteil dieses Repositories
- Kein freies Weitergeben der Rohassets
- Bei fehlendem Ordner `Content/CitySampleVehicles` wird automatisch CarConcept als Fallback verwendet

### Fototexturen (ambientCG)

Dächer, Plätze, Wiesen und das Putzkorn der Fassaden stammen aus Fotomaterialien von [ambientCG](https://ambientcg.com) unter **CC0 1.0** (gemeinfrei, keine Namensnennung nötig): `RoofingTiles011A`, `PavingStones051`, `Grass004`, `PaintedPlaster017`, jeweils 1K-JPG. Im Repo liegen nur die daraus abgeleiteten Modulatoren und Tiefenkarten (`Tools/Texturen/T_*_D.png` und `T_*_N.png`, erzeugt von `Tools/baue_fototexturen.py`), nicht die Originalpakete. Die Tiefenkarten (Normalmaps, DirectX-Ausrichtung) hängen in denselben Materialien: Fassaden über ihre Texturkoordinaten, Dach, Pflaster und Wiese weltbezogen (`WorldAlignedNormal`) - im streifenden Licht bekommen Putz, Ziegel und Steine damit echtes Relief statt einer glatten Fläche.

### Waffenmodelle

```text
Content/SourceData/Waffen
Content/Art/Waffen
```

- Quelle: Quaternius, Toon Shooter Game Kit
- Lizenz: [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/)
- Detailhinweise: `Content/SourceData/Waffen/LIZENZ.md`

### NPC-Modelle

```text
Content/SourceData/People
Content/Art/People
```

- Farmer, Casual, Worker: Quaternius, Ultimate Modular Men Pack
- Lizenz: [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/)
- Detailhinweise: `Content/SourceData/People/LIZENZ.md`

***

## Git LFS

Unreal-Assets und große Datenbestände werden über Git LFS verwaltet.

```text
.uasset
.umap
große JSON-Dateien
```

Nach jedem frischen Clone ist zwingend auszuführen:

```powershell
git lfs pull
```

Prüfen, ob LFS verfügbar ist:

```powershell
git lfs version
```

Falls Git LFS noch nicht installiert ist:

```powershell
git lfs install
```

***

## Roadmap

Mögliche nächste Ausbaustufen:

- bessere LOD-Übergänge für Fahrzeuge
- dynamische Echtzeit-Skylight-Capture oder echte Nacht-Cubemap
- stabiler Chaos-Vehicle-Physics-Ansatz mit Skeletal Vehicle Mesh
- Spurbreiten aus Straßendaten ableiten
- erweiterte Kreuzungslogik mit Warteschlangen
- realistischere Fahrzeugnavigation und Abbiegeverhalten
- weichere Fußgängeranimationen und Verhaltenszustände
- weitere Stadtmöblierung und Interaktionen
- Profiling auf RTX- und Gaming-Hardware
- weitere Auftragsarten und Läden, Fahrzeuge kaufen

***

## Hinweise

GTALaLaBerg ist ein technisch-kreatives Projekt zur Visualisierung, Erkundung und spielerischen Interaktion mit einem digitalen Stadtmodell von Landsberg am Lech. Es besteht keine Verbindung zu Rockstar Games, Grand Theft Auto oder deren Markeninhabern.

Die Nutzung der Bezeichnung „GTA" im Projektnamen beschreibt die spielerische Open-World-Inspiration und stellt keine offizielle Zugehörigkeit dar.
