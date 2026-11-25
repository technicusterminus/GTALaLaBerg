shader_type spatial;
render_mode cull_disabled, diffuse_lambert_wrap;

uniform sampler2D zweig : source_color, filter_linear_mipmap_anisotropic, repeat_disable;
varying vec3 farbton;

void vertex() {
	farbton = COLOR.rgb;
	float phase = TIME * 1.35 + MODEL_MATRIX[3].x * 0.31 + MODEL_MATRIX[3].z * 0.29;
	VERTEX.x += sin(phase) * (1.0 - UV.y) * (0.008 + max(VERTEX.y, 0.0) * 0.018);
}

void fragment() {
	vec4 muster = texture(zweig, UV);
	ALPHA = muster.a;
	float texelbreite = max(length(dFdx(UV)), length(dFdy(UV))) * 128.0;
	ALPHA_SCISSOR_THRESHOLD = mix(0.40, 0.19, smoothstep(2.0, 6.0, texelbreite));
	vec3 c = OUTPUT_IS_SRGB ? farbton : pow(farbton, vec3(2.2));
	ALBEDO = c * mix(0.74, 1.24, muster.r);
	ROUGHNESS = 0.93;
	SPECULAR = 0.12;
	BACKLIGHT = ALBEDO * 0.25;
}
