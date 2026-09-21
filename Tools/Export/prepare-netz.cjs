// Strassengraph fuer die Polizei (siehe LaLaBergPolizei): Knoten mit Hoehe,
// gerichtete Kanten nach der Einbahnregel, beschraenkt auf die groesste stark
// zusammenhaengende Komponente - dieselbe, auf der auch die KI-Rundkurse
// liegen (siehe prepare-verkehr.cjs). Nur dort kommt ein Streifenwagen von
// jedem Knoten zu jedem anderen, ohne gegen eine Einbahnstrasse zu fahren.
//
// Eigene Datei statt eines Felds in verkehr.json: prepare-verkehr.cjs wuerfelt
// die Rundkurse bei jedem Lauf neu, der Graph soll sich ohne das erneuern
// lassen.
//
//   node prepare-netz.cjs
//
// Schreibt Content/SourceData/Verkehr/netz.json:
//   p: [x, y, z, ...] Knoten in Unreal-Zentimetern (nur die benutzten)
//   e: [von, nach, klasse, ...] gerichtete Kanten (Indizes in p/3)
'use strict';
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const root = process.env.LALABERG_QUELLE
  ? path.resolve(process.env.LALABERG_QUELLE)
  : path.resolve(__dirname, '../../../GTA LaLaBerg');
const REPO = path.resolve(__dirname, '../..');
const read = rel => fs.readFileSync(path.join(root, rel), 'utf8');

const box = { window: null, Math, Float32Array, Uint8Array, Buffer, Map, console };
box.window = box;
vm.createContext(box);
for (const datei of ['data/citydata.js', 'data/terrain.js', 'data/phase9data.js', 'data/phase11data.js', 'js/terrain.js'])
  vm.runInContext(read(datei), box, { timeout: 30000 });
const city = box.CITY, Terrain = box.Terrain;
Terrain.init(box.TERRAIN, city);

const ORIGIN = [605, -100], M = 100;
const ux = x => Math.round((x - ORIGIN[0]) * M);
const uz = z => Math.round((z - ORIGIN[1]) * M);
const boden = (x, z) => Terrain.surfaceAt(x, z) + 0.16;

const graph = city.graph;
const anzahl = graph.p.length / 2;
const vor = Array.from({ length: anzahl }, () => []);
const rueck = Array.from({ length: anzahl }, () => []);
const kanten = [];
for (const [a, b, klasse, einbahn] of graph.e) {
  const weite = Math.hypot(graph.p[2 * b] - graph.p[2 * a], graph.p[2 * b + 1] - graph.p[2 * a + 1]);
  if (weite < 0.5) continue;
  if (einbahn >= 0) { vor[a].push(b); rueck[b].push(a); kanten.push([a, b, klasse]); }
  if (einbahn <= 0) { vor[b].push(a); rueck[a].push(b); kanten.push([b, a, klasse]); }
}
// Kosaraju wie in prepare-verkehr.cjs
const gesehen = new Uint8Array(anzahl), ordnung = [];
for (let s = 0; s < anzahl; s++) {
  if (gesehen[s]) continue;
  const stapel = [[s, 0]]; gesehen[s] = 1;
  while (stapel.length) {
    const oben = stapel[stapel.length - 1];
    if (oben[1] < vor[oben[0]].length) {
      const w = vor[oben[0]][oben[1]++];
      if (!gesehen[w]) { gesehen[w] = 1; stapel.push([w, 0]); }
    } else { ordnung.push(oben[0]); stapel.pop(); }
  }
}
const komponente = new Int32Array(anzahl).fill(-1);
const groesse = [];
for (let i = ordnung.length - 1; i >= 0; i--) {
  const s = ordnung[i];
  if (komponente[s] >= 0) continue;
  const id = groesse.length; let anz = 0; const stapel = [s]; komponente[s] = id;
  while (stapel.length) {
    const v = stapel.pop(); anz++;
    for (const w of rueck[v]) if (komponente[w] < 0) { komponente[w] = id; stapel.push(w); }
  }
  groesse.push(anz);
}
let beste = 0;
for (let i = 1; i < groesse.length; i++) if (groesse[i] > groesse[beste]) beste = i;

const neu = new Int32Array(anzahl).fill(-1);
const p = [], e = [];
const index = n => {
  if (neu[n] < 0) {
    neu[n] = p.length / 3;
    const x = graph.p[2 * n], z = graph.p[2 * n + 1];
    p.push(ux(x), uz(z), Math.round(boden(x, z) * M));
  }
  return neu[n];
};
for (const [a, b, klasse] of kanten) {
  if (komponente[a] !== beste || komponente[b] !== beste) continue;
  e.push(index(a), index(b), klasse);
}
const out = path.resolve(REPO, 'Content/SourceData/Verkehr/netz.json');
fs.writeFileSync(out, JSON.stringify({ schema: 1, p, e }));
console.log(JSON.stringify({ knoten: p.length / 3, kanten: e.length / 3, bytes: fs.statSync(out).size }));
