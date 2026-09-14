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
- **Leben:** 371 Passanten als gebaute Stadtgeometrie, dazu 1078 geparkte Autos, 70 fahrende KI-Autos, 90 KI-Passanten und 44 Ampeln an amtlichen Standorten (OSM `highway=traffic_signals`) als eigenständige Akteure (`Tools/Export/prepare-verkehr.cjs`). Die geparkten Autos stehen ohne Route an ihrem amtlichen Stellplatz (dieselben Daten, die früher als Kasten-Geometrie ins Stadt-Mesh gebacken waren), lassen sich aber wie jedes fahrende KI-Auto übernehmen. Die fahrenden KI-Autos fahren ihre Straße ab und zurück, bremsen und weichen seitlich aus vor einem Auto oder dem Spieler voraus und vor Rot; die KI-Passanten gehen ihren Gehweg ab und zurück und bremsen/weichen vor einem anderen Passanten oder dem Spieler - mit einem echten, lizenzierten Skeletal Mesh samt Idle-/Walk-Animation (CC0, Quaternius), fehlen die Assets in einem Checkout, fällt jede Figur auf das von Hand gebaute Gelenk-Rig aus Hüfte, Knie und Schulter zurück (siehe „Bekannte Grenzen“).
- **Fahrzeugvielfalt:** Der fahrbare Wagen und alle KI-Autos (fahrend wie geparkt) wählen zufällig eines von 14 lizenzierten Modellen - das CarConcept-Modell (CC BY 4.0) oder einen von 13 realistischen Fahrzeugtypen aus Epics kostenlosem „City Sample Vehicles“-Paket (Fab, siehe „Datenquellen und Lizenzen“): Vans, mehrere PKW-Formen, Busse, Lkw. Jedes Modell zeigt Türen, Fenster, Scheinwerfer, Rückleuchten, Felgen statt einer Kasten-Silhouette. Nur nah am Spieler (< 80 m): weiter entfernte KI-Autos zeigen ein leichtes Fern-Mesh, sonst brach die Bildrate mit vielen Detail-Autos gleichzeitig ein (`ALaLaBergAutoPool`, eine Instanced-Static-Mesh-Instanz je Wagen und Teil statt einer eigenen Komponente, je Fahrzeugtyp ein eigener Pool-Satz - siehe `LaLaBergWagenTypen`). Derselbe Trick für den CarConcept-Kasten: bei 1078 geparkten Autos kostete je ein eigenes Netz ebenso viele Draw-Calls und drückte die Bildrate von 26 auf 17 fps - `ALaLaBergKastenPool` hält stattdessen eine Instanced-Static-Mesh-Instanz je Lackfarbe (siehe „Bekannte Grenzen“).
- **Jedes Auto ist nehmbar:** Steht kein fahrbarer Wagen in der Nähe, wird das nächste KI-Auto (fahrend oder geparkt) an Ort und Stelle zu einem fahrbaren Wagen (das KI-Auto verschwindet dafür) - wie in einem echten GTA lässt sich jedes Auto auf der Straße oder am Stellplatz stehlen, nicht nur das eine dafür vorgesehene.
- **Fahrbarer Wagen:** Federung über vier Strahlen, Antrieb bis etwa 120 km/h, Bremse und Parkbremse. Er steht beim Start auf der nächsten freien Straße am Klinikum.
- **Spielfigur in dritter Person:** Kamera an einem Federarm hinter/über der Figur statt in der Ego-Perspektive. Bevorzugt dasselbe echte, lizenzierte Skeletal Mesh wie die KI-Passanten (CC0, Quaternius) - fehlen die Assets in einem Checkout, fällt die Figur auf den handgebauten Gelenk-Rig zurück (Hüfte/Knie/Schulter, Gang ans Tempo gekoppelt). Die Waffe hängt am rechten Arm statt frei an der Kamera zu schweben, in beiden Fällen an derselben Stelle - der (dann unsichtbare) Kasten-Rig bleibt als Aufhängepunkt bestehen, auch wenn das Skelett sichtbar ist.
- **Waffen – ein abgerundetes Spektrum, alle mit Paintball statt Geschossen:** Pistole, MP (Dauerfeuer), Schrotflinte (7 Kugeln im Streukegel) und ein Werfer (langsam, große Wucht, großer Klecks). Ein Treffer hinterlässt einen Farbklecks (Decal) an der Trefferstelle, der mit der getroffenen Fläche mitfährt/-geht; die Fläche selbst behält ihre Farbe. Trifft er den fahrbaren Wagen, einen KI-Wagen oder einen KI-Passanten, bremst der kurz bzw. stolpert, ohne sich umzufärben.
- **Einblendungen:** Tacho, „E – Einsteigen“ neben einem Wagen, Fadenkreuz mit Waffenname und Schusszahl, eine Tastenleiste und die Ortsanzeige. Die zeigt links unten den Platz, das Wahrzeichen oder die Straße, darunter Ortsteil und Stadt (263 Straßen, 12 Plätze, 40 Wahrzeichen).
- **Menü:** Startbild, Pause, Einstellungen für Auflösung, Fenstermodus, Grafikstufe 0–3 und Lautstärke.

