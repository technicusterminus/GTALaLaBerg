'use strict';
// Von Claude. Weitet die Unreal-Ausleitung vom Hauptplatz auf die ganze
// Stadt aus: alle Haeuser mit Dach, das Strassennetz, die Bahnstrecke und
// das eingefaerbte Gelaende. Schema und Zieldatei bleiben unveraendert,
// damit der bestehende C++-Ladepfad nichts merkt: eine Sektion je Farbe.
//
//   node "tools/unreal/prepare-stadt.cjs"

const fs = require('node:fs'), path = require('node:path'), vm = require('node:vm'), crypto = require('node:crypto');
const { ShapeUtils, Vector2 } = require('three');

// Rohdaten (data/citydata.js usw.) liegen im Spielprojekt "GTA LaLaBerg",
// nicht in diesem Repo. LALABERG_QUELLE zeigt auf dessen Wurzel; ohne
// Angabe wird der Nachbarordner neben diesem Repo angenommen.
const root = process.env.LALABERG_QUELLE
  ? path.resolve(process.env.LALABERG_QUELLE)
  : path.resolve(__dirname, '../../../GTA LaLaBerg');
const REPO = path.resolve(__dirname, '../..');
const read = rel => fs.readFileSync(path.join(root, rel), 'utf8');
const QUELLEN = ['data/citydata.js', 'data/terrain.js', 'data/phase9data.js', 'data/phase11data.js', 'js/terrain.js'];

// Die Spielmodule erwarten ein window. Ein Sandkasten genuegt ihnen.
const box = { window: null, Math: Math, Float32Array: Float32Array, Uint8Array: Uint8Array, Buffer: Buffer, Map: Map, console: console };
box.window = box;
vm.createContext(box);
for (const datei of QUELLEN) vm.runInContext(read(datei), box, { timeout: 30000 });

const city = box.CITY;
const Terrain = box.Terrain;
Terrain.init(box.TERRAIN, city);

const ORIGIN = [605, -100];                 // Hauptplatz bleibt der Nullpunkt
const M = 100;                              // Unreal rechnet in Zentimetern
const boden = (x, z) => Terrain.heightAt(x, z);

// ---------------------------------------------------------------- Sektionen
// Eine Sektion je Klasse und Farbe. Der Farbraum wird auf 5 Stufen je Kanal
// gerastert; darunter sieht man den Unterschied ohnehin nicht mehr.
const sektionen = new Map();
// Pflaster feiner: mit fuenf Stufen wurde aus Steingrau ein helles 0,5 und
// der Hauptplatz stand fast weiss in der Sonne.
const FEIN = new Set(['Plaza']);
function ziel(klasse, rgb) {
  const stufen = FEIN.has(klasse) ? 8 : 4;
  const q = rgb.map(v => Math.round(Math.max(0, Math.min(1, v)) * stufen) / stufen);
  const name = klasse + (stufen === 4 ? '_' : '_f') + q.map(v => Math.round(v * stufen)).join('');
  let s = sektionen.get(name);
  if (!s) { s = { name, color: q, positions: [], indices: [], uvs: [] }; sektionen.set(name, s); }
  return s;
}
const farbe = v => [(v >> 16 & 255) / 255, (v >> 8 & 255) / 255, (v & 255) / 255];
function mische(a, b, t) {
  const k = (v, s) => (v >> s) & 255;
  const m = s => Math.round(k(a, s) * (1 - t) + k(b, s) * t);
  return (m(16) << 16) | (m(8) << 8) | m(0);
}

// Spielkoordinaten (x, hoehe, z) -> Unreal (x, y, z) in Zentimetern
function punkt(s, x, y, z, u, v) {
  s.positions.push(Math.round((x - ORIGIN[0]) * M), Math.round((z - ORIGIN[1]) * M), Math.round(y * M));
  // Waende bekommen eine eigene Texturkoordinate: waagerecht die abgelaufene
  // Fassadenlaenge, senkrecht die Hoehe ueber dem Sockel. Nur so sitzen die
  // Fenster geschossweise und nicht in einem Weltraster quer durchs Haus.
  if (u !== undefined) s.uvs.push(+u.toFixed(4), +v.toFixed(4));
  return s.positions.length / 3 - 1;
}
// Unreal zeichnet die Vorderseite im Uhrzeigersinn, das Spiel und alle
// Vorlagen hier laufen andersherum. Ohne diese Umkehr wird jede Flaeche
// weggeschnitten - und bei zweiseitigem Material klappt die Normale um, dann
// steht die ganze Stadt im Dunkeln.
//
// Zugleich die einzige Stelle, an der entartete Dreiecke abgefangen werden:
// Flaechen ohne Ausdehnung entstehen an First-Enden, in Dachgauben und bei
// doppelten Stuetzpunkten. Sie sind unsichtbar, aber der StaticMesh-Import
// bricht daran ab, weil sich fuer sie keine Normale bilden laesst.
let entartet = 0;
function dreieck(s, a, b, c) {
  const p = s.positions, ia = a * 3, ib = b * 3, ic = c * 3;
  const ux = p[ib] - p[ia], uy = p[ib + 1] - p[ia + 1], uz = p[ib + 2] - p[ia + 2];
  const vx = p[ic] - p[ia], vy = p[ic + 1] - p[ia + 1], vz = p[ic + 2] - p[ia + 2];
  const nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
  if (nx * nx + ny * ny + nz * nz < 4.0) { entartet++; return; }   // unter 1 cm2
  s.indices.push(a, c, b);
}

// Fuer Bauteile, die nach der alten, umgekehrten Annahme gebaut sind (siehe
// sichtVon): dieselbe Flaeche mit gewendeter Sichtseite.
function flaecheGewendet(s, a, b, c, d) { flaeche(s, d, c, b, a); }

// Ein Viereck aus vier Weltpunkten, Umlaufsinn wie in der Vorlage.
function flaeche(s, a, b, c, d) {
  const i0 = punkt(s, a[0], a[1], a[2]), i1 = punkt(s, b[0], b[1], b[2]);
  const i2 = punkt(s, c[0], c[1], c[2]), i3 = punkt(s, d[0], d[1], d[2]);
  dreieck(s, i0, i1, i2); dreieck(s, i0, i2, i3);
}

// ------------------------------------------------------------ Hoehenraster
// Dasselbe 10-m-Raster, aus dem spaeter die Gelaendeflaeche entsteht. Wer
// etwas auf den Boden legt, muss genau dieses Raster abfragen: das Gelaende
// interpoliert zwischen den Rasterpunkten geradlinig und liegt in Mulden
// hoeher als die genaue Hoehenabfrage. Strassen und Plaetze verschwanden
// deshalb unter der Wiese.
const B = city.bounds, SCHRITT = 10;
// Bounded hospital pilot, source coordinates in metres; no other district
// is changed by the terrain sampling repair.
function imKlinikum(x, z) { return x >= -1150 && x <= -850 && z >= -100 && z <= 250; }
const cols = Math.ceil((B.x1 - B.x0) / SCHRITT) + 1, rows = Math.ceil((B.z1 - B.z0) / SCHRITT) + 1;
const hoehen = new Float64Array(cols * rows);
for (let r = 0; r < rows; r++) for (let c = 0; c < cols; c++) hoehen[r * cols + c] = boden(B.x0 + c * SCHRITT, B.z0 + r * SCHRITT);

function rasterHoehe(x, z, exakt = false) {
  const fx = (x - B.x0) / SCHRITT, fz = (z - B.z0) / SCHRITT;
  let c = Math.floor(fx), r = Math.floor(fz);
  c = Math.max(0, Math.min(cols - 2, c));
  r = Math.max(0, Math.min(rows - 2, r));
  const tx = Math.max(0, Math.min(1, fx - c)), tz = Math.max(0, Math.min(1, fz - r));
  const h00 = hoehen[r * cols + c], h10 = hoehen[r * cols + c + 1];
  const h01 = hoehen[(r + 1) * cols + c], h11 = hoehen[(r + 1) * cols + c + 1];
  // Hospital pilot: sample the actual two terrain triangles, not a bilinear
  // saddle surface which can lie below the rendered ground.
  if (exakt) {
    return tx >= tz
      ? h00 * (1 - tx) + h10 * (tx - tz) + h11 * tz
      : h00 * (1 - tz) + h11 * tx + h01 * (tz - tx);
  }
  return (h00 * (1 - tx) + h10 * tx) * (1 - tz) + (h01 * (1 - tx) + h11 * tx) * tz;
}
// Bruecken haengen nicht am Gelaende: dort gilt die Fahrbahnhoehe.
const auflage = (x, z) => Math.max(rasterHoehe(x, z), Terrain.surfaceAt(x, z));

// -------------------------------------------------------- Altstadtpalette
// Nach Fotos und Beschreibungen der Landsberger Innenstadt: der Hauptplatz
// ist von pastellfarbenen Buergerhaeusern des 15. und 16. Jahrhunderts
// gefasst, das Historische Rathaus traegt eine pastellrosa Stuckfassade von
// Dominikus Zimmermann. Die amtlichen Daten kennen diese Farben nicht, sie
// liefern fuer fast jedes Haus denselben blassen Grauton. Innerhalb der
// Altstadt bekommt deshalb jedes Haus einen Ton aus dieser Palette - fest
// gewaehlt aus seiner Lage, damit derselbe Bau immer gleich aussieht.
const ALTSTADT = { x0: 470, x1: 1240, z0: -520, z1: 520 };
const PUTZ = [
  0xEFE7DA,   // gebrochenes Weiss
  0xE9D9AE,   // Ocker
  0xE7C9C1,   // Altrosa
  0xDBE2D0,   // blasses Gruen
  0xD7DDE3,   // Graublau
  0xE4D4B6,   // Sandgelb
  0xE0CBB4,   // warmes Beige
  0xEDDCC6,   // helles Elfenbein
];
// Biberschwanz in Ton gebrannt: rotbraun mit Streuung, kein Lachsrosa.
const DACH = [0x8A4A33, 0x7B4130, 0x9A5A3E, 0x6E3A2C, 0x84462F];

// Die Farben kamen im Spiel blasser heraus, als sie in der Liste stehen:
// die Fototextur liegt als Modulator darueber und zieht jeden Ton zur Mitte.
// Deshalb die Saettigung hier schon anheben, statt im Material nachzudrehen -
// dort traefe es auch Dach, Pflaster und Wiese.
function kraeftiger(farbe, faktor) {
  const r = (farbe >> 16) & 255, g = (farbe >> 8) & 255, b = farbe & 255;
  const mittel = (r + g + b) / 3;
  const zieh = v => Math.max(0, Math.min(255, Math.round(mittel + (v - mittel) * faktor)));
  return (zieh(r) << 16) | (zieh(g) << 8) | zieh(b);
}
const PUTZ_KRAEFTIG = PUTZ.map(c => kraeftiger(c, 1.45));

