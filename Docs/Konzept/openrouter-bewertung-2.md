# Bewertung: ca. 25–30 % Look-Nähe

**Was schon passt:** Farbpalette der Fassaden (Pastell), rote Dächer, Treppengiebel-Silhouetten, warmes Abendlicht und die Platzform.

**Was fehlt:** Fotorealismus und Materialtiefe. Die Fassaden wirken flach und verwaschen, der Boden ist einfarbig, die Dachziegel sind viel zu groß skaliert, die Bäume sehen aus wie Blobs, und es gibt kaum Requisiten.

*Hinweis:* 1 FPS ist kein Look-Thema, blockiert aber alles. Zuerst Instancing, LOD und Texturatlas einführen, sonst sind die Schritte 2–5 nicht spielbar.

---

## Die 5 wirksamsten nächsten Schritte

**1. Fassaden mit echter Tiefe (Code/Geometrie + Bildmodell)**
- **Geometrie:** Fensterlaibungen etwa 15–25 cm tief, Gesimse und Fensterverdachungen als Mesh, geschwungene Barockgiebel als Profilextrusion, Fensterläden als eigene Quads.
- **Textur:** Pro Haustyp eine saubere, scharfe Fassadentextur statt der verrauschten.
- *Prompt (z. B. Gemini Flash Image / GPT-Image):* `orthographic frontal facade texture of a Bavarian baroque townhouse, pastel pink plaster, white stucco window frames, green wooden shutters, flat even lighting, no perspective, no shadows, seamless edges, 2048x4096`

**2. Licht und Post-Processing (Code)**
- ACES-Tonemapping und SSAO, damit die Fugen und Laibungen dunkler werden.
- Weiche Schatten (PCF oder Cascaded Shadow Maps).
- Leichter Bloom sowie Atmosphären- und Höhennebel, der Richtung Horizont warm verläuft.
- Himmelsverlauf von Pfirsich nach Hellblau.
- Diese Maßnahmen bringen den größten Realismus-Sprung pro Codezeile.

**3. Kopfsteinpflaster für den Platzboden (Bildmodell + Code)**
- Albedo-, Normal- und Roughness-Map, gekachelt mit Detail-Textur und Makro-Variation gegen sichtbare Wiederholung.
- *Prompt:* `seamless tileable top-down texture of old European granite cobblestone pavement, grey-beige stones, sandy joints, photoreal, even diffuse light, no shadows, 2048x2048`
- Normal-Map danach per Code aus der Höhe ableiten.

**4. Dachziegel-Maßstab und Material (Code + Bildmodell)**
- UV-Skalierung etwa um Faktor 4–6 verkleinern.
- Normal-Map ergänzen und den Farbton pro Haus variieren (dunkleres Ziegelrot, Patina).
- Dachüberstände und Gauben mit Rahmen modellieren.
- *Prompt:* `seamless tileable texture of weathered red clay beaver-tail roof tiles (Biberschwanz), slight moss and color variation, top-down, photoreal, 2048x2048`

**5. Requisiten und Vegetation (Geometrie + Bildmodell für Texturen)**
- Die Kugelbäume durch Folgendes ersetzen:
  - Laternen an Wandarmen
  - Blumenkästen an Fenstern
  - Pflanzkübel aus Holz
  - Café-Stühle und -Tische
  - Poller
  - Marienbrunnen mit Säule, Statue und schmiedeeisernem Gitter
  - parkende Autos
- *Prompt (Alpha-Texturen):* `ornate wrought iron fence panel, black, isolated on transparent background, front view, game texture`

---

## Audio (optional, kein Look-Gewinn)
Eine Atmo hebt die Immersion trotzdem spürbar.
- *Prompt (Audio-Modell):* `ambient soundscape of a quiet Bavarian old town square at sunset, distant church bells, footsteps on cobblestones, fountain splashing, soft chatter, swallows, loopable 60s`
KOSTEN 0.055972
