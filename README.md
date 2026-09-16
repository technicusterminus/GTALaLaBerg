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

### Verkehrs- und Stadtleben

| Element | Anzahl |
|---|---:|
| Statische Passanten als Stadtgeometrie | 371 |
| Geparkte Fahrzeuge | 1.078 |
| Fahrende KI-Autos | 70 |
| KI-Passanten | 90 |
| Ampeln | 44 |

Die Verkehrslogik basiert auf vorbereiteten Wegen, OpenStreetMap-Straßendaten und Kreuzungsinformationen:

- KI-Autos fahren Strecken ab und kehren am Routenende um.
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

Der fahrbare Wagen selbst fährt aktuell über eine vereinfachte Federung aus vier Strahlen statt echter Chaos-Vehicle-Physik - siehe „Bekannte Grenzen" für den Stand eines begonnenen, aber nicht übernommenen Umbaus.

### Spielfigur

Die Spielfigur wird aus einer Third-Person-Kamera gesteuert.

- Kamera über Federarm hinter und über der Figur
- Bevorzugt echte CC0-Skeletal-Meshes von Quaternius
- Drei mögliche Figurenvarianten: `Farmer`, `Casual` und `Worker`
- Zufällige Farbvarianten für Kleidung und Körpergröße
- KI-Passanten verwenden dieselben Varianten
- Fallback auf ein selbstgebautes, animiertes Gelenk-Rig, falls die Skeletal Assets nicht verfügbar sind
- Die Waffe ist physisch am rechten Arm positioniert und schwebt nicht frei an der Kamera

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

### HUD und Menüs

Das HUD enthält:

- Tacho im Fahrzeug
- kontextuelle Einsteigeaufforderung `E – Einsteigen`
- Fadenkreuz
- aktiver Waffenname
- Schussanzahl
- Steuerungshinweise
- Ortsanzeige mit Straße, Platz oder Wahrzeichen
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
| `-LaLaBergFahrtest` | Einsteigen und vier Sekunden Vollgas | `LALABERG_FAHRTEST PASS` ab 8 m Weg auf allen Rädern |
| `-LaLaBergFoto` | Hauptplatz, Straße, Fahrzeug, Luftbild | Vier Screenshots |
| `-LaLaBergHimmelEchtzeit` | Test der Echtzeit-Himmelsaufnahme | Diagnose im Log/Bild |
| `-LaLaBergNacht` | Setzt die Tageszeit auf Mitternacht | Für Nachtbildtests |
| `-LaLaBergGpu` | GPU-Zeiten pro Renderphase | Profilingdaten im Log |
| `-LaLaBergWaffentest` | Alle Waffen plus Treffer auf Fahrzeug und Wand | `LALABERG_WAFFENTEST PASS` |
| `-LaLaBergVerkehrFoto` | Screenshot von erstem KI-Auto und KI-Passanten | `LALABERG_VERKEHRFOTO PASS autos=70 passanten=90` |
| `-LaLaBergLechFoto` | Luftaufnahme über dem Lech | Prüft `M_Lech` |
| `-LaLaBergKoerperFoto` | Prüft Figur, Arm und Waffenhaltung | Screenshot |
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

Im installierten Paket liegt dieser Pfad typischerweise unter:

```text
%LOCALAPPDATA%\GTALaLaBerg\Saved\Logs\LaLaBerg-Test.txt
```

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

Ein Umbau auf echte Chaos-Vehicle-Physik wurde begonnen, jedoch nicht übernommen - der fahrbare Wagen läuft weiterhin über die ursprüngliche Federstrahl-Physik (vier Strahlen).

Untersucht wurde `UChaosWheeledVehicleMovementComponent` mit:

- Motor- und Getriebekennfeldern
- Vorderradlenkung
- Hinterradantrieb
- Radkontaktprüfung
- Federkompression
- Federkraft von ungefähr 125.000 N
- Reifenreibung von 0,70
- Antriebsmoment von ungefähr 986 Nm

