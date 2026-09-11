// Vorlage fuer den fahrbaren Wagen: dieselbe Form wie die geparkten Wagen
// (wagen-form.cjs), in Unreal-Zentimetern um den Wagenmittelpunkt am Boden.
// x in Fahrtrichtung, y nach rechts, z nach oben.
//
//   node tools/unreal/prepare-wagen.cjs
//
// Schreibt ../GTALaLaBerg/Content/SourceData/Fahrzeug/wagen.json. Teile mit
// "lack": true faerbt das Spiel in der Farbe des jeweiligen Wagens.
'use strict';
const fs = require('fs');
const path = require('path');
const WagenForm = require('./wagen-form.cjs');

const LACK = 0x010203;                          // Platzhalter, wird im Spiel ersetzt
const teile = new Map();
function teil(klasse, rgb) {
  const name = klasse + '_' + rgb.toString(16);
  if (!teile.has(name)) teile.set(name, { klasse, lack: rgb === LACK,
    color: [(rgb >> 16 & 255) / 255, (rgb >> 8 & 255) / 255, (rgb & 255) / 255], positions: [], indices: [] });
  return teile.get(name);
}
function punkt(t, p) {
  t.positions.push(Math.round(p[0] * 1000) / 10, Math.round(p[1] * 1000) / 10, Math.round(p[2] * 1000) / 10);
  return t.positions.length / 3 - 1;
}
// Wie in prepare-stadt.cjs: Unreal zeichnet die Vorderseite andersherum.
let entartet = 0;
function dreieck(t, a, b, c) {
  const p = t.positions, ia = a * 3, ib = b * 3, ic = c * 3;
  const ux = p[ib] - p[ia], uy = p[ib + 1] - p[ia + 1], uz = p[ib + 2] - p[ia + 2];
  const vx = p[ic] - p[ia], vy = p[ic + 1] - p[ia + 1], vz = p[ic + 2] - p[ia + 2];
  const nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
  if (nx * nx + ny * ny + nz * nz < 1e-4) { entartet++; return; }
  t.indices.push(a, c, b);
}

WagenForm.wagen({
  viereck: (k, rgb, A, B, C, D) => {
    const t = teil(k, rgb);
    const i = [A, B, C, D].map(q => punkt(t, q));
    dreieck(t, i[0], i[1], i[2]); dreieck(t, i[0], i[2], i[3]);
  },
  dreieck: (k, rgb, a, b, c) => {
    const t = teil(k, rgb);
    const i = [a, b, c].map(q => punkt(t, q));
    dreieck(t, i[0], i[1], i[2]);
  },
}, LACK);

const liste = [...teile.values()];
const out = path.resolve(__dirname, '../../Content/SourceData/Fahrzeug/wagen.json');
fs.mkdirSync(path.dirname(out), { recursive: true });
fs.writeFileSync(out, JSON.stringify({ schema: 1, einheit: 'cm', radRadius: WagenForm.RAD_R * 100,
  spur: WagenForm.SPUR * 100, achsen: [WagenForm.ACHSE_H * 100, WagenForm.ACHSE_V * 100], teile: liste }));
console.log(JSON.stringify({ teile: liste.map(t => t.klasse + (t.lack ? '(Lack)' : '') + ':' + t.indices.length / 3),
  dreiecke: liste.reduce((n, t) => n + t.indices.length / 3, 0), entartet, bytes: fs.statSync(out).size }));
