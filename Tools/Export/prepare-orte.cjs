// Ortsnamen fuer die Einblendung im Spiel: benannte Strassen als Linienzug,
// benannte Plaetze als Umriss, Wahrzeichen als Punkt mit Umkreis und die
// Ortsteile. Alles in Unreal-Zentimetern, Nullpunkt Hauptplatz - dieselbe
// Umrechnung wie in prepare-stadt.cjs.
//
//   node tools/unreal/prepare-orte.cjs
//
// Schreibt ../GTALaLaBerg/Content/SourceData/Orte/orte.json. Getrennt von der
// Stadtausleitung, damit ein neuer Name nicht 80 MB Geometrie neu erzeugt.
'use strict';
const fs = require('fs');
const path = require('path');
const vm = require('vm');

// Rohdaten (data/citydata.js usw.) liegen im Spielprojekt "GTA LaLaBerg",
// nicht in diesem Repo. LALABERG_QUELLE zeigt auf dessen Wurzel; ohne
// Angabe wird der Nachbarordner neben diesem Repo angenommen.
const root = process.env.LALABERG_QUELLE
  ? path.resolve(process.env.LALABERG_QUELLE)
  : path.resolve(__dirname, '../../../GTA LaLaBerg');
const REPO = path.resolve(__dirname, '../..');
const box = { Math, Float32Array, Uint8Array, Buffer, Map, console };
box.window = box;
vm.createContext(box);
for (const datei of ['data/citydata.js', 'data/phase9data.js', 'data/phase11data.js'])
  vm.runInContext(fs.readFileSync(path.join(root, datei), 'utf8'), box, { timeout: 30000 });
const city = box.CITY;

const ORIGIN = [605, -100];
const M = 100;
const ux = x => Math.round((x - ORIGIN[0]) * M);
const uy = z => Math.round((z - ORIGIN[1]) * M);
const linie = p => { const a = []; for (let i = 0; i + 1 < p.length; i += 2) a.push(ux(p[i]), uy(p[i + 1])); return a; };

// Strassen: alle Stuecke gleichen Namens zusammen, jedes Stueck bleibt ein
// eigener Linienzug - verbunden sind sie in den Daten nicht.
const strassen = new Map();
for (const r of city.roads) {
  if (!r.n || !r.p || r.p.length < 4) continue;
  if (!strassen.has(r.n)) strassen.set(r.n, []);
  strassen.get(r.n).push(linie(r.p));
}

const plaetze = city.plazas.filter(p => p.n && p.p && p.p.length >= 6).map(p => ({ n: p.n, p: linie(p.p) }));

// Wahrzeichen, an denen man sich orientiert. Laeden und Imbisse nicht - die
// wuerden die Strassennamen staendig verdraengen.
const ART = { gate: 35, tower: 25, church: 40, museum: 30, theatre: 30, station: 60, hospital: 140,
              townhall: 40, fire: 40, police: 35 };
const marken = [];
for (const l of [...city.landmarks, ...city.pois]) {
  if (!l.n || !(l.k in ART)) continue;
  if (/^Landsberg am Lech$/.test(l.n)) continue;
  // "Klinikum Landsberg am Lech" ueber "Landsberg am Lech" ist doppelt: die
  // Stadt steht ohnehin in der Zeile darunter.
  const n = l.n.replace(/\s+Landsberg am Lech$/, '').replace(/\s+der Stadt$/, '');
  // Doppelte Eintraege aus beiden Quellen nur einmal
  if (marken.some(m => m.n === n && Math.hypot(m.x - ux(l.x), m.y - uy(l.z)) < 5000)) continue;
  marken.push({ n, x: ux(l.x), y: uy(l.z), r: ART[l.k] * M });
}

// Ortsteile aus den amtlichen Ortspunkten; die Altstadt als Kasten wie in
// prepare-stadt.cjs, weil sie in den Daten keinen eigenen Punkt hat.
const ortsteile = city.pois.filter(p => p.k === 'place' && p.n && !/^Landsberg am Lech$/.test(p.n))
  .map(p => ({ n: p.n, x: ux(p.x), y: uy(p.z), r: 450 * M }));
const ALTSTADT = { x0: 470, x1: 1240, z0: -520, z1: 520 };

const ergebnis = {
  schema: 1,
  stadt: 'Landsberg am Lech',
  altstadt: [ux(ALTSTADT.x0), uy(ALTSTADT.z0), ux(ALTSTADT.x1), uy(ALTSTADT.z1)],
  strassen: [...strassen].map(([n, teile]) => ({ n, teile })),
  plaetze, marken, ortsteile,
};
const out = path.resolve(REPO, 'Content/SourceData/Orte/orte.json');
fs.mkdirSync(path.dirname(out), { recursive: true });
fs.writeFileSync(out, JSON.stringify(ergebnis));
console.log(JSON.stringify({ strassen: ergebnis.strassen.length, plaetze: plaetze.length, marken: marken.length,
  ortsteile: ortsteile.map(o => o.n), bytes: fs.statSync(out).size }));
