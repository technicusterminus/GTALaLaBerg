# Bewertung: ca. 30 %

**Was schon passt:** warmer Himmelsverlauf, Pflaster-Textur, grüne Fensterläden mit Blumenkästen, grundsätzlich Pastell-Absicht.

**Was fehlt:**
- Fassaden sind überbelichtet und ausgewaschen (fast weiß statt rosa, gelb, mint).
- Kein goldenes Streiflicht und keine langen Schatten.
- Kein Kontakt-AO an Sockeln.
- Keine Tiefenstaffelung durch Dunst.
- Die Ferne ist eine leere Sandfläche.

**Hinweis:** 2 FPS verfälscht Tests. Erst Performance prüfen, sonst sind SSAO, SSIL und Glow kaum bewertbar.

## 6 wirksamste Änderungen

Die Skalen der Shader-Parameter sind angenommen und ggf. an euren Shader anzupassen.

**1. Belichtung und Tonemapping runter, Weiß-Clipping beseitigen**
- Environment: `tonemap_mode = AgX` (oder Filmic), `tonemap_exposure = 0.85`, `tonemap_white = 6.0`
- Shader: `belichtung = -0.3` (bzw. 0.8 als Faktor), `kontrast = 1.15`

**2. Sonne als tiefe Goldstunde**
- DirectionalLight3D: Höhe **12–16°**, Azimut entlang der Gassenachse (Gegenlicht schräg)
- `light_color = (1.0, 0.76, 0.52)`, `light_energy = 2.2`
- `shadow_enabled = true`, `shadow_blur = 1.5`, `directional_shadow_max_distance = 120`

**3. Ambient kühler und schwächer (Schatten bläulich, Licht warm)**
- `ambient_light_source = Sky`, `ambient_light_sky_contribution = 0.6`, `ambient_light_energy = 0.55`
- Shader: `schatten_hebung = 0.03`, `split = 0.3` (Schatten Richtung Blau-Violett, Lichter Richtung Orange)

**4. Sättigung der Fassaden zurückholen**
- Shader: `saettigung = 1.25`, `gruen_daempfung = 0.15` (Bäume nicht giftig, Läden bleiben salbeigrün)
- Albedo der Fassaden max. ca. 0.75 Helligkeit, nicht 0.95

**5. SSAO und SSIL für Kontakt und Bodenanbindung**
- `ssao_enabled = true`, `ssao_radius = 1.5`, `ssao_intensity = 2.5`, `ssao_power = 1.5`
- `ssil_enabled = true`, `ssil_intensity = 0.8` (warmes Bounce-Licht vom Pflaster auf die Fassaden)

**6. Dunst, Bloom und Weichheit für den Malerei-Look**
- Environment: `fog_enabled = true`, `fog_light_color = (0.95, 0.80, 0.65)`, `fog_density = 0.004`, `fog_sun_scatter = 0.3`, `fog_aerial_perspective = 0.4`
- Shader: `dunst = 0.25`, `bloom_staerke = 0.35`, `bloom_schwelle = 0.8`, `weich = 0.15`, `vignette = 0.25`

## Nur durch Geometrie oder Inhalt lösbar

- **Giebel und Dächer:** Schweif- und Treppengiebel, sichtbare rote Ziegeldächer. Aktuell nur flache Kästen ohne Dachkante.
- **Fassadenrelief:** Gesimse, Fensterlaibungen mit 10–20 cm Tiefe, Sockel und Stuck. Erst das erzeugt Schattenlinien im Streiflicht.
- **Unterschiedliche Hausfarben:** pro Haus eine eigene, kräftigere Pastellfarbe (rosa, ocker, mint, gelb) statt verlaufender Weißtöne.
- **Fluchtpunkt:** Platz bzw. Gasse mit Blickziel (Schmalzturm), dazu eine Straßenflucht. Die leere Sandfläche in der Ferne durch Bebauung oder Pflaster ersetzen.
- **Props:** Brunnen/Mariensäule, Café-Tische, Blumenkübel, Laternen, parkende Autos, Poller.
- **Pflaster:** Steine kleiner skalieren (ca. 0.5×), Normal- und Roughness-Map stärker, Fugen dunkler.
- **Kamera:** tiefer (ca. 1.6–2 m) und entlang der Straßenachse statt von oben gegen eine Wand.
KOSTEN 0.060476
