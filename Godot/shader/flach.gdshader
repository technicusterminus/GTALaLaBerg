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
// 0 = flach, 1 = Gelaende (Wiesenflecken, Erde am Hang), 2 = Wasser (Wellen, Glitzern).
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
