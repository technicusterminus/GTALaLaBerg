// Autolack und Glas mit glatten Vertex-Normalen (anders als flach.gdshader, das Flaechen
// flach schattiert). Lack: glaenzend mit Randlicht; Glas: dunkel mit Himmelsspiegelung.
// Mit instanziert = true kommt der Lack aus der Instanzfarbe eines MultiMesh.
shader_type spatial;

uniform vec3 farbe = vec3(0.5);
uniform bool lackiert = false;
uniform bool instanziert = false;
uniform bool glas = false;
uniform float rauheit : hint_range(0.0, 1.0) = 0.35;
uniform float metall : hint_range(0.0, 1.0) = 0.0;
uniform vec3 leuchten = vec3(0.0);
// Linienzeichnung (Tuerfugen, Fenster, Leisten) als Detail auf dem Lack: Seitenansicht und
// Draufsicht, auf ihre Umrisse beschnitten (u ueber die Laenge, vorn = 0).
uniform bool detail_an = false;
uniform sampler2D tex_seite : source_color, filter_linear_mipmap, repeat_disable;
uniform sampler2D tex_oben : source_color, filter_linear_mipmap, repeat_disable;
uniform float laenge = 4.3;
uniform float seite_hoehe = 1.59;
uniform float oben_breite = 1.94;
uniform sampler2D tex_front : source_color, filter_linear_mipmap, repeat_disable;
uniform sampler2D tex_heck : source_color, filter_linear_mipmap, repeat_disable;
uniform float front_breite = 2.08;
uniform float heck_breite = 2.08;

// Je Wagen (set_instance_shader_parameter): Bremslicht 0..1, Blinker -1 links, +1 rechts,
// 2 Warnblinker, 0 aus; Abblendlicht 0..1. Die Lampen selbst stehen in der Zeichnung: rote
// Flaechen hinten, orange an den Ecken, helle Scheinwerfer vorn.
instance uniform float bremse = 0.0;
instance uniform float blinker = 0.0;
instance uniform float licht = 0.0;

// Relief: im Alpha der Ansichten steht eine Hoehenkarte (Tools/baue_auto_modelle.py,
// dunkel = tief). Daraus Parallaxe (Fugen, Grill, Scheiben liegen sichtbar tiefer) und
// gekippte Normalen (Kanten fangen Licht). relief: Tiefe in Metern.
uniform float relief = 0.012;

varying vec3 instanzfarbe;
varying vec3 lok;
varying vec3 lnorm;
varying vec3 lok_blick;   // vom Punkt zur Kamera, in Wagenkoordinaten
varying vec3 achse_x;     // Wagenachsen im Blickraum (fuer die gekippten Normalen)
varying vec3 achse_y;
varying vec3 achse_z;

void vertex() {
	instanzfarbe = COLOR.rgb;
	lok = VERTEX;
	lnorm = NORMAL;
	vec3 kamera = (inverse(MODEL_MATRIX) * vec4(INV_VIEW_MATRIX[3].xyz, 1.0)).xyz;
	lok_blick = kamera - VERTEX;
	achse_x = normalize((MODELVIEW_MATRIX * vec4(1.0, 0.0, 0.0, 0.0)).xyz);
	achse_y = normalize((MODELVIEW_MATRIX * vec4(0.0, 1.0, 0.0, 0.0)).xyz);
	achse_z = normalize((MODELVIEW_MATRIX * vec4(0.0, 0.0, 1.0, 0.0)).xyz);
}

// Eine Ansicht mit Parallaxe: p ist der Punkt in der Ebene (a, b) der Ansicht, n die
// Blickkomponente senkrecht dazu, uv(p) die Abbildung. Liefert Farbe und in stoer die
// Neigung (dh/da, dh/db) in Metern je Meter.
vec3 ansicht(sampler2D tex, vec2 uv, vec2 uv_je_m, vec2 blick_ebene, float blick_senk,
		out vec2 stoer) {
	float h = texture(tex, uv).a;
	vec2 schub = -blick_ebene / max(abs(blick_senk), 0.25) * (1.0 - h) * relief;
	vec2 uv2 = uv + schub * uv_je_m;
	vec2 e = 1.5 / vec2(textureSize(tex, 0));
	float ha = texture(tex, uv2 + vec2(e.x, 0.0)).a - texture(tex, uv2 - vec2(e.x, 0.0)).a;
	float hb = texture(tex, uv2 + vec2(0.0, e.y)).a - texture(tex, uv2 - vec2(0.0, e.y)).a;
	stoer = vec2(ha / (2.0 * e.x) * uv_je_m.x, hb / (2.0 * e.y) * uv_je_m.y) * relief;
	return texture(tex, uv2).rgb;
}

