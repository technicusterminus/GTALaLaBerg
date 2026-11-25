# Filter-Beratung (Claude Opus über OpenRouter)

# Malstil-Analyse: Konzeptbild gegen Spielbild

## Das Konzeptbild ist kein Gemälde

Die wichtigste Erkenntnis vorab: **Das Konzeptbild ist nicht „gemalt“.** Es zeigt keine sichtbaren Pinselstriche, keine Konturlinien, keine Papierstruktur und keine Kuwahara-Flecken. Es ist ein weich ausgeleuchtetes, leicht stilisiertes **Digital-Painting/Render im Golden-Hour-Look**. Dein Kuwahara und die Sobel-Konturen führen deshalb in die falsche Richtung: Sie erzeugen einen Ölfilter-Comic-Look, den das Zielbild gar nicht hat.

| Merkmal | Konzeptbild | Dein Spiel |
|---|---|---|
| Pinsel/Struktur | Glatte Verläufe, feines Detail bleibt erhalten (Pflaster, Stuck) | Verrauschte Gras-/Baumtexturen, Aliasing |
| Kanten | Weich, aber nicht verwischt; keine Outlines | Hart, flimmernd |
| Palette | Pastell (Rosa, Ocker, Mint, Creme), Terrakotta, Pfirsich-Himmel; fast kein Grün | Neongrün dominiert, reines Himmelblau |
| Licht | Tiefe Sonne (~10°) von hinten links, goldene Streiflichter, lange Schatten | Hohe Mittagssonne, neutralweiß |
| Schatten | Angehoben (nie schwarz), kühl-violett, weich | Schwarz-grau, hart |
| Dunst | Warmer Luftperspektiv-Dunst, Ferne heller und entsättigt | Keiner |
| Kontrast | Niedrig bis mittel, filmische Schulter, sanftes Glühen in den Lichtern | Hoch, digital |
| Sättigung | Mitteltöne gedämpft, nur warme Lichter satt | Überall hoch |

**Grenze jedes Filters:** Das Konzept zeigt eine dichte Altstadt mit Giebeln, Pflaster und Ornamenten. Dein Bild zeigt Rasen, Straßen und Vorstadt. Ein Post-Shader kann Licht und Farbe angleichen, aber keinen Inhalt erzeugen. „1:1“ geht nur über Szene, Licht und Material. Der Shader liefert vielleicht 25 % des Looks.

**Zu den 3 FPS:** Das liegt nicht allein an deinem Kuwahara (~70 Taps), aber er ist teuer. Der neue Shader braucht etwa 9 Texturzugriffe.

## Shader

