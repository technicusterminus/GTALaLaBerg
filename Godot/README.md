# GTA LaLaBerg in Godot 4.5

Landsberg am Lech als begeh- und befahrbare Stadt mit einer Geschichte: dem
**Farbkrieg**. Dies ist die Godot-Umsetzung; die Quelldaten liegen in
`Content/SourceData` (Git LFS), die Geschichte in `Tools/drehbuch`.

## Starten

```bash
git lfs pull --include="Content/SourceData/**"
python3 Tools/konvertiere_sektoren.py      # braucht numpy, erzeugt Godot/daten
godot --headless --editor --path Godot --import
godot --path Godot                         # oder den Ordner Godot/ im Editor öffnen
```

Mit bereits erzeugten Stadtdaten den Konverter und den Import erneut ausführen, damit Godot
Fassaden-UVs, Stadttexturen, Pflanzen und Leitlinien lädt.

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

**Stadt.** 90 Sektoren, nach Entfernung nachgeladen, mit Kollision (`stadt.gd`).
Fassaden mit Fensterläden, Schaufenstern und gemauerten Sockeln. Am Hauptplatz, am Klinikum
und am Lech stehen eigene Pflanzungen. Die Wiesen und Baumkronen folgen den
Stadtsektoren. Über der Stadt fliegen Vogelschwärme und ein Passagierflugzeug.

**Verkehr.** 70 KI-Autos auf Rundkursen, 214 Passanten, 1078 geparkte Autos
je Sektor als MultiMesh, 44 Ampeln mit Rot-Gelb und Räumzeit. KI-Autos bremsen vor Autos,
Rot, Kurven, dem Spieler und wartepflichtigen Kreuzungen (`verkehrsbremse.gd`).

**Kampf und Folgen.** Vier Paintball-Waffen, Farbkugeln mit Klecks und Wirkung im
Umkreis, Leben für Spieler, Passanten und Autos, Sturz- und Anprallschaden,
Krankenhaus mit Behandlungskosten. Die Polizei (`polizei.gd`) fahndet mit bis zu fünf
Sternen, schickt Streifen über den Straßengraphen, sucht, sperrt ab drei Sternen die
Straße und nimmt fest. Ab drei Sternen steigen aus einer Streife in der Nähe zwei Beamte
(`polizist.gd`) und laufen mit 5,5 m/s auf den Gesuchten zu (in Uniform: das Arbeiter-Modell, umgefärbt); wer erreicht wird, ist
festgenommen. Mit dem Wagen davonfahren geht, Paintball-Treffer werfen sie kurz aus dem Tritt.

**Sonderfahrzeuge.** `sonderfahrzeug.gd`: ein Panzer (W/S fahren, A/D drehen, der Turm
folgt dem Blick, Maus links feuert alle drei Sekunden eine Großkaliber-Farbkugel) und ein
Hubschrauber (Leertaste steigen, W/S fliegen, A/D drehen). Beide stehen in der Stadt und
werden mit F bestiegen; Aussteigen nur am Boden.

**Überholen.** Steht ein Hindernis (stehendes Auto oder der Spieler im Wagen) länger als zwei
Sekunden vor einem KI-Auto, die Gegenspur ist 45 m weit frei und keine rote Ampel in Sicht, schert
das Auto aus, fährt 25 % schneller vorbei und ordnet sich wieder ein (`verkehrsauto.gd`,
`verkehrsbremse.gd`).

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

### Grafik manuell prüfen

1. Nach Konvertierung und Import den Prolog starten. Beim Kameraflug am Klinikum
   den vorspringenden Glaseingang, die gelb-blauen Fensterbänder und den
   Karl-Mohrenweis-Platz prüfen. Aus der Nähe werden Äste und einzelne Blattgruppen
   der Laubbäume sichtbar.
2. Zum Hauptplatz fahren und vom Marienbrunnen in Richtung Historisches Rathaus
   blicken. Die Platzfront hat fünf Fensterachsen, drei Torbögen und helle Stuckdetails.
   An den Nachbarhäusern sind Fensterläden, Schaufenster und Steinsockel sichtbar.
3. Auf einer Hauptstraße prüfen, ob Leitlinien der Fahrbahn folgen und vor
   Kreuzungen aussetzen. Eine Ampel soll abseits der Fahrbahn stehen und von
   Rot über Rot-Gelb zu Grün und anschließend Gelb wechseln.
