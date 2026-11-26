shader_type spatial;
render_mode cull_disabled;

void fragment() {
	vec2 masche = vec2(UV.x + UV.y * 1.55, UV.x - UV.y * 1.55) / 0.051;
	vec2 abstand = abs(fract(masche) - 0.5);
	float pixel = max(fwidth(masche.x), fwidth(masche.y));
	float staerke = mix(0.037, 0.0, smoothstep(0.32, 0.90, pixel));
	if (min(abstand.x, abstand.y) > staerke) {
		discard;
	}
	ALBEDO = vec3(0.22, 0.24, 0.23);
	METALLIC = 0.65;
	ROUGHNESS = 0.49;
}