// Ausserhalb der Altstadt kennt die Flaechennutzung keine Farben und liefert
// fuer fast jedes Haus denselben blassen Grauton - ganze Strassenzuege
// standen in Betongrau. Landsberger Vorstadt: warmer Ocker, Sandgelb,
// Cremeweiss, ein blasses Gruen und ein Graublau, dazwischen einzelne
// kraeftigere Toene.
const VORSTADT = [
  0xE8D6AE,   // Ocker
  0xEFE6D2,   // Cremeweiss
  0xDCD2BC,   // Sand
  0xD6DCCE,   // blasses Gruen
  0xD2DAE0,   // Graublau
  0xE6D2C2,   // Altrosa hell
  0xEDE4D0,   // Elfenbein
  0xDDCFB2,   // Naturputz
];
const VORSTADT_KRAEFTIG = VORSTADT.map(c => kraeftiger(c, 1.35));

function streuung(x, z) {          // gleiche Lage, gleiche Farbe
  const h = Math.abs(Math.round(x * 7.3) * 73856093 ^ Math.round(z * 7.3) * 19349663);
  return h % 1000 / 1000;
}
function inAltstadt(o) {
  return o[0] > ALTSTADT.x0 && o[0] < ALTSTADT.x1 && o[1] > ALTSTADT.z0 && o[1] < ALTSTADT.z1;
}

// Zwei Bauten praegen das Bild und stehen in den amtlichen Daten nur als
// namenlose Kloetze. Beide sind nach Fotos und Beschreibungen gefasst:
//
//  - Historisches Rathaus, Westseite des Hauptplatzes: viergeschossiger Bau
//    mit Walmdach und fuenfachsiger Fassade, 1719 von Dominikus Zimmermann
//    mit Stuck ueberzogen, zarte Farbigkeit - die Fotos zeigen Pastellrosa
//    mit weissem Stuck.
//  - Klinikum an der Roemerau-Terrasse, Neubau von 1968, seit 1990 in
//    mehreren Abschnitten erweitert: heller Putzbau mit Flachdach.
const WAHRZEICHEN = [
  { n: 'Historisches Rathaus', x: 555.2, z: -130.6, r: 22, wc: 0xE9CBC6, rc: 0x7B4130, h: 15.0, rt: 2, rh: 4.6 },
  { n: 'Klinikum',             x: -985.0, z: 42.3,  r: 70, wc: 0xE9E7E2, rc: 0x6A6E70, umfeld: true },
  // Schmalzturm ("Schoener Turm"), Tor der ersten Stadtmauer am hoechsten
  // Punkt des Hauptplatzes, 13. Jh.: Kleeblattfriese, bunte glasierte
  // Dachziegel in den Stadtfarben, Sturmglockenaufsatz (Wikipedia). Die
  // Daten nennen 23,7 m Gesamthoehe; die Hausdaten nur 11,5 m Wand - damit
  // stand der Turm kaum ueber den Buergerhaeusern.
  { n: 'Schmalzturm',          x: 654.5, z: -118.5, r: 6, wc: 0xECE3D1, rc: 0x3E6B4A, h: 17.0, rt: 3, rh: 6.7, turm: true },
];
// Nur das naechstgelegene Haus ist das Wahrzeichen selbst. Die Nachbarn am
// Hauptplatz sollen nicht alle rosa werden; auf dem Klinikgelaende dagegen
// gehoert der ganze helle Putz zusammen.
for (const w of WAHRZEICHEN) {
  let best = w.r, ziel = -1;
  city.buildings.forEach((bd, i) => {
    const d = Math.hypot(bd.o[0] - w.x, bd.o[1] - w.z);
    if (d < best) { best = d; ziel = i; }
  });
  w.index = ziel;
}
function wahrzeichenFuer(bd, i) {
  for (const w of WAHRZEICHEN) {
    if (w.index === i) return w;
    if (w.umfeld && Math.hypot(bd.o[0] - w.x, bd.o[1] - w.z) <= w.r) return { wc: w.wc, rc: w.rc };
  }
  return null;
}
let gefasst = 0;

// Ein umlaufendes Band am Haus: Sockel, Gesims, Ladenzone. Die aeussere
// Normale einer Wandkante ist (dz, -dx) - dieselbe Rechnung, die schon die
// Wandflaechen nach aussen zeigen laesst.
function haussband(s, p, yUnten, yOben, vor) {
  const n = p.length / 2;
  for (let i = 0; i < n; i++) {
    const j = (i + 1) % n;
    const x0 = p[i * 2], z0 = p[i * 2 + 1], x1 = p[j * 2], z1 = p[j * 2 + 1];
    const dx = x1 - x0, dz = z1 - z0, len = Math.hypot(dx, dz);
    if (len < 0.05) continue;
    const nx = dz / len * vor, nz = -dx / len * vor;
    const a0x = x0 + nx, a0z = z0 + nz, a1x = x1 + nx, a1z = z1 + nz;
    // Aussenseite (gewendet, siehe sichtVon)
    flaecheGewendet(s, [a1x, yUnten, a1z], [a0x, yUnten, a0z], [a0x, yOben, a0z], [a1x, yOben, a1z]);
    // Oberseite bis zur Wand zurueck
    if (vor > 0.01) flaeche(s, [x1, yOben, z1], [x0, yOben, z0], [a0x, yOben, a0z], [a1x, yOben, a1z]);
  }
}

function wandFlaeche(s, a, b, c, d, uv) {
  const i0 = punkt(s, a[0], a[1], a[2], uv[0], uv[1]);
  const i1 = punkt(s, b[0], b[1], b[2], uv[2], uv[3]);
  const i2 = punkt(s, c[0], c[1], c[2], uv[4], uv[5]);
  const i3 = punkt(s, d[0], d[1], d[2], uv[6], uv[7]);
  // Gewendet: nach aussen sichtbar (siehe sichtVon)
  dreieck(s, i0, i2, i1); dreieck(s, i0, i3, i2);
}

function walze(s, cx, cz, y0, y1, r0, r1, seiten) {
  const u = [], o = [];
  for (let i = 0; i < seiten; i++) {
    const a = i / seiten * Math.PI * 2;
    u.push(punkt(s, cx + Math.cos(a) * r0, y0, cz + Math.sin(a) * r0));
    o.push(punkt(s, cx + Math.cos(a) * r1, y1, cz + Math.sin(a) * r1));
  }
  for (let i = 0; i < seiten; i++) {
    const j = (i + 1) % seiten;
    dreieck(s, u[i], u[j], o[j]);
    dreieck(s, u[i], o[j], o[i]);
  }
  return { u, o };
}
function deckel(s, cx, cz, y, r, seiten) {
  const mitte = punkt(s, cx, y, cz);
  const rand = [];
  for (let i = 0; i < seiten; i++) {
    const a = i / seiten * Math.PI * 2;
    rand.push(punkt(s, cx + Math.cos(a) * r, y, cz + Math.sin(a) * r));
  }
  for (let i = 0; i < seiten; i++) dreieck(s, rand[i], rand[(i + 1) % seiten], mitte);
}
// ------------------------------------------------------------------ Haeuser
let wandFlaechen = 0, dachFlaechen = 0, uebersprungen = 0, umgefaerbt = 0, baenderFlaechen = 0, kamine = 0, brandmauern = 0, tueren = 0;

let gaubenZiel = null, gaubenZahl = 0;
function dachbau(s, o, ytop, rh, mode, ueberstand) {
  const cx = o[0], cz = o[1], ang = o[2];
  const hw = o[3] / 2 + ueberstand, hd = o[4] / 2 + ueberstand;
  const c = Math.cos(ang), sn = Math.sin(ang);
  let ax, az, bx, bz, la, lb;
  if (hw >= hd) { ax = c * hw; az = sn * hw; bx = -sn * hd; bz = c * hd; la = hw; lb = hd; }
  else { ax = -sn * hd; az = c * hd; bx = c * hw; bz = sn * hw; la = hd; lb = hw; }
  const first = mode === 1 ? la : (mode === 2 ? Math.max(0, la - lb) : 0);
  const rx = (ax / la) * first, rz = (az / la) * first, yr = ytop + rh;
  const e00 = [cx - ax - bx, ytop, cz - az - bz], e10 = [cx + ax - bx, ytop, cz + az - bz];
  const e11 = [cx + ax + bx, ytop, cz + az + bz], e01 = [cx - ax + bx, ytop, cz - az + bz];
  const r0 = [cx - rx, yr, cz - rz], r1 = [cx + rx, yr, cz + rz];
  if (mode === 4) {                                  // Pultdach
    flaecheGewendet(s, [e01[0], yr, e01[2]], [e11[0], yr, e11[2]], e10, e00);
    flaecheGewendet(s, e01, e11, [e11[0], yr, e11[2]], [e01[0], yr, e01[2]]);
    dachFlaechen += 2; return;
  }
  flaecheGewendet(s, r0, r1, e10, e00);
  flaecheGewendet(s, r1, r0, e01, e11);
  if (gaubenZiel && rh >= 2.2) {
    const laenge = Math.hypot(e10[0] - e00[0], e10[2] - e00[2]);
    const anzahl = Math.max(1, Math.min(4, Math.floor(laenge / 6.5)));
    for (let k = 0; k < anzahl; k++) {
      const u = (k + 0.5) / anzahl;
      gaubenZahl += gaube(gaubenZiel.front, gaubenZiel.dach, e00, e10, r0, r1, u, 1.5);
      gaubenZahl += gaube(gaubenZiel.front, gaubenZiel.dach, e11, e01, r1, r0, u, 1.5);
    }
  }
  if (first > 0.05 || mode !== 1) {                   // Walm- oder Giebelseite
    // gewendet, siehe sichtVon
    dreieck(s, punkt(s, e11[0], e11[1], e11[2]), punkt(s, r1[0], r1[1], r1[2]), punkt(s, e10[0], e10[1], e10[2]));
    dreieck(s, punkt(s, e00[0], e00[1], e00[2]), punkt(s, r0[0], r0[1], r0[2]), punkt(s, e01[0], e01[1], e01[2]));
  }
  dachFlaechen += 2;
}

