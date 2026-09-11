// Die eine Autoform fuer die ganze Stadt: geparkte Wagen (prepare-stadt.cjs)
// und der fahrbare Wagen im Spiel (prepare-wagen.cjs -> wagen.json) entstehen
// aus derselben Beschreibung. Vorher gab es zwei Kopien, und beide sahen aus
// wie zwei gestapelte Platten ueber schwebenden Raedern.
//
// Masse nach einem Kompaktwagen: 4,28 m lang, 1,79 m breit, 1,47 m hoch,
// Radstand 2,62 m, Reifen 205/55 R16 (Radius 0,315 m).
//
// Koordinaten: u in Fahrtrichtung, v nach rechts, h nach oben, Meter, Boden
// bei h = 0. Jede Flaeche wird ueber ausgabe.viereck(klasse, rgb, A, B, C, D)
// bzw. ausgabe.dreieck(klasse, rgb, a, b, c) abgegeben; sichtbar ist sie von
// der Seite, in die kreuz(B - A, C - B) zeigt. Dieselbe Regel gilt fuer
// flaeche() und dreieck() in prepare-stadt.cjs.
'use strict';

const ACHSE_H = -1.31, ACHSE_V = 1.31;     // Hinter- und Vorderachse
const RAD_R = 0.315, RAD_B = 0.21;         // Reifenradius und -breite
const BOGEN_R = 0.385;                     // Radlauf etwas groesser als das Rad
const SPUR = 0.76;                         // Reifenmitte von der Wagenmitte
const SCHWELLER = 0.20;                    // Unterkante zwischen den Achsen

// Querschnitte von hinten nach vorn: halbe Breite, Guertellinie, Dachhoehe.
// Unterkante (yb) wird aus den Radlaeufen berechnet, wo nichts angegeben ist.
const SCHNITTE = [
  { u: -2.14, hw: 0.800, yb: 0.34, gurt: 0.86, dach: 0.91 },   // Heckstossfaenger
  { u: -2.10, hw: 0.865, yb: 0.27, gurt: 0.95, dach: 1.00 },
  { u: -2.00, hw: 0.885, yb: 0.22, gurt: 0.99, dach: 1.04 },
  { u: -1.86, hw: 0.895,           gurt: 1.00, dach: 1.05 },   // Heckscheibe unten
  { u: -1.695, hw: 0.895,          gurt: 1.00, dach: 1.22 },
  { u: -1.60, hw: 0.895,           gurt: 1.00, dach: 1.30 },
  { u: -1.50, hw: 0.895,           gurt: 0.99, dach: 1.38 },
  { u: -1.40, hw: 0.895,           gurt: 0.99, dach: 1.42 },   // C-Saeule
  { u: -1.31, hw: 0.895,           gurt: 0.98, dach: 1.44 },
  { u: -1.20, hw: 0.895,           gurt: 0.98, dach: 1.45 },
  { u: -1.05, hw: 0.895,           gurt: 0.97, dach: 1.46 },
  { u: -0.925, hw: 0.895,          gurt: 0.97, dach: 1.46 },
  { u: -0.40, hw: 0.895,           gurt: 0.96, dach: 1.47 },
  { u: -0.07, hw: 0.895,           gurt: 0.955, dach: 1.47 },  // B-Saeule
  { u:  0.07, hw: 0.895,           gurt: 0.95, dach: 1.47 },
  { u:  0.30, hw: 0.895,           gurt: 0.94, dach: 1.46 },
  { u:  0.55, hw: 0.893,           gurt: 0.93, dach: 1.42 },   // Windschutzscheibe oben
  { u:  0.80, hw: 0.890,           gurt: 0.92, dach: 1.30 },
  { u:  0.925, hw: 0.890,          gurt: 0.92, dach: 1.22 },
  { u:  1.05, hw: 0.888,           gurt: 0.91, dach: 1.10 },
  { u:  1.20, hw: 0.885,           gurt: 0.905, dach: 0.965 }, // Motorhaube
  { u:  1.31, hw: 0.880,           gurt: 0.89, dach: 0.945 },
  { u:  1.45, hw: 0.870,           gurt: 0.87, dach: 0.925 },
  { u:  1.695, hw: 0.850,          gurt: 0.84, dach: 0.89 },
  { u:  1.90, hw: 0.820, yb: 0.24, gurt: 0.78, dach: 0.83 },
  { u:  2.05, hw: 0.780, yb: 0.28, gurt: 0.72, dach: 0.765 },
  { u:  2.14, hw: 0.700, yb: 0.34, gurt: 0.66, dach: 0.695 },  // Bugstossfaenger
];

function unterkante(s) {
  if (s.yb !== undefined) return s.yb;
  let y = SCHWELLER;
  for (const a of [ACHSE_H, ACHSE_V]) {
    const d = Math.abs(s.u - a);
    if (d < BOGEN_R) y = Math.max(y, RAD_R + Math.sqrt(BOGEN_R * BOGEN_R - d * d));
  }
  return y;
}

