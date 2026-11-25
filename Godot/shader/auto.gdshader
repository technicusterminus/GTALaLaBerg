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

varying vec3 instanzfarbe;
varying vec3 lok;
varying vec3 lnorm;

void vertex() {
	instanzfarbe = COLOR.rgb;
	lok = VERTEX;
	lnorm = NORMAL;
}

// Die Texturen der vier Ansichten, nach der Richtung der Flaeche gemischt. Neutrales Grau
// gilt als Lack (und wird mit der Wagenfarbe getoent), alles andere (Lichter, Gitter, Chrom,
// Glas, Reifen) bleibt, wie es gezeichnet ist. In a: wie sehr die Stelle Scheibe ist
// (dunkles, ungesaettigtes Grau oben am Wagen).
vec4 zeichnung(vec3 lack_farbe) {
	float u = 0.5 - lok.x / laenge;
	vec3 ts = texture(tex_seite, vec2(u, 1.0 - lok.y / seite_hoehe)).rgb;
	vec3 to = texture(tex_oben, vec2(u, lok.z / oben_breite + 0.5)).rgb;
	// Von vorn gesehen liegt die rechte Wagenseite (+Z) links im Bild, von hinten rechts.
	vec3 tf = texture(tex_front, vec2(0.5 - lok.z / front_breite, 1.0 - lok.y / seite_hoehe)).rgb;
	vec3 th = texture(tex_heck, vec2(0.5 + lok.z / heck_breite, 1.0 - lok.y / seite_hoehe)).rgb;
	float ws = pow(abs(lnorm.z), 2.0);
	float wo = pow(max(lnorm.y, 0.0), 2.0);
	float wf = pow(max(lnorm.x, 0.0), 2.0);
	float wh = pow(max(-lnorm.x, 0.0), 2.0);
	vec3 tc = (ts * ws + to * wo + tf * wf + th * wh) / max(ws + wo + wf + wh, 0.001);
	float lum = dot(tc, vec3(0.333));
	float saett = max(max(tc.r, tc.g), tc.b) - min(min(tc.r, tc.g), tc.b);
	// Lack ist im Blatt helles Grau (um 0.72); dunkles Grau ist Glas, Gummi, Kunststoff.
	float ist_lack = (1.0 - smoothstep(0.03, 0.10, saett)) * smoothstep(0.32, 0.44, lum);
	float glasig = (1.0 - smoothstep(0.04, 0.12, saett)) * (1.0 - smoothstep(0.24, 0.34, lum))
			* step(0.10, lum) * smoothstep(0.3, 0.7, lok.y / seite_hoehe);
	vec3 lackton = lack_farbe * clamp(lum / 0.62, 0.0, 1.3);
	return vec4(mix(tc, lackton, ist_lack), glasig);
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

void fragment() {
	vec3 grund = (lackiert && instanziert) ? instanzfarbe : farbe;
	float rand = pow(1.0 - clamp(dot(NORMAL, VIEW), 0.0, 1.0), 3.0);
	if (glas) {
		ALBEDO = mix(vec3(0.015, 0.025, 0.035), vec3(0.30, 0.42, 0.55), rand);
		ROUGHNESS = 0.05;
		SPECULAR = 0.9;
	} else {
		vec4 z = (lackiert && detail_an) ? zeichnung(grund) : vec4(grund, 0.0);
		float glasig = z.a;
		ALBEDO = z.rgb;
		// Scheiben aus der Zeichnung: glatt, mit Himmel im Rand.
		ALBEDO = mix(ALBEDO, mix(ALBEDO * 0.6, vec3(0.30, 0.42, 0.55), rand), glasig);
		ROUGHNESS = mix(rauheit, 0.05, glasig);
		SPECULAR = lackiert ? 0.8 : 0.4;
		METALLIC = metall;
		// Heller Rand und leichter Himmelsschimmer auf waagerechten Flaechen: wirkt wie Klarlack.
		EMISSION = leuchten + grund * rand * 0.08;
		if (lackiert && detail_an) {
			EMISSION += lampen();
		}
	}
}
