// Wegpunkte fuer den KI-Verkehr: Autos fahren auf den laengsten Strassen der
// Stadt hin und her, Passanten gehen auf beidseitigen Gehwegen der Altstadt.
// Beide sind vorher gebaute Geometrie im Stadt-Mesh (siehe fahrzeug()/
// passant() in prepare-stadt.cjs) - hier entstehen zusaetzliche, eigen-
// staendige Figuren, die tatsaechlich fahren/gehen und auf einen Treffer
// reagieren koennen (siehe LaLaBergVerkehrsauto, LaLaBergPassantKI).
//
//   node prepare-verkehr.cjs
//
// Schreibt Content/SourceData/Verkehr/verkehr.json. LALABERG_QUELLE zeigt
// auf die Wurzel des Spielprojekts "GTA LaLaBerg" (siehe prepare-stadt.cjs).
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
const boden = (x, z) => Terrain.surfaceAt(x, z) + 0.16;   // wie eine Strasse in prepare-stadt.cjs
const ALTSTADT = { x0: 470, x1: 1240, z0: -520, z1: 520 };
const inAltstadt = (x, z) => x > ALTSTADT.x0 && x < ALTSTADT.x1 && z > ALTSTADT.z0 && z < ALTSTADT.z1;

// Eine Polylinie auf einen Punktabstand von rund "schritt" Metern resampeln,
// damit Autos und Passanten in gleichmaessigen Schritten Wegpunkte abfahren
// statt an amtlichen Vermessungspunkten, die mal 2 m, mal 40 m auseinander liegen.
function resample(p, schritt) {
  const punkte = [];
  for (let i = 0; i < p.length; i += 2) punkte.push([p[i], p[i + 1]]);
  const aus = [punkte[0]];
  let rest = schritt;
  for (let i = 0; i + 1 < punkte.length; i++) {
    let [ax, az] = punkte[i]; const [bx, bz] = punkte[i + 1];
    let seglen = Math.hypot(bx - ax, bz - az);
    while (seglen >= rest) {
      const t = rest / seglen;
      const px = ax + (bx - ax) * t, pz = az + (bz - az) * t;
      aus.push([px, pz]);
      ax = px; az = pz; seglen -= rest; rest = schritt;
    }
    rest -= seglen;
  }
  const letzte = punkte[punkte.length - 1];
  const l = aus[aus.length - 1];
  if (Math.hypot(letzte[0] - l[0], letzte[1] - l[1]) > 1.0) aus.push(letzte);
  return aus;
}

function laenge(p) {
  let l = 0;
  for (let i = 0; i + 3 < p.length; i += 2) l += Math.hypot(p[i + 2] - p[i], p[i + 3] - p[i + 1]);
  return l;
}

// Autos: die laengsten Strassen der Stadt, damit eine Fahrt auch eine Fahrt
// ist und nicht nach drei Sekunden am Ende der Sackgasse endet.
const KANDIDATEN = city.roads.filter(r => r.p && r.p.length >= 6 && laenge(r.p) > 45 && !Terrain.isWater(r.p[0], r.p[1]));
KANDIDATEN.sort((a, b) => laenge(b.p) - laenge(a.p));
const AUTO_ROUTEN = 70;
const autos = [];
for (const r of KANDIDATEN.slice(0, AUTO_ROUTEN)) {
  const punkte = resample(r.p, 9);
  if (punkte.length < 3) continue;
  const weg = punkte.map(([x, z]) => [ux(x), uz(z), Math.round(boden(x, z) * M)]);
  autos.push({ w: Math.max(3.0, r.w || 3.0), p: weg.flat() });
}

// Passanten: beidseitig entlang derselben Strassen, aber nur in der Alt-
// stadt - dort ist die Dichte gewuenscht, nicht auf der Umgehungsstrasse.
const PASSANT_ROUTEN = 90;
const passanten = [];
for (const r of KANDIDATEN) {
  if (passanten.length >= PASSANT_ROUTEN) break;
  const mitte = r.p;
  if (!inAltstadt(mitte[0], mitte[1])) continue;
  const punkte = resample(r.p, 6);
  if (punkte.length < 3) continue;
  for (const seite of [1, -1]) {
    if (passanten.length >= PASSANT_ROUTEN) break;
    const versatz = seite * ((r.w || 3.0) / 2 + 1.6);
    const weg = [];
    for (let i = 0; i < punkte.length; i++) {
      const [x, z] = punkte[i];
      const [nx, nz] = punkte[Math.min(i + 1, punkte.length - 1)];
      const [px, pz] = punkte[Math.max(i - 1, 0)];
      const dx = nx - px, dz = nz - pz, len = Math.hypot(dx, dz) || 1;
      const ox = x + (dz / len) * versatz, oz = z + (-dx / len) * versatz;
      weg.push(ux(ox), uz(oz), Math.round(boden(ox, oz) * M));
    }
    passanten.push({ p: weg });
  }
}

const ergebnis = { schema: 1, autos, passanten };
const out = path.resolve(REPO, 'Content/SourceData/Verkehr/verkehr.json');
fs.mkdirSync(path.dirname(out), { recursive: true });
fs.writeFileSync(out, JSON.stringify(ergebnis));
console.log(JSON.stringify({ autos: autos.length, passanten: passanten.length, bytes: fs.statSync(out).size }));