Die Radmesswerte waren plausibel, die Karosserie beschleunigte jedoch nicht. Auch eine Verzehnfachung des Drehmoments änderte daran nichts.

Zusätzliche Engine-Probleme mussten bereits umgangen werden:

- `CanCreateVehicle()` verlangt in UE 5.8 offenbar einen Bone-Namen pro Rad, selbst wenn kein Skeletal Mesh verwendet wird.
- Die Radgeometrie musste an die Rumpf-Kollisionsbox angepasst werden.
- Die Ursache der fehlenden Beschleunigung liegt vermutlich tiefer im experimentellen ChaosVehiclesPlugin oder in einer nicht verbundenen Komponente des Setups.

Eine vorbereitete Community-Anfrage befindet sich unter:

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

- Der Spurversatz der KI-Autos richtet sich jetzt nach der echten Fahrbahnbreite aus den Straßendaten (`verkehr.json`, Feld `w`) statt nach einem für jede Straße gleichen Festwert - auf schmalen Straßen bleibt entsprechend weniger Platz zum Ausweichen, ohne über den Fahrbahnrand hinauszufahren. Kein Spurwechselmodell und keine mehrspurige Fahrspurwahl innerhalb einer Fahrtrichtung.
- Kein Queue-System pro Kreuzung.
- Der Anhalteabstand an einer Kreuzung richtet sich jetzt nach deren geschätzter tatsächlicher Breite (aus der Straßenklasse der wichtigsten angeschlossenen Straße, kalibriert an echten Breitendaten) statt nach einem für jede Kreuzung gleichen Festwert - an einer breiten oder mehrarmigen Kreuzung steht ein wartendes Auto entsprechend weiter vom Mittelpunkt entfernt. Keine präzise Fahrspurgeometrie für Abbiegevorgänge selbst - ein Auto folgt weiterhin der Straßenmitte durch die Kreuzung statt einer echten Abbiegekurve.
- Die Ampel-Phasenlogik teilt eine Kreuzung jetzt in so viele nicht überlappende Zeitfenster wie sie tatsächlich unterschiedliche Fahrbahnachsen hat (statt fest zwei Phasen für vier Arme) - eine breite Kreuzung mit mehr als vier Armen bekommt entsprechend mehr Phasen, statt eine ihrer Achsen fälschlich mit einer anderen zu teilen.
- Einzelfälle können trotz Graph-Gruppierung in getrennten Ampelgruppen landen.
- Der fahrbare Wagen besitzt aktuell keine Spurhalteassistenz oder Fahrspurbindung.

### Figuren

- Der rechte Arm hält die Waffe aktuell in einer festen Pose.
- Das Arm-Rig reagiert nicht dynamisch auf Zielrichtung.
- Fehlt ein zufällig ausgewähltes Skeletal Asset, wird nicht auf eine andere verfügbare Figur ausgewichen, sondern auf das selbstgebaute Fallback-Rig zurückgegriffen.
- Das Fallback-Rig trägt nicht zu klassischen Mesh-Distanzfeldern bei.

### Kein Missionssystem

Das Projekt enthält derzeit kein Missions-, Quest-, Polizei-, Economy- oder Persistenzsystem. Der Fokus liegt auf Stadtmodell, Exploration, Interaktion, technischer Pipeline und Performance.

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
- Arm-IK und Zielanimation für die Spielfigur
- weichere Fußgängeranimationen und Verhaltenszustände
- weitere Stadtmöblierung und Interaktionen
- Profiling auf RTX- und Gaming-Hardware
- optionale Missions- oder Sandbox-Systeme

***

## Hinweise

GTALaLaBerg ist ein technisch-kreatives Projekt zur Visualisierung, Erkundung und spielerischen Interaktion mit einem digitalen Stadtmodell von Landsberg am Lech. Es besteht keine Verbindung zu Rockstar Games, Grand Theft Auto oder deren Markeninhabern.

Die Nutzung der Bezeichnung „GTA" im Projektnamen beschreibt die spielerische Open-World-Inspiration und stellt keine offizielle Zugehörigkeit dar.