4. Am Lech die Uferpflanzung und die Wasseroberfläche aus der Nähe ansehen.
   Für einen zweiten Durchlauf `--rendering-method gl_compatibility` ergänzen.
5. Am Hindenburgring die Straßenfassade des historischen JVA-Torbaus ansehen.
   Rundtürme, Kupferhauben, Walmdach und die beiden Fensterreihen müssen aus
   Straßenperspektive als zusammenhängender Bau wirken.
6. An der Platanenstraße den Rundbau, den roten Ganztagsanbau und die Turnhalle
   prüfen. Hinter der Halle folgen Sportfeld und Laufbahn. Im Schulhof sind das
   rote Spielfeld, der Teich und die verbindenden Wege sichtbar.
7. Von der Breslauer Straße über den REWE-Parkplatz und die Klinikzufahrt zur
   Bürgermeister-Dr.-Hartmann-Straße fahren. Marktfront, Stellplätze, Grüninseln,
   Kinderhaus und Pflegeschule lassen sich entlang dieser Strecke ansehen.
   Weiter südlich stehen die Flachdachhäuser neben den älteren Satteldachhäusern.
8. An der Waldheimer Straße zur Katharinenschule fahren und vom Sportfeld auf
   die Hofseite schauen. Steildach, Gauben und Fassadenuhr sind von dort sichtbar.
9. Vom REWE entlang der Breslauer Straße über Römerhang, Von-den-Hoff-Straße
   und Mühlweg bis zum Danziger Platz fahren. Mittelinseln, Radweg, Haltestellen
   und Querungsmarkierungen folgen dem Verlauf der vorhandenen Straße.
10. Am Danziger Platz die getrennten Zuläufe, Abbiegespuren, Inseln und Furten
    ansehen. Weiter über die Katharinenstraße zur Karolinenbrücke fahren. Roter
    Radstreifen, Gehwege, Brückenköpfe und Fahrbahn sollen durchgehend sein.
11. Von der Karolinenbrücke aus prüfen, ob das Westufer begrünt und die
    Altstadtkante gemauert erscheint. Mit Forward+ spiegeln sich die Brücke und
    die Altstadt im Wasser. Weiter flussabwärts unterscheiden sich Lady-Herkomer-Steg,
    Lechsteg und Sandauer Brücke an Deckbreite, Tragwerk und Geländern.

