shader_type spatial;
render_mode cull_disabled;

uniform vec3 farbe = vec3(0.4);
uniform float rauheit = 0.86;
uniform bool oben = false;
// 0 = fest, 1 = Gelände, 2 = Wasser, 3 = Fassade, 4 = Sockel, 5 = Ladenzone.
uniform int stil = 0;
uniform bool hat_detail = false;
uniform sampler2D detail : hint_default_white, filter_linear_mipmap_anisotropic, repeat_enable;
uniform float kachel_m = 2.0;
uniform bool ist_dach = false;

varying vec3 welt;
varying vec3 blickposition;

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

float rechteck(vec2 p, vec2 halb, float aa) {
	vec2 d = abs(p) - halb;
	return 1.0 - smoothstep(-aa, aa, max(d.x, d.y));
}

float weg(vec2 p, vec2 a, vec2 b, float breite) {
	vec2 v = b - a;
	float anteil = clamp(dot(p - a, v) / dot(v, v), 0.0, 1.0);
	return 1.0 - smoothstep(breite, breite + 0.38, distance(p, a + v * anteil));
}

vec3 schulhof(vec3 c, vec2 p) {
	float aa = clamp(max(fwidth(p.x), fwidth(p.y)) * 0.55, 0.025, 0.35);
	vec2 rasen = p - vec2(-1131.0, -602.0);
	float spielfeld = rechteck(rasen, vec2(24.0, 28.5), 0.60);
	c = mix(c, vec3(0.092, 0.151, 0.065) * (0.93 + 0.11 * cos(rasen.y * 0.69)), spielfeld);
	vec2 hof = p - vec2(-1114.6, -548.2);
	vec2 laufbahn = p - vec2(-1104.8, -600.2);
	float rot = max(rechteck(hof, vec2(10.1, 13.6), 0.18),
			rechteck(laufbahn, vec2(2.70, 31.5), 0.13));
	float aussen = rechteck(hof, vec2(8.7, 11.6), aa) - rechteck(hof, vec2(8.61, 11.51), aa);
	float mitte = rechteck(hof, vec2(8.7, 0.045), aa);
	float kreis = 1.0 - smoothstep(0.035, 0.035 + aa, abs(length(hof) - 2.15));
	float spur = 1.0 - smoothstep(0.036, 0.036 + aa, abs(fract((laufbahn.x + 2.04) / 1.33 + 0.5) - 0.5) * 1.33);
	spur = max(spur, rechteck(vec2(0.0, abs(laufbahn.y) - 27.0), vec2(1.0, 0.045), aa));
	vec3 belag = vec3(0.31, 0.124, 0.096) * (0.83 + 0.24 * wert(p * 0.68));
	c = mix(c, belag, rot);
	c = mix(c, vec3(0.52, 0.45, 0.33), max(max(aussen, mitte), kreis) * 0.78);
	c = mix(c, vec3(0.52, 0.45, 0.33), spur * rechteck(laufbahn, vec2(2.18, 29.7), aa) * 0.83);
	float rundweg = 1.0 - smoothstep(1.35, 2.0, abs(distance(p, vec2(-1147.15, -498.8)) - 23.0));
	float wege = max(rundweg, weg(p, vec2(-1151.0, -521.0), vec2(-1157.0, -572.0), 1.42));
	wege = max(wege, weg(p, vec2(-1157.0, -572.0), vec2(-1157.0, -630.5), 1.04));
	wege = max(wege, weg(p, vec2(-1157.0, -573.0), vec2(-1107.5, -573.0), 1.0));
	wege = max(wege, weg(p, vec2(-1185.0, -582.0), vec2(-1173.0, -592.0), 1.18));
	wege = max(wege, weg(p, vec2(-1173.0, -592.0), vec2(-1187.0, -610.0), 1.10));
	return mix(c, vec3(0.25, 0.227, 0.182) * (0.90 + 0.18 * wert(p)), wege);
}

