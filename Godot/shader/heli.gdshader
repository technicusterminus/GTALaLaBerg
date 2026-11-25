// Lackierung des Hubschraubers in Modellkoordinaten (das Modell hat keine UVs): Rettungs-Gelb,
// weisser Zierstreifen, dunkle Linie, getoente Cockpit- und Seitenscheiben, graue Kufen.
// rumpf_lage/rumpf_groesse: AABB des Rumpfnetzes (Laenge entlang x, vorn = +x, oben = +z).
shader_type spatial;

uniform vec3 rumpf_lage = vec3(-6.3, -1.1, 0.0);
uniform vec3 rumpf_groesse = vec3(9.1, 2.25, 3.0);
uniform vec3 lack = vec3(0.90, 0.60, 0.02);
uniform bool ausgebrannt = false;

varying vec3 lokal;

void vertex() {
	lokal = (VERTEX - rumpf_lage) / rumpf_groesse;   // 0..1 je Achse
}

void fragment() {
	float x = lokal.x;   // 0 = Heck, 1 = Nase
	float z = lokal.z;   // 0 = Kufen, 1 = Rotorkopf
	vec3 c = lack;
	float rau = 0.32;
	float met = 0.0;
	// Weisser Bauch und Zierstreifen, dazwischen eine dunkle Linie.
	if (z < 0.36 && x > 0.40) {
		c = vec3(0.80, 0.80, 0.78);
	}
	if (abs(z - 0.38) < 0.018 && x > 0.30) {
		c = vec3(0.08, 0.09, 0.10);
	}
	if (abs(z - 0.50) < 0.035 && x < 0.45) {
		c = vec3(0.80, 0.80, 0.78);   // Streifen am Heckausleger
	}
	// Scheiben: Cockpit vorn oben, Seitenfenster der Kabine.
	bool cockpit = x > 0.80 && z > 0.40 && z < 0.82;
	bool seite = x > 0.58 && x < 0.79 && z > 0.50 && z < 0.74 && abs(lokal.y - 0.5) > 0.30;
	if (cockpit || seite) {
		float rand = pow(1.0 - clamp(dot(NORMAL, VIEW), 0.0, 1.0), 2.0);
		c = mix(vec3(0.02, 0.04, 0.06), vec3(0.35, 0.48, 0.60), rand);
		rau = 0.05;
	}
	// Kufen und Gestaenge unten: Metallgrau.
	if (z < 0.10) {
		c = vec3(0.22, 0.23, 0.24);
		met = 0.6;
		rau = 0.4;
	}
	if (ausgebrannt) {
		c = vec3(0.05, 0.045, 0.04);
		rau = 0.9;
	}
	ALBEDO = c;
	ROUGHNESS = rau;
	METALLIC = met;
	SPECULAR = 0.6;
}
