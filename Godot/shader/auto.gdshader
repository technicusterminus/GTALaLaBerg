// Autolack, Raeder, Innenraum mit glatten Vertex-Normalen. Der Lack kommt aus vier
// Ansichten (Fototexturen von OpenRouter) mit Masken fuer Lack, Glas und Lampen, Relief aus
// einer Hoehenkarte und Spiegelungen einer Strassenumgebung. Scheiben: auto_glas.gdshader.
shader_type spatial;

#include "res://shader/auto_gemein.gdshaderinc"

void fragment() {
	vec3 grund = (lackiert && instanziert) ? instanzfarbe : farbe;
	float nv = clamp(dot(NORMAL, VIEW), 0.0, 1.0);
	vec3 r_welt = normalize((INV_VIEW_MATRIX * vec4(reflect(-VIEW, NORMAL), 0.0)).xyz);
	vec3 bild = umwelt(r_welt);
	if (rad_teile) {
		// Rad, Bremse und Innenraum: das Teil steht in UV2.x.
		int teil = int(UV2.x + 0.5);
		vec3 c = vec3(0.03);
		float met = 0.3;
		float rau = 0.6;
		if (teil == 0) { c = vec3(0.028, 0.028, 0.03); met = 0.0; rau = 0.85; }
		else if (teil == 1) { c = felge_farbe * 1.25; met = 0.55; rau = 0.22; }
		else if (teil == 2) { c = vec3(0.34, 0.33, 0.32); met = 0.8; rau = 0.4; }
		else if (teil == 3) { c = sattel_farbe; met = 0.1; rau = 0.3; }
		else if (teil == 5) { c = innen_farbe; met = 0.0; rau = 0.8; }          // Sitze
		else if (teil == 6) { c = vec3(0.30, 0.29, 0.28); met = 0.0; rau = 0.95; } // Himmel
		else if (teil == 7) { c = vec3(0.035, 0.035, 0.04); met = 0.1; rau = 0.6; } // Armaturen
		else if (teil == 8) { c = innen_farbe * 0.8 + 0.03; met = 0.0; rau = 0.85; } // Verkleidung
		float f = mix(0.04 + 0.5 * pow(1.0 - nv, 5.0), 0.8, met);
		ALBEDO = c * (1.0 - met * 0.6);
		METALLIC = met * 0.6;
		ROUGHNESS = rau;
		SPECULAR = 0.6;
		EMISSION = bild * f * spiegelung * mix(vec3(1.0), c, 0.5) * (1.0 - rau * 0.8);
	} else if (glas) {
		float f = 0.06 + 0.94 * pow(1.0 - nv, 4.0);
		ALBEDO = vec3(0.012, 0.018, 0.024) * (1.0 - f);
		ROUGHNESS = 0.04;
		SPECULAR = 0.9;
		EMISSION = bild * mix(0.18, 0.9, f) * spiegelung;
	} else {
		vec3 farbe_z = grund;
		float glas_m = 0.0;
		float chrom = metall;
		vec3 leucht = vec3(0.0);
		if (lackiert && detail_an) {
			vec3 stoerung;
			farbe_z = zeichnung(grund, glas_m, chrom, leucht, stoerung);
			NORMAL = normalize(NORMAL + (achse_x * stoerung.x + achse_y * stoerung.y
					+ achse_z * stoerung.z) * 1.2);
			nv = clamp(dot(NORMAL, VIEW), 0.0, 1.0);
			r_welt = normalize((INV_VIEW_MATRIX * vec4(reflect(-VIEW, NORMAL), 0.0)).xyz);
			bild = umwelt(r_welt);
		}
		// Klarlack: etwas kraeftiger als physikalisch, damit die Spiegelung auch von schraeg
		// oben (Spielkamera) sichtbar ist. Gezeichnetes Glas ausserhalb der Scheiben: dunkel.
		float fresnel_lack = 0.08 + 0.92 * pow(1.0 - nv, 4.0);
		float f_glas = 0.06 + 0.94 * pow(1.0 - nv, 4.0);
		float f = mix(lackiert ? fresnel_lack : 0.04, f_glas, glas_m);
		f = mix(f, 0.85, chrom);
		ALBEDO = mix(farbe_z, farbe_z * 0.35, glas_m);
		ALBEDO = mix(ALBEDO, vec3(0.04), chrom) * (1.0 - f * 0.6);
		ROUGHNESS = mix(mix(rauheit * 0.5, 0.04, glas_m), 0.08, chrom);
		SPECULAR = lackiert ? 0.9 : 0.4;
		METALLIC = chrom * 0.6;
		EMISSION = leuchten + bild * f * spiegelung * (lackiert ? 0.65 : 0.4) + leucht;
	}
}