vec3 schulwand(vec3 c, vec2 uv) {
	vec2 d = welt.xz - vec2(-1147.15, -498.8);
	if (welt.x <= -1167.5 && welt.x >= -1213.0 && welt.z > -500.1 && welt.z < -480.0
			&& welt.y < 24.60 && length(d) > 21.65) {
		vec2 p = vec2(fract(uv.x * 1.55) - 0.5, welt.y - 22.65);
		float fenster = rechteck(p, vec2(0.32, 1.09), 0.03);
		float lamellen = 0.76 + 0.24 * cos(welt.y * 72.0)
				* (1.0 - smoothstep(0.018, 0.055, fwidth(welt.y)));
		return mix(vec3(0.37, 0.070, 0.054), vec3(0.186, 0.207, 0.214) * lamellen, fenster);
	}
	if (welt.x > -1156.1 && welt.x < -1129.9 && welt.z > -563.4 && welt.z < -532.3) {
		float band = smoothstep(24.6, 24.72, welt.y) - smoothstep(26.17, 26.26, welt.y);
		float sprosse = smoothstep(0.44, 0.48, abs(fract(uv.x * 2.0) - 0.5));
		return mix(vec3(0.48, 0.49, 0.45), vec3(0.064, 0.104, 0.135), band * (1.0 - sprosse));
	}
	if (length(d) < 22.2 && length(d) > 19.7 && welt.y < 27.10) {
		float bogen = atan(d.y, d.x);
		float raster = bogen * (32.0 / 6.2831853);
		float hoehendetail = mod(welt.y - 20.3, 3.52) - 1.72;
		vec2 p = vec2(fract(raster) - 0.5, hoehendetail);
		float aa = clamp(fwidth(raster) * 0.55, 0.01, 0.18);
		float metall = 0.85 + 0.15 * cos(bogen * 846.0)
				* (1.0 - smoothstep(0.0018, 0.0065, fwidth(bogen)));
		float blau = smoothstep(22.91, 22.97, welt.y) - smoothstep(24.0, 24.06, welt.y);
		vec3 blech = mix(vec3(0.53, 0.55, 0.56) * metall, vec3(0.054, 0.122, 0.24), blau);
		float rahmen = rechteck(p, vec2(0.355, 1.16), aa);
		float glas = rechteck(p, vec2(0.323, 1.05), aa);
		float jalousie = step(0.40, hash21(vec2(floor(raster), floor((welt.y - 20.3) / 3.52))));
		vec3 fuellung = mix(vec3(0.061, 0.114, 0.145), vec3(0.54, 0.56, 0.57) * metall, jalousie);
		blech = mix(blech, vec3(0.51, 0.125, 0.081), rahmen);
		return mix(blech, fuellung, glas);
	}
	return c;
}

vec3 klinikwand(vec2 uv, inout float rau, inout float glanz) {
	vec2 raster = vec2(uv.x * 1.65, uv.y);
	vec2 p = fract(raster) - 0.5;
	vec2 f = fwidth(raster);
	float aa = clamp(max(f.x, f.y) * 0.62, 0.001, 0.14);
	float blech = abs(fract(welt.y / 0.25) - 0.5);
	float falz = (1.0 - smoothstep(0.030, 0.030 + aa, blech))
			* (1.0 - smoothstep(0.04, 0.20, fwidth(welt.y / 0.25)));
	vec3 c = vec3(0.235, 0.282, 0.325) * (1.0 - 0.14 * falz);
	float obere_reihe = step(39.2, welt.y);
	vec2 einfassung = vec2(mix(0.495, 0.443, obere_reihe), mix(0.188, 0.299, obere_reihe));
	float rahmen = rechteck(p, einfassung, aa);
	float fuellung = rechteck(p, einfassung - vec2(0.040, 0.025), aa);
	float nummer = mod(floor(raster.x), 5.0);
	float akzent = rechteck(p - vec2(0.329, 0.0), vec2(0.101, einfassung.y - 0.034), aa)
			* (1.0 - obere_reihe);
	vec3 akzentfarbe = mix(vec3(0.04, 0.20, 0.34), vec3(0.68, 0.42, 0.054), step(1.0, mod(nummer, 3.0)));
	float scheibenton = hash21(floor(raster));
	vec3 glas = mix(vec3(0.035, 0.069, 0.089), vec3(0.118, 0.184, 0.218), scheibenton);
	glas *= mix(0.79, 1.24, smoothstep(-0.2, 0.31, p.y));
	c = mix(c, vec3(0.65, 0.69, 0.70), rahmen * 0.87);
	c = mix(c, glas, fuellung);
	c = mix(c, akzentfarbe, akzent * fuellung);
	rau = mix(0.6, 0.27, fuellung * (1.0 - akzent));
	glanz = mix(0.25, 0.47, fuellung);
	return c;
}