// Die Texturen der vier Ansichten, nach der Richtung der Flaeche gemischt. Neutrales Grau
// gilt als Lack (und wird mit der Wagenfarbe getoent), alles andere (Lichter, Gitter, Chrom,
// Glas, Reifen) bleibt, wie es gezeichnet ist. In a: wie sehr die Stelle Scheibe ist
// (dunkles, ungesaettigtes Grau oben am Wagen).
vec4 zeichnung(vec3 lack_farbe, out vec3 stoerung) {
	float u = 0.5 - lok.x / laenge;
	vec3 v = normalize(lok_blick);
	vec2 sg; vec2 og; vec2 fg; vec2 hg;
	// Seite: Ebene (x, y); u faellt mit x, v faellt mit y.
	vec3 ts = ansicht(tex_seite, vec2(u, 1.0 - lok.y / seite_hoehe),
			vec2(-1.0 / laenge, -1.0 / seite_hoehe), v.xy, v.z, sg);
	// Oben: Ebene (x, z).
	vec3 to = ansicht(tex_oben, vec2(u, lok.z / oben_breite + 0.5),
			vec2(-1.0 / laenge, 1.0 / oben_breite), v.xz, v.y, og);
	// Von vorn gesehen liegt die rechte Wagenseite (+Z) links im Bild, von hinten rechts.
	vec3 tf = ansicht(tex_front, vec2(0.5 - lok.z / front_breite, 1.0 - lok.y / seite_hoehe),
			vec2(-1.0 / front_breite, -1.0 / seite_hoehe), v.zy, v.x, fg);
	vec3 th = ansicht(tex_heck, vec2(0.5 + lok.z / heck_breite, 1.0 - lok.y / seite_hoehe),
			vec2(1.0 / heck_breite, -1.0 / seite_hoehe), v.zy, v.x, hg);
	float ws = pow(abs(lnorm.z), 2.0);
	float wo = pow(max(lnorm.y, 0.0), 2.0);
	float wf = pow(max(lnorm.x, 0.0), 2.0);
	float wh = pow(max(-lnorm.x, 0.0), 2.0);
	float wsum = max(ws + wo + wf + wh, 0.001);
	vec3 tc = (ts * ws + to * wo + tf * wf + th * wh) / wsum;
	// Neigung als Stoerung der Normalen in Wagenachsen (Anstieg der Hoehe kippt die Normale
	// gegen den Anstieg).
	stoerung = (vec3(-sg.x, -sg.y, 0.0) * ws + vec3(-og.x, 0.0, -og.y) * wo
			+ vec3(0.0, -fg.y, -fg.x) * wf + vec3(0.0, -hg.y, -hg.x) * wh) / wsum;
	float lum = dot(tc, vec3(0.333));
	float saett = max(max(tc.r, tc.g), tc.b) - min(min(tc.r, tc.g), tc.b);
	// Lack ist im Blatt helles Grau (um 0.72); dunkles Grau ist Glas, Gummi, Kunststoff.
	float ist_lack = (1.0 - smoothstep(0.03, 0.10, saett)) * smoothstep(0.32, 0.44, lum);
	float glasig = (1.0 - smoothstep(0.04, 0.12, saett)) * (1.0 - smoothstep(0.24, 0.34, lum))
			* step(0.10, lum) * smoothstep(0.3, 0.7, lok.y / seite_hoehe);
	vec3 lackton = lack_farbe * clamp(lum / 0.62, 0.0, 1.3);
	// Chrom: sehr hell und ungesaettigt in der Zeichnung selbst (vor dem Toenen), heller als
	// jeder Lack im Blatt.
	float chrom = smoothstep(0.84, 0.92, lum) * (1.0 - smoothstep(0.03, 0.08, saett));
	// a: positiv Scheibe, negativ Chrom (beides schliesst sich aus: dunkel gegen hell).
	return vec4(mix(tc, lackton, ist_lack * (1.0 - chrom)), glasig - chrom);
}

