// Flachschattierung ohne Normalen in den Daten: die Flaechennormale kommt aus
// den Bildschirmableitungen. Das spart die Normalen im Speicher und zeigt
// immer zur Kamera, egal wie die Dreiecke gewickelt sind.
shader_type spatial;
render_mode cull_disabled;

// Linear gelesen wie in der Unreal-Version, daher ohne source_color.
uniform vec3 farbe = vec3(0.7);
uniform float rauheit : hint_range(0.0, 1.0) = 0.9;
uniform float glanz : hint_range(0.0, 1.0) = 0.0;
// Waagerechte Flaechen (Boden, Strassen, Daecher): feste Normale nach oben. Die Daten
// haben keine Vertex-Normalen, ohne sie greift der Normal-Bias der Schatten
// nicht und der Boden zeigt Ringe (Shadow-Acne).
uniform bool oben = false;
// 0 = flach, 1 = Gelaende (Wiesenflecken, Erde am Hang), 2 = Wasser (Wellen, Glitzern),
// 3 = Hausfassade (eingelassene Fenster mit Laibung, Rahmen, Bank und Laeden, Putz),
// 4 = Ziegeldach, 5 = Pflaster, 6 = Laub.
uniform int stil = 0;

// Fotoaehnliche Details (vom Bildmodell erzeugt, nahtlos gemacht); fehlen sie, bleibt es bei
// den gerechneten Mustern (dann sind die Texturen weiss).
uniform sampler2D detail_dach : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D detail_putz : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D detail_pflaster : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D detail_wiese : source_color, filter_linear_mipmap, repeat_enable;

varying vec3 welt;

// Mip-Stufe nach der Pixelgroesse in Metern; selbst bestimmt, weil Textur-Zugriffe in
// Verzweigungen sonst keine gueltigen Ableitungen haben.
float stufe(float px, float meter) {
	return clamp(log2(max(px, 0.00001) / (meter / 512.0)), 0.0, 9.0);
}

float hash21(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float wert(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash21(i), hash21(i + vec2(1.0, 0.0)), f.x),
			mix(hash21(i + vec2(0.0, 1.0)), hash21(i + vec2(1.0, 1.0)), f.x), f.y);
}

