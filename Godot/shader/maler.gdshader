// Golden-Hour-Look wie das Zielbild ziel-hauptplatz-real.png (Vorschlag von Claude Opus ueber
// OpenRouter, siehe Docs/Konzept/openrouter-filter.md): kantenerhaltende Glättung, weiches Glühen,
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