```glsl
// Golden-Hour-Look wie Konzeptbild: kantenerhaltende Glättung, weiches Glühen,
// warmer Dunst, Grün-Dämpfung, filmische Kurve, Split-Toning, Vignette, Korn.
shader_type canvas_item;

uniform sampler2D bild : hint_screen_texture, filter_linear_mipmap;

uniform float weich : hint_range(0.0, 1.0) = 0.55;
uniform float bloom_staerke : hint_range(0.0, 1.5) = 0.4;
uniform float bloom_schwelle : hint_range(0.0, 0.95) = 0.55;
uniform float dunst : hint_range(0.0, 1.0) = 0.35;
uniform vec3 dunst_farbe : source_color = vec3(0.95, 0.82, 0.70);
uniform float horizont : hint_range(0.0, 1.0) = 0.42;
uniform float belichtung : hint_range(0.5, 2.0) = 1.1;
uniform float kontrast : hint_range(0.5, 1.5) = 0.85;
uniform float schatten_hebung : hint_range(0.0, 0.2) = 0.07;
uniform vec3 schatten_ton : source_color = vec3(0.55, 0.52, 0.68);
uniform vec3 licht_ton : source_color = vec3(1.0, 0.84, 0.62);
uniform float split : hint_range(0.0, 1.0) = 0.4;
uniform float saettigung : hint_range(0.0, 2.0) = 0.88;
uniform float gruen_daempfung : hint_range(0.0, 1.0) = 0.55;
uniform float vignette : hint_range(0.0, 1.0) = 0.3;
uniform float korn : hint_range(0.0, 0.05) = 0.012;

float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

float hash(vec2 p) {
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}

void fragment() {
	vec2 uv = SCREEN_UV;
	vec2 px = SCREEN_PIXEL_SIZE;
	vec3 c = texture(bild, uv).rgb;

	// 1) Kantenerhaltende Glättung (Mini-Bilateral, 4 bilineare Taps = 16 Pixel)
	vec3 summe = c;
	float gew = 1.0;
	vec2 offs[4] = vec2[](vec2(1.5, 0.5), vec2(-0.5, 1.5), vec2(-1.5, -0.5), vec2(0.5, -1.5));
	for (int i = 0; i < 4; i++) {
		vec3 t = texture(bild, uv + offs[i] * px).rgb;
		vec3 d = t - c;
		float w = exp(-dot(d, d) * 35.0);
		summe += t * w;
		gew += w;
	}
	c = mix(c, summe / gew, weich);

	// 2) Weiches Glühen aus der Mipmap-Kette (praktisch gratis)
	vec3 b = textureLod(bild, uv, 3.0).rgb * 0.5
	       + textureLod(bild, uv, 5.0).rgb * 0.3
	       + textureLod(bild, uv, 6.5).rgb * 0.2;
	b = max(b - bloom_schwelle, vec3(0.0)) / (1.0 - bloom_schwelle);
	c += b * licht_ton * bloom_staerke;

	// 3) Luftperspektive (Screen-Space-Näherung: Band um den Horizont)
	float band = exp(-pow((uv.y - horizont) / 0.2, 2.0));
	float oben = 1.0 - smoothstep(0.0, horizont, uv.y);
	c = mix(c, dunst_farbe * (0.8 + 0.4 * luma(c)), dunst * (band * 0.6 + oben * 0.25));

	// 4) Grün dämpfen und Richtung Oliv/Ocker schieben
	float gm = clamp((c.g - max(c.r, c.b)) * 5.0, 0.0, 1.0);
	float l = luma(c);
	vec3 oliv = mix(vec3(l), c, 0.45) * vec3(1.08, 1.0, 0.82);
	c = mix(c, oliv, gm * gruen_daempfung);

	// 5) Belichtung und filmische Kurve: weiche Schulter, flacher Kontrast
	c *= belichtung;
	c = c / (1.0 + c * 0.6) * 1.6;
	c = mix(vec3(0.45), c, kontrast);
	c = max(c, vec3(0.0));

	// 6) Schatten anheben (nie schwarz) und Split-Toning
	l = luma(c);
	c = c * (1.0 - schatten_hebung) + schatten_ton * schatten_hebung;
	vec3 ton = mix(schatten_ton, licht_ton, smoothstep(0.08, 0.75, l));
	c = mix(c, c * ton / max(luma(ton), 1e-3), split);

	// 7) Sättigung: Mitten dämpfen, warme Lichter leicht anheben
	l = luma(c);
	float s = saettigung + 0.25 * smoothstep(0.5, 0.9, l);
	c = mix(vec3(l), c, s);

	// 8) Warme Vignette
	vec2 v = uv - 0.5;
	float vig = smoothstep(0.85, 0.25, length(v * vec2(1.0, 1.2)));
	c *= mix(vec3(0.78, 0.70, 0.66), vec3(1.0), mix(1.0, vig, vignette));

	// 9) Feines Korn und Dithering gegen Banding im Himmel
	c += (hash(FRAGCOORD.xy + fract(TIME) * 61.0) - 0.5) * korn;

	COLOR = vec4(clamp(c, 0.0, 1.0), 1.0);
}
```

## Die Stufen kurz erklärt

1. **Bilateral-Glättung** ersetzt den Kuwahara. Sie beruhigt verrauschte Gras- und Blatttexturen sowie Aliasing, ohne Kanten oder feine Details zu zerstören. So entsteht der „saubere“ Malerei-Eindruck.
2. **Mip-Bloom** erzeugt das goldene Glühen um Himmel und Sonnenflächen mit drei Zugriffen.
3. **Dunst** liefert die Luftperspektive. Ohne Tiefentextur im canvas_item ist das nur eine Näherung; den echten Dunst macht der Fog im Environment (siehe unten).
4. **Grün-Dämpfung**: Das Konzept hat kaum Grün. Neongrün ist der größte Farbbruch in deinem Bild.
5. **Filmische Kurve** komprimiert die Lichter und senkt den Kontrast.
6. **Schatten anheben und Split-Toning**: violett-kühle Schatten, goldene Lichter. Das ist der Kern der Abendstimmung.
7. **Selektive Sättigung**: gedämpfte Mitten, satte warme Lichter.
8. **Warme Vignette** lenkt den Blick ins Bildzentrum.
9. **Korn und Dither** verhindern Banding im Verlauf des Himmels.

