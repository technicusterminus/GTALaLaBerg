// Baumkrone: die Laubtextur (vom Bildmodell erzeugt) ueber Weltkoordinaten, getoent mit der
// Instanzfarbe (leichte Abweichung je Baum), oben heller, unten dunkler, rund schattiert.
shader_type spatial;

uniform sampler2D laub : source_color, filter_linear_mipmap, repeat_enable;
uniform vec3 grundton = vec3(1.0);

varying vec3 welt;
varying vec4 ton;

void vertex() {
	welt = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	ton = COLOR;
}

void fragment() {
	vec2 uv = welt.xz * 0.55 + welt.y * vec2(0.9, 0.6);
	vec3 blatt = texture(laub, uv).rgb;
	float oben = clamp(NORMAL.y * 0.5 + 0.55, 0.0, 1.0);
	ALBEDO = blatt * grundton * ton.rgb * (0.55 + 0.7 * oben);
	ROUGHNESS = 0.95;
	SPECULAR = 0.1;
}