// Ein Hauseingang an der laengsten Fassade: Tuerblatt, Gewaende, Sturz. Ohne
// Tuer hat ein Haus keinen Massstab - man sieht nicht, wie gross es ist.
function tuer(sTuer, sStein, p, yBase) {
  const n = p.length / 2;
  let besteLaenge = 0, bi = -1;
  for (let i = 0; i < n; i++) {
    const j = (i + 1) % n;
    const len = Math.hypot(p[j * 2] - p[i * 2], p[j * 2 + 1] - p[i * 2 + 1]);
    if (len > besteLaenge) { besteLaenge = len; bi = i; }
  }
  if (bi < 0 || besteLaenge < 3.2) return false;
  const j = (bi + 1) % n;
  const x0 = p[bi * 2], z0 = p[bi * 2 + 1], x1 = p[j * 2], z1 = p[j * 2 + 1];
  const dx = x1 - x0, dz = z1 - z0, len = Math.hypot(dx, dz);
  const ux = dx / len, uz = dz / len;
  const nx = dz / len, nz = -dx / len;             // nach aussen
  // Nicht in der Mitte, sondern auf einer Fensterachse - das wirkt gewachsen.
  const t = 0.5 + (streuung(x0 + dx * 0.5, z0 + dz * 0.5) - 0.5) * 0.5;
  const mx = x0 + dx * t, mz = z0 + dz * t;
  const halb = 0.62, oben = yBase + 2.35;
  const P = (a, v) => [mx + ux * a + nx * v, 0, mz + uz * a + nz * v];
  const A = P(-halb, 0.06), B = P(halb, 0.06);
  flaecheGewendet(sTuer, [B[0], yBase, B[2]], [A[0], yBase, A[2]], [A[0], oben, A[2]], [B[0], oben, B[2]]);
  // Gewaende und Sturz aus Stein, 12 cm vor der Wand
  const G = 0.16;
  const A2 = P(-halb - G, 0.10), B2 = P(halb + G, 0.10);
  const A1 = P(-halb, 0.10), B1 = P(halb, 0.10);
  flaecheGewendet(sStein, [A1[0], yBase, A1[2]], [A2[0], yBase, A2[2]], [A2[0], oben + G, A2[2]], [A1[0], oben + G, A1[2]]);
  flaecheGewendet(sStein, [B2[0], yBase, B2[2]], [B1[0], yBase, B1[2]], [B1[0], oben + G, B1[2]], [B2[0], oben + G, B2[2]]);
  flaecheGewendet(sStein, [B2[0], oben, B2[2]], [A2[0], oben, A2[2]], [A2[0], oben + G, A2[2]], [B2[0], oben + G, B2[2]]);
  return true;
}

// Eine schmale Mauer zwischen zwei Dachhaelften: die Brandmauer, die in
// jeder Altstadtzeile die Haeuser trennt und ueber das Dach hinausragt.
function mauer(s, ax, az, bx, bz, yUnten, yOben, dicke) {
  const dx = bx - ax, dz = bz - az, len = Math.hypot(dx, dz);
  if (len < 0.2) return;
  const nx = dz / len * dicke * 0.5, nz = -dx / len * dicke * 0.5;
  const P = [[ax - nx, az - nz], [bx - nx, bz - nz], [bx + nx, bz + nz], [ax + nx, az + nz]];
  for (let i = 0; i < 4; i++) {
    const j = (i + 1) % 4;
    flaeche(s, [P[j][0], yUnten, P[j][1]], [P[i][0], yUnten, P[i][1]],
               [P[i][0], yOben, P[i][1]], [P[j][0], yOben, P[j][1]]);
  }
  flaeche(s, [P[1][0], yOben, P[1][1]], [P[0][0], yOben, P[0][1]],
             [P[3][0], yOben, P[3][1]], [P[2][0], yOben, P[2][1]]);
}

// Eine Altstadtzeile ist keine Halle mit einem einzigen Dach, sondern eine
// Reihe schmaler Haeuser, jedes mit eigenem Giebel und Brandmauer dazwischen.
// Die amtlichen Daten fassen die Zeile zu einem Grundriss zusammen; hier wird
// sie fuer das Dach wieder aufgeteilt.
function zeilenDach(dach, kamin, o, ytop, rh, mode, ueberstand, wcMisch) {
  const laengs = o[3] >= o[4];
  const lang = laengs ? o[3] : o[4], quer = laengs ? o[4] : o[3];
  const teile = Math.max(1, Math.min(9, Math.round(lang / 11.5)));
  if (teile === 1 || lang < 15) { dachbau(dach, o, ytop, rh, mode, ueberstand); return 0; }
  const c = Math.cos(o[2]), sn = Math.sin(o[2]);
  const ux = laengs ? c : -sn, uz = laengs ? sn : c;     // Richtung der Zeile
  const vx = -uz, vz = ux;                                // quer dazu
  const breite = lang / teile;
  let mauern = 0;
  for (let i = 0; i < teile; i++) {
    const versatz = ((i + 0.5) / teile - 0.5) * lang;
    const cx = o[0] + ux * versatz, cz = o[1] + uz * versatz;
    // Jedes Haus etwas anders hoch - eine Zeile steht nie auf einer Linie.
    const t = streuung(cx, cz);
    const eigen = rh * (0.86 + t * 0.28);
    const teilO = laengs ? [cx, cz, o[2], breite, quer] : [cx, cz, o[2], quer, breite];
    dachbau(dach, teilO, ytop, eigen, mode, ueberstand);
    if (i > 0) {
      const g = ((i / teile) - 0.5) * lang;
      const mx = o[0] + ux * g, mz = o[1] + uz * g;
      mauer(kamin, mx - vx * quer * 0.5, mz - vz * quer * 0.5,
                   mx + vx * quer * 0.5, mz + vz * quer * 0.5,
                   ytop - 0.5, ytop + eigen + 0.14, 0.26);
      mauern++;
    }
  }
  return mauern;
}

// Eine Schleppgaube auf der Dachflaeche: Fensterfront, zwei Wangen, kleines
// Dach. Eine ungegliederte Dachflaeche von 20 m Laenge gibt es in keiner
// Altstadt - die Gauben geben ihr Massstab.
function gaube(sFront, sDach, e0, e1, r0, r1, mitte, breite) {
  const misch = (a, b, t) => [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t];
  const traufe = (u) => misch(e0, e1, u);
  const first = (u) => misch(r0, r1, u);
  const auf = (u, t) => misch(traufe(u), first(u), t);
  const lang = Math.hypot(e1[0] - e0[0], e1[2] - e0[2]);
  if (lang < 5.0) return 0;
  const halb = breite / (2 * lang);
  const u0 = Math.max(0.06, mitte - halb), u1 = Math.min(0.94, mitte + halb);
  if (u1 - u0 < 0.02) return 0;
  const tU = 0.30, tO = 0.62;                   // unteres und oberes Drittel
  const F0 = auf(u0, tU), F1 = auf(u1, tU);
  const B0 = auf(u0, tO), B1 = auf(u1, tO);
  const hoch = 1.35;
  const T0 = [F0[0], F0[1] + hoch, F0[2]], T1 = [F1[0], F1[1] + hoch, F1[2]];
  // Aussenrichtung: von der Firstlinie zur Traufe, waagerecht
  const ax = traufe(mitte)[0] - first(mitte)[0], az = traufe(mitte)[2] - first(mitte)[2];
  const dx = F1[0] - F0[0], dz = F1[2] - F0[2];
  // Die Reihenfolge, deren Aussennormale (dz,-dx) nach aussen zeigt
  const richtig = (dz * ax - dx * az) > 0;
  const A = richtig ? F0 : F1, B = richtig ? F1 : F0;
  const TA = richtig ? T0 : T1, TB = richtig ? T1 : T0;
  const CA = richtig ? B0 : B1, CB = richtig ? B1 : B0;
  flaecheGewendet(sFront, [B[0], B[1], B[2]], [A[0], A[1], A[2]], [TA[0], TA[1], TA[2]], [TB[0], TB[1], TB[2]]);
  // Wangen
  flaecheGewendet(sDach, [A[0], A[1], A[2]], [CA[0], CA[1], CA[2]], [CA[0], CA[1] + 0.12, CA[2]], [TA[0], TA[1], TA[2]]);
  flaecheGewendet(sDach, [CB[0], CB[1], CB[2]], [B[0], B[1], B[2]], [TB[0], TB[1], TB[2]], [CB[0], CB[1] + 0.12, CB[2]]);
  // Gaubendach
  flaecheGewendet(sDach, [TB[0], TB[1], TB[2]], [TA[0], TA[1], TA[2]],
                 [CA[0], CA[1] + 0.12, CA[2]], [CB[0], CB[1] + 0.12, CB[2]]);
  return 1;
}

function flachdach(s, p, ytop) {
  const n = p.length / 2;
  const flach = [];
  for (let i = 0; i < n; i++) flach.push(new Vector2(p[i * 2], p[i * 2 + 1]));
  let tris;
  try { tris = ShapeUtils.triangulateShape(flach, []); } catch (e) { tris = []; }
  if (!tris.length) return false;
  const basis = s.positions.length / 3;
  for (const v of flach) punkt(s, v.x, ytop + 0.45, v.y);
  // Wie beim Pflaster: der Umlaufsinn muss andersherum gelesen werden, sonst
  // zeigt die Deckflaeche nach unten. Von oben klaffte dann ein Loch im Dach,
  // von unten stand eine schwarze Platte im Himmel.
  const flip = ShapeUtils.area(flach) < 0;
  for (const t of tris) dreieck(s, basis + t[0], basis + t[flip ? 2 : 1], basis + t[flip ? 1 : 2]);
  dachFlaechen++;
  return true;
}

// Flaechen, deren Sichtseite vorgegeben ist (n in Spielkoordinaten).
//
// Die Regel, am 11.09.2026 am Luftbild belegt: sichtbar ist eine Flaeche von
// der Seite, in die  -kreuz(B - A, C - B)  zeigt (Spielkoordinaten x, Hoehe,
// z; dreieck() tauscht fuer Unreal zwei Ecken, und die Achsenvertauschung
// y<->z spiegelt noch einmal). Waende, Baender, Tueren, Gauben und das
// Rechteckdach waren nach der umgekehrten Annahme gebaut und zeigten nach
// innen - von oben sah man in offene Kaesten, von der Strasse auf die
// Innenseiten der hinteren Waende. Sie sind unten ausdruecklich umgedreht.
function sichtVon(A, B, C, n) {
  const u = [B[0] - A[0], B[1] - A[1], B[2] - A[2]], v = [C[0] - B[0], C[1] - B[1], C[2] - B[2]];
  const k = [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]];
  return k[0] * n[0] + k[1] * n[1] + k[2] * n[2] <= 0;
}
function flaecheNach(s, A, B, C, D, n) {
  if (sichtVon(A, B, C, n)) flaeche(s, A, B, C, D); else flaeche(s, D, C, B, A);
}
function dreieckNach(s, a, b, c, n) {
  const [x, y, z] = sichtVon(a, b, c, n) ? [a, b, c] : [a, c, b];
  dreieck(s, punkt(s, x[0], x[1], x[2]), punkt(s, y[0], y[1], y[2]), punkt(s, z[0], z[1], z[2]));
}

