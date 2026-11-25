# Prüfung mit OpenRouter (Claude Opus 5.5)

Vergleich Zielbild gegen den Stand von 2026-11-21 und wo Modelle sinnvoll helfen. Rohtext der Antwort.

## Befund pro Bild

**Bild 2 (Heli):**
- **Performance:** 3 FPS.
- **Licht:** Flach, ohne erkennbare Schatten und ohne warmes Seitenlicht. Der Himmel ist ein harter Verlauf mit grauem Dunstband statt Abendlicht.
- **Dächer:** Glatt rot, ohne Ziegelstruktur. Das große Dach vorne zeigt verzerrte, verschmierte Textur.
- **Fassaden:** Bläulich-weiß und überall gleich. Es fehlen Putzfarben, Sockel und Gesimse.
- **Boden:** Olivbraun und gleichförmig. Die Straßenränder sind breite beige Bänder statt Bordsteinen und Pflaster.
- **Bäume:** Identische Kugelbäume.
- **Heli:** Untexturiert und überstrahlt.
- **Lech:** Nicht sichtbar.

**Bild 3 (Straße):**
- **Performance:** 2 FPS.
- **Giebel:** Die Giebeldreiecke sind weiß-transparent und wirken wie Pappe. Vermutlich ist der Wandshader dort falsch oder Normalen bzw. Material fehlen.
- **Lichtfehler:** Ein rosa Lichtfleck liegt auf der Fassade.
- **Bäume:** Die Blattkronen flimmern grün verrauscht (Weltkoordinaten-Textur ohne Mipmap oder Alpha).
- **Boden:** Gelbbrauner Rasen. Der Asphalt ist ohne Markierung und Bordstein.
- **Laternenmast:** Viel zu dick.
- **Figuren:** Einfach, aber noch am ehesten akzeptabel.

**Bild 4 (Auto):**
- **Modell:** Brauchbar.
- **Material:** Wirkt billig. Die Scheiben sind schwarz ohne Spiegelung, Lack ohne Umgebungsreflexion, die Felgen sind glänzende Kugeln.
- **Umgebung:** Leerer grauer Boden, flacher Himmel.

## Maßnahmen nach Wirkung

**1. Performance (2–3 FPS)**
- **Lösung:** Nur Code, kein Modell hilft.
  - Draw-Calls zählen (Profiler, Monitor).
  - Gebäude pro Chunk zu einem Mesh mergen.
  - MultiMesh für Bäume, Gauben und Autos.
  - Sichtweitenbegrenzung mit LODs; Fern-Häuser als simple Quader mit einer Textur.
  - Schatten nur nah (2 Splits).
  - Licht-Anzahl pro Objekt prüfen (Compatibility = Forward, jede Laterne kostet).
- **Opus/Gemini:** Sinnvoll für die Analyse des Konverters und der Szene.
- **Aufwand/Risiko:** Mittel. Geringes Risiko, Voraussetzung für alles andere.

**2. Licht, Himmel, Tonemapping**
- **Lösung:** Code/Shader.
  - Tiefstehende warme DirectionalLight mit Schatten.
  - ProceduralSky in Goldtönen, Filmic/ACES, Tiefennebel in Hellblau-Gold.
  - Ambient vom Himmel.
  - Ambient Occlusion im Python-Konverter als Vertexfarbe einbacken (Kanten, Bodennähe, Innenhöfe), da Compatibility kein SSAO hat.
  - Rosa Lichtfleck: Omni-Licht oder Light-Mask prüfen.
- **Modell:** Nur Gemini Image zur Moodboard-Kontrolle der Farbstimmung.
- **Aufwand/Risiko:** Klein. Geringes Risiko, größter Sofort-Effekt.

**3. Dachflächen**
- **Problem:** UVs entlang der Dachneigung fehlen. Weltkoordinaten-Projektion verzerrt auf Schrägen.
- **Lösung Code:**
  - Im Konverter pro Dachfläche UV entlang Falllinie und Traufe erzeugen.
  - Farbvariation pro Gebäude (Hash).
  - Firstziegel- und Traufkanten-Streifen als eigene schmale Geometrie.
