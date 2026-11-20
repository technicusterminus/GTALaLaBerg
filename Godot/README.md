# GTA LaLaBerg in Godot 4.5

Landsberg am Lech als begeh- und befahrbare Stadt mit einer Geschichte: dem
**Farbkrieg**. Dies ist die Godot-Umsetzung; die Quelldaten liegen in
`Content/SourceData` (Git LFS), die Geschichte in `Tools/drehbuch`.

## Starten

```bash
git lfs pull --include="Content/SourceData/**"
python3 Tools/konvertiere_sektoren.py      # braucht numpy, erzeugt Godot/daten (~106 MB)
godot --path Godot                         # oder den Ordner Godot/ im Editor öffnen
```

Die Hauptgeschichte beginnt von selbst: Du wachst im Klinikum auf.

## Steuerung

| Taste | Aktion |
|---|---|
| WASD / Pfeile | gehen bzw. fahren |
| Maus | Kamera; linke Maustaste schießt |
| Shift, Leertaste | rennen, springen (im Auto: Handbremse) |
| 1 bis 4 | Waffe wählen (Pistole, MP, Schrotflinte, Werfer; Letztere zu kaufen) |
| F | ein- und aussteigen; ein KI-Auto zu nehmen ist Diebstahl |
| ↑ ↓ Enter | im Laden wählen und kaufen |
| Enter, Leertaste, Klick | im Film zur nächsten Zeile |
| 1 bis 3 | im Film eine Entscheidung treffen |
| Esc | Maus freigeben |

## Was drin ist

**Stadt.** 90 Sektoren mit 2,4 Mio. Dreiecken, nach Entfernung nachgeladen, mit
Kollision (`stadt.gd`). Spieler zu Fuß, fahrbares Auto, Kamera, HUD mit Ortsanzeige.

**Verkehr.** 70 KI-Autos auf Rundkursen, 204 Passanten (Kapseln), 1078 geparkte Autos
je Sektor als MultiMesh, 44 Ampeln mit Phasenplan. KI-Autos bremsen vor Autos,
Rot, Kurven, dem Spieler und wartepflichtigen Kreuzungen (`verkehrsbremse.gd`).

**Kampf und Folgen.** Vier Paintball-Waffen, Farbkugeln mit Klecks und Wirkung im
Umkreis, Leben für Spieler, Passanten und Autos, Sturz- und Anprallschaden,
Krankenhaus mit Behandlungskosten. Die Polizei (`polizei.gd`) fahndet mit bis zu fünf
Sternen, schickt Streifen über den Straßengraphen, sucht, sperrt ab drei Sternen die
Straße und nimmt fest.

**Sonderfahrzeuge.** `sonderfahrzeug.gd`: ein Panzer (W/S fahren, A/D drehen, der Turm
folgt dem Blick, Maus links feuert alle drei Sekunden eine Großkaliber-Farbkugel) und ein
Hubschrauber (Leertaste steigen, W/S fliegen, A/D drehen). Beide stehen in der Stadt und
werden mit F bestiegen; Aussteigen nur am Boden.

**Wirtschaft.** Aufträge in sechs Arten (Lieferung, Taxi, Rennen, Verfolgung,
Krankenwagen, Streife), Paintball-Laden, Lackiererei, ein Konto mit Spielstand, Werten
(Ausdauer, Zielsicherheit, Fahren, Ruf), Fundstücken und Merkern.

**Ton und Radio.** `ton.gd` spielt die Klänge aus `Tools/Sounds` (Motor nach Tempo, Schuss je
Waffe, Klecks, Sirene der Streifen, Rotor, Panzerdiesel, Schritte). `radio.gd` ist das
Autoradio: R schaltet Aus, Blasmusik, Klassik, Lech. Die Musik erzeugt
`Tools/erzeuge_radio.py` mit Googles Lyria 3 über OpenRouter (Schlüssel
`OPENROUTER_API_KEY`, ein Song etwa 0,08 USD) nach `Godot/daten/radio`; ohne die Dateien
laufen die gerechneten Schleifen.

**Schießbude.** `schiessbude.gd`: wer zu Fuß in den orangen Ring tritt, startet eine Runde
von 30 Sekunden. Sechs Scheiben klappen hoch (höchstens drei zugleich); jede Paintball-Kugel,
die eine trifft, zählt 20 €. Die Bestleistung steht im Konto.

**Farbdosen.** `farbdosen.gd` stellt fünfzig Dosen mit festem Zufall in das Straßennetz
(mindestens 90 m auseinander); Einsammeln bringt 25 €, jede zehnte einen Staffelpreis.