**Kosten:** 9 Texturzugriffe, davon 3 auf kleinen Mips. Auf einer Mittelklasse-GPU sind das in 1080p etwa 0,3–0,5 ms.

**Wichtig zum Einbau:**
- Den Filter-ColorRect auf eine **CanvasLayer unter dem HUD** legen. Sonst werden UI und Minimap mitgefiltert.
- Falls die Mipmaps im Compatibility-Renderer leer bleiben, wirkt der Bloom flach. Dann `bloom_staerke = 0` setzen und den Glow im Environment nutzen.

## Die drei unverzichtbaren Änderungen

### 1. Licht: tiefe, warme Sonne

- **DirectionalLight3D**
  - `rotation_degrees.x ≈ -10` bis `-15` (tiefer Stand, Gegen- oder Streiflicht wie im Konzept)
  - `light_color = Color(1.0, 0.78, 0.55)`, `light_energy ≈ 1.3–1.6`
  - `shadow_enabled = true`, `shadow_blur ≈ 2.0–3.0` für weiche Schatten
  - `directional_shadow_max_distance` passend wählen, damit die langen Schatten nicht abreißen
- **Projekt-Einstellung:** `rendering/lights_and_shadows/directional_shadow/soft_shadow_filter_quality` auf Soft Medium oder höher.

### 2. Environment: Himmel, Ambient-Licht, Fog, Tonemapping

- **ProceduralSkyMaterial**
  - `sky_top_color ≈ (0.62, 0.70, 0.80)`
  - `sky_horizon_color ≈ (0.98, 0.84, 0.70)`
  - `ground_horizon_color` gleich der Horizontfarbe
- **Ambient-Licht:** `ambient_light_source = Sky`, `ambient_light_energy ≈ 0.7`. Das hellt die Schatten farbig auf und ersetzt teilweise das fehlende SSAO.
- **Fog**
  - `fog_enabled = true`
  - `fog_light_color ≈ (0.95, 0.80, 0.66)`
  - `fog_density ≈ 0.003–0.006`
  - `fog_sun_scatter ≈ 0.4`
  - `fog_aerial_perspective ≈ 0.5`
- **Tonemapping:** `tonemap_mode = Filmic` (oder AgX ab 4.4).
- **Glow:** `glow_enabled = true` mit niedriger Intensität, sofern im Compatibility-Renderer verfügbar.

### 3. Materialien und Inhalt: Palette und Oberflächen

- **Albedo:** Fassaden in Pastell (Rosa, Ocker, Mint, Creme), Dächer in Terrakotta.
- **Gras:** großflächig ersetzen durch Pflaster und Plätze. Wo Gras bleibt, Albedo auf Oliv `≈ (0.45, 0.48, 0.25)` setzen, die Textur entrauschen und `texture_filter = Linear Mipmap Anisotropic` verwenden.
- **Rauheit:** Fassaden und Boden `roughness 0.85–0.95`, `metallic = 0`. Das gibt matte, „gemalte“ Flächen.
- **Ambient Occlusion:** Da kein SSAO verfügbar ist, AO in Vertex-Colors oder Texturen backen, oder **LightmapGI** nutzen (im Compatibility-Renderer bake-fähig). So entstehen die weichen Kontaktschatten an Fenstern, Giebeln und Sockeln.
- **Geometrie:** Fensterlaibungen, Gesimse und Giebel als echte Geometrie modellieren. Erst die Detaildichte erzeugt den Konzept-Look.

**Reihenfolge:** Zuerst Licht und Environment umstellen, dann die Materialien anpassen, und den Shader zuletzt nur noch feintunen. Die Defaults oben sind auf den Zustand *nach* diesen Änderungen abgestimmt.
Kosten (USD): 0.179608