// Leuchten aus der Zeichnung: Rueck- und Bremslicht (rot, nach hinten), Blinker (orange),
// Scheinwerfer (fast weiss, nach vorn).
vec3 lampen() {
	float u = 0.5 - lok.x / laenge;
	vec3 tf = texture(tex_front, vec2(0.5 - lok.z / front_breite, 1.0 - lok.y / seite_hoehe)).rgb;
	vec3 th = texture(tex_heck, vec2(0.5 + lok.z / heck_breite, 1.0 - lok.y / seite_hoehe)).rgb;
	vec3 ts = texture(tex_seite, vec2(u, 1.0 - lok.y / seite_hoehe)).rgb;
	float wf = smoothstep(0.3, 0.8, lnorm.x);
	float wh = smoothstep(0.3, 0.8, -lnorm.x);
	float ws = smoothstep(0.5, 0.9, abs(lnorm.z)) * (1.0 - wf) * (1.0 - wh);
	vec3 hinten = th * wh + ts * ws * step(0.85, u);
	vec3 vorn = tf * wf + ts * ws * step(u, 0.15);
	float rot = smoothstep(0.25, 0.45, hinten.r - max(hinten.g, hinten.b)) * step(0.35, hinten.r);
	vec3 alle = hinten + vorn;
	float orange = smoothstep(0.15, 0.3, alle.r - alle.b) * step(0.5, alle.r)
			* step(0.35 * alle.r, alle.g) * step(alle.g, 0.8 * alle.r);
	float hell = smoothstep(0.82, 0.95, dot(vorn, vec3(0.333)))
			* (1.0 - smoothstep(0.04, 0.12, max(max(vorn.r, vorn.g), vorn.b) - min(min(vorn.r, vorn.g), vorn.b)));
	float takt = step(0.5, fract(TIME * 1.5));
	float seite_an = blinker > 1.5 ? 1.0 : step(0.5, blinker * sign(lok.z));
	vec3 e = vec3(1.0, 0.04, 0.02) * rot * (0.35 + 3.0 * bremse);
	e += vec3(1.0, 0.45, 0.05) * orange * seite_an * takt * 3.0 * step(0.5, abs(blinker));
	e += vec3(1.0, 0.97, 0.9) * hell * (0.15 + 1.6 * licht);
	return e;
}

// Gespiegelte Umgebung fuer Klarlack, Chrom und Glas (der Compatibility-Renderer hat keine
// Bildschirmspiegelung): Himmel wie in welt.gd, am Horizont eine dunkle Kante aus Haeusern
// und Baeumen, darunter Asphalt. r ist die Spiegelrichtung in Weltkoordinaten.
uniform float spiegelung : hint_range(0.0, 2.0) = 1.0;
// Rad und Bremse (Wagenmesh.rad_netz): das Teil steht in UV2.x.
uniform bool rad_teile = false;
uniform vec3 felge_farbe = vec3(0.62);
uniform vec3 sattel_farbe = vec3(0.25);

vec3 umwelt(vec3 r) {
	float h = r.y;
	vec3 oben = vec3(0.30, 0.50, 0.82);
	vec3 horizont = vec3(0.98, 0.84, 0.70);
	vec3 himmel = mix(horizont, oben, pow(clamp(h, 0.0, 1.0), 0.45));
	// Sonne tief im Westen (welt.gd: -15 Grad): warmer Fleck im Spiegelbild.
	vec3 sonne = normalize(vec3(-0.75, 0.26, 0.6));
	himmel += vec3(1.0, 0.75, 0.45) * pow(max(dot(r, sonne), 0.0), 220.0) * 6.0;
	himmel += vec3(1.0, 0.7, 0.4) * pow(max(dot(r, sonne), 0.0), 12.0) * 0.25;
	float az = atan(r.z, r.x);
	float kante = 0.05 + 0.09 * abs(sin(az * 3.0) * sin(az * 7.3 + 1.0))
			+ 0.05 * step(0.6, fract(sin(floor(az * 14.0) * 43.7) * 917.3));
	vec3 haeuser = mix(vec3(0.20, 0.17, 0.15), vec3(0.42, 0.37, 0.30),
			0.5 + 0.5 * sin(az * 37.0));
	vec3 boden = mix(vec3(0.10, 0.10, 0.11), vec3(0.35, 0.31, 0.27), exp(h * 14.0));
	// Weiche Uebergaenge: harte Kanten werden auf leicht gewellten Flaechen zu Zickzack.
	vec3 oben_teil = mix(haeuser, himmel, smoothstep(kante - 0.03, kante + 0.05, h));
	return mix(boden, oben_teil, smoothstep(-0.04, 0.03, h));
}