// Dach nach dem echten Umriss fuer Winkel- und Hofhaeuser. dachbau() legt
// sein Dach ueber das umschliessende Rechteck; bei einem U-foermigen Haus
// deckte es den offenen Hof mit ab und schwebte dort als Platte ueber dem
// Nichts - so stand die Sparkasse am Hauptplatz im Bild. Hier steigen von
// jeder Traufe Dachflaechen nach innen, oben schliesst ein flacher Streifen.
function schneidenSich(a, b, c, d) {
  const o = (p, q, r) => Math.sign((q[0] - p[0]) * (r[1] - p[1]) - (q[1] - p[1]) * (r[0] - p[0]));
  return o(a, b, c) * o(a, b, d) < 0 && o(c, d, a) * o(c, d, b) < 0;
}
function umrissDach(s, p, ytop, rh) {
  const n = p.length / 2;
  let pts = [];
  for (let i = 0; i < n; i++) pts.push([p[i * 2], p[i * 2 + 1]]);
  const flaecheVon = q => ShapeUtils.area(q.map(v => new Vector2(v[0], v[1])));
  if (flaecheVon(pts) < 0) pts.reverse();
  const A0 = flaecheVon(pts);
  let umfang = 0;
  for (let i = 0; i < n; i++) { const j = (i + 1) % n; umfang += Math.hypot(pts[j][0] - pts[i][0], pts[j][1] - pts[i][1]); }
  // Einzug: Dachneigung rund 40 Grad, aber hoechstens gut zwei Fuenftel der
  // mittleren Fluegeltiefe - sonst laufen die Flaechen ueber Kreuz.
  let t = Math.min(rh * 1.19, 0.42 * A0 / (umfang / 2));
  const innen = (i, j) => {                                   // Innennormale der Kante i->j
    const dx = pts[j][0] - pts[i][0], dz = pts[j][1] - pts[i][1], l = Math.hypot(dx, dz) || 1;
    return [-dz / l, dx / l];
  };
  for (let versuch = 0; versuch < 4; versuch++, t *= 0.6) {
    if (t < 0.8) return false;
    const q = [];
    for (let i = 0; i < n; i++) {
      const h = (i + n - 1) % n, j = (i + 1) % n;
      const n1 = innen(h, i), n2 = innen(i, j);
      let bx = n1[0] + n2[0], bz = n1[1] + n2[1];
      const bl = Math.hypot(bx, bz);
      if (bl < 1e-6) { bx = n1[0]; bz = n1[1]; } else { bx /= bl; bz /= bl; }
      const cosH = Math.max(0.4, bx * n1[0] + bz * n1[1]);   // spitze Ecken nicht ins Unendliche
      q.push([pts[i][0] + bx * t / cosH, pts[i][1] + bz * t / cosH]);
    }
    const Aq = flaecheVon(q);
    if (!(Aq > 0 && Aq < A0)) continue;
    let kreuzt = false;
    for (let i = 0; i < n && !kreuzt; i++)
      for (let k = i + 2; k < n && !kreuzt; k++)
        if (!(i === 0 && k === n - 1) && schneidenSich(q[i], q[(i + 1) % n], q[k], q[(k + 1) % n])) kreuzt = true;
    if (kreuzt) continue;
    const yr = ytop + Math.min(rh, t * 0.84);
    for (let i = 0; i < n; i++) {
      const j = (i + 1) % n;
      const dx = pts[j][0] - pts[i][0], dz = pts[j][1] - pts[i][1], l = Math.hypot(dx, dz);
      if (l < 0.05) continue;
      flaecheNach(s, [pts[i][0], ytop, pts[i][1]], [pts[j][0], ytop, pts[j][1]],
                     [q[j][0], yr, q[j][1]], [q[i][0], yr, q[i][1]], [dz / l, 1, -dx / l]);
    }
    let tris;
    try { tris = ShapeUtils.triangulateShape(q.map(v => new Vector2(v[0], v[1])), []); } catch (e) { tris = []; }
    for (const tr of tris) dreieckNach(s, [q[tr[0]][0], yr, q[tr[0]][1]], [q[tr[1]][0], yr, q[tr[1]][1]],
                                          [q[tr[2]][0], yr, q[tr[2]][1]], [0, 1, 0]);
    dachFlaechen += n + 1;
    return true;
  }
  return false;
}
let umrissDaecher = 0, viereckDaecher = 0;

// Punkte eines Umrisses um t nach innen versetzt (t < 0: nach aussen), fuer
// Umrisse mit positiver Flaeche. Spitze Ecken werden begrenzt.
function einzugPunkte(pts, t) {
  const n = pts.length, q = [];
  const innen = (i, j) => {
    const dx = pts[j][0] - pts[i][0], dz = pts[j][1] - pts[i][1], l = Math.hypot(dx, dz) || 1;
    return [-dz / l, dx / l];
  };
  for (let i = 0; i < n; i++) {
    const h = (i + n - 1) % n, j = (i + 1) % n;
    const n1 = innen(h, i), n2 = innen(i, j);
    let bx = n1[0] + n2[0], bz = n1[1] + n2[1];
    const bl = Math.hypot(bx, bz);
    if (bl < 1e-6) { bx = n1[0]; bz = n1[1]; } else { bx /= bl; bz /= bl; }
    const cosH = Math.max(0.4, bx * n1[0] + bz * n1[1]);
    q.push([pts[i][0] + bx * t / cosH, pts[i][1] + bz * t / cosH]);
  }
  return q;
}
const positiv = pts => (ShapeUtils.area(pts.map(v => new Vector2(v[0], v[1]))) < 0 ? pts.slice().reverse() : pts);
const mitteVon = pts => [pts.reduce((a, v) => a + v[0], 0) / pts.length, pts.reduce((a, v) => a + v[1], 0) / pts.length];
// Aussennormale einer Kante, weg von der Mitte des Umrisses
function aussen(a, b, m) {
  const dx = b[0] - a[0], dz = b[1] - a[1], l = Math.hypot(dx, dz) || 1;
  let nx = dz / l, nz = -dx / l;
  if (nx * ((a[0] + b[0]) / 2 - m[0]) + nz * ((a[1] + b[1]) / 2 - m[1]) < 0) { nx = -nx; nz = -nz; }
  return [nx, nz];
}

// Turmhelm: Pyramide in waagerechten Baendern wechselnder Farbe - die
// glasierten Ziegel des Schmalzturms - und oben ein achteckiger Aufsatz fuer
// die Sturmglocke.
const HELM = [0x3E6B4A, 0x8E3A2E, 0xC9A646, 0x3E6B4A, 0xE8E2D2, 0x8E3A2E];
function turmhelm(p, ytop, rh, wc) {
  const q = einzugPunkte(positiv(p), -0.25);              // kleiner Dachvorsprung
  const m = mitteVon(q), n = q.length;
  const spitze = 0.80, baender = HELM.length;
  const auf = (v, t) => [v[0] + (m[0] - v[0]) * t, ytop + rh * t, v[1] + (m[1] - v[1]) * t];
  for (let k = 0; k < baender; k++) {
    const t0 = k / baender * spitze, t1 = (k + 1) / baender * spitze;
    const s = ziel('Roof', farbe(HELM[k]));
    for (let i = 0; i < n; i++) {
      const j = (i + 1) % n, o = aussen(q[i], q[j], m);
      flaecheNach(s, auf(q[i], t0), auf(q[j], t0), auf(q[j], t1), auf(q[i], t1), [o[0], 0.8, o[1]]);
    }
  }
  const yA = ytop + rh * spitze;
  const r = Math.max(0.6, Math.min(...q.map(v => Math.hypot(v[0] - m[0], v[1] - m[1]))) * (1 - spitze) * 0.9);
  walze(ziel('Gesims', farbe(wc)), m[0], m[1], yA - 0.3, yA + 1.5, r, r, 8);
  walze(ziel('Roof', farbe(0x3E6B4A)), m[0], m[1], yA + 1.5, yA + 3.4, r * 1.15, 0.05, 8);
  walze(ziel('Stone', farbe(0xC9A646)), m[0], m[1], yA + 3.4, yA + 4.2, 0.06, 0.06, 4);
  dachFlaechen += n * baender;
}

// Sattel- (mode 1) oder Walmdach (mode 2) auf den vier echten Ecken eines
// Grundrisses statt auf dessen umschliessendem Rechteck. Bei schiefen
// Vierecken ragte das Rechteckdach ueber die Waende hinaus und schwebte.
// Die Giebeldreiecke bekommen die Putzfarbe (sGiebel), nicht die Ziegel.
function viereckDach(s, sGiebel, q0, ytop, rh, mode, ueberstand) {
  let q = einzugPunkte(positiv(q0), -ueberstand);
  const L = (a, b) => Math.hypot(b[0] - a[0], b[1] - a[1]);
  if (L(q[1], q[2]) + L(q[3], q[0]) > L(q[0], q[1]) + L(q[2], q[3])) q = [q[1], q[2], q[3], q[0]];
  const [A, B, C, D] = q, m = mitteVon(q);
  const Mda = [(D[0] + A[0]) / 2, (D[1] + A[1]) / 2], Mbc = [(B[0] + C[0]) / 2, (B[1] + C[1]) / 2];
  const la = L(Mda, Mbc) / 2, lb = (L(B, C) + L(D, A)) / 4;
  const first = mode === 1 ? la : Math.max(0, la - lb);
  const dx = (Mbc[0] - Mda[0]) / (2 * la || 1), dz = (Mbc[1] - Mda[1]) / (2 * la || 1);
  const mm = [(Mda[0] + Mbc[0]) / 2, (Mda[1] + Mbc[1]) / 2];
  const yr = ytop + rh;
  const r0 = [mm[0] - dx * first, yr, mm[1] - dz * first], r1 = [mm[0] + dx * first, yr, mm[1] + dz * first];
  const E = v => [v[0], ytop, v[1]];
  const oAB = aussen(A, B, m), oCD = aussen(C, D, m), oBC = aussen(B, C, m), oDA = aussen(D, A, m);
  flaecheNach(s, E(A), E(B), r1, r0, [oAB[0], 1, oAB[1]]);
  flaecheNach(s, E(C), E(D), r0, r1, [oCD[0], 1, oCD[1]]);
  const ende = mode === 1 && sGiebel ? sGiebel : s, steil = mode === 1 ? 0 : 1;
  dreieckNach(ende, E(B), E(C), r1, [oBC[0], steil, oBC[1]]);
  dreieckNach(ende, E(D), E(A), r0, [oDA[0], steil, oDA[1]]);
  if (gaubenZiel && rh >= 2.2) {
    const anzahl = Math.max(1, Math.min(4, Math.floor(L(A, B) / 6.5)));
    for (let k = 0; k < anzahl; k++) {
      const u = (k + 0.5) / anzahl;
      gaubenZahl += gaube(gaubenZiel.front, gaubenZiel.dach, E(A), E(B), r0, r1, u, 1.5);
      gaubenZahl += gaube(gaubenZiel.front, gaubenZiel.dach, E(C), E(D), r1, r0, u, 1.5);
    }
  }
  dachFlaechen += 2;
}