// Gibt es in dieser Zelle ein Fenster? Zellen von 2,7 m Breite und 3,2 m Hoehe, jede zehnte
// bleibt leer, damit die Fassade nicht wie ein Raster wirkt.
vec3 fassade_detail(vec3 wand, vec3 n, vec3 vw, float px) {
	vec3 t = normalize(cross(vec3(0.0, 1.0, 0.0), n));
	float u = dot(welt, t);
	float v = welt.y;
	const float BREITE = 2.7;
	const float HOEHE = 3.2;
	vec2 zelle = vec2(floor(u / BREITE), floor(v / HOEHE));
	vec2 lokal = vec2(u - zelle.x * BREITE, v - zelle.y * HOEHE);
	// Die Wahl haengt nur an der Zelle und der Wandrichtung, sonst flimmert ein Fenster.
	float zufall = hash21(zelle + floor(n.xz * 5.0 + 0.5) * 13.7);
	// Putz: feines Korn und grosse Flecken.
	float korn = wert(vec2(u, v) * 9.0) * 0.5 + wert(vec2(u, v) * 1.3) * 0.5;
	vec3 putz = wand * (0.93 + 0.14 * korn);
	vec3 putz_foto = textureLod(detail_putz, vec2(u, v) / 3.0, stufe(px, 3.0)).rgb;
	vec3 putz_mittel = textureLod(detail_putz, vec2(0.5), 9.0).rgb;
	if (dot(putz_mittel, vec3(0.333)) > 0.02) {
		putz = wand * clamp(putz_foto / max(putz_mittel, vec3(0.05)), 0.65, 1.35);
	}
	if (zufall > 0.9) {
		return putz;
	}
	// Die Oeffnung: 1,15 m breit, 1,55 m hoch, Brusthoehe 0,95 m.
	vec2 mitte = vec2(BREITE * 0.5, 0.95 + 0.775);
	vec2 halb = vec2(0.575, 0.775);
	vec2 d = abs(lokal - mitte) - halb;
	float aussen = max(d.x, d.y);
	// Fensterbank: ein Streifen unter der Oeffnung, davor ein Schatten auf dem Putz.
	vec2 bank = abs(lokal - vec2(mitte.x, 0.95 - 0.04)) - vec2(halb.x + 0.12, 0.04);
	if (max(bank.x, bank.y) < 0.0) {
		return wand * 1.25 + vec3(0.04);
	}
	float schatten_unten = smoothstep(0.0, 0.35, 0.95 - 0.08 - lokal.y)
			* step(abs(lokal.x - mitte.x), halb.x + 0.2) * step(lokal.y, 0.95 - 0.08)
			* step(0.95 - 0.5, lokal.y);
	putz *= 1.0 - 0.28 * schatten_unten;
	// Fensterlaeden an der Haelfte der Fenster.
	if (zufall > 0.45 && zufall <= 0.8) {
		vec2 ld = vec2(abs(lokal.x - mitte.x) - halb.x, abs(lokal.y - mitte.y) - halb.y);
		if (ld.x > 0.0 && ld.x < 0.46 && ld.y < 0.0) {
			vec3 ladenfarbe = zufall < 0.62 ? vec3(0.05, 0.16, 0.07) : (zufall < 0.72
					? vec3(0.18, 0.08, 0.04) : vec3(0.55, 0.55, 0.52));
			float latten = 0.85 + 0.15 * step(0.5, fract(lokal.y * 11.0));
			return ladenfarbe * latten;
		}
	}
	if (aussen > 0.0) {
		// Dunkler Schatten um die Oeffnung, oben am staerksten (Sturz).
		float rand = smoothstep(0.3, 0.0, aussen);
		float sturz = step(0.0, lokal.y - mitte.y) * 0.5 + 0.5;
		return putz * (1.0 - 0.35 * rand * sturz);
	}
	// In der Oeffnung: durch die Tiefe schauen (Parallaxe), Laibung und Rahmen sichtbar.
	float tiefe = 0.28;
	float vn = max(dot(vw, n), 0.25);
	vec2 versatz = -vec2(dot(vw, t), vw.y) / vn * tiefe;
	vec2 q = lokal + versatz;
	vec2 dq = abs(q - mitte) - (halb - vec2(0.07));
	if (max(dq.x, dq.y) > 0.0) {
		// Laibung (die Seitenflaeche der Mauer im Fenster) und der helle Rahmen.
		float rahmen = step(max(abs(lokal.x - mitte.x) - halb.x, abs(lokal.y - mitte.y) - halb.y), -0.06);
		return mix(wand * 0.55, wand * 0.9 + vec3(0.12), 1.0 - rahmen);
	}
	// Glas: dunkel mit Himmelsspiegelung oben und einem Vorhang bei manchen Fenstern.
	float hoehe = clamp((q.y - 0.95) / 1.55, 0.0, 1.0);
	vec3 glas = mix(vec3(0.02, 0.03, 0.045), vec3(0.10, 0.17, 0.26), hoehe);
	float steg = step(abs(q.x - mitte.x), 0.025) + step(abs(q.y - mitte.y - 0.1), 0.025);
	glas = mix(glas, wand * 0.8 + vec3(0.08), clamp(steg, 0.0, 1.0));
	if (zufall > 0.2 && zufall < 0.34) {
		glas = mix(glas, vec3(0.45, 0.38, 0.28), 0.55 * smoothstep(0.0, 0.6, 1.0 - hoehe));
	}
	return glas;
}

// Mit der Entfernung verschwimmen die Einzelheiten zu einem mittleren Farbton, sonst
// flimmern die Fenster (ein Pixel deckt dann mehrere Zentimeter bis Meter ab).
vec3 fassade(vec3 wand, vec3 n, vec3 vw, float px) {
	float pixel = px * 1.6;
	float weich = smoothstep(0.035, 0.12, pixel);
	vec3 mittel = mix(wand * 0.97, vec3(0.05, 0.08, 0.12), 0.30);
	if (weich >= 1.0) {
		return mittel;
	}
	return mix(fassade_detail(wand, n, vw, px), mittel, weich);
}