// Kabine heisst: Dach deutlich ueber der Guertellinie.
const kabine = s => s.dach - s.gurt > 0.12;

// 14 Punkte je Schnitt, gegen den Uhrzeigersinn in der v-h-Ebene: rechte
// Flanke hoch, Fensterlinie, Dach, links herunter, Boden zurueck.
function ring(s) {
  const yb = unterkante(s);
  const tief = Math.min(yb + 0.12, s.gurt - 0.10);
  const rw = kabine(s) ? s.hw - 0.20 : s.hw - 0.22;          // Dach schmaler: eingezogene Fenster
  const gw = s.hw - (kabine(s) ? 0.08 : 0.10);
  const halb = [
    [s.hw - 0.05, yb], [s.hw, tief], [s.hw, s.gurt - 0.07], [s.hw - 0.03, s.gurt],
    [gw, s.gurt + 0.02], [rw, s.dach - (kabine(s) ? 0.06 : 0.015)], [rw - 0.12, s.dach],
  ];
  const links = halb.slice().reverse().map(([v, h]) => [-v, h]);
  return halb.concat(links).map(([v, h]) => [s.u, v, h]);
}

const minus = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
const kreuz = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
const skalar = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];

// Flaechen, deren Sichtseite vorgegeben ist: die Reihenfolge wird notfalls
// umgedreht. Fuer alles ausser dem Rumpf - dort bestimmt der Ring die Richtung.
function viereckNach(aus, k, rgb, A, B, C, D, n) {
  if (skalar(kreuz(minus(B, A), minus(C, B)), n) < 0) aus.viereck(k, rgb, D, C, B, A);
  else aus.viereck(k, rgb, A, B, C, D);
}
function dreieckNach(aus, k, rgb, a, b, c, n) {
  if (skalar(kreuz(minus(b, a), minus(c, b)), n) < 0) aus.dreieck(k, rgb, a, c, b);
  else aus.dreieck(k, rgb, a, b, c);
}

// Ein Kasten, Sichtseiten nach aussen.
function kasten(aus, k, rgb, u0, u1, v0, v1, h0, h1) {
  const P = (u, v, h) => [u, v, h];
  viereckNach(aus, k, rgb, P(u1, v0, h0), P(u1, v1, h0), P(u1, v1, h1), P(u1, v0, h1), [1, 0, 0]);
  viereckNach(aus, k, rgb, P(u0, v0, h0), P(u0, v1, h0), P(u0, v1, h1), P(u0, v0, h1), [-1, 0, 0]);
  viereckNach(aus, k, rgb, P(u0, v1, h0), P(u1, v1, h0), P(u1, v1, h1), P(u0, v1, h1), [0, 1, 0]);
  viereckNach(aus, k, rgb, P(u0, v0, h0), P(u1, v0, h0), P(u1, v0, h1), P(u0, v0, h1), [0, -1, 0]);
  viereckNach(aus, k, rgb, P(u0, v0, h1), P(u1, v0, h1), P(u1, v1, h1), P(u0, v1, h1), [0, 0, 1]);
  viereckNach(aus, k, rgb, P(u0, v0, h0), P(u1, v0, h0), P(u1, v1, h0), P(u0, v1, h0), [0, 0, -1]);
}

// Eine flache Scheibe quer zur Fahrtrichtung (Normale +-v).
function scheibe(aus, k, rgb, uc, v, hc, r, seiten, nv, nurOben) {
  const n = [0, nv, 0];
  const bis = nurOben ? seiten / 2 : seiten;
  const P = i => { const a = i / seiten * Math.PI * 2; return [uc + Math.cos(a) * r, v, hc + Math.sin(a) * r]; };
  for (let i = 0; i < bis; i++) dreieckNach(aus, k, rgb, [uc, v, hc], P(i), P(i + 1), n);
}

// Rad: Reifenwalze, Flanken, Felge und Nabe aussen.
function rad(aus, uc, vc) {
  const REIFEN = 0x1B1B1D, FELGE = 0xAEB2B6, NABE = 0x55595E;
  const aussen = Math.sign(vc), seiten = 14;
  const vi = vc - aussen * RAD_B / 2, va = vc + aussen * RAD_B / 2;
  for (let i = 0; i < seiten; i++) {
    const a0 = i / seiten * Math.PI * 2, a1 = (i + 1) / seiten * Math.PI * 2;
    const c0 = [Math.cos(a0) * RAD_R, Math.sin(a0) * RAD_R], c1 = [Math.cos(a1) * RAD_R, Math.sin(a1) * RAD_R];
    const mitte = (a0 + a1) / 2;
    viereckNach(aus, 'Reifen', REIFEN,
      [uc + c0[0], vi, RAD_R + c0[1]], [uc + c1[0], vi, RAD_R + c1[1]],
      [uc + c1[0], va, RAD_R + c1[1]], [uc + c0[0], va, RAD_R + c0[1]],
      [Math.cos(mitte), 0, Math.sin(mitte)]);
  }
  scheibe(aus, 'Reifen', REIFEN, uc, va, RAD_R, RAD_R, seiten, aussen);
  scheibe(aus, 'Reifen', REIFEN, uc, vi, RAD_R, RAD_R, seiten, -aussen);
  scheibe(aus, 'Auto', FELGE, uc, va + aussen * 0.004, RAD_R, 0.205, 10, aussen);
  scheibe(aus, 'Auto', NABE, uc, va + aussen * 0.008, RAD_R, 0.055, 6, aussen);
}