// Die vier Ecken eines fast rechteckigen Vielecks: je Ecke des umschliessenden
// Rechtecks die naechste Grundrissecke. Ein Haus mit Erker oder Knick bekommt
// so ein Dach auf seinen Waenden statt auf dem Rechteck. null, wenn die vier
// nicht verschieden sind oder nicht in Umlaufreihenfolge liegen.
function hauptEcken(ecken, o) {
  const c = Math.cos(o[2]), sn = Math.sin(o[2]), hw = o[3] / 2, hd = o[4] / 2;
  const rechteck = [[-hw, -hd], [hw, -hd], [hw, hd], [-hw, hd]].map(([a, b]) => [o[0] + c * a - sn * b, o[1] + sn * a + c * b]);
  const wahl = rechteck.map(r => {
    let beste = -1, d = Infinity;
    ecken.forEach((e, i) => { const k = Math.hypot(e[0] - r[0], e[1] - r[1]); if (k < d) { d = k; beste = i; } });
    return beste;
  });
  if (new Set(wahl).size !== 4) return null;
  // Umlaufreihenfolge im Vieleck pruefen (zyklisch aufsteigend oder absteigend)
  const n = ecken.length, schritt = (a, b) => (b - a + n) % n;
  const vor = schritt(wahl[0], wahl[1]) + schritt(wahl[1], wahl[2]) + schritt(wahl[2], wahl[3]) + schritt(wahl[3], wahl[0]);
  const rueck = schritt(wahl[1], wahl[0]) + schritt(wahl[2], wahl[1]) + schritt(wahl[3], wahl[2]) + schritt(wahl[0], wahl[3]);
  if (vor !== n && rueck !== n) return null;
  return wahl.map(i => ecken[i]);
}

// Eine Zeile auf viereckigem Grundriss in schmale Haeuser geteilt - wie
// zeilenDach(), aber entlang der echten Traufkanten.
function zeilenViereck(dach, kamin, sGiebel, q0, ytop, rh, mode, ueberstand) {
  let q = positiv(q0);
  const L = (a, b) => Math.hypot(b[0] - a[0], b[1] - a[1]);
  if (L(q[1], q[2]) + L(q[3], q[0]) > L(q[0], q[1]) + L(q[2], q[3])) q = [q[1], q[2], q[3], q[0]];
  const [A, B, C, D] = q;
  const lang = (L(A, B) + L(D, C)) / 2;
  const teile = Math.max(1, Math.min(9, Math.round(lang / 11.5)));
  if (teile === 1 || lang < 15) { viereckDach(dach, sGiebel, q, ytop, rh, mode, ueberstand); return 0; }
  const P = (a, b, t) => [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t];
  let mauern = 0;
  for (let i = 0; i < teile; i++) {
    const t0 = i / teile, t1 = (i + 1) / teile;
    const teil = [P(A, B, t0), P(A, B, t1), P(D, C, t1), P(D, C, t0)];
    const mc = mitteVon(teil);
    const eigen = rh * (0.86 + streuung(mc[0], mc[1]) * 0.28);
    viereckDach(dach, sGiebel, teil, ytop, eigen, mode, ueberstand);
    if (i > 0) {
      const a = P(A, B, t0), b = P(D, C, t0);
      mauer(kamin, a[0], a[1], b[0], b[1], ytop - 0.5, ytop + eigen + 0.14, 0.26);
      mauern++;
    }
  }
  return mauern;
}

for (let bi = 0; bi < city.buildings.length; bi++) {
  const bd = city.buildings[bi];
  const p = bd.p, n = p.length / 2;
  if (n < 3) { uebersprungen++; continue; }
  // Das Haus setzt sich auf die mittlere Gelaendehoehe seiner Ecken. Nahm man
  // die tiefste, kletterte der Hang an den Waenden hoch; nahm man die
  // hoechste, schwebten Haeuser am Abhang.
  let tief = Infinity, hoch = -Infinity;
  for (let i = 0; i < n; i++) {
    const h = rasterHoehe(p[i * 2], p[i * 2 + 1]);
    if (h < tief) tief = h;
    if (h > hoch) hoch = h;
  }
  const yBase = (tief + hoch) * 0.5;
  const yFuss = tief - 0.9;                 // Sockel bis unter die tiefste Ecke
  // Wahrzeichen behalten ihre eigenen Farben - Schmalzturm, Bayertor und
  // die Stadtpfarrkirche sind in den Daten schon einzeln erfasst.
  let wc = bd.wc, rc = bd.rc, hoehe = bd.h, dachArt = bd.rt, dachHoehe = bd.rh;
  const w = wahrzeichenFuer(bd, bi);
  if (w) {
    wc = w.wc; rc = w.rc;
    if (w.h !== undefined) hoehe = w.h;
    if (w.rt !== undefined) dachArt = w.rt;
    if (w.rh !== undefined) dachHoehe = w.rh;
    gefasst++;
  }
  // Die Staustufe 15 steht im Fluss und kam als schwarzer Klotz heraus. So
  // ein Wehr ist Sichtbeton, kein Wohnhaus.
  if (w) { /* schon gefasst */ }
  else if (Terrain.isWater(bd.o[0], bd.o[1])) { wc = 0xB4B4AE; rc = 0x9C9C96; }
  else if (!bd.n && inAltstadt(bd.o)) {
    const t = streuung(bd.o[0], bd.o[1]);
    wc = PUTZ_KRAEFTIG[Math.floor(t * PUTZ_KRAEFTIG.length) % PUTZ_KRAEFTIG.length];
    rc = DACH[Math.floor(t * 997) % DACH.length];
    umgefaerbt++;
  } else if (inAltstadt(bd.o)) {
    // Benannte Haeuser behalten ihren Putz, aber die Dachfarbe der Daten ist
    // die Notfallangabe - die Sparkasse am Hauptplatz trug ein gelbes Dach.
    rc = DACH[Math.floor(streuung(bd.o[0], bd.o[1]) * 997) % DACH.length];
  } else {
    // Vorstadt: dieselbe Behandlung wie in der Altstadt, nur mit der
    // eigenen Palette. Feste Wahl aus der Lage, damit dasselbe Haus immer
    // gleich aussieht; das Dach kommt in jedem Fall aus der Ziegelreihe,
    // die Dachfarben der Daten sind eine Notfallangabe (im Bild leuchteten
    // ganze Zeilen orange).
    const t = streuung(bd.o[0], bd.o[1]);
    wc = VORSTADT_KRAEFTIG[Math.floor(t * VORSTADT_KRAEFTIG.length) % VORSTADT_KRAEFTIG.length];
    rc = DACH[Math.floor(t * 997) % DACH.length];
    umgefaerbt++;
  }
  const yTop = yBase + hoehe;

  // Die amtlichen Daten nennen Dachhoehen bis 11 m bei 11,5 m Wandhoehe. Auf
  // den zusammengefassten Blockgrundrissen der Altstadt werden daraus
  // Riesengiebel, die alles ueberragen - genau der kastige, chaotische
  // Eindruck. Ein Altstadtdach steigt rund 45 Grad und bleibt unter der
  // halben Bautiefe.
  const halbeTiefe = Math.min(bd.o[3], bd.o[4]) * 0.5;
  const grundflaeche = bd.o[3] * bd.o[4];
  // Wie viel des umschliessenden Rechtecks der Grundriss ausfuellt. Winkel-
  // und Hofhaeuser liegen deutlich darunter und bekommen ein Dach nach Umriss.
  const fuellt = Math.abs(ShapeUtils.area(Array.from({ length: n }, (_, i) => new Vector2(p[i * 2], p[i * 2 + 1])))) / grundflaeche;
  // Auch Vierecke: ein schiefes Trapez fuellt sein Rechteck ebenso schlecht,
  // und das Rechteckdach ragte dann als Pyramide ueber die Waende hinaus.
  const nachUmriss = fuellt < 0.82;
  // In der Altstadt gibt es keine Flachdaecher. Wo die Daten eines nennen,
  // ist es die Notfallangabe fuer ein Haus, das nie vermessen wurde - und es
  // legte eine waagerechte Platte ueber die Dachlandschaft.
  if (dachArt === 0 && hoehe >= 6 && inAltstadt(bd.o) && !w) {
    dachArt = grundflaeche > 650 ? 2 : 1;
    dachHoehe = Math.min(halbeTiefe * 0.8, 5.0);
  }
  // Dachtyp 3 ist der Turmhelm. Ueber einem Kirchenschiff wird daraus eine
  // Pyramide von 30 m Hoehe, die als Platte ueber der Altstadt schwebt - so
  // stand die Stadtpfarrkirche im Bild. Ein Schiff traegt ein steiles
  // Satteldach; die Pyramide bleibt den schmalen Tuermen.
  if (dachArt === 3 && grundflaeche > 220) {
    dachArt = 1;
    dachHoehe = Math.min(dachHoehe, halbeTiefe * 1.25, 11.0);
  } else if (dachArt !== 0) {
    dachHoehe = Math.min(dachHoehe, halbeTiefe * 0.95, hoehe * 0.55, 8.0);
    // Untergrenze: manche Datensaetze nennen fast keine Dachhoehe. Aus einem
    // 40-m-Grundriss wird dann eine waagerechte Platte, die ueber der Stadt
    // schwebt. Ein Ziegeldach steigt immer.
    dachHoehe = Math.max(dachHoehe, Math.min(halbeTiefe * 0.42, 3.2));
  }
  if (dachArt === 3) {
    dachHoehe = Math.min(dachHoehe, halbeTiefe * 3.2);
    // Ein 40 m breiter Block traegt keinen durchgehenden Giebel; ein Walmdach
    // liegt ruhiger und ist bei so grossen Grundrissen auch das Uebliche.
    if (dachArt === 1 && grundflaeche > 650) dachArt = 2;
  }

  const wand = ziel('Wall', farbe(wc));
  const ACHSE = 2.60, GESCHOSS = 3.20;   // Fensterachse und Geschosshoehe
  // Die Kachel soll unten mit dem Erdgeschossboden anfangen und oben
  // moeglichst glatt unter der Traufe enden.
  const geschosse = Math.max(1, Math.round(hoehe / GESCHOSS));
  const vOben = hoehe >= 4.2 ? geschosse : hoehe / GESCHOSS;
  let lauf = 0;
  for (let i = 0; i < n; i++) {
    const j = (i + 1) % n;
    const x0 = p[i * 2], z0 = p[i * 2 + 1], x1 = p[j * 2], z1 = p[j * 2 + 1];
    const len = Math.hypot(x1 - x0, z1 - z0);
    if (len < 0.05) continue;
    // Jede Wand faengt bei einer ganzen Achse an, damit keine halben Fenster
    // um die Hausecke laufen.
    const achsen = Math.max(1, Math.round(len / ACHSE));
    const u0 = 0, u1 = achsen;
    const vUnten = -0.6 / GESCHOSS;
    // Bis 60 cm unter die tiefste Ecke: schliesst die Fuge zum Gelaende.
    wandFlaeche(wand, [x1, yFuss, z1], [x0, yFuss, z0], [x0, yTop, z0], [x1, yTop, z1],
      [u1, vUnten, u0, vUnten, u0, vOben, u1, vOben]);
    wandFlaechen++;
    lauf += len;
  }
  // Sockel, Ladenzone, Traufgesims und Kamin. Ohne sie bleibt jedes Haus ein
  // glatter Kasten - genau das, was ein Altstadthaus nie ist.
  if (hoehe >= 3.0) {
    const sockel = ziel('Sockel', farbe(mische(wc, 0x6E6A62, 0.55)));
    haussband(sockel, p, yFuss, yBase + 0.52, 0.10);
    baenderFlaechen++;

    const gesims = ziel('Gesims', farbe(mische(wc, 0xFFFFFF, 0.30)));
    haussband(gesims, p, yTop - 0.34, yTop + 0.04, 0.28);
    baenderFlaechen++;

    if (hoehe >= 6.0 && inAltstadt(bd.o) && !w) {
      // Erdgeschoss als Ladenzone: dunkle Auslage statt Wohnfenster.
      const laden = ziel('Laden', farbe(0x35322E));
      haussband(laden, p, yBase + 0.55, yBase + 2.85, 0.03);
      baenderFlaechen++;
    }
  }
  if (dachArt !== 0 && dachHoehe >= 0.4 && hoehe >= 5.0 && !nachUmriss) {
    const kamin = ziel('Kamin', farbe(mische(rc, 0x4A4038, 0.45)));
    const kx = bd.o[0] + Math.cos(bd.o[2]) * bd.o[3] * 0.22;
    const kz = bd.o[1] + Math.sin(bd.o[2]) * bd.o[3] * 0.22;
    // Ein Kamin steht knapp ueber dem First, nicht als Mast darueber.
    const oben = yTop + Math.min(dachHoehe, 3.0) * 0.8 + 0.85;
    walze(kamin, kx, kz, yTop - 0.4, oben, 0.46, 0.40, 4);
    deckel(kamin, kx, kz, oben, 0.50, 4);
    kamine++;
  }

  if (hoehe >= 4.0) {
    const tuerS = ziel('Tuer', farbe(mische(rc, 0x2A1F16, 0.55)));
    const gewS = ziel('Gesims', farbe(mische(wc, 0xFFFFFF, 0.42)));
    if (tuer(tuerS, gewS, p, yBase + 0.02)) tueren++;
  }

  const dach = ziel('Roof', farbe(rc));
  const ecken = Array.from({ length: n }, (_, i) => [p[i * 2], p[i * 2 + 1]]);
  if (dachArt === 0 || dachHoehe < 0.4) { if (!flachdach(dach, p, yTop)) uebersprungen++; }
  else if (w && w.turm) turmhelm(ecken, yTop, dachHoehe, wc);
  else if (dachArt === 3) dachbau(dach, bd.o, yTop, dachHoehe, dachArt, 0.12);
  else if (nachUmriss && umrissDach(dach, p, yTop, dachHoehe)) umrissDaecher++;
  else {
    // Die Brandmauer ist verputzt wie das Haus, nur etwas grauer - als
    // weisses Blatt ueber dem First faellt sie unangenehm auf.
    const brand = ziel('Sockel', farbe(mische(wc, 0x8A8378, 0.42)));
    gaubenZiel = hoehe >= 6 && dachHoehe >= 2.2
      ? { front: ziel('Tuer', farbe(mische(wc, 0x2E3238, 0.62))), dach: ziel('Roof', farbe(mische(rc, 0x2A2420, 0.25))) }
      : null;
    const vier = n === 4 ? ecken : hauptEcken(ecken, bd.o);
    if (vier) {
      // Giebeldreieck in der Farbe des Hauses, wie ein verputzter Giebel
      const giebel = ziel('Sockel', farbe(mische(wc, 0x8A8378, 0.10)));
      brandmauern += zeilenViereck(dach, brand, giebel, vier, yTop, dachHoehe, dachArt, 0.30);
      viereckDaecher++;
    } else brandmauern += zeilenDach(dach, brand, bd.o, yTop, dachHoehe, dachArt, 0.30, wc);
    gaubenZiel = null;
  }
}