vec3 fassade(vec3 c, vec2 uv, inout float rau, inout float glanz) {
	vec2 uv_dx = dFdx(uv);
	vec2 uv_dy = dFdy(uv);
	float det = uv_dx.x * uv_dy.y - uv_dx.y * uv_dy.x;
	if (abs(det) < 0.0000001) {
		return c;
	}
	// Aus den UV-Ableitungen folgt der gemeinsame Bezugspunkt einer Wand.
	// Farbe und Fensterbauart bleiben über die Stockwerke hinweg konstant.
	vec3 dx = dFdx(welt);
	vec3 dy = dFdy(welt);
	vec3 u = (dx * uv_dy.y - dy * uv_dx.y) / det;
	vec3 v = (dy * uv_dx.x - dx * uv_dy.x) / det;
	// Der LoD2-Hauptbau hat 18,87 m Fassadenhöhe bei 5,1875 UV-Einheiten.
	if (welt.x > -1655.0 && welt.x < -1534.0 && welt.z > 78.0 && welt.z < 198.0
			&& abs(v.y - 3.6376) < 0.045) {
		return klinikwand(uv, rau, glanz);
	}
	vec2 anker = (welt - u * uv.x - v * uv.y).xz;
	vec2 kennung = floor(anker / 4.0 + 0.5);
	float typ = hash21(kennung);
	float details = hash21(kennung + 19.0);
	bool altstadt = welt.x > -240.0 && welt.x < 410.0 && abs(welt.z) < 430.0;
	vec2 koordinaten = vec2(uv.x, 1.0 - uv.y);
	// Die Platzfront des Historischen Rathauses hat eine eigene Fensterordnung.
	bool rathaus = abs(welt.x + 40.5) < 0.23 && abs(welt.z + 27.25) < 6.22
			&& welt.y > 6.0 && welt.y < 22.22;
	if (rathaus) {
		koordinaten.y = 1.0 - (uv.y - 0.85) * (3.0 / 4.15);
	}
	vec2 p = fract(koordinaten) - vec2(0.5, 0.48);
	float aa = clamp(max(fwidth(uv.x), fwidth(uv.y)) * 0.62, 0.001, 0.115);
	float roh = texture(detail, koordinaten).r;
	float putz = clamp(roh, 0.37, 0.52);
	float koernung = wert(welt.xz * 0.71 + welt.y * 0.52);
	c *= mix(0.88, 1.13, typ) * mix(0.91, 1.06, koernung) * mix(0.94, 1.05, (putz - 0.37) / 0.15);
	if (altstadt) {
		vec3 pigment = mix(vec3(0.43, 0.28, 0.19), vec3(0.19, 0.31, 0.27), details);
		c = mix(c, pigment, 0.13 + 0.23 * typ);
	}
	vec2 halb = altstadt ? vec2(mix(0.138, 0.205, typ), mix(0.209, 0.282, details)) :
			vec2(mix(0.175, 0.214, details), mix(0.160, 0.219, typ));
	vec3 rahmenfarbe = mix(vec3(0.68, 0.57, 0.42), vec3(0.34, 0.28, 0.19),
			details * (altstadt ? 0.73 : 0.33));
	if (rathaus) {
		vec3 stuck = vec3(0.72, 0.59, 0.42);
		vec3 rosa = vec3(0.52, 0.25, 0.18);
		c = stuck * mix(0.83, 0.99, koernung);
		if (uv.y < 0.85) {
			vec2 tor = vec2(fract(uv.x * 0.6) - 0.5, (uv.y + 0.1875) / 1.0375);
			float kontur = length(vec2(tor.x, max(0.0, tor.y - 0.61) * 0.79));
			float tuer = 1.0 - smoothstep(0.146 - aa, 0.146 + aa, kontur);
			float bogen = 1.0 - smoothstep(0.168 - aa, 0.168 + aa, kontur);
			c = mix(c, rosa, bogen * 0.88);
			return mix(c, vec3(0.025, 0.044, 0.052), tuer);
		}
		rahmenfarbe = vec3(0.55, 0.37, 0.18);
		float etage = floor((uv.y - 0.85) * (3.0 / 4.15));
		halb = vec2(0.227, mix(0.206, 0.287, clamp(etage / 2.0, 0.0, 1.0)));
		// Gesimse, Lisenen und zwischen den Fenstern angedeutete Stuckmedaillons.
		float gesims = rechteck(vec2(0.0, p.y + 0.439), vec2(1.0, 0.027), aa);
		float lisene = smoothstep(0.456 - aa, 0.456 + aa, abs(p.x));
		float medal = 1.0 - smoothstep(0.015, 0.015 + aa,
				abs(length(vec2(p.x, (p.y - 0.38) * 1.42)) - 0.093));
		c = mix(c, rosa, max(max(gesims, lisene) * 0.70, medal * 0.77));
	} else if (altstadt && details > 0.47) {
		float breite = mix(0.054, 0.079, typ);
		vec2 laden = vec2(abs(p.x) - halb.x - breite - 0.039, p.y);
		float maske = rechteck(laden, vec2(breite, halb.y + 0.006), aa);
		float lamellen = 0.94 + 0.10 * cos(p.y * 156.0) * (1.0 - smoothstep(0.01, 0.06, aa));
		vec3 lack = mix(vec3(0.106, 0.19, 0.11), vec3(0.23, 0.095, 0.063), typ);
		c = mix(c, lack * lamellen, maske);
	}
	float fasche = rechteck(p, halb + vec2(0.039, 0.031), aa);
	float sims = rechteck(p - vec2(0.0, halb.y + 0.043), vec2(halb.x + 0.054, 0.016), aa);
	float glas = rechteck(p, halb, aa);
	float sprosse = rechteck(p, vec2(0.010, halb.y), aa);
	if (typ < 0.62 || altstadt) {
		sprosse = max(sprosse, rechteck(p - vec2(0.0, 0.02 - details * 0.09),
				vec2(halb.x, 0.009), aa));
	}
	float scheibe = glas * (1.0 - sprosse);
	float fensterton = hash21(kennung + floor(koordinaten) * vec2(0.73, 1.24));
	vec3 spiegelung = mix(vec3(0.029, 0.044, 0.050), vec3(0.103, 0.148, 0.170), fensterton);
	spiegelung *= mix(1.25, 0.72, smoothstep(-halb.y, halb.y, p.y));
	c = mix(c, rahmenfarbe, fasche * mix(0.62, 0.94, details));
	c = mix(c, rahmenfarbe * 0.76, glas * sprosse);
	c = mix(c, rahmenfarbe * 0.81, sims);
	c = mix(c, spiegelung, scheibe);
	rau = mix(rau, 0.32, scheibe);
	glanz = mix(glanz, 0.43, scheibe);
	return c;
}