Referenzen zum Bestandsbau des Klinikums sind das [Eingangsfoto des Klinikums
von 2025](https://www.klinikum-landsberg.de/wp-content/uploads/Umwelterklaerung-2025_Klinikum-Landsberg-am-Lech.pdf)
und das [Luftbild des Karl-Mohrenweis-Platzes](https://www.kreisbote.de/lokales/landsberg/klinikum-landsberg-benennt-vorplatz-nach-spd-stadtrat-seinen-standort-verdankt-11131899.html).
Beim JVA-Torbau stammen Standort und Fassadenachse aus den Stadtdaten.
Die Form der öffentlich sichtbaren Details folgt dem
[Foto des Eingangsgebäudes](https://commons.wikimedia.org/wiki/File:JVA_Landsberg_Lech_-_Eingangsgeb%C3%A4ude.JPG).
Die Stadtdaten liefern auch die Umrisse für das Dach des historischen Hauptbaus
und die Gebäude der Platanenschule. Für Dächer und Freiflächen der Schule dienten
die [Schulseite](https://www.platanenschule-landsberg.de/%C3%BCber-uns/) und das
[amtliche Orthofoto](https://geodatenonline.bayern.de/geodatenonline/seiten/wms_dop20cm)
als Referenz. Dachaufbauten und Ausstattung sind visuell angenähert.

An der Hartmannstraße und in der Katharinenvorstadt erzeugt der Konverter aus
abgezeichneten Flächen zwei Belagstexturen. Die Stadt behält ihre Gelände-
und Straßengeometrie. Stellplatzreihen, Plattenwege und Sportbeläge folgen
dem [amtlichen Orthofoto](https://geodatenonline.bayern.de/geodatenonline/seiten/wms_dop20cm).
Für Gebäude und Außenanlagen dienten die
[Bestandsfotos im Umweltbericht der Stadt](https://www.landsberg.de/fileadmin/user_upload/download/Rathaus-Bauleitplanung/Bebauungsplanentwurf_2450_Klinikum_UMWELTBERICHT.pdf#page=23),
die [Marktseite](https://www.rewe.de/marktseite/landsberg-am-lech/461688/rewe-markt-breslauer-str-27/),
die [Seite des Kinderhauses](https://www.landsberg.de/rathaus/meine-stadt/kinderbetreuung/kinderhaus-an-der-roemerauterrasse/)
und das [Hoffoto der Katharinenschule](https://www.katharinenschule-landsberg.de/).
Fassaden und Hofausstattung sind modelliert und auf den vorhandenen
Gebäudegrundrissen bzw. auf dem Gelände eingepasst.

Für die Breslauer Straße orientieren sich Lage und Abfolge von Inseln und
Querungen am amtlichen Orthofoto und am
[Umbau der Stadt](https://www.landsberg.de/rathaus/aktuelles/pressemitteilungen/lob-fuer-umbau-der-breslauer-strasse/).
Der Konverter erzeugt dafür eine schmale, entlang des Straßenzugs ausgerichtete
Belagstextur. Das Straßennetz liefert die Fahrbahnhöhen und Verkehrsampeln für
den Abschnitt.

Für Danziger Platz, Katharinenstraße und Lechufer wurden Fahrbahnränder,
Inseln und Wege anhand des [amtlichen Orthofotos](https://geodatenonline.bayern.de/geodatenonline/seiten/wms_dop20cm)
eingepasst. Am Lech wurden die vier Lechquerungen gemäß dem
[Landsberger Stadtplan](https://www.landsberg.de/fileadmin/user_upload/download/Tourismus-Stadtfuehrungen/LLbrosch_stadtfuehrungen_ONLINE_2025.pdf)
modelliert. Brückenbreiten und sichtbare Bauformen orientieren sich an den
[Aufnahmen der Karolinen- und Sandauer Brücke](https://www.karl-gotsch.de/Album/D_S_andere.htm),
den [Bauangaben zum Lady-Herkomer-Steg](https://www.gls.at/DE/verkehrswegebau?inhaltID=534)
und dem [Foto des Lechstegs](https://commons.wikimedia.org/wiki/File:Landsberg_am_Lech,_Lechsteg.JPG).
Das [Karolinenwehr](https://www.landsberg.de/tourismus/erlebnisse/sehenswuerdigkeiten/lechwehr/)
liegt modelliert zwischen den beiden Uferbereichen.

Kleinere Bach- und Bahnquerungen greifen auf die in der Karte liegenden
Brückenachsen von [© OpenStreetMap-Mitwirkenden](https://www.openstreetmap.org/copyright)
zurück. Deckhöhen und Geländerdetails sind dort aus Gelände und Bildmaterial
angenähert. Die A96-Lechbrücke liegt außerhalb der bebauten Spielkarte.

## Noch offen gegenüber der Unreal-Version

Stand der Unreal-Version: Git-Historie, Commit `dea4cec`; ihre Dokumentation steht
in `Docs/Unreal-Archiv/README-Unreal-Archiv.md`.

- Insassen in den Autos (die Scheiben sind im Flachstil undurchsichtig, ein Fahrer wäre unsichtbar)
- Vereinfachte Silhouetten bei fernen Passanten
- Keine Bildraten- oder Größenmessung gegenüber Unreal

## Hinweise

- Godot 4.5.1 lädt die Stadt auch im Headless-Rauchtest. Die Ansichten von
  Danziger Platz und Lech wurden unter Windows mit Vulkan und Forward+ geprüft.
- Die Dreiecks-Wicklung wird im Konverter gedreht (siehe dessen Kopf); ohne das trägt
  der Boden von oben nicht.
- `AnimatableBody3D` mit `sync_to_physics` liest `global_position` nach dem Setzen noch
  bis zum nächsten Physikbild mit dem alten Wert; `Verkehrsauto.position_jetzt()` ist
  deshalb die verlässliche Position.
- Fahrzeuge brauchen die eingebaute Physik (`VehicleBody3D`), nicht Jolt.
- Für einen Export müssen `*.msh,*.col,*.json` im Export-Filter für Nicht-Ressourcen
  stehen, sonst fehlen die Stadtdaten im Build.
- `drehbuch.json` ist bewusst nicht in Git LFS (siehe `.gitattributes`).