// --------------------------------------------------------- Baender am Boden
// Strassen, Wege und Gleise liegen als Mittellinie mit Breite vor. Jedes
// Segment bekommt ein eigenes Rechteck, das der Gelaendehoehe folgt.
// Split road polygons along the SAME cells and diagonal as the terrain.
// More samples alone cannot prevent a wide road triangle cutting a ridge.
function klinikumFlaeche(s, quad, hoch) {
  const clip = (poly, a, b) => {
    const side = p => (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0]);
    const out = [];
    for (let i=0; i<poly.length; i++) {
      const p=poly[i], q=poly[(i+1)%poly.length], dp=side(p), dq=side(q);
      if (dp >= -1e-9) out.push(p);
      if ((dp >= 0) !== (dq >= 0)) {
        const t=dp/(dp-dq); out.push([p[0]+t*(q[0]-p[0]),p[1]+t*(q[1]-p[1])]);
      }
    }
    return out;
  };
  const xs=quad.map(p=>p[0]), zs=quad.map(p=>p[1]);
  const c0=Math.max(0,Math.floor((Math.min(...xs)-B.x0)/SCHRITT));
  const c1=Math.min(cols-2,Math.floor((Math.max(...xs)-B.x0)/SCHRITT));
  const r0=Math.max(0,Math.floor((Math.min(...zs)-B.z0)/SCHRITT));
  const r1=Math.min(rows-2,Math.floor((Math.max(...zs)-B.z0)/SCHRITT));
  for(let r=r0;r<=r1;r++) for(let c=c0;c<=c1;c++) {
    const x=B.x0+c*SCHRITT,z=B.z0+r*SCHRITT,d=SCHRITT;
    for(const tri of [[[x,z],[x+d,z],[x+d,z+d]],[[x,z],[x+d,z+d],[x,z+d]]]) {
      let poly=quad;
      for(let e=0;e<3 && poly.length;e++) poly=clip(poly,tri[e],tri[(e+1)%3]);
      if(poly.length<3) continue;
      const ids=poly.map(([px,pz])=>punkt(s,px,Math.max(rasterHoehe(px,pz,true),Terrain.surfaceAt(px,pz))+Math.max(hoch,0.035),pz));
      for(let k=1;k+1<ids.length;k++) dreieck(s,ids[0],ids[k],ids[k+1]);
    }
  }
}

// Dieselbe Zellenteilung fuer die ganze Stadt, nicht nur fuer den
// Klinikums-Versuch: ueberall sonst schnitten Gelaendedreiecke in die
// Fahrbahn, und es sah aus, als waechse Gras mitten aus dem Asphalt.
const GENAU_UEBERALL = true;

// Ein Streifen neben der Mittellinie: von/bis sind seitliche Abstaende in
// Metern (mit Vorzeichen), also z.B. 3,5 bis 5,1 fuer den rechten Gehweg.
// Dazu die senkrechte Bordkante an der Strassenseite - ohne sie schwebt der
// Gehweg als Platte ueber dem Boden.
function seitenband(s, pts, von, bis, hoch) {
  const n = pts.length / 2;
  let stueck = 0;
  for (let i = 0; i < n - 1; i++) {
    const x0 = pts[i * 2], z0 = pts[i * 2 + 1], x1 = pts[i * 2 + 2], z1 = pts[i * 2 + 3];
    const dx = x1 - x0, dz = z1 - z0, len = Math.hypot(dx, dz);
    if (len < 0.2) continue;
    const ux = -dz / len, uz = dx / len;
    const teile = Math.max(1, Math.ceil(len / 8));
    for (let k = 0; k < teile; k++) {
      const t0 = k / teile, t1 = (k + 1) / teile;
      const ax = x0 + dx * t0, az = z0 + dz * t0, bx = x0 + dx * t1, bz = z0 + dz * t1;
      const ai = [ax + ux * von, az + uz * von], bi = [bx + ux * von, bz + uz * von];
      const aa = [ax + ux * bis, az + uz * bis], ba = [bx + ux * bis, bz + uz * bis];
      const o = p => [p[0], auflage(p[0], p[1]) + hoch, p[1]];
      const u = p => [p[0], auflage(p[0], p[1]), p[1]];
      flaeche(s, o(ai), o(bi), o(ba), o(aa));
      // Bordkante zur Fahrbahn hin.
      flaeche(s, u(ai), o(ai), o(bi), u(bi));
      stueck++;
    }
  }
  return stueck;
}

function band(s, pts, breite, hoch) {
  const n = pts.length / 2;
  let stueck = 0;
  for (let i = 0; i < n - 1; i++) {
    const x0 = pts[i * 2], z0 = pts[i * 2 + 1], x1 = pts[i * 2 + 2], z1 = pts[i * 2 + 3];
    const dx = x1 - x0, dz = z1 - z0, len = Math.hypot(dx, dz);
    if (len < 0.2) continue;
    const nx = -dz / len * breite / 2, nz = dx / len * breite / 2;
    // Laengere Segmente unterteilen, sonst schneidet das Band den Hang.
    const teile = Math.max(1, Math.ceil(len / 8));
    const h = (x, z) => auflage(x, z) + hoch;
    for (let k = 0; k < teile; k++) {
      const t0 = k / teile, t1 = (k + 1) / teile;
      const ax = x0 + dx * t0, az = z0 + dz * t0, bx = x0 + dx * t1, bz = z0 + dz * t1;
      // Andersherum als bei den Gelaendezellen: fuer ein Band laengs der
      // Fahrtrichtung ergibt die andere Reihenfolge die Oberseite. Vorher
      // zeigten alle Fahrbahnen nach unten und waren weggeschnitten.
      if (GENAU_UEBERALL || imKlinikum(ax-nx,az-nz) || imKlinikum(bx-nx,bz-nz) ||
          imKlinikum(bx+nx,bz+nz) || imKlinikum(ax+nx,az+nz)) {
        klinikumFlaeche(s, [[ax-nx,az-nz],[bx-nx,bz-nz],[bx+nx,bz+nz],[ax+nx,az+nz]], hoch);
      } else flaeche(s,
        [ax - nx, h(ax - nx, az - nz), az - nz], [bx - nx, h(bx - nx, bz - nz), bz - nz],
        [bx + nx, h(bx + nx, bz + nz), bz + nz], [ax + nx, h(ax + nx, az + nz), az + nz]);
      stueck++;
    }
  }
  return stueck;
}