vec3 ladenzone(vec2 uv, inout float rau, inout float glanz) {
	float aa = clamp(max(fwidth(uv.x), fwidth(uv.y)) * 0.6, 0.001, 0.105);
	vec2 p = vec2(fract(uv.x) - 0.5, uv.y);
	float ton = wert(welt.xz / 12.0);
	vec3 holz = mix(vec3(0.12, 0.057, 0.036), vec3(0.066, 0.12, 0.093), ton);
	vec3 leiste = mix(holz * 1.65, vec3(0.28, 0.21, 0.13), ton * 0.52);
	float tuer = step(0.66, hash21(vec2(floor(uv.x), floor(welt.y - uv.y * 2.30 + 0.5))));
	float basis = mix(0.42, 0.36, tuer);
	vec2 fenster = p - vec2(0.0, basis);
	float halb = mix(0.269, 0.332, tuer);
	float rahmen = rechteck(fenster, vec2(0.408, halb + 0.034), aa);
	float glas = rechteck(fenster, vec2(0.373, halb), aa);
	float sprossen = max(rechteck(fenster, vec2(0.013, halb), aa),
			rechteck(fenster - vec2(0.0, halb * 0.52), vec2(0.373, 0.012), aa));
	float deckleiste = rechteck(p - vec2(0.0, 0.903), vec2(0.482, 0.064), aa);
	float sockel = 1.0 - smoothstep(0.046, 0.052 + aa, uv.y);
	vec3 c = mix(holz, leiste, max(rahmen, deckleiste * 0.7));
	c = mix(c, vec3(0.22, 0.18, 0.13), sockel);
	float licht = smoothstep(0.19, 0.82, uv.y);
	vec3 scheibe = mix(vec3(0.024, 0.041, 0.046), vec3(0.104, 0.156, 0.168), licht);
	scheibe += vec3(0.07, 0.048, 0.026) * ton * (1.0 - licht);
	glas *= 1.0 - sprossen;
	rau = mix(0.82, 0.28, glas);
	glanz = mix(0.19, 0.42, glas);
	return mix(c, scheibe, glas);
}