- **Modell:** Gemini 3 Pro Image.
  - Prompt: „Seamless tileable top-down texture of weathered orange Bavarian clay roof tiles (Biberschwanz), even lighting, no perspective, 1024px.“
  - Ergebnis: Albedo-Kachel. Danach in Python per Offset-Blend wirklich nahtlos machen; Normal- und Rauheitsmap per Skript aus Helligkeit ableiten.
- **Aufwand/Risiko:** Mittel. Mittleres Risiko (Kachelnähte, Wiederholungsmuster).

**4. Giebel und Silhouetten**
- **Lösung:** Code/Geometrie, kein Modell.
  - Giebeldreiecke mit Fassadenmaterial und korrekten Normalen versehen.
  - Dachüberstand (0,4–0,6 m) und Ortgang ergänzen.
  - Kamine und Gauben in variierender Zahl.
  - Sockel- und Traufgesims als Leisten.
  - Höhen und Dachneigungen variieren.
- **Begründung:** Das Zielbild wirkt vor allem durch Überstände, Schatten und Kanten.
- **Aufwand/Risiko:** Mittel. Geringes Risiko.

**5. Fassaden**
- **Modell:** GPT-5 Image oder Gemini.
  - Prompt: „Orthographic front-facing texture atlas of 8 Bavarian old-town windows with wooden shutters and stone frames on cream plaster, flat lighting, grid layout.“
  - Ergebnis: Fensteratlas. Der Shader wählt pro Fenster zufällig eine Variante.
- **Code:**
  - Putzpalette (Creme, Ocker, Hellgelb, Altrosa) pro Gebäude.
  - Putztextur seamless (gleiches Verfahren wie bei den Dächern).
  - Verschmutzung unten per Shader-Gradient.
- **Aufwand/Risiko:** Mittel. Geringes Risiko.

**6. Boden, Gelände, Straßen**
- **Modell:** Gemini Image für nahtlose Kacheln (sattgrünes Gras, Asphalt, Kopfsteinpflaster, Kies, Feld gemäht/Getreide).
  - Prompt: „Seamless tileable top-down photo texture of fresh green summer lawn, even light, no shadows.“
- **Code:**
  - Makro-Variation per Noise gegen Kachelwiederholung.
  - Bordsteine als Geometrie, Gehwege als eigene Fläche.
  - Feldgrenzen aus OSM-Landuse.
  - Leichte Geländehöhen am Lechhang.
- **Grenze:** Das Modell liefert nur Pixel. Die Wirkung kommt aus Variation und Bordsteinen.
- **Aufwand/Risiko:** Mittel. Geringes Risiko.

**7. Bäume**
- **Problem:** Das Flimmern ist ein Shaderfehler (Weltkoordinaten-Textur auf Kronen).
- **Lösung:** Kronen aus wenigen gekreuzten Alpha-Ebenen oder Kugelclustern mit Blattkarte, Alpha Scissor, Mipmaps. 3–4 Varianten mit Größen- und Farbvariation.
- **Modell:** Gemini Image.
  - Prompt: „Sprite sheet of 4 isolated leaf cluster textures, broadleaf and conifer, on pure transparent/green-screen background, top-lit, game-ready.“
  - Ergebnis: Blattatlas. Freistellen per Skript, da Bildmodelle keine echte Transparenz garantieren.
- **Aufwand/Risiko:** Mittel. Mittleres Risiko (Freistell-Kanten).

**8. Wasser und Lech (fehlt sichtbar)**
- **Lösung:** Code/Shader.
  - Flussfläche aus OSM, Uferböschung als Geometrie, Lechwehr als Stufe mit Schaumstreifen.
  - Wassershader mit zwei scrollenden prozeduralen Normal-Noises, Fresnel zur Himmelsfarbe, türkisgrüner Tiefenfarbe (Lech-typisch).
- **Modell:** Nicht sinnvoll. Bildmodelle liefern schlechte Normalmaps, Noise im Shader ist besser.
- **Aufwand/Risiko:** Mittel. Geringes Risiko, hoher Wiedererkennungswert für Landsberg.