const STRASSE = [0x3c3c3f, 0x44443f, 0x4a4438, 0x585048];
// Ein Gehweg gehoert an die Strasse zwischen Haeusern, nicht an den Feldweg
// zwischen zwei Aeckern. Raster aus 25-m-Zellen um jeden Gebaeudemittel-
// punkt; innerorts ist, wo in der eigenen oder einer Nachbarzelle ein Haus
// steht - also im Umkreis von rund 35 m.
const BEBAUT = new Set();
const bauZelle = (x, z) => Math.floor(x / 25) + ':' + Math.floor(z / 25);
for (const bd of city.buildings) if (bd && bd.o) BEBAUT.add(bauZelle(bd.o[0], bd.o[1]));
function innerorts(x, z) {
  for (let dx = -1; dx <= 1; dx++) for (let dz = -1; dz <= 1; dz++)
    if (BEBAUT.has(bauZelle(x + dx * 25, z + dz * 25))) return true;
  return false;
}
// Bordsteinhoehe und Gehwegbreite in Metern, wie an einer gewoehnlichen
// Stadtstrasse.
const BORD = 0.14, GEHWEG_BREIT = 1.8;
let strassenTeile = 0;
let gehwegTeile = 0;
for (const r of city.roads) {
  const breite = Math.max(2, r.w || 3);
  // Ein Altstadtweg besteht nicht aus einer Asphaltflaeche mitten in der
  // Wiese. Der breite Pflasterstreifen macht Bordkante und Gehweg lesbar und
  // verdeckt zugleich das Terrain direkt unter dem Strassenband.
  const mx = r.p[0], mz = r.p[1];
  if (inAltstadt([mx, mz])) {
    // Nur vier Zentimeter ueber dem Scan: Pflaster hat eine Bordkante, wirkt
    // aber nicht wie eine aufgeklebte Platte.
    gehwegTeile += band(ziel('Sidewalk', farbe(0x77736B)), r.p, breite + 1.45, 0.04);
  } else if (innerorts(mx, mz)) {
    // Ausserhalb der Altstadt lag die Fahrbahn bisher blank in der Wiese.
    // Jetzt beidseits ein Gehweg mit Bordstein - Platz fuer die Passanten
    // und eine erkennbare Strassenkante statt eines ausgefransten Rands.
    // Eigene Klasse "Gehweg": in der Altstadt bleibt es Pflaster, hier
    // kommen Betonplatten drauf (siehe LaLaBergImportCommandlet, M_Gehweg).
    const s = ziel('Gehweg', farbe(0xA9A59C));
    const innen = breite / 2;
    gehwegTeile += seitenband(s, r.p, innen, innen + GEHWEG_BREIT, BORD);
    gehwegTeile += seitenband(s, r.p, -innen - GEHWEG_BREIT, -innen, BORD);
  }
  // Asphalt liegt fast buendig auf dem Terrain; der Gehweg bleibt sichtbar
  // hoeher und bildet damit die reale Bordsteinkante.
  strassenTeile += band(ziel('Road', farbe(STRASSE[Math.min(r.c || 0, 3)])), r.p, breite, 0.012);
}
let gleisTeile = 0;
for (const r of city.rails) gleisTeile += band(ziel('Rail', farbe(0x4a4038)), r.p, r.w || 3.2, 0.20);

// ------------------------------------------------------------------ Plaetze
// Der Hauptplatz ist gepflastert, nicht begruent. Im Gelaenderaster steht an
// dieser Stelle aber Wiese, weil die Flaechennutzung dort nichts hergibt.
// Die Platzumringe liegen als eigene Daten vor und werden hier als Pflaster
// ueber das Gelaende gelegt.
let platzTeile = 0;
for (const pl of city.plazas || []) {
  const p = pl.p;
  if (!p || p.length < 8) continue;
  const flach = [];
  for (let i = 0; i < p.length / 2; i++) flach.push(new Vector2(p[i * 2], p[i * 2 + 1]));
  let tris;
  try { tris = ShapeUtils.triangulateShape(flach, []); } catch (e) { continue; }
  if (!tris.length) continue;
  const s = ziel('Plaza', farbe(0x6E6A64));
  const basis = s.positions.length / 3;
  for (const v of flach) punkt(s, v.x, auflage(v.x, v.y) + 0.16, v.y);
  const dreh = ShapeUtils.area(flach) < 0;
  for (const t of tris) { dreieck(s, basis + t[0], basis + t[dreh ? 2 : 1], basis + t[dreh ? 1 : 2]); platzTeile++; }
}

// ----------------------------------------------------------------- Gelaende
let gelaendeTeile = 0;
for (let r = 0; r < rows - 1; r++) {
  for (let c = 0; c < cols - 1; c++) {
    const x = B.x0 + c * SCHRITT, z = B.z0 + r * SCHRITT;
    // Wasserflaechen bekommen eine eigene Klasse: sie brauchen im Spiel ein
    // anderes Material als Wiese und Acker.
    const mx = x + SCHRITT / 2, mz = z + SCHRITT / 2;
    // Der Lech fuehrt Gletscherwasser und ist gruentuerkis, nicht blaugrau.
    // Die Flaechennutzung kennt nur "Wasser", die Farbe kommt von hier.
    const nass = Terrain.isWater(mx, mz);
    // In der Altstadt ist der Boden gepflastert, nicht begruent. Das
    // Flaechennutzungsraster kennt dort nur "keine Angabe" und lieferte
    // deshalb Wiese - mitten auf dem Hauptplatz.
    const pflaster = !nass && inAltstadt([mx, mz]);
    const s = pflaster
      ? ziel('Plaza', farbe(0x6E6A64))
      : ziel(nass ? 'Water' : 'Ground', farbe(nass ? 0x4E7F72 : Terrain.groundColorAt(mx, mz)));
    flaeche(s,
      [x, hoehen[r * cols + c], z], [x + SCHRITT, hoehen[r * cols + c + 1], z],
      [x + SCHRITT, hoehen[(r + 1) * cols + c + 1], z + SCHRITT], [x, hoehen[(r + 1) * cols + c], z + SCHRITT]);
    gelaendeTeile++;
  }
}

// -------------------------------------------------------------------- Auto
// Die Form steht in wagen-form.cjs - dieselbe fuer die geparkten Wagen hier
// und den fahrbaren Wagen im Spiel. Hier wird sie nur an ihren Platz gestellt.
const WagenForm = require('./wagen-form.cjs');
let autos = 0;
function fahrzeug(x, z, richtung, lack) {
  const ca = Math.cos(richtung), sa = Math.sin(richtung);
  const y = auflage(x, z);
  const W = p => [x + ca * p[0] - sa * p[1], y + p[2], z + sa * p[0] + ca * p[1]];
  WagenForm.wagen({
    viereck: (k, rgb, A, B, C, D) => flaeche(ziel(k, farbe(rgb)), W(A), W(B), W(C), W(D)),
    dreieck: (k, rgb, a, b, c) => {
      const s = ziel(k, farbe(rgb));
      const i = [a, b, c].map(q => { const w = W(q); return punkt(s, w[0], w[1], w[2]); });
      dreieck(s, i[0], i[1], i[2]);
    },
  }, lack);
  autos++;
}

// Landsberger Strassenbild: viel Silber und Grau, dazwischen ein paar Farben.
const LACKE = [0xB9BCC0, 0x8E9296, 0x2E3236, 0xE8E9EA, 0x6E7276, 0x1F3A5C,
               0x7A2A24, 0x2C4A32, 0xC8C2B4, 0x4A5058];

// --------------------------------------------------------------- Passanten
// Eine Figur aus fuenf Koerpern: zwei Beine, Rumpf, Kopf, dazu die Arme am
// Rumpf angedeutet. Aus zwanzig Metern Entfernung genuegt das - was fehlt,
// wenn niemand da ist, ist nicht die Anatomie, sondern der Massstab.
const HOSEN = [0x2B3038, 0x3A3C42, 0x24303E, 0x4A4034, 0x2E2E30];
const JACKEN = [0x8C3B32, 0x2F4A5E, 0x3E5B42, 0xB4A78C, 0x36393E, 0x6E4B7A, 0xC9C3B8];
// Ein Koerper mit ovalem Querschnitt, quer zur Blickrichtung breiter als
// tief. Aus Walzen wird sonst ein Poller, kein Mensch.
function ovalWalze(s, cx, cz, y0, y1, q0, t0, q1, t1, ca, sa, seiten) {
  const A = [], B = [];
  for (let i = 0; i < seiten; i++) {
    const a = i / seiten * Math.PI * 2;
    const co = Math.cos(a), si = Math.sin(a);
    const uq = -sa, uqz = ca;                  // quer zur Blickrichtung
    A.push(punkt(s, cx + uq * co * q0 + ca * si * t0, y0, cz + uqz * co * q0 + sa * si * t0));
    B.push(punkt(s, cx + uq * co * q1 + ca * si * t1, y1, cz + uqz * co * q1 + sa * si * t1));
  }
  for (let i = 0; i < seiten; i++) {
    const j = (i + 1) % seiten;
    dreieck(s, A[i], A[j], B[j]);
    dreieck(s, A[i], B[j], B[i]);
  }
  for (let i = 1; i + 1 < seiten; i++) {
    dreieck(s, A[0], A[i + 1], A[i]);
    dreieck(s, B[0], B[i], B[i + 1]);
  }
}

let passanten = 0;
function passant(x, z, richtung) {
  const y = auflage(x, z);
  const t = streuung(x * 3.1, z * 3.1);
  const gross = 1.62 + t * 0.22;                 // 1,62 m bis 1,84 m
  const ca = Math.cos(richtung), sa = Math.sin(richtung);
  const hose = ziel('Stoff', farbe(HOSEN[Math.floor(t * 331) % HOSEN.length]));
  const jacke = ziel('Stoff', farbe(JACKEN[Math.floor(t * 613) % JACKEN.length]));
  const haut = ziel('Haut', farbe(0xC9A184));
  const seit = (v, t) => [x - sa * v + ca * t, z + ca * v + sa * t];
  const beinL = gross * 0.46;
  for (const v of [-0.105, 0.105]) {
    const [bx, bz] = seit(v, 0);
    ovalWalze(hose, bx, bz, y + 0.02, y + beinL, 0.075, 0.085, 0.085, 0.095, ca, sa, 5);
  }
  // Rumpf: an der Schulter breit, an der Taille schmaler
  ovalWalze(jacke, x, z, y + beinL - 0.03, y + gross * 0.82, 0.155, 0.115, 0.205, 0.125, ca, sa, 7);
  // Arme haengen seitlich am Rumpf
  for (const v of [-0.225, 0.225]) {
    const [ax, az] = seit(v, 0);
    ovalWalze(jacke, ax, az, y + gross * 0.52, y + gross * 0.80, 0.052, 0.058, 0.062, 0.068, ca, sa, 5);
  }
  ovalWalze(haut, x, z, y + gross * 0.82, y + gross * 0.865, 0.062, 0.062, 0.055, 0.058, ca, sa, 5);
  ovalWalze(haut, x, z, y + gross * 0.865, y + gross, 0.095, 0.098, 0.082, 0.088, ca, sa, 6);
  passanten++;
}

