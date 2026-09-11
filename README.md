# GTALaLaBerg – Unreal 5.8

Landsberg am Lech im Maßstab 1:1 als begehbare und befahrbare Stadt. Dies ist das native Unreal-Projekt. Der Projektname und der vollständige Projektpfad enthalten **keine Leerzeichen** – das bleibt so.

- Projektdatei: `GTALaLaBerg.uproject`
- Editor-Buildtarget: `GTALaLaBergEditor`, Spiel-Buildtarget: `GTALaLaBerg`
- Der interne C++-Modulname ist `LaLaBerg`; das ist kein zweites Projekt.

Codex und Claude arbeiten beide in diesem Projekt. Zwischen den beiden Assistenten gibt es keine automatische Nachrichtenverbindung. Wer eine Datei des anderen ändert, liest vorher den aktuellen Stand und nennt die Änderung im Bericht.

**Import und Spiellauf nie gleichzeitig.** Ein laufendes Spiel (`-game`) hält die Stadt-Assets geöffnet. Ein gleichzeitiger `-run=LaLaBergImport` kann sie dann nicht ersetzen und meldet `LALABERG_STATIC_SAVE_FAILED`; am 11.09.2026 traf das drei Meshes in S_N2_N3. Vor einem Spiellauf deshalb prüfen, dass kein `UnrealEditor-Cmd.exe` läuft, und vor einem Import, dass kein `UnrealEditor.exe -game` läuft.

## Was drin ist

- **Die ganze Stadt:** 7207 Gebäude aus amtlichen LoD2-Daten, Laser-Gelände, Straßen, Plätze, Lech mit Wehr und 13 320 Bäume. Das Ganze sind 2060 statische Nanite-Meshes in 101 Sektoren zu je 500 m, geladen in etwa 6 s.
- **Altstadt nach Fotovorlage:** Putzfarben und Dachdeckung der Innenstadt, Sockel, Gesimse, Ladenzonen, Gauben, Türen, Brandmauern und in 11,5-m-Häuser geteilte Zeilen. Dazu Rathaus, Marienbrunnen und Hauptplatz sowie der Klinikum-Campus.
- **Leben:** 1078 geparkte Autos und 371 Passanten. Die Autos haben Radkästen, Felgen, Fenster mit Säulen, Scheinwerfer, Rückleuchten, Kennzeichen und Spiegel.
- **Fahrbarer Wagen:** Federung über vier Strahlen, Antrieb bis etwa 120 km/h, Bremse und Parkbremse. Er steht beim Start auf der nächsten freien Straße am Klinikum.
- **Einblendungen:** Tacho, „E – Einsteigen“ neben einem Wagen, eine Tastenleiste und die Ortsanzeige. Die zeigt links unten den Platz, das Wahrzeichen oder die Straße, darunter Ortsteil und Stadt (263 Straßen, 12 Plätze, 40 Wahrzeichen).
- **Menü:** Startbild, Pause, Einstellungen für Auflösung, Fenstermodus, Grafikstufe 0–3 und Lautstärke.

## Steuerung

| Zu Fuß | Im Wagen |
|---|---|
| WASD gehen, Maus umsehen | W/S Gas und rückwärts, A/D lenken |
| Leertaste springen | Leertaste (halten) bremsen |
| E einsteigen (bis 8 m Abstand) | E oder R aussteigen |
| Esc Menü | Esc Menü |

## Datenkette

```
Tools/Export/prepare-stadt.cjs                   → Content/SourceData/stadt.json
Tools/SectorPipeline/sectorize-city.cjs          → Content/SourceData/Sectors/*.json + manifest.json
UnrealEditor-Cmd … -run=LaLaBergImport -Sector=alle → /Game/City/Sectors/<id>/SM_<id>_<Klasse>
Tools/baue_texturen.py, Tools/baue_materialien.py → /Game/Art (Texturen, Materialien)
Tools/Export/prepare-orte.cjs                    → Content/SourceData/Orte/orte.json (Straßen-, Platz- und Ortsnamen)
Tools/Export/prepare-wagen.cjs                   → Content/SourceData/Fahrzeug/wagen.json (Form des fahrbaren Wagens)
```

Die Autoform steht an genau einer Stelle, in `Tools/Export/wagen-form.cjs`. Die geparkten Wagen (über `prepare-stadt.cjs`) und der fahrbare Wagen (über `wagen.json`) entstehen beide daraus. Wer die Form ändert, lässt beide Ausleitungen neu laufen.

Die Exporter brauchen die Rohdaten des Spielprojekts „GTA LaLaBerg“ (`data/citydata.js` usw.), die nicht in diesem Repo liegen. `LALABERG_QUELLE` zeigt auf dessen Wurzel, ohne Angabe wird der Nachbarordner angenommen. Einmalig `npm install` in `Tools/Export`.

Das Spiel lädt die Stadt aus den Assets. Liegen noch keine vor, baut es sie zur Laufzeit aus `stadt.json` – das ist langsamer, dient aber als Rückfall.