## Steuerung

| Zu Fuß | Im Wagen |
|---|---|
| WASD gehen, Maus umsehen | W/S Gas und rückwärts, A/D lenken |
| Leertaste springen | Leertaste (halten) bremsen |
| Maus links (halten) feuern | – |
| 1–4 Waffe wählen (Pistole/MP/Schrotflinte/Werfer) | – |
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
Tools/Export/prepare-verkehr.cjs                 → Content/SourceData/Verkehr/verkehr.json (Wegpunkte für KI-Autos/-Passanten, Ampelstandorte)
Tools/baue_farbklecks.py                         → M_Farbklecks (Decal-Material für Paintball-Treffer)
Tools/baue_waffenmetall.py, Tools/erzeuge_waffentextur.py → M_Waffenmetall (Material der Waffen-Ansichtsmodelle)
Tools/importiere_waffen.py                       → /Game/Art/Waffen (lizenzierte Waffenmodelle, CC0)
Tools/pruefe_waffenmasse.py                      → Diagnose: Bounding Box je Waffen-Mesh ins Log (kein Content-Ergebnis)
Tools/pruefe_carconcept.py                       → Diagnose: Bounding Box je CarConcept-Teilnetz ins Log (kein Content-Ergebnis)
Tools/importiere_npc.py                          → /Game/Art/People (NPC-Figur "Farmer": Skeletal Mesh + Animationen)
Tools/pruefe_npc_assets.py, pruefe_npc_skelette.py, pruefe_npc_anim.py → Diagnose: NPC-Assets/Skelett-Zuordnung/Animationslaenge ins Log
Tools/erzeuge_sounds.py, Tools/importiere_sounds.py → /Game/Audio (Schuss-, Einschlag-, Schritt- und Motorsound)
```

Die Autoform steht an genau einer Stelle, in `Tools/Export/wagen-form.cjs`. Alle KI-Autos (fahrend wie geparkt, über `wagen.json` zur Laufzeit) und der fahrbare Wagen entstehen daraus. Wer die Form ändert, lässt `prepare-wagen.cjs` neu laufen.

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
| `-LaLaBergWaffentest` | Rüstet nacheinander alle vier Waffenarten aus, je ein Bild des Ansichtsmodells direkt nach dem Ausrüsten (deckte die falsch gedrehte Werfer-Muendung auf, die im gemeinsamen Abschlussbild unterging), dann Schuss auf den fahrbaren Wagen und auf eine Hauswand: `LALABERG_WAFFENTEST PASS` ab einem gezählten Treffer |
| `-LaLaBergVerkehrFoto` | Teleportiert zum ersten KI-Auto und zum ersten KI-Passanten, je ein Bild: `LALABERG_VERKEHRFOTO PASS autos=70 passanten=90` (Ampelzahl steht in `LALABERG_VERKEHR`) |
| `-LaLaBergLechFoto` | Blick über den Lech aus der Luft, prüft `M_Lech` im Bild |
| `-LaLaBergKoerperFoto` | Waagerechte Kamera auf freiem Feld statt des schräg auf den Wagen gerichteten Waffentests - prüft Körper/Arm/Waffe der Spielfigur ohne den verzerrenden Blickwinkel |
| `-LaLaBergAmpelTest` | Prüft über einen vollen Ampelzyklus, ob zwei Ampeln derselben Kreuzungsgruppe je gleichzeitig Grün zeigen: `LALABERG_AMPELTEST PASS verstoesse=0 ampeln=44` |

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

- **Lumen aktiv, aber kein Tag-Nacht-Wechsel:** Seit die Stadt aus echten Nanite-StaticMeshes besteht (nicht mehr als ProceduralMesh zur Laufzeit), liefert Lumen über den Nanite-Surface-Cache saubere globale Beleuchtung ohne klassische Mesh-Distanzfelder (`r.DynamicGlobalIlluminationMethod=1`, `r.ReflectionMethod=1` in `DefaultEngine.ini`, per `-LaLaBergFoto` bestätigt). Nur die verbliebenen ProceduralMeshComponents (Spielfigur, KI-Passanten-Kasten-Rig, CarConcept-Kasten-Fallback) liefern selbst keine Distanzfelder und tragen nichts zur Verschattung bei - ein kleinerer Kompromiss. Das Himmelslicht bleibt eine feste Cubemap, weil die Echtzeit-Aufnahme in dieser Welt kein Licht liefert; ein Tag-Nacht-Wechsel braucht deshalb weiterhin Arbeit.
- **Skelett-Mesh bei Spielfigur und KI-Passanten, aber nur eine Kleidung:** Dasselbe lizenzierte, echt geriggte CC0-Modell (Quaternius, siehe „Datenquellen und Lizenzen“) mit echter Idle-/Walk-Animation ersetzt den Kasten-Rig bei der Spielfigur und allen KI-Passanten - aber nur die eine importierte Figur ("Farmer"), alle sehen also gleich aus (keine vorbereiteten Farbvarianten im Paket, nur einzeln benannte Material-Slots ohne Alternativen). Fehlen die Assets in einem Checkout, fällt jede Figur auf den von Hand gebauten Gelenk-Rig zurück (`LaLaBergKoerperTeile.h`, keine UAnimSequence). Der rechte Arm der Spielfigur haelt die Waffe in einer festen Pose, ohne sich beim Zielen zu beugen. Waffen und Fahrzeuge sind eigene, lizenzierte Meshes (CC0/CC BY 4.0/Fab). Kein Missionssystem.
- **Fahrzeug-Detailgrad ist ein Sichtweiten-Kompromiss, kein Filmqualitäts-Fuhrpark:** Das CarConcept-Modell ist ein handmodelliertes, aber kein fotoreales Asset - für Detailgrad auf dem Niveau eines aktuellen Rennspiels bräuchte es eigens angefertigte, hochauflösende Fahrzeugmodelle mit Textur-Atelier, das übersteigt diesen Rahmen. Zusätzlich zeigen nur Autos innerhalb von 80 m um den Spieler das Detailmodell (`LOD_ABSTAND` in `LaLaBergVerkehrsauto.cpp`) - alle KI-Autos gleichzeitig im Detailmodell drückten die Bildrate im Test auf 2 fps. Weiter entfernte Autos springen beim Über-/Unterschreiten der Schwelle sichtbar zwischen Kasten und Detailmodell um, es gibt keine weichen Zwischenstufen. Der Kasten selbst ist bei den 1078 geparkten Autos ebenfalls gepoolt (`ALaLaBergKastenPool`, eine Instanz je Lackfarbe statt eines eigenen Netzes je Auto) - ohne diesen Pool fiel die Bildrate im Fahrtest von 26 auf 17 fps.
- **Ampeln ohne echten OSM-Kreuzungsgraph, aber echte Vorfahrt an unsignalisierten Kreuzungen:** `Tools/Export/prepare-verkehr.cjs` gruppiert die 44 Ampeln über den Abstand (45 m) zu 17 Kreuzungen und teilt jede Gruppe in zwei Phasen anhand der Fahrbahnachse (`gier`) – ungefähr gleiche Achse = dieselbe Phase, ungefähr senkrechte Achse = die andere. Innerhalb einer Kreuzung schaltet nie mehr als eine Phase gleichzeitig auf Grün (`LaLaBergAmpel::SetzeGruppe`, geprüft über `-LaLaBergAmpelTest`: 0 Verstöße über einen vollen Zyklus). Das ist ein geometrisches Näherungsverfahren, keine echte OSM-Kreuzungsrelation – bei ungewöhnlichen Kreuzungsformen (mehr als zwei Achsen, sehr lange Kreuzungen) kann die Zweiteilung danebenliegen. Abseits der Ampeln nutzt `prepare-verkehr.cjs` dagegen den echten Straßengraphen aus den Quelldaten (`city.graph`, Knoten mit ≥ 3 angeschlossenen Straßen = wirkliche Kreuzung, 1726 Stück) samt amtlicher Straßenklasse (`roads[].c`) für echtes Vorfahrtsrecht: ein KI-Auto von der unwichtigeren Straße bremst, wenn ein anderes von der wichtigeren naht, bei gleicher Klasse gilt Rechts-vor-Links (`ALaLaBergVerkehrsauto::BremseVorKreuzung`). Keine echte Ankunftsreihenfolge und kein Anhalten-und-Warten-bis-frei, nur ein Vorrang-Bremsen wie bei den anderen Bremsfunktionen auch – und keine Fahrspur-Zuordnung, eine breite Kreuzung mit mehr als vier Armen kann danebenliegen.
- **Keine Straßenführung, kein Fahrspur-Bewusstsein:** Der fahrbare Wagen hält keine Spur. KI-Autos und KI-Passanten bremsen vor einem gleichartigen Hindernis voraus und vor dem Spieler (zu Fuß oder im Wagen); steht das Hindernis, weichen sie inzwischen mit einem festen seitlichen Versatz nach rechts aus (sanft ein- und wieder ausgeblendet) statt nur anzuhalten – vor einer roten Ampel bleibt es dagegen beim Anhalten. Der Versatz kennt weder eine echte Fahrspur noch prüft er, ob rechts daneben überhaupt noch Platz ist.

## Datenquellen und Lizenzen

Code, Konfiguration, Werkzeuge und Materialien stehen unter der [MIT-Lizenz](LICENSE). Die Stadtdaten nicht – für sie gilt:

- **Straßen, Plätze, Namen, Gebäudeumrisse, Bäume:** © OpenStreetMap-Mitwirkende, [ODbL 1.0](https://opendatacommons.org/licenses/odbl/). Die daraus abgeleiteten Stadtdaten in `Content/SourceData` und die Stadt-Assets in `Content/City` stehen ebenfalls unter der ODbL.
- **Gebäudehöhen und Dachformen (LoD2):** Bayerische Vermessungsverwaltung – [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/); Daten für GTA LaLaBerg bearbeitet.
- **Schmalzturm:** Beschreibung nach [Wikipedia](https://de.wikipedia.org/wiki/Schmalzturm_(Landsberg_am_Lech)); die Farbbänder des Helms sind eine Annäherung.
- **Fahrzeugmodell „Car Concept“** (`Content/SourceData/Vehicles`, `Content/Art/Vehicles`): [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/), siehe `CarConcept-LICENSE.md` im selben Ordner.
- **„City Sample Vehicles“** (`Content/CitySampleVehicles`, 13 Fahrzeugtypen): Epic Games, kostenlos über Fab („Free For Life“) - anders als die übrigen Inhalte hier keine CC-Lizenz, sondern die Fab-Content-Lizenz (Nutzung innerhalb eigener Unreal-Engine-Projekte, keine freie Weitergabe der Rohdateien). Deshalb **nicht** Teil dieses Repos (`.gitignore`) - wer das Projekt klont, holt sich das Paket selbst kostenlos über den Fab-Reiter im Editor (Content Browser → Fab → „City Sample Vehicles“ suchen → „Zum Projekt hinzufügen“ → „Alles speichern“) und bekommt die Fahrzeugvielfalt dann automatisch dazu. Fehlt der Ordner, fällt jedes Auto auf das CarConcept-Modell zurück (`LaLaBergWagenTypen::BaueTeile` liefert dann einfach `false`).
- **Waffenmodelle** (`Content/SourceData/Waffen`, `Content/Art/Waffen`): [Public Domain (CC0 1.0)](https://creativecommons.org/publicdomain/zero/1.0/), Quaternius/Toon Shooter Game Kit, siehe `LIZENZ.md` im selben Ordner.
- **NPC-Figur „Farmer"** (`Content/SourceData/People`, `Content/Art/People`): [Public Domain (CC0 1.0)](https://creativecommons.org/publicdomain/zero/1.0/), Quaternius/Ultimate Modular Men Pack, siehe `LIZENZ.md` im selben Ordner.
- **Git LFS:** `.uasset`, `.umap` und die großen JSON-Dateien liegen in Git LFS. Nach dem Klonen `git lfs pull`.