// Der ganze Wagen. lack: Karosseriefarbe als 0xRRGGBB. Liefert nichts; alle
// Flaechen gehen an die Ausgabe. Teile im Lack tragen die Klasse 'Auto' und
// die Lackfarbe - so kann der fahrbare Wagen sie spaeter umfaerben.
function wagen(aus, lack) {
  // Glas neutral grau: die Stadtausleitung rastert Farben auf 5 Stufen je
  // Kanal, und 0x1E2327 wurde dabei zu Tuerkis (0 / 0,25 / 0,25).
  const GLAS = 0x404040, UNTEN = 0x202224;
  const ringe = SCHNITTE.map(ring);
  const n = ringe[0].length;
  for (let i = 1; i < SCHNITTE.length; i++) {
    const a = SCHNITTE[i - 1], b = SCHNITTE[i];
    const steil = Math.abs(b.dach - a.dach) / (b.u - a.u) > 0.45 && (kabine(a) || kabine(b));
    const seitenfenster = !steil && a.dach - a.gurt > 0.40 && b.dach - b.gurt > 0.40 &&
                          !(a.u < 0 && b.u > 0);                     // B-Saeule bleibt Lack
    for (let k = 0; k < n; k++) {
      const j = (k + 1) % n;
      let klasse = 'Auto', farbe = lack;
      if (k === n - 1) { klasse = 'Reifen'; farbe = UNTEN; }         // Unterboden
      else if ((k === 4 || k === 8) && seitenfenster) { klasse = 'Glas'; farbe = GLAS; }
      else if (k >= 5 && k <= 7 && steil) { klasse = 'Glas'; farbe = GLAS; }   // Front- und Heckscheibe
      aus.viereck(klasse, farbe, ringe[i - 1][k], ringe[i - 1][j], ringe[i][j], ringe[i][k]);
    }
  }
  // Deckel hinten und vorn
  const hinten = ringe[0], vorn = ringe[ringe.length - 1];
  for (let k = 1; k + 1 < n; k++) {
    dreieckNach(aus, 'Auto', lack, hinten[0], hinten[k], hinten[k + 1], [-1, 0, 0]);
    dreieckNach(aus, 'Auto', lack, vorn[0], vorn[k], vorn[k + 1], [1, 0, 0]);
  }

  // Radkaesten: dunkle Innenwand hinter jedem Rad, sonst sieht man durch den
  // Radlauf unter dem Wagen hindurch.
  for (const a of [ACHSE_H, ACHSE_V]) {
    for (const s of [1, -1]) {
      const v = s * (SPUR - RAD_B / 2 - 0.03);
      scheibe(aus, 'Reifen', UNTEN, a, v, RAD_R, BOGEN_R, 12, s, true);
      scheibe(aus, 'Reifen', UNTEN, a, v, RAD_R, BOGEN_R, 12, -s, true);
      rad(aus, a, s * SPUR);
    }
  }

  // Front: Scheinwerfer, Kuehlergrill, Kennzeichen. Knapp vor dem Deckel.
  const uf = 2.146, ur = -2.146;
  const quer = (u, v0, v1, h0, h1, nu, k, rgb) =>
    viereckNach(aus, k, rgb, [u, v0, h0], [u, v1, h0], [u, v1, h1], [u, v0, h1], [nu, 0, 0]);
  for (const s of [1, -1]) {
    quer(uf, s * 0.40, s * 0.64, 0.56, 0.645, 1, 'Glas', 0xE4EAEE);            // Scheinwerfer
    quer(ur, s * 0.50, s * 0.76, 0.72, 0.84, -1, 'Auto', 0x9A1C18);             // Rueckleuchten
  }
  quer(uf, -0.32, 0.32, 0.49, 0.60, 1, 'Reifen', UNTEN);                        // Grill
  quer(uf + 0.001, -0.26, 0.26, 0.365, 0.475, 1, 'Stone', 0xEDEDE8);            // Kennzeichen vorn
  quer(ur - 0.001, -0.26, 0.26, 0.50, 0.61, -1, 'Stone', 0xEDEDE8);             // Kennzeichen hinten

  // Aussenspiegel an der A-Saeule
  for (const s of [1, -1]) {
    const v0 = s * 0.885, v1 = s * 1.03;
    kasten(aus, 'Auto', lack, 0.50, 0.64, Math.min(v0, v1), Math.max(v0, v1), 0.97, 1.07);
  }
}

module.exports = { wagen, RAD_R, SPUR, ACHSE_H, ACHSE_V };
