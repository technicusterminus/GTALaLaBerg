shader_type spatial;
render_mode cull_disabled;

uniform bool wind = false;
varying vec3 farbton;
varying vec3 sichtposition;

void vertex() {
	farbton = COLOR.rgb;
	vec3 mitte = MODEL_MATRIX[3].xyz;
	if (wind) {
		VERTEX.z += sin(TIME * 1.66 + mitte.x * 0.38 + mitte.z * 0.24) * VERTEX.y * VERTEX.y * 0.14;
	}
	sichtposition = (MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
}

void fragment() {
	vec3 n = normalize(cross(dFdx(sichtposition), dFdy(sichtposition)));
	if (dot(n, VIEW) < 0.0) {
		n = -n;
	}
	NORMAL = n;
	ALBEDO = OUTPUT_IS_SRGB ? farbton : pow(max(farbton, vec3(0.0)), vec3(2.2));
	ROUGHNESS = 0.95;
	SPECULAR = 0.14;
}