// Ziegeldach: Reihen von Biberschwaenzen, jede Reihe um eine halbe Ziegelbreite versetzt,
// je Ziegel ein eigener Ton, dunkle Fugen und Schatten unter der ueberlappenden Reihe.
vec3 dach(vec3 farbe_daten, vec3 wn, float px) {
	vec3 t = normalize(cross(vec3(0.0, 1.0, 0.0), wn));
	vec3 hang = cross(wn, t);
	vec2 uv = vec2(dot(welt, t), dot(welt, hang));
	float reihe = uv.y / 0.36;
	float rid = floor(reihe);
	float spalte = (uv.x + 0.15 * mod(rid, 2.0)) / 0.30;
	float sid = floor(spalte);
	vec2 f = vec2(fract(spalte), fract(reihe));
	float id = hash21(vec2(sid, rid));
	// Terrakotta; die Datenfarbe faerbt leicht um (graue und braune Daecher bleiben moeglich).
	vec3 terrakotta = mix(vec3(0.40, 0.11, 0.045), vec3(0.66, 0.25, 0.10), id);
	vec3 foto = textureLod(detail_dach, uv / 3.2, stufe(px, 3.2)).rgb;
	float foto_da = dot(textureLod(detail_dach, uv, 9.0).rgb, vec3(0.333));
	vec3 ton = mix(terrakotta, terrakotta * (farbe_daten / max(0.15, dot(farbe_daten, vec3(0.333)))), 0.3);
	ton *= 0.82 + 0.34 * wert(uv * 0.55) + 0.06 * (wert(uv * 6.0) - 0.5);
	float naht = smoothstep(0.0, 0.07, f.y) * smoothstep(0.0, 0.05, f.x) * smoothstep(1.0, 0.95, f.x);
	float ueberlappung = mix(0.62, 1.0, smoothstep(0.0, 0.55, f.y));
	vec3 procedural = ton * (0.5 + 0.5 * naht) * ueberlappung;
	vec3 detail = foto_da > 0.02 ? foto * (0.85 + 0.3 * wert(uv * 0.55))
			* mix(vec3(1.0), farbe_daten / max(0.15, dot(farbe_daten, vec3(0.333))), 0.2) : procedural;
	float pixel = px * 1.6;
	float weich = smoothstep(0.12, 0.45, pixel);
	return mix(detail, vec3(0.50, 0.17, 0.07) * (0.8 + 0.4 * wert(uv * 0.55)), weich);
}

// Pflaster: Steine von 22 x 14 cm im Verband, je Stein ein anderer Grauton, dunkle Fugen.
vec3 pflaster(vec3 farbe_daten, float px) {
	vec2 uv = welt.xz;
	float reihe = uv.y / 0.14;
	float rid = floor(reihe);
	float spalte = (uv.x + 0.11 * mod(rid, 2.0)) / 0.22;
	vec2 f = vec2(fract(spalte), fract(reihe));
	float id = hash21(vec2(floor(spalte), rid));
	vec3 stein = farbe_daten * (0.72 + 0.4 * id) * (0.9 + 0.2 * wert(uv * 0.8));
	vec3 foto = textureLod(detail_pflaster, uv / 1.9, stufe(px, 1.9)).rgb;
	float foto_da = dot(textureLod(detail_pflaster, uv, 9.0).rgb, vec3(0.333));
	if (foto_da > 0.02) {
		vec3 gefaerbt = foto * mix(vec3(1.0), farbe_daten * 2.2, 0.3);
		return mix(gefaerbt, farbe_daten * 0.9, smoothstep(0.08, 0.3, px * 1.6));
	}
	float fuge = smoothstep(0.0, 0.1, f.x) * smoothstep(1.0, 0.9, f.x) * smoothstep(0.0, 0.12, f.y)
			* smoothstep(1.0, 0.88, f.y);
	float pixel = px * 1.6;
	float weich = smoothstep(0.08, 0.3, pixel);
	return mix(stein * (0.45 + 0.55 * fuge), farbe_daten * 0.9, weich);
}

void vertex() {
	welt = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	if (oben) {
		NORMAL = vec3(0.0, 1.0, 0.0);
	}
}