**9. Fahrzeuge (Autos, Heli, Panzer)**
- **Autos:**
  - ReflectionProbe oder Sky-Reflexion aktivieren.
  - Glas mit Fresnel statt Schwarz.
  - Felgen-Material matt mit Textur.
  - Mehrere Lackfarben.
- **Heli:**
  - Teile mit eigenen Materialien versehen (Rumpf, Glas dunkel spiegelnd, Kufen grau).
  - Livery als einfaches Decal oder Farbband, weniger Emission.
  - Optional per Gemini Image eine Lackierungs-Vorlage als Seitenansicht, nur bei vorhandenen UVs.
- **Panzer:** CC0-Modelle (Quaternius, Kenney, Poly Pizza) statt Eigenbau.
- **Grenze:** Kein Modell hilft bei 3D. Zum Vorentwurf nur als Konzeptbild sinnvoll.
- **Aufwand/Risiko:** Klein bis mittel. Geringes Risiko.

**10. Figuren**
- **Lösung:** CC0- oder Mixamo-Charaktere mit Animationen; Kleidungsfarben per Shader variieren.
- **Modell:** Nur Gemini Image für Dialogporträts im einheitlichen Stil.
  - Prompt: „Portrait of a Landsberg courier NPC, stylized semi-realistic, warm evening light, consistent character sheet style.“
  - Mit Referenzbild für Konsistenz arbeiten.
- **Grenze:** Für Mesh und Animation hilft kein Modell.
- **Aufwand/Risiko:** Mittel. Mittleres Risiko (Stilbruch der Porträts).

**11. UI/HUD**
- **Problem:** Zu viel nackter Text oben links, kein Stil.
- **Code:**
  - Ein Hauptziel prominent, Rest einklappbar.
  - Halbtransparente Panels, eigene Schrift, Icons statt Wörter.
  - Tacho als Grafik, Minimap rund und gerahmt.
- **Modell:** GPT-5 Image.
  - Prompt: „Game HUD mockup and flat icon set (health, money, weapon, mission marker, taxi, race) in Bavarian blue-white style, clean vector look.“
  - Ergebnis: Vorlage und Icons. Nachbearbeiten bzw. für Schärfe als SVG nachbauen.
- **Aufwand/Risiko:** Klein. Geringes Risiko.

**12. Ton, Musik, Zwischenszenen**
- **Musik:** Lyria 3 ist sinnvoll, z. B. 6–10 Loops (Fahrt, Heli, Verfolgung, Menü, Altstadt).
  - Prompt: „Upbeat 90-second loopable open-world driving track, light brass and modern beat, subtle Bavarian folk motif.“
  - Loop-Punkte manuell schneiden.
- **Stimmen:** gpt-audio für NPC-Sätze, Funk-Durchsagen und Radio-Moderation.
  - Dialogtexte von Claude oder Gemini schreiben lassen, die auch Missionen und Ortsbezug liefern.
- **Geräusche:** SFX (Motor, Rotor, Schritte) besser aus Freesound oder CC0 nehmen; dafür ist keines der Modelle stark.
- **Video:** Gemini Omni nur für Intro, Menühintergrund oder Trailer-Moodfilm. Für echte Zwischenszenen ist es ungeeignet, weil Gebäude, Figuren und Fahrzeuge nicht zum Spiel passen und nicht konsistent sind. Besser sind In-Engine-Szenen mit AnimationPlayer und Kamerapfad.
- **Aufwand/Risiko:** Klein. Geringes Risiko für Musik und Stimmen, hohes Risiko bei Video.

## Wo Modelle klar nicht helfen

Performance, Geometrie und Silhouetten, Dachüberstände, Schatten und AO, LOD, Wassershader, Normalmaps, 3D-Fahrzeuge und Figuren, Animation, Physik, konsistente Zwischenszenen.

Den größten Hebel haben in dieser Reihenfolge: FPS, Licht, Dach- und Giebelgeometrie, danach Texturen.