// ------------------------------------------------------------ Marienbrunnen
// Mitte des Hauptplatzes: Brunnen von 1783 mit der Maria Immaculata, der
// Schutzheiligen Bayerns. Achteckiges Becken, Postament, Saeule, Figur.
function marienbrunnen(bx, bz) {
  const y = auflage(bx, bz);
  const stein = ziel('Stone', farbe(0xCBC5B6));
  walze(stein, bx, bz, y, y + 0.86, 2.60, 2.60, 8);      // Beckenaussenwand
  walze(stein, bx, bz, y + 0.86, y + 0.72, 2.60, 2.28, 8); // Beckenrand
  walze(stein, bx, bz, y + 0.72, y + 0.10, 2.28, 2.28, 8); // Beckeninnenwand
  walze(stein, bx, bz, y + 0.86, y + 1.62, 0.62, 0.52, 4); // Postament
  walze(stein, bx, bz, y + 1.62, y + 4.30, 0.30, 0.24, 8); // Saeule
  deckel(stein, bx, bz, y + 4.30, 0.42, 8);                // Kapitell
  const figur = ziel('Figure', farbe(0xE3DDD0));
  walze(figur, bx, bz, y + 4.34, y + 5.10, 0.34, 0.26, 6); // Gewand
  walze(figur, bx, bz, y + 5.10, y + 5.55, 0.22, 0.12, 6); // Kopf und Krone
  const wasser = ziel('Water', farbe(0x4E7F72));
  deckel(wasser, bx, bz, y + 0.58, 2.26, 8);
}
marienbrunnen(605.3, -105.2);

// Geparkte Wagen stehen nicht mehr als Kasten-Geometrie hier im Stadt-Mesh -
// dieselben amtlichen Stellplaetze liefert jetzt prepare-verkehr.cjs als
// Positionsliste fuer echte, einsteigbare KI-Auto-Akteure (CarConcept-Modell
// statt Kasten, siehe LaLaBergGameMode::LadeVerkehr). fahrzeug()/LACKE
// bleiben stehen, falls der fahrbare Wagen sie einmal braucht.

// Passanten entlang der Strassen in der Altstadt: am Gehsteigrand, in
// Fahrtrichtung oder dagegen, in unregelmaessigen Abstaenden.
for (const r of city.roads) {
  const pts = r.p, n = pts.length / 2;
  const breite = Math.max(2, r.w || 3);
  for (let i = 0; i < n - 1; i++) {
    const x0 = pts[i * 2], z0 = pts[i * 2 + 1], x1 = pts[i * 2 + 2], z1 = pts[i * 2 + 3];
    if (!inAltstadt([(x0 + x1) / 2, (z0 + z1) / 2])) continue;
    const dx = x1 - x0, dz = z1 - z0, len = Math.hypot(dx, dz);
    if (len < 6) continue;
    const schritte = Math.floor(len / 22);
    for (let k = 0; k < schritte; k++) {
      const f = (k + 0.5) / schritte;
      const px = x0 + dx * f, pz = z0 + dz * f;
      const t = streuung(px * 5.7, pz * 5.7);
      if (t < 0.45) continue;                    // nicht ueberall steht jemand
      const nx = -dz / len, nz = dx / len;
      const seite = t > 0.72 ? 1 : -1;
      const ab = breite / 2 + 0.9 + t * 0.6;
      const gx = px + nx * ab * seite, gz = pz + nz * ab * seite;
      if (Terrain.isWater(gx, gz)) continue;
      passant(gx, gz, Math.atan2(dz, dx) + (t > 0.6 ? Math.PI : 0) + (t - 0.5) * 0.8);
    }
  }
}

// ------------------------------------------------------------------- Baeume
// Die Baumstandorte liegen als flache Liste x, z, Groesse vor. Jeder Baum
// bekommt einen fuenfseitigen Stamm und eine achtseitige Krone aus zwei
// Kegeln - 26 Dreiecke, mit gemeinsamen Stuetzpunkten, sonst wird die Datei
// zu gross. Ohne sie wirkt die Stadt kahl.
const STAMM = 0x4A3A2C, LAUB = 0x3F7538, NADEL = 0x2C4A24;
let baeume = 0;
function ring(s, cx, cz, y, r, seiten) {
  const erster = s.positions.length / 3;
  for (let i = 0; i < seiten; i++) {
    const a = i / seiten * Math.PI * 2;
    punkt(s, cx + Math.cos(a) * r, y, cz + Math.sin(a) * r);
  }
  return erster;
}
const baumListe = city.trees || [];
for (let i = 0; i + 2 < baumListe.length; i += 3) {
  const x = baumListe[i], z = baumListe[i + 1], gr = baumListe[i + 2] || 1;
  if (Terrain.isWater(x, z)) continue;
  const y = rasterHoehe(x, z);
  // Feste Streuung aus der Lage: derselbe Baum bleibt derselbe Baum.
  const t = streuung(x, z);
  const nadel = t > 0.62;
  const s = ziel('Tree', farbe(nadel ? NADEL : LAUB));
  const hoehe = (nadel ? 8.2 : 6.8) * gr * (0.85 + t * 0.3);
  const kroneR = (nadel ? 1.35 : 1.85) * gr * (0.85 + (1 - t) * 0.3);
  const stammH = hoehe * 0.34, stammR = 0.16 * gr;

  const st = ziel('Trunk', farbe(STAMM));
  const unten = ring(st, x, z, y - 0.2, stammR * 1.25, 5);
  const oben = ring(st, x, z, y + stammH, stammR, 5);
  for (let k = 0; k < 5; k++) {
    const j = (k + 1) % 5;
    dreieck(st, unten + k, unten + j, oben + j);
    dreieck(st, unten + k, oben + j, oben + k);
  }

  // Mehrere unregelmaessige Kronenringe statt einer einzigen Diamantkrone.
  // Der Baum bleibt performant und bekommt dennoch Volumen, eine sichtbare
  // Unterkrone und je nach Lage eine eigene Silhouette.
  const seiten = 10;
  const y0 = y + stammH * 0.82;
  if (nadel) {
    const r0 = ring(s, x, z, y0, kroneR * 0.92, seiten);
    const r1 = ring(s, x, z, y0 + (hoehe - (y0 - y)) * 0.30, kroneR * 0.72, seiten);
    const r2 = ring(s, x, z, y0 + (hoehe - (y0 - y)) * 0.56, kroneR * 0.46, seiten);
    const top = punkt(s, x, y + hoehe, z);
    for (let k = 0; k < seiten; k++) {
      const j = (k + 1) % seiten;
      dreieck(s, r0 + k, r0 + j, r1 + j); dreieck(s, r0 + k, r1 + j, r1 + k);
      dreieck(s, r1 + k, r1 + j, r2 + j); dreieck(s, r1 + k, r2 + j, r2 + k);
      dreieck(s, r2 + k, r2 + j, top);
    }
  } else {
    const r0 = ring(s, x, z, y0, kroneR * 0.70, seiten);
    const r1 = ring(s, x, z, y0 + (hoehe - stammH) * 0.20, kroneR, seiten);
    const r2 = ring(s, x, z, y0 + (hoehe - stammH) * 0.56, kroneR * 0.82, seiten);
    const r3 = ring(s, x, z, y0 + (hoehe - stammH) * 0.82, kroneR * 0.45, seiten);
    const top = punkt(s, x, y + hoehe, z);
    for (let k = 0; k < seiten; k++) {
      const j = (k + 1) % seiten;
      dreieck(s, r0 + k, r0 + j, r1 + j); dreieck(s, r0 + k, r1 + j, r1 + k);
      dreieck(s, r1 + k, r1 + j, r2 + j); dreieck(s, r1 + k, r2 + j, r2 + k);
      dreieck(s, r2 + k, r2 + j, r3 + j); dreieck(s, r2 + k, r3 + j, r3 + k);
      dreieck(s, r3 + k, r3 + j, top);
    }
  }
  baeume++;
}

// ------------------------------------------------------------------ Ausgabe
const liste = [...sektionen.values()].filter(s => s.indices.length);
for (const s of liste) {
  if (s.uvs.length !== s.positions.length / 3 * 2) delete s.uvs;   // sonst deutet C++ sie falsch
}
for (const s of liste) {
  if (!s.positions.every(Number.isFinite)) throw new Error('Ungueltige Position in ' + s.name);
  const max = s.positions.length / 3;
  if (s.indices.some(i => !Number.isInteger(i) || i < 0 || i >= max)) throw new Error('Ungueltiger Index in ' + s.name);
}
const dreiecke = liste.reduce((a, s) => a + s.indices.length / 3, 0);
const ergebnis = {
  schema: 1, units: 'centimeters', originGameXZ: ORIGIN, terrainBaseMeters: box.TERRAIN.min,
  buildingCount: city.buildings.length, faceCount: wandFlaechen + dachFlaechen,
  buildingTriangles: dreiecke, spawnHeightCm: Math.round(boden(ORIGIN[0], ORIGIN[1]) * M),
  // Startpunkt: vor dem Klinikum, wie im Bestandsspiel. Die Hoehe kommt aus
  // dem Gelaende, die Figur setzt sich beim Start selbst darauf ab.
  spawn: (() => {
    const ort = (city.pois || []).find(p => /^Klinikum$/i.test(p.n || '')) || { n: 'Hauptplatz', x: ORIGIN[0], z: ORIGIN[1] };
    return { name: ort.n, x: Math.round((ort.x - ORIGIN[0]) * M), y: Math.round((ort.z - ORIGIN[1]) * M),
             z: Math.round(boden(ort.x, ort.z) * M), blick: 0 };
  })(),
  sources: QUELLEN.map(file => ({ file, sha256: crypto.createHash('sha256').update(read(file)).digest('hex') })),
  sections: liste,
};
const out = process.env.LALABERG_AUSGABE ? path.resolve(process.env.LALABERG_AUSGABE) : path.resolve(REPO, 'Content/SourceData/stadt.json');
fs.mkdirSync(path.dirname(out), { recursive: true });
fs.writeFileSync(out, JSON.stringify(ergebnis));
console.log(JSON.stringify({
  ausgabe: out, mb: +(fs.statSync(out).size / 1048576).toFixed(1), sektionen: liste.length,
  haeuser: city.buildings.length, wandFlaechen, dachFlaechen, baenderFlaechen, kamine, brandmauern, tueren, gauben: gaubenZahl, umrissDaecher, viereckDaecher, uebersprungen, umgefaerbt, gefasst,
  strassenTeile, gehwegTeile, gleisTeile, platzTeile, autos, passanten, entartet, gelaendeTeile, baeume, dreiecke, spawnHeightCm: ergebnis.spawnHeightCm,
}, null, 1));
