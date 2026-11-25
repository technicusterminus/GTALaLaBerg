// Scheiben mit Durchsicht: dieselben Ansichten wie der Lack (auto_gemein.gdshaderinc); wo die
// Glasmaske steht, ist die Flaeche getoent durchsichtig mit Spiegelung (zum Rand hin fast
// ganz Spiegel), sonst (Saeulen, Rahmen, Dach) deckend wie der Lack. Dahinter der Innenraum.
shader_type spatial;
render_mode blend_mix, depth_draw_always, cull_back;

#include "res://shader/auto_gemein.gdshaderinc"

void fragment() {
	vec3 grund = (lackiert && instanziert) ? instanzfarbe : farbe;
	float glas_m = 1.0;
	float chrom = 0.0;
	vec3 leucht = vec3(0.0);
	vec3 farbe_z = grund;
	if (detail_an) {
		vec3 stoerung;
		farbe_z = zeichnung(grund, glas_m, chrom, leucht, stoerung);
	}
	float nv = clamp(dot(NORMAL, VIEW), 0.0, 1.0);
	vec3 r_welt = normalize((INV_VIEW_MATRIX * vec4(reflect(-VIEW, NORMAL), 0.0)).xyz);
	vec3 bild = umwelt(r_welt);
	float f_glas = 0.07 + 0.93 * pow(1.0 - nv, 4.0);
	float f_lack = 0.08 + 0.92 * pow(1.0 - nv, 4.0);
	float f = mix(f_lack, f_glas, glas_m);
	vec3 deckend = mix(farbe_z, vec3(0.04), chrom) * (1.0 - f * 0.6);
	ALBEDO = mix(deckend, vec3(0.010, 0.014, 0.018), glas_m);
	ROUGHNESS = mix(rauheit * 0.5, 0.03, glas_m);
	SPECULAR = 0.9;
	METALLIC = chrom * 0.6;
	EMISSION = bild * f * spiegelung * mix(0.65, 1.0, glas_m) + leucht;
	// Getoentes Glas: von vorn gut durchsichtig, zum Rand hin deckend (Fresnel).
	ALPHA = mix(1.0, mix(0.32, 0.95, f_glas), glas_m);
}
