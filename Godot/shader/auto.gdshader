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
// Glas, Reifen) bleibt, wie es gezeichnet ist.
vec3 zeichnung(vec3 lack_farbe) {
	float u = 0.5 - lok.x / laenge;
	vec3 ts = texture(tex_seite, vec2(u, 1.0 - lok.y / seite_hoehe)).rgb;
	vec3 to = texture(tex_oben, vec2(u, lok.z / oben_breite + 0.5)).rgb;
	vec3 tf = texture(tex_front, vec2(lok.z / front_breite + 0.5, 1.0 - lok.y / seite_hoehe)).rgb;
	vec3 th = texture(tex_heck, vec2(-lok.z / heck_breite + 0.5, 1.0 - lok.y / seite_hoehe)).rgb;
	float ws = pow(abs(lnorm.z), 2.0);
	float wo = pow(max(lnorm.y, 0.0), 2.0);
	float wf = pow(max(lnorm.x, 0.0), 2.0);
	float wh = pow(max(-lnorm.x, 0.0), 2.0);
	vec3 tc = (ts * ws + to * wo + tf * wf + th * wh) / max(ws + wo + wf + wh, 0.001);
	float lum = dot(tc, vec3(0.333));
	float saett = max(max(tc.r, tc.g), tc.b) - min(min(tc.r, tc.g), tc.b);
	float ist_lack = (1.0 - smoothstep(0.03, 0.10, saett)) * smoothstep(0.10, 0.22, lum);
	vec3 lackton = lack_farbe * clamp(lum / 0.38, 0.0, 1.4);
	return mix(tc, lackton, ist_lack);
}

void fragment() {
	vec3 grund = (lackiert && instanziert) ? instanzfarbe : farbe;
	float rand = pow(1.0 - clamp(dot(NORMAL, VIEW), 0.0, 1.0), 3.0);
	if (glas) {
		ALBEDO = mix(vec3(0.015, 0.025, 0.035), vec3(0.30, 0.42, 0.55), rand);
		ROUGHNESS = 0.05;
		SPECULAR = 0.9;
	} else {
		ALBEDO = (lackiert && detail_an) ? zeichnung(grund) : grund;
		ROUGHNESS = rauheit;
		SPECULAR = lackiert ? 0.8 : 0.4;
		METALLIC = metall;
		// Heller Rand und leichter Himmelsschimmer auf waagerechten Flaechen: wirkt wie Klarlack.
		EMISSION = leuchten + grund * rand * 0.08;
	}
}
