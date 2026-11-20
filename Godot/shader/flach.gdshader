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
// 3 = Hausfassade (eingelassene Fenster mit Laibung, Rahmen, Bank und Laeden, Putz).
uniform int stil = 0;

varying vec3 welt;

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
vec3 fassade_detail(vec3 wand, vec3 n, vec3 vw) {
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
vec3 fassade(vec3 wand, vec3 n, vec3 vw) {
	vec3 t = normalize(cross(vec3(0.0, 1.0, 0.0), n));
	float pixel = fwidth(dot(welt, t)) + fwidth(welt.y);
	float weich = smoothstep(0.035, 0.12, pixel);
	vec3 mittel = mix(wand * 0.97, vec3(0.05, 0.08, 0.12), 0.30);
	if (weich >= 1.0) {
		return mittel;
	}
	return mix(fassade_detail(wand, n, vw), mittel, weich);
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
	vec3 c = farbe;
	float rau = rauheit;
	if (stil == 1) {
		// Grosse und kleine Flecken statt einer einzigen Flaechenfarbe, am steilen Hang
		// kommt die Erde durch.
		float g = wert(welt.xz / 41.0) * 0.55 + wert(welt.xz / 8.0) * 0.3
				+ wert(welt.xz / 1.9) * 0.15;
		c *= 0.80 + 0.40 * g;
		vec3 wn = normalize(cross(dFdx(welt), dFdy(welt)));
		float steil = smoothstep(0.93, 0.72, abs(wn.y));
		c = mix(c, vec3(0.20, 0.13, 0.07) * (0.8 + 0.4 * g), steil);
	} else if (stil == 3) {
		vec3 wn = normalize(cross(dFdx(welt), dFdy(welt)));
		if (abs(wn.y) < 0.35) {
			vec3 vw = normalize((INV_VIEW_MATRIX * vec4(VIEW, 0.0)).xyz);
			c = fassade(farbe, wn, vw);
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
