// Wie flach.gdshader, aber der Lack kommt aus der Instanzfarbe eines MultiMesh:
// alle geparkten Autos eines Sektors teilen sich so einen Zeichenaufruf je Teil.
shader_type spatial;
render_mode cull_disabled;

uniform vec3 farbe = vec3(0.7);
uniform bool lackiert = false;
uniform float rauheit : hint_range(0.0, 1.0) = 0.9;
uniform float glanz : hint_range(0.0, 1.0) = 0.0;

varying vec3 lack;

void vertex() {
	lack = COLOR.rgb;
}

void fragment() {
	vec3 n = normalize(cross(dFdx(VERTEX), dFdy(VERTEX)));
	if (dot(n, VIEW) < 0.0) {
		n = -n;
	}
	NORMAL = n;
	ALBEDO = lackiert ? lack : farbe;
	ROUGHNESS = rauheit;
	SPECULAR = 0.2 + 0.6 * glanz;
	METALLIC = 0.0;
}
