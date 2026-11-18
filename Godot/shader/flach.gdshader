// Flachschattierung ohne Normalen in den Daten: die Flaechennormale kommt aus
// den Bildschirmableitungen. Das spart die Normalen im Speicher und zeigt
// immer zur Kamera, egal wie die Dreiecke gewickelt sind.
shader_type spatial;
render_mode cull_disabled;

// Linear gelesen wie in der Unreal-Version, daher ohne source_color.
uniform vec3 farbe = vec3(0.7);
uniform float rauheit : hint_range(0.0, 1.0) = 0.9;
uniform float glanz : hint_range(0.0, 1.0) = 0.0;
// Waagerechte Flaechen (Boden, Strassen): feste Normale nach oben. Die Daten
// haben keine Vertex-Normalen, ohne sie greift der Normal-Bias der Schatten
// nicht und der Boden zeigt Ringe (Shadow-Acne).
uniform bool oben = false;

void vertex() {
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
	ALBEDO = farbe;
	ROUGHNESS = rauheit;
	SPECULAR = 0.2 + 0.6 * glanz;
	METALLIC = 0.0;
}
