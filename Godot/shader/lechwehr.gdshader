shader_type spatial;
render_mode cull_disabled;

float zufall(vec2 zelle) {
	vec3 p = fract(zelle.xyx * vec3(0.1031, 0.1030, 0.0973));
	p += dot(p, p.yzx + 33.33);
	return fract((p.x + p.y) * p.z);
}

float rauschen(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(zufall(i), zufall(i + vec2(1.0, 0.0)), f.x),
			mix(zufall(i + vec2(0.0, 1.0)), zufall(i + vec2(1.0)), f.x), f.y);
}

void fragment() {
	float s = UV.x + UV2.x * 5.61;
	vec2 fluss = vec2(s, UV.y * 2.03 - TIME * 0.85);
	float schwall = rauschen(fluss * vec2(0.58, 0.92));
	float faden = rauschen(fluss * vec2(3.7, 1.8) + schwall * 2.4);
	float spritzer = rauschen(fluss * vec2(12.0, 6.0) + faden);
	spritzer = mix(spritzer, 0.5, smoothstep(0.04, 0.18, fwidth(s)));
	float kante = smoothstep(0.12, 0.94, UV.y + (schwall - 0.5) * 0.48);
	float schaum = smoothstep(0.20, 0.94, 0.38 * kante + 0.28 * schwall + 0.26 * faden + 0.18 * spritzer);
	ALBEDO = mix(vec3(0.13, 0.26, 0.26), vec3(0.83, 0.87, 0.84), schaum);
	ROUGHNESS = mix(0.25, 0.67, schaum);
	SPECULAR = 0.41;
}