void fragment() {
	vec3 grund = (lackiert && instanziert) ? instanzfarbe : farbe;
	float nv = clamp(dot(NORMAL, VIEW), 0.0, 1.0);
	float rand = pow(1.0 - nv, 3.0);
	vec3 r_welt = normalize((INV_VIEW_MATRIX * vec4(reflect(-VIEW, NORMAL), 0.0)).xyz);
	vec3 bild = umwelt(r_welt);
	// Klarlack: etwas kraeftiger als physikalisch, damit die Spiegelung auch von schraeg oben
	// (Spielkamera) sichtbar ist.
	float fresnel_lack = 0.08 + 0.92 * pow(1.0 - nv, 4.0);
	if (rad_teile) {
		int teil = int(UV2.x + 0.5);
		vec3 c = vec3(0.03);
		float met = 0.3;
		float rau = 0.6;
		if (teil == 0) { c = vec3(0.028, 0.028, 0.03); met = 0.0; rau = 0.85; }
		else if (teil == 1) { c = felge_farbe; met = 0.9; rau = 0.2; }
		else if (teil == 2) { c = vec3(0.34, 0.33, 0.32); met = 0.8; rau = 0.4; }
		else if (teil == 3) { c = sattel_farbe; met = 0.1; rau = 0.3; }
		float f = mix(0.04 + 0.5 * pow(1.0 - nv, 5.0), 0.8, met);
		ALBEDO = c * (1.0 - met * 0.6);
		METALLIC = met * 0.6;
		ROUGHNESS = rau;
		SPECULAR = 0.6;
		EMISSION = bild * f * spiegelung * mix(vec3(1.0), c, 0.5) * (1.0 - rau * 0.8);
	} else if (glas) {
		float f = 0.06 + 0.94 * pow(1.0 - nv, 4.0);
		ALBEDO = vec3(0.012, 0.018, 0.024) * (1.0 - f);
		ROUGHNESS = 0.04;
		SPECULAR = 0.9;
		EMISSION = bild * mix(0.18, 0.9, f) * spiegelung;
	} else {
		vec3 stoerung = vec3(0.0);
		vec4 z = vec4(grund, 0.0);
		if (lackiert && detail_an) {
			z = zeichnung(grund, stoerung);
			NORMAL = normalize(NORMAL + (achse_x * stoerung.x + achse_y * stoerung.y
					+ achse_z * stoerung.z) * 1.2);
			nv = clamp(dot(NORMAL, VIEW), 0.0, 1.0);
			r_welt = normalize((INV_VIEW_MATRIX * vec4(reflect(-VIEW, NORMAL), 0.0)).xyz);
			bild = umwelt(r_welt);
			fresnel_lack = 0.08 + 0.92 * pow(1.0 - nv, 4.0);
		}
		float glasig = max(z.a, 0.0);
		vec3 farbe_z = z.rgb;
		// Chrom: Leisten, Grillrahmen, Endrohre aus der Zeichnung, sonst metallische Teile.
		float chrom = (lackiert && detail_an) ? max(-z.a, 0.0) : metall;
		// Scheiben aus der Zeichnung: dunkel, glatt, stark spiegelnd zum Rand hin.
		float f_glas = 0.06 + 0.94 * pow(1.0 - nv, 4.0);
		float f = mix(lackiert ? fresnel_lack : 0.04, f_glas, glasig);
		f = mix(f, 0.85, chrom);
		ALBEDO = mix(farbe_z, farbe_z * 0.35, glasig);
		ALBEDO = mix(ALBEDO, vec3(0.04), chrom) * (1.0 - f * 0.6);
		ROUGHNESS = mix(mix(rauheit * 0.5, 0.04, glasig), 0.08, chrom);
		SPECULAR = lackiert ? 0.9 : 0.4;
		METALLIC = chrom * 0.6;
		EMISSION = leuchten + bild * f * spiegelung * (lackiert ? 0.65 : 0.4);
		if (lackiert && detail_an) {
			EMISSION += lampen();
		}
	}
}
