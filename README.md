# GTALaLaBerg – Godot 4.5

> **Landsberg am Lech als begehbare und befahrbare Stadt im Maßstab 1:1.**
> Godot-Umsetzung mit amtlichen LoD2-Gebäudedaten, Laserscan-Gelände, OpenStreetMap-Straßennetz, Sektor-Streaming, KI-Verkehr, Polizei und einer eigenen Geschichte.

Das Projekt lag zuvor als Unreal-Engine-5.8-Projekt vor. Es wurde auf Godot umgestellt; die ausführliche
Unreal-Dokumentation steht unverändert in [`Docs/Unreal-Archiv/README-Unreal-Archiv.md`](Docs/Unreal-Archiv/README-Unreal-Archiv.md),
der Unreal-Stand in der Git-Historie (Commit `dea4cec`).

| Bezeichnung | Wert |
|---|---|
| Projekt | `Godot/project.godot` |
| Engine | Godot 4.5 |
| Stadt | 90 Sektoren à 500 × 500 m, 2,4 Mio. Dreiecke |
| Geschichte | „Farbkrieg“: 22 Hauptmissionen, 3 Enden |
| Quelldaten | `Content/SourceData` (Git LFS) |
| Lizenz für Code/Tools | MIT |
| Stadt- und Geodaten | ODbL / CC BY 4.0, siehe Lizenzabschnitt |

## Schnellstart

```bash
git lfs pull --include="Content/SourceData/**"
python3 Tools/konvertiere_sektoren.py      # braucht numpy, erzeugt Godot/daten (~106 MB)
godot --path Godot
```

Steuerung, Stand und Rauchtest stehen in [`Godot/README.md`](Godot/README.md).

## Stand

Umgesetzt (alles mit einem Rauchtest geprüft, siehe [`Godot/README.md`](Godot/README.md)):

- **Stadt und Verkehr:** Sektor-Streaming mit Kollision, Spieler und Auto, 70 KI-Autos,
  204 Passanten, 1078 geparkte Autos, 44 Ampeln, bremsendes Verkehrsverhalten
- **Kampf und Polizei:** vier Paintball-Waffen, Schaden und Krankenhaus, Fahndung mit
  bis zu fünf Sternen, Streifen, Straßensperre, Festnahme
- **Wirtschaft und Reviere:** sechs Auftragsarten, Paintball-Laden, Lackiererei,
  Konto und Spielstand, vier Reviere mit Wahrzeichen, Bewährung und Kopf der Mannschaft
- **Geschichte:** das Drehbuch **Farbkrieg** – Prolog, vier Akte und ein Finale mit drei
  Enden, 22 Hauptmissionen, 54 Szenen, 244 Zeilen Dialog, gespielt mit Kamerafahrten,
  Untertiteln und Entscheidungen. Lesbar in [`Docs/Drehbuch.md`](Docs/Drehbuch.md)

- **Sonderfahrzeuge:** ein Panzer (Kanone mit Großkaliber-Farbkugel) und ein Hubschrauber
  zum Einsteigen

- **Aussehen:** animierte Menschenfiguren (Quaternius, CC0), Autos in vier Karosserieformen
  und zwölf Lacken, Wiesenflecken und Erde am Hang im Gelände, bewegtes Wasser
- **Karte und Menü:** Minikarte, Vollkarte (M) und Pausenmenü (Esc)

Aus der Unreal-Version noch nicht portiert: Radio, Ton,
Schießbude, Farbdosen.

## Werkzeuge

| Pfad | Zweck |
|---|---|
| `Tools/konvertiere_sektoren.py` | Sektor-JSON und Daten → Godot-Binärdaten und Kopien |
| `Tools/drehbuch/` | schreibt und prüft das Drehbuch (`schreibe.py`), erzeugt `drehbuch.json` und `Docs/Drehbuch.md` |
| `Tools/SectorPipeline/` | Sektorbildung der Stadt (Node.js) |
| `Tools/Export/` | Aufbereitung von OSM-, LoD2-, Verkehrs- und Ortsdaten (Node.js) |
| `Tools/erzeuge_*.py`, `Tools/baue_*.py` (ohne Unreal) | Texturen, Sounds, Musik, Modelle |

## Lizenzen und Datenquellen

Der Quellcode, die Konfiguration, Werkzeuge und selbst erzeugten Materialien stehen unter der [MIT-Lizenz](LICENSE). Das gilt auch für die Geschichte „Farbkrieg“ (Figuren, Handlung, Dialoge in `Tools/drehbuch/`): Sie ist frei erfunden, ähnliche Namen sind Zufall; nur die Orte sind echt.

### OpenStreetMap

Straßen, Plätze, Namen, Gebäudeumrisse und Bäume stammen aus OpenStreetMap.

- Copyright: © OpenStreetMap-Mitwirkende
- Lizenz: [ODbL 1.0](https://opendatacommons.org/licenses/odbl/)
- Abgeleitete Daten in `Content/SourceData` und die daraus erzeugten Godot-Daten stehen ebenfalls unter ODbL.

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
```

- Lizenz: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)
- Zusätzliche Hinweise: `Content/SourceData/Vehicles/CarConcept-LICENSE.md`

### City Sample Vehicles

Die Unreal-Version nutzte zusaetzlich Epics Fab-Paket **City Sample Vehicles** (Fab Content License, nicht Teil dieses Repositories). Die Godot-Version verwendet es nicht.

### Fototexturen (ambientCG)

Dächer, Plätze, Wiesen und das Putzkorn der Fassaden stammen aus Fotomaterialien von [ambientCG](https://ambientcg.com) unter **CC0 1.0** (gemeinfrei, keine Namensnennung nötig): `RoofingTiles011A`, `PavingStones051`, `Grass004`, `PaintedPlaster017`, jeweils 1K-JPG. Im Repo liegen nur die daraus abgeleiteten Modulatoren und Tiefenkarten (`Tools/Texturen/T_*_D.png` und `T_*_N.png`, erzeugt von `Tools/baue_fototexturen.py`), nicht die Originalpakete. Die Tiefenkarten (Normalmaps, DirectX-Ausrichtung) hängen in denselben Materialien: Fassaden über ihre Texturkoordinaten, Dach, Pflaster und Wiese weltbezogen (`WorldAlignedNormal`) - im streifenden Licht bekommen Putz, Ziegel und Steine damit echtes Relief statt einer glatten Fläche.

### Waffenmodelle

```text
Content/SourceData/Waffen
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

Die großen Quelldaten (Stadtsektoren, Modelle, Karte) werden über Git LFS verwaltet:
`Content/SourceData/**` (JSON, PNG, FBX, GLB).

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
