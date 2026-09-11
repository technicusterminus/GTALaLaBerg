# P1 – Stadtsektoren

Stand: 07.09.2026 · Status: Datenstufe implementiert und geprüft; statische Unreal-Assets noch offen.

## Ergebnis

`Tools/SectorPipeline/sectorize-city.cjs` zerlegt `Content/SourceData/stadt.json` deterministisch in räumliche 500-m-Zellen. Ausgabe ist `Content/SourceData/Sectors/manifest.json` mit einer Datei je Sektor.

Letzte bestandene Prüfung:

- Quelle während des gesamten Laufs unverändert: ja.
- Quellen-SHA-256: `33186409F403FDF06AA50F1F124124E3FC2A16C538AFE2970876D910C4F69A9B`.
- Sektoren: 101, zuzüglich Manifestdatei.
- Dreiecke: 1.125.936; Ausgabe und Manifest vollständig konsistent.
- Größter Sektor: 2.494.329 Bytes statt einer Stadtdatei von rund 54 MB.
- Einheiten, Weltursprung, Höhenbasis, Materialklassen, Farben und vorhandene Fassaden-UVs bleiben erhalten.
- Dreiecke werden anhand ihres Schwerpunkts genau einem Sektor zugeordnet; Indexlisten werden lokal neu abgebildet.

Während der ersten Wiederholungsprüfung wurde `stadt.json` parallel verändert. Dieser Lauf wurde nicht als reproduzierbarer Nachweis verwendet. Die Pipeline prüft deshalb Dateigröße, Änderungszeit und SHA-256 unmittelbar vor Veröffentlichung. Ändert sich die Quelle während der Erzeugung, wird die neue Ausgabe verworfen und die vorherige veröffentlichte Ausgabe bleibt erhalten.

## Bewusste Grenze

Die Sektordateien sind weiterhin abgeleitete JSON-Daten. Sie verbessern noch nicht allein das sichtbare Spiel und sie sind keine statischen Unreal-Meshes. `LaLaBergGameMode.cpp` lädt aktuell weiterhin bevorzugt die monolithische `stadt.json` und erzeugt bewegliche `UProceduralMeshComponent`-Objekte.

Der nächste P1-Schritt ist ein Editor-Importer, der jeden Sektor in getrennte statische Unreal-Assets und anschließend eine partitionierte Produktionswelt überführt. Erst nach Build, Kollisionsprüfung und visueller Prüfung darf die bisherige Laufzeitstrecke abgelöst werden.

## Erneute Prüfung

Im Projektordner:

`node Tools\SectorPipeline\sectorize-city.cjs`

Danach muss das Manifest zur aktuellen Quelle passen und die Summe aller Sektor-Dreiecke exakt `triangleCount` ergeben. Ein grüner Datencheck ist kein Nachweis für korrekte Normalen, Materialien, Kollision oder Grafik.
