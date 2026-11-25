// Gemeinsamer Teil von auto.gdshader (Lack, Reifen, Innenraum) und auto_glas.gdshader
// (Scheiben mit Durchsicht): Uniforms, Ansichten mit Masken und Relief, Lampen, Umgebung.


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
// Masken je Ansicht (Tools/baue_auto_modelle.py, vom Bildmodell segmentiert): R Lack,
// G Glas, B Lampen, A Hoehe (1 oben; Fugen, Grill, Einlaesse tief).
uniform sampler2D maske_seite : filter_linear_mipmap, repeat_disable;
uniform sampler2D maske_oben : filter_linear_mipmap, repeat_disable;
uniform sampler2D maske_front : filter_linear_mipmap, repeat_disable;
uniform sampler2D maske_heck : filter_linear_mipmap, repeat_disable;

// Je Wagen (set_instance_shader_parameter): Bremslicht 0..1, Blinker -1 links, +1 rechts,
// 2 Warnblinker, 0 aus; Abblendlicht 0..1. Die Lampen selbst stehen in der Zeichnung: rote
// Flaechen hinten, orange an den Ecken, helle Scheinwerfer vorn.
instance uniform float bremse = 0.0;
instance uniform float blinker = 0.0;
instance uniform float licht = 0.0;

// Relief: Hoehe im Alpha der Masken (Tools/baue_auto_modelle.py). Daraus Parallaxe (Fugen, Grill, Scheiben liegen sichtbar tiefer) und
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

// Eine Ansicht mit Parallaxe: uv die Abbildung, uv_je_m ihre Ableitung nach den beiden
// Ebenenachsen, blick_ebene/blick_senk der Blick in Ebene und senkrecht dazu. Liefert die
// Farbe, in m die Masken (Lack, Glas, Lampe) und in stoer die Neigung der Hoehe (m je m).
vec3 ansicht(sampler2D tex, sampler2D mtex, vec2 uv, vec2 uv_je_m, vec2 blick_ebene,
		float blick_senk, out vec3 m, out vec2 stoer) {
	float h = texture(mtex, uv).a;
	vec2 schub = -blick_ebene / max(abs(blick_senk), 0.25) * (1.0 - h) * relief;
	vec2 uv2 = uv + schub * uv_je_m;
	vec2 e = 1.5 / vec2(textureSize(mtex, 0));
	float ha = texture(mtex, uv2 + vec2(e.x, 0.0)).a - texture(mtex, uv2 - vec2(e.x, 0.0)).a;
	float hb = texture(mtex, uv2 + vec2(0.0, e.y)).a - texture(mtex, uv2 - vec2(0.0, e.y)).a;
	stoer = vec2(ha / (2.0 * e.x) * uv_je_m.x, hb / (2.0 * e.y) * uv_je_m.y) * relief;
	m = texture(mtex, uv2).rgb;
	return texture(tex, uv2).rgb;
}

