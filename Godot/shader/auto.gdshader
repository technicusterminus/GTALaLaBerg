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

varying vec3 instanzfarbe;

void vertex() {
	instanzfarbe = COLOR.rgb;
}

void fragment() {
	vec3 grund = (lackiert && instanziert) ? instanzfarbe : farbe;
	float rand = pow(1.0 - clamp(dot(NORMAL, VIEW), 0.0, 1.0), 3.0);
	if (glas) {
		ALBEDO = mix(vec3(0.015, 0.025, 0.035), vec3(0.30, 0.42, 0.55), rand);
		ROUGHNESS = 0.05;
		SPECULAR = 0.9;
	} else {
		ALBEDO = grund;
		ROUGHNESS = rauheit;
		SPECULAR = lackiert ? 0.8 : 0.4;
		METALLIC = metall;
		// Heller Rand und leichter Himmelsschimmer auf waagerechten Flaechen: wirkt wie Klarlack.
		EMISSION = leuchten + grund * rand * 0.25;
	}
}