**Reviere.** Vier Reviere mit je vier Wahrzeichen-Säulen, Bewährung, Ruf und einem Kopf
der Mannschaft, den man stellen muss (`revier.gd`).

**Drehbuch.** `drehbuch.gd` führt Missionen aus Stufen aus; `kino.gd` spielt Szenen mit
Kamerafahrten, Letterbox, Untertiteln, Figuren, Titelkarten, Funk und Entscheidungen.
Die Hauptgeschichte hat 22 Missionen, 54 Szenen, 244 Zeilen Dialog und drei Enden; dazu
kommen 22 Nebenaufträge der Mannschaften. Das lesbare Skript steht in
[`Docs/Drehbuch.md`](../Docs/Drehbuch.md).

## Die Geschichte ändern

Das Drehbuch entsteht aus Python-Modulen in `Tools/drehbuch/` (nicht die JSON von Hand
bearbeiten):

```bash
python3 Tools/drehbuch/schreibe.py         # prüft die Daten, schreibt drehbuch.json und Docs/Drehbuch.md
python3 Tools/konvertiere_sektoren.py      # kopiert alles nach Godot/daten
```

Beim Schreiben wird geprüft: jede Kennung ist eindeutig, jede Voraussetzung existiert,
es gibt keine Zyklen, jeder Ortsname steht in `orte.json`, jede Figur ist bekannt.
Stufenarten: `hin`, `bring`, `markiere`, `stelle`, `halte`, `im_wagen`, `warte_revier`
(Spiel) und `szene`, `titel`, `funk`, `wahl`, `wenn`, `setze`, `gib` (Film und Zustand).

## Prüfen

Rauchtest ohne Fenster (endet nach rund zwei Minuten, Exit-Code 0 nur, wenn alles stimmt):

```bash
godot --path Godot --headless -- --rauchtest
```

Er prüft unter anderem: Boden und Kollision, Konto, Treffer auf Passanten und Autos,
Krankenhaus, Fahndung bis zur Festnahme, Straßensperre, Ampelplan und Bremsen, Aufträge,
Laden und Lackiererei, Verfolgung, Reviernahme und die ganze Hauptgeschichte im
Zeitraffer mit allen drei Enden (knapp 60 Prüfungen, `LALABERG_PRUEFUNG ... ok=`).

Bilder (nicht headless; ohne GPU mit Software-OpenGL unter Xvfb):

```bash
xvfb-run godot --rendering-method gl_compatibility --rendering-driver opengl3 \
  --path Godot -- --rauchtest --bild=bild.png         # Spielbild nach 15 s
xvfb-run godot --rendering-method gl_compatibility --rendering-driver opengl3 \
  --path Godot -- --filmtest --bild=film.png          # vier Bilder aus dem Prolog
```

`--ohneschatten` schaltet die Sonnenschatten ab (zum Eingrenzen von Bildfehlern).
Rauch- und Filmtest schreiben nur in eine eigene Spielstanddatei.

## Noch offen gegenüber der Unreal-Version

Stand der Unreal-Version: Git-Historie, Commit `dea4cec`; ihre Dokumentation steht
in `Docs/Unreal-Archiv/README-Unreal-Archiv.md`.

- Waffenmodelle, Insassen in den Autos, Figuren für die Polizei und die Kapseln in der Ferne
- Überholen und der Kontakt-Einsatz der Polizei bei drei Sternen
- Keine Bildraten- oder Größenmessung gegenüber Unreal

## Hinweise

- Geprüft unter Godot 4.5.1: Import ohne Fehler, Rauchtest bestanden (headless und
  mit Software-OpenGL). Nicht geprüft: echte GPU-Leistung, Vulkan-Renderer, Ton.
- Die Dreiecks-Wicklung wird im Konverter gedreht (siehe dessen Kopf); ohne das trägt
  der Boden von oben nicht.
- `AnimatableBody3D` mit `sync_to_physics` liest `global_position` nach dem Setzen noch
  bis zum nächsten Physikbild mit dem alten Wert; `Verkehrsauto.position_jetzt()` ist
  deshalb die verlässliche Position.
- Fahrzeuge brauchen die eingebaute Physik (`VehicleBody3D`), nicht Jolt.
- Für einen Export müssen `*.msh,*.col,*.json` im Export-Filter für Nicht-Ressourcen
  stehen, sonst fehlen die Stadtdaten im Build.
- `drehbuch.json` ist bewusst nicht in Git LFS (siehe `.gitattributes`).