void vertex() {
	welt = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	blickposition = (MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
	if (oben) {
		// Auch der Schattenpass braucht eine Normale für seinen Normal-Bias.
		NORMAL = vec3(0.0, 1.0, 0.0);
	}
}

void fragment() {
	vec3 n = normalize(cross(dFdx(blickposition), dFdy(blickposition)));
	if (dot(n, VIEW) < 0.0) {
		n = -n;
	}
	vec3 wn = mat3(INV_VIEW_MATRIX) * n;
	vec3 c = farbe;
	float rau = rauheit;
	float glanz = 0.25;
	bool platanenschule = welt.x > -1214.0 && welt.x < -1099.0 && welt.z > -634.0 && welt.z < -475.0;

	if (stil == 1) {
		float variation = wert(welt.xz / 27.0) * 0.72 + wert(welt.xz / 5.0) * 0.28;
		// Begrünter Uferstreifen am Lech, auf die Geländeflächen beschränkt.
		float ufer_x = -278.0 + (welt.z - 160.0) * 0.35;
		float ufer = (1.0 - smoothstep(12.0, 23.0, abs(welt.x - ufer_x)))
				* smoothstep(121.0, 150.0, welt.z) * (1.0 - smoothstep(214.0, 245.0, welt.z));
		c = mix(c, vec3(0.113, 0.158, 0.067), ufer * 0.91);
		c *= mix(0.72, 1.23, variation);
		float hang = 1.0 - smoothstep(0.73, 0.94, abs(wn.y));
		c = mix(c, vec3(0.16, 0.115, 0.071) * mix(0.85, 1.25, variation), hang * 0.8);
		if (platanenschule && welt.x > -1192.0) {
			c = schulhof(c, welt.xz);
		}
	} else if (stil == 2) {
		// Die Dreiecke des Flussbetts sollen sich in der Spiegelung nicht abzeichnen.
		float phase1 = dot(welt.xz, vec2(2.4, 1.4));
		float phase2 = dot(welt.xz, vec2(-1.6, 3.2));
		float w1 = sin(phase1 + TIME * 1.35) * (1.0 - smoothstep(0.25, 1.3, fwidth(phase1)));
		float w2 = sin(phase2 - TIME * 1.7) * (1.0 - smoothstep(0.25, 1.3, fwidth(phase2)));
		vec3 welle = vec3(w1 * 0.036 - w2 * 0.019, 1.0, w1 * 0.021 + w2 * 0.038);
		n = normalize(mat3(VIEW_MATRIX) * welle);
		float spiegelung = pow(1.0 - clamp(dot(n, VIEW), 0.0, 1.0), 4.0);
		c *= mix(0.75, 1.19, wert(welt.xz / 12.0));
		c = mix(c, vec3(0.26, 0.37, 0.40), spiegelung * 0.65);
		glanz = 0.72;
	} else if (stil == 4) {
		float entlang = abs(wn.z) > abs(wn.x) ? welt.x : welt.z;
		float reihe = floor(welt.y / 0.37);
		vec2 mauer = vec2(entlang / 0.82 + mod(reihe, 2.0) * 0.5, welt.y / 0.37);
		vec2 in_stein = fract(mauer);
		float kante = min(min(in_stein.x, 1.0 - in_stein.x), min(in_stein.y, 1.0 - in_stein.y));
		float fugen = 1.0 - smoothstep(0.022, 0.022 + 1.2 * max(fwidth(mauer.x), fwidth(mauer.y)), kante);
		fugen *= 1.0 - smoothstep(0.07, 0.32, max(fwidth(mauer.x), fwidth(mauer.y)));
		float stein = hash21(floor(mauer));
		c *= mix(0.83, 1.16, stein) * mix(1.0, 0.74, fugen);
	} else if (stil == 5 && abs(wn.y) < 0.65) {
		c = ladenzone(UV, rau, glanz);
	}

	if (hat_detail) {
		if (stil == 3) {
			c = fassade(c, UV, rau, glanz);
		} else {
			vec3 muster = texture(detail, welt.xz / kachel_m).rgb;
			float struktur = dot(muster, vec3(0.299, 0.587, 0.114));
			c *= clamp(mix(1.0, struktur * 2.0, 0.54), 0.68, 1.23);
		}
	}
	if (platanenschule && stil == 3) {
		c = schulwand(c, UV);
	}
	if (ist_dach && platanenschule) {
		vec2 d = welt.xz - vec2(-1147.15, -498.8);
		if (length(d) < 22.30 && welt.y > 27.04 && welt.y < 28.05) {
			float winkel = atan(d.y, d.x);
			float naht = 1.0 - smoothstep(0.011, 0.011 + min(0.22, fwidth(winkel) * 16.0),
					abs(fract(winkel * (72.0 / 6.2831853)) - 0.5));
			c = vec3(0.56, 0.60, 0.63) * (1.0 - 0.18 * naht);
			rau = 0.57;
		} else if (welt.x > -1156.1 && welt.x < -1129.9 && welt.z > -563.4 && welt.z < -532.3
				&& welt.y > 27.97 && welt.y < 28.19) {
			float rand = rechteck(welt.xz - vec2(-1143.0, -547.9), vec2(12.0, 14.34), 0.20);
			float moos = wert(welt.xz * 1.19);
			c = mix(vec3(0.53, 0.55, 0.51), mix(vec3(0.17, 0.19, 0.085), vec3(0.28, 0.29, 0.123), moos), rand);
		} else if (welt.x <= -1167.5 && welt.z > -500.1 && welt.z < -480.0 && welt.y < 25.06) {
			c = vec3(0.32, 0.33, 0.32) * (0.91 + 0.15 * wert(welt.xz * 0.71));
		}
	}

	NORMAL = n;
	ALBEDO = OUTPUT_IS_SRGB ? pow(c, vec3(1.0 / 2.2)) : c;
	ROUGHNESS = rau;
	SPECULAR = glanz;
}