// Die vier Ansichten, nach der Richtung der Flaeche gemischt. Lack (Maske R) wird mit der
// Wagenfarbe getoent, alles andere bleibt, wie es gezeichnet ist. Aus: glas (Maske G),
// chrom (sehr helle, ungesaettigte Teile ausserhalb von Lack, Glas, Lampen), leucht (die
// Emission der Lampen: Rueck-/Bremslicht, Blinker, Scheinwerfer) und die Normalenstoerung.
vec3 zeichnung(vec3 lack_farbe, out float glas_m, out float chrom, out vec3 leucht,
		out vec3 stoerung) {
	float u = 0.5 - lok.x / laenge;
	vec3 v = normalize(lok_blick);
	vec2 sg; vec2 og; vec2 fg; vec2 hg;
	vec3 ms; vec3 mo; vec3 mf; vec3 mh;
	vec3 ts = ansicht(tex_seite, maske_seite, vec2(u, 1.0 - lok.y / seite_hoehe),
			vec2(-1.0 / laenge, -1.0 / seite_hoehe), v.xy, v.z, ms, sg);
	vec3 to = ansicht(tex_oben, maske_oben, vec2(u, lok.z / oben_breite + 0.5),
			vec2(-1.0 / laenge, 1.0 / oben_breite), v.xz, v.y, mo, og);
	// Von vorn gesehen liegt die rechte Wagenseite (+Z) links im Bild, von hinten rechts.
	vec3 tf = ansicht(tex_front, maske_front, vec2(0.5 - lok.z / front_breite, 1.0 - lok.y / seite_hoehe),
			vec2(-1.0 / front_breite, -1.0 / seite_hoehe), v.zy, v.x, mf, fg);
	vec3 th = ansicht(tex_heck, maske_heck, vec2(0.5 + lok.z / heck_breite, 1.0 - lok.y / seite_hoehe),
			vec2(1.0 / heck_breite, -1.0 / seite_hoehe), v.zy, v.x, mh, hg);
	float ws = pow(abs(lnorm.z), 2.0);
	float wo = pow(max(lnorm.y, 0.0), 2.0);
	float wf = pow(max(lnorm.x, 0.0), 2.0);
	float wh = pow(max(-lnorm.x, 0.0), 2.0);
	float wsum = max(ws + wo + wf + wh, 0.001);
	vec3 tc = (ts * ws + to * wo + tf * wf + th * wh) / wsum;
	vec3 mk = (ms * ws + mo * wo + mf * wf + mh * wh) / wsum;
	stoerung = (vec3(-sg.x, -sg.y, 0.0) * ws + vec3(-og.x, 0.0, -og.y) * wo
			+ vec3(0.0, -fg.y, -fg.x) * wf + vec3(0.0, -hg.y, -hg.x) * wh) / wsum;
	float lum = dot(tc, vec3(0.333));
	float saett = max(max(tc.r, tc.g), tc.b) - min(min(tc.r, tc.g), tc.b);
	float lack = smoothstep(0.3, 0.7, mk.r);
	glas_m = smoothstep(0.3, 0.7, mk.g);
	float lampe = smoothstep(0.3, 0.7, mk.b);
	chrom = (1.0 - lack) * (1.0 - glas_m) * (1.0 - lampe)
			* smoothstep(0.78, 0.9, lum) * (1.0 - smoothstep(0.03, 0.08, saett));
	// Lampen: rot (Rueck- und Bremslicht), orange (Blinker), sonst weiss (Scheinwerfer und
	// Tagfahrlicht, nur vorn).
	float rot = lampe * smoothstep(0.08, 0.25, tc.r - max(tc.g, tc.b));
	float orange = lampe * (1.0 - rot) * smoothstep(0.15, 0.3, tc.r - tc.b)
			* step(0.35 * tc.r, tc.g) * step(tc.g, 0.85 * tc.r);
	float vorn = (wf + ws * step(u, 0.22)) / wsum;
	float weiss = lampe * (1.0 - rot) * (1.0 - orange) * clamp(vorn * 1.5, 0.0, 1.0);
	float takt = step(0.5, fract(TIME * 1.5));
	float seite_an = blinker > 1.5 ? 1.0 : step(0.5, blinker * sign(lok.z));
	leucht = vec3(1.0, 0.04, 0.02) * rot * (0.35 + 3.0 * bremse)
			+ vec3(1.0, 0.45, 0.05) * orange * seite_an * takt * 3.0 * step(0.5, abs(blinker))
			+ vec3(1.0, 0.98, 0.94) * weiss * (0.35 + 1.6 * licht);
	vec3 lackton = lack_farbe * clamp(lum / 0.62, 0.0, 1.3);
	// Scheinwerfer- und Rueckleuchtenglas: klar, Innenleben heller (nicht getoent).
	return mix(tc, lackton, lack);
}

// Gespiegelte Umgebung fuer Klarlack, Chrom und Glas (der Compatibility-Renderer hat keine
// Bildschirmspiegelung): Himmel wie in welt.gd, am Horizont eine dunkle Kante aus Haeusern
// und Baeumen, darunter Asphalt. r ist die Spiegelrichtung in Weltkoordinaten.
uniform float spiegelung : hint_range(0.0, 2.0) = 1.0;
// Rad und Bremse (Wagenmesh.rad_netz): das Teil steht in UV2.x.
uniform bool rad_teile = false;
uniform vec3 felge_farbe = vec3(0.62);
uniform vec3 sattel_farbe = vec3(0.25);
uniform vec3 innen_farbe = vec3(0.06, 0.06, 0.065);   // Sitze, Verkleidung

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