## Prüfläufe

Alle Prüfläufe starten mit `UnrealEditor.exe GTALaLaBerg.uproject -game -windowed -ResX=1600 -ResY=900 <Schalter> -log` und beenden sich selbst. Sie schreiben `LALABERG_…`-Zeilen in `Saved/Logs/GTALaLaBerg.log`, Bilder landen unter `Saved/Screenshots/WindowsEditor`.

Das gepackte Spiel nimmt dieselben Schalter: `GTALaLaBerg.exe -windowed -ResX=1600 -ResY=900 -LaLaBergFahrtest`. Shipping schreibt kein Log. Der Fahrtest hält Ergebnis, Bildrate und Programmlaufzeit deshalb zusätzlich in `Saved/Logs/LaLaBerg-Test.txt` fest; im Paket liegt die Datei unter `%LOCALAPPDATA%\GTALaLaBerg\Saved`.

| Schalter | Prüft |
|---|---|
| `-LaLaBergSmoke` | Die Stadt steht, die Figur steht auf Boden: `LALABERG_SMOKE PASS` |
| `-LaLaBergFahrtest` | Figur tritt an den Wagen (Bild mit Einblendung), steigt über die E-Taste ein und fährt vier Sekunden Vollgas (Bild mit Tacho): `LALABERG_FAHRTEST PASS` ab 8 m Weg auf allen Rädern |
| `-LaLaBergFoto` | Vier Ansichten: Hauptplatz, Straße, Wagen, Luftbild über dem Hauptplatz |
| `-LaLaBergHimmelEchtzeit` | Himmelslicht aus Echtzeit-Aufnahme statt fester Cubemap (siehe Grenzen) |
| `-LaLaBergGpu` | Zusammen mit `-LaLaBergFahrtest`: GPU-Zeiten je Renderschritt ins Log |

## Paket bauen

```
RunUAT.bat BuildCookRun -project=<Pfad>\GTALaLaBerg.uproject -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -iostore -archive -archivedirectory=<Zielordner> -prereqs
```

Das Ziel sollte außerhalb eines Cloud-Ordners liegen – das Paket ist mehrere hundert MB groß. Die Stadt und die Materialien werden nur über ihren Pfad geladen; `Config/DefaultGame.ini` zwingt sie deshalb in den Cook. Das Sektoren-Manifest wird als Datei mitgepackt.

## Leistung

Gemessen am 11.09.2026 im Fahrtest mit `-LaLaBergGpu` (`ProfileGPU` mitten in der Fahrt), 1600×900, auf der AMD Radeon PRO Graphics des Entwicklungsrechners:

| | GPU je Bild | davon Schatten |
|---|---|---|
| Kaskadenschatten (bis dahin) | 36,2 ms | 19,9 ms |
| Virtual Shadow Maps (jetzt) | 19,3 ms | 1,2 ms |

Die Kaskadenschatten rasterten die ganze Nanite-Stadt je Kaskade neu, viermal pro Bild. VSM sind dafür gebaut. Mit einer RTX 3060 wurde noch nicht gemessen.

## Bekannte Grenzen

- **Kein Lumen:** Die Beleuchtung nutzt SSGI und SSR statt Lumen. Das Himmelslicht ist eine feste Cubemap, weil die Echtzeit-Aufnahme in dieser Welt kein Licht liefert; die Ursache ist offen. Ein Tag-Nacht-Wechsel braucht deshalb noch Arbeit.
- **Einfache Wagen und Passanten:** Wagen und Passanten sind aus Querschnitten gebaut, keine Modelle mit Skelett. Es gibt keinen Verkehr und keine Missionen.
- **Keine Straßenführung:** Der Wagen hält keine Spur. Wer von der Straße fährt, fährt über die Wiese.

## Datenquellen und Lizenzen

Code, Konfiguration, Werkzeuge und Materialien stehen unter der [MIT-Lizenz](LICENSE). Die Stadtdaten nicht – für sie gilt:

- **Straßen, Plätze, Namen, Gebäudeumrisse, Bäume:** © OpenStreetMap-Mitwirkende, [ODbL 1.0](https://opendatacommons.org/licenses/odbl/). Die daraus abgeleiteten Stadtdaten in `Content/SourceData` und die Stadt-Assets in `Content/City` stehen ebenfalls unter der ODbL.
- **Gebäudehöhen und Dachformen (LoD2):** Bayerische Vermessungsverwaltung – [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/); Daten für GTA LaLaBerg bearbeitet.
- **Schmalzturm:** Beschreibung nach [Wikipedia](https://de.wikipedia.org/wiki/Schmalzturm_(Landsberg_am_Lech)); die Farbbänder des Helms sind eine Annäherung.
- **Git LFS:** `.uasset`, `.umap` und die großen JSON-Dateien liegen in Git LFS. Nach dem Klonen `git lfs pull`.