void fragment() {
	vec3 n = normalize(cross(dFdx(VERTEX), dFdy(VERTEX)));
	if (dot(n, VIEW) < 0.0) {
		n = -n;
	}
	NORMAL = n;
	// Ableitungen nur hier, in einheitlichem Ablauf: innerhalb von Verzweigungen sind sie
	// undefiniert und flimmern.
	// Aus der Sicht-Position (kleine Zahlen, genau) statt aus Weltkoordinaten um 1600 m,
	// deren Ableitungen nah an der Kamera rauschen.
	vec3 wn = normalize((INV_VIEW_MATRIX * vec4(n, 0.0)).xyz);
	float px = length(fwidth(VERTEX));
	vec3 c = farbe;
	float rau = rauheit;
	if (stil == 1) {
		// Grosse und kleine Flecken statt einer einzigen Flaechenfarbe, am steilen Hang
		// kommt die Erde durch.
		float g = wert(welt.xz / 41.0) * 0.55 + wert(welt.xz / 8.0) * 0.3
				+ wert(welt.xz / 1.9) * 0.15;
		c *= 0.80 + 0.40 * g;
		vec3 wiese = textureLod(detail_wiese, welt.xz / 5.5, stufe(px, 5.5)).rgb;
		vec3 wiese_mittel = textureLod(detail_wiese, vec2(0.5), 9.0).rgb;
		if (dot(wiese_mittel, vec3(0.333)) > 0.02) {
			c *= clamp(wiese / max(wiese_mittel, vec3(0.04)), 0.55, 1.6);
		}
		// Grosse Felder: Gruenland mit Flecken aus reifem Weizen und gemaehten Streifen.
		float feld = wert(welt.xz / 230.0 + 3.1);
		float reif = smoothstep(0.58, 0.68, feld);
		float streifen = 0.5 + 0.5 * sin(dot(welt.xz, vec2(0.83, 0.56)) * 3.9);
		vec3 gruen = vec3(0.13, 0.27, 0.05) * (0.85 + 0.35 * g) * (0.93 + 0.07 * streifen);
		vec3 weizen = vec3(0.34, 0.29, 0.12) * (0.85 + 0.3 * g) * (0.9 + 0.1 * streifen);
		c = mix(c, mix(gruen, weizen, reif), 0.8);
		float steil = smoothstep(0.93, 0.72, abs(wn.y));
		c = mix(c, vec3(0.20, 0.13, 0.07) * (0.8 + 0.4 * g), steil);
	} else if (stil == 4) {
		if (abs(wn.y) > 0.2) {
			c = dach(farbe, wn, px);
		}
	} else if (stil == 5) {
		c = pflaster(farbe, px);
	} else if (stil == 6) {
		// Laub: kleine Blattbueschel in hellen und dunklen Gruentoenen.
		float busch = wert(welt.xz * 2.3 + welt.y * 1.7) * 0.6 + wert(welt.xz * 7.0 + welt.y * 5.0) * 0.4;
		float weich = smoothstep(0.15, 0.5, px * 1.7);
		c = mix(farbe * (0.6 + 0.8 * busch) * vec3(0.95, 1.1, 0.8), farbe, weich);
	} else if (stil == 3) {
		if (abs(wn.y) < 0.35) {
			vec3 vw = normalize((INV_VIEW_MATRIX * vec4(VIEW, 0.0)).xyz);
			c = fassade(farbe, wn, vw, px);
		}
	} else if (stil == 2) {
		float w1 = wert(welt.xz / 11.0 + vec2(TIME * 0.05, TIME * 0.03));
		float w2 = wert(welt.xz / 2.6 - vec2(TIME * 0.11, TIME * 0.07));
		c *= 0.78 + 0.45 * w1;
		float rand = pow(1.0 - clamp(dot(n, VIEW), 0.0, 1.0), 3.0);
		c = mix(c, vec3(0.30, 0.45, 0.65), rand * 0.65);
		EMISSION = vec3(0.9, 0.95, 1.0) * pow(w2, 9.0) * 0.55;
		rau = 0.06;
	}
	ALBEDO = c;
	ROUGHNESS = rau;
	SPECULAR = 0.2 + 0.6 * glanz;
	METALLIC = 0.0;
}
