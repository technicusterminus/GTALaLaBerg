# GTA LaLaBerg in Godot 4.5

Landsberg am Lech als begeh- und befahrbare Stadt. Dies ist die Godot-Umsetzung;
die Quelldaten liegen weiterhin in `Content/SourceData` (Git LFS).

## Starten

```bash
git lfs pull --include="Content/SourceData/**"
python3 Tools/konvertiere_sektoren.py      # braucht numpy, erzeugt Godot/daten (~106 MB)
godot --path Godot                         # oder den Ordner Godot/ im Editor oeffnen
```

Rauchtest ohne Fenster (endet nach 25 s mit `LALABERG_RAUCHTEST` in der Ausgabe):

```bash
godot --path Godot --headless -- --rauchtest
```

## Steuerung

| Taste | Aktion |
|---|---|
| WASD / Pfeile | gehen bzw. fahren |
| Maus | Kamera |
| Shift, Leertaste | rennen, springen (im Auto: Handbremse) |
| F | ein-/aussteigen |
| Esc | Maus freigeben |

## Stand

Umgesetzt: Sektor-Streaming der Stadt (90 Sektoren, 2,4 Mio. Dreiecke) mit
Kollision, Spieler zu Fuss, ein fahrbares Auto (Karosserie aus `wagen.json`),
Ortsanzeige, Tempo und Bildrate.

Noch offen gegenueber der Unreal-Version: Verkehr, Passanten, Polizei,
Drehbuch und Missionen, Laeden und Konto, Waffen, Wasser, Radio, Menue.
Die Unreal-Teile (`Source/`, `Config/`, `*.uproject`, `Content/Art`, `Content/City`)
bleiben stehen, bis diese Etappe auf einem Rechner nachweislich laeuft.

## Hinweise

- Die Skripte sind mit `gdtoolkit` auf Syntax geprueft (`gdparse`, `gdlint`),
  aber noch nicht in Godot gestartet.
- Fahrzeuge brauchen die eingebaute Physik (`VehicleBody3D`), nicht Jolt.
- Fuer einen Export muessen `*.msh,*.col,*.json` im Export-Filter fuer
  Nicht-Ressourcen stehen, sonst fehlen die Stadtdaten im Build.
