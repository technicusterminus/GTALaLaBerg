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
// Echte Kreuzungstopologie aus dem Strassengraphen (city.graph) statt reiner
// Abstandsgruppierung wie bei den Ampeln unten: ein Knoten mit mindestens
// drei angeschlossenen Strassen ist eine wirkliche Kreuzung. Jede Kreuzung
// traegt die niedrigste (= wichtigste) angeschlossene Strassenklasse
// (roads[].c, 0 = Hauptstrasse) - LaLaBergVerkehrsauto::BremseVorKreuzung
// nutzt das fuer echtes Vorfahrtsrecht: ein Auto von der unwichtigeren
// Strasse wartet, wenn ein anderes von der wichtigeren naht.
const graph = city.graph || { p: [], e: [] };
const kreuzungsKlasse = new Map();
{
  const grad = new Map();
  for (const [a, b, klasse] of graph.e) {
    grad.set(a, (grad.get(a) || 0) + 1);
    grad.set(b, (grad.get(b) || 0) + 1);
    for (const n of [a, b]) {
      const bisher = kreuzungsKlasse.get(n);
      if (bisher === undefined || klasse < bisher) kreuzungsKlasse.set(n, klasse);
    }
  }
  for (const [n, g] of grad) if (g < 3) kreuzungsKlasse.delete(n);
}
const kreuzungsPunkte = [...kreuzungsKlasse.keys()].map(n => ({
  x: graph.p[2 * n], z: graph.p[2 * n + 1], klasse: kreuzungsKlasse.get(n),
}));
// Wegpunkttoleranz: die Fahrbahnbreite selbst, nicht der 45-m-Ampelradius -
// eine Kreuzung soll nur zaehlen, wenn die Route wirklich dort vorbeikommt.
const NAHE_KREUZUNG = 8;
function findeKreuzungen(p) {
  const treffer = [];
  for (let i = 0; i < kreuzungsPunkte.length; i++) {
    const k = kreuzungsPunkte[i];
    for (let j = 0; j + 1 < p.length; j += 2) {
      if (Math.hypot(p[j] - k.x, p[j + 1] - k.z) <= NAHE_KREUZUNG) { treffer.push(i); break; }
    }
  }
  return treffer;
}

const autos = [];
for (const r of KANDIDATEN.slice(0, AUTO_ROUTEN)) {
  const punkte = resample(r.p, 9);
  if (punkte.length < 3) continue;
  const weg = punkte.map(([x, z]) => [ux(x), uz(z), Math.round(boden(x, z) * M)]);
  autos.push({ w: Math.max(3.0, r.w || 3.0), klasse: r.c ?? 3, kreuzungen: findeKreuzungen(r.p), p: weg.flat() });
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

// Ampeln: die amtlichen Standorte echter Lichtsignalanlagen (OSM
// highway=traffic_signals). Richtung aus der Strassenachse am naechsten
// Punkt - eine Ampel steht quer zur Fahrbahn, nicht zufaellig gedreht.
// Kleine erste Passantengruppe auf den geraden Hauptstrassenabschnitten
// am Klinikum. Altstadt-Routen bleiben unveraendert erhalten.
const klinikumStrassen = city.roads.filter(r => /hartmann/i.test(r.n || '')
  && r.w >= 5.5 && r.p.length === 4 && r.p[1] >= 0 && r.p[1] < 200);
let klinikumRouten = 0;
for (const r of klinikumStrassen.slice(0, 2)) {
  const punkte = resample(r.p, 3);
  const dx = r.p[2] - r.p[0], dz = r.p[3] - r.p[1];
  const len = Math.hypot(dx, dz);
  if (len < 1) continue;
  for (const seite of [-1, 1]) {
    const versatz = seite * (r.w / 2 + 1.0);
    const rand = punkte.map(([x,z]) => [x + dz / len * versatz, z - dx / len * versatz]);
    if (rand.some(([x,z]) => Terrain.isWater(x,z))) continue;
    passanten.push({gebiet: 'Klinikum-Hartmann', p: rand.flatMap(([x,z]) =>
      [ux(x), uz(z), Math.round(boden(x,z) * M)])});
    klinikumRouten++;
  }
}
console.log(JSON.stringify({ klinikumRouten }));
function naechsteStrassenrichtung(x, z) {
  let beste = Infinity, richtung = [1, 0];
  for (const r of city.roads) {
    const p = r.p;
    for (let i = 0; i + 3 < p.length; i += 2) {
      const ax = p[i], az = p[i + 1], bx = p[i + 2], bz = p[i + 3];
      const dx = bx - ax, dz = bz - az, len2 = dx * dx + dz * dz || 1;
      const t = Math.max(0, Math.min(1, ((x - ax) * dx + (z - az) * dz) / len2));
      const px = ax + dx * t, pz = az + dz * t;
      const d = Math.hypot(x - px, z - pz);
      if (d < beste) { beste = d; const l = Math.hypot(dx, dz) || 1; richtung = [dx / l, dz / l]; }
    }
  }
  return richtung;
}
const ampelnRoh = (city.signals || []).map(s => {
  const [rx, rz] = naechsteStrassenrichtung(s.x, s.z);
  // Quer zur Fahrbahn drehen, damit die Ampel dem Verkehr zugewandt steht.
  const gierGrad = Math.atan2(rz, rx) * 180 / Math.PI + 90;
  return { x: s.x, z: s.z, gier: gierGrad };
});

// Kreuzungsgruppen: bevorzugt die echten Kreuzungsknoten aus dem Strassen-
// graphen (kreuzungsPunkte, siehe oben - dieselben, die auch KI-Autos fuer
// Vorfahrt nutzen) statt reiner Abstandsgruppierung. Eine Ampel direkt an
// ihrer echten Kreuzung erkannt (naechster Knoten mit >=3 Strassen
// innerhalb 60 m) teilt deren Gruppen-ID mit jeder anderen Ampel am selben
// Knoten. Ampeln ohne nahen echten Knoten (z.B. ein Fussgaengerueberweg
// mitten auf einer Strecke, keine echte Kreuzung) fallen auf die alte
// Abstandsgruppierung (45 m, Union-Find) unter sich zurueck, gemischt wird
// nie: eine Ampel mit echter Kreuzung gruppiert sich nur mit einer anderen
// an derselben echten Kreuzung, nicht ueber den Abstand. Innerhalb einer
// Gruppe schalten weiterhin zwei Phasen abwechselnd auf Gruen: Ampeln,
// deren Fahrbahnachse (aus "gier") ungefaehr gleich oder um 180 Grad
// gedreht verlaeuft, gehoeren zur selben Phase - ungefaehr senkrechte
// Achsen zur anderen.
function naechsteKreuzung(x, z) {
  let beste = Infinity, index = -1;
  for (let i = 0; i < kreuzungsPunkte.length; i++) {
    const k = kreuzungsPunkte[i];
    const d = Math.hypot(x - k.x, z - k.z);
    if (d < beste) { beste = d; index = i; }
  }
  return beste <= 60 ? index : -1;
}
// Ankerpunkt je Ampel: der eigene, echte Kreuzungsknoten, falls einer nahe
// genug liegt - sonst die Ampel-Position selbst. Ein grosser Kreuzungsbereich
// kann in den Quelldaten aus mehreren nahe beieinanderliegenden Knoten
// bestehen (z.B. getrennte Fahrbahnrichtungen) - deshalb gruppieren nicht
// per exakter Knoten-Uebereinstimmung, sondern per Abstand zwischen den
// Ankerpunkten, mit engerem Radius fuer zwei echte Knoten (praeziser als
// rohe Ampel-Positionen) und dem alten, weiteren Radius als Ruckfall.
const anker = ampelnRoh.map(a => {
  const idx = naechsteKreuzung(a.x, a.z);
  return idx >= 0 ? { x: kreuzungsPunkte[idx].x, z: kreuzungsPunkte[idx].z, echt: true }
                   : { x: a.x, z: a.z, echt: false };
});
const KREUZUNGS_RADIUS = 45, ECHTE_KREUZUNG_RADIUS = 30;
const eltern = ampelnRoh.map((_, i) => i);
function find(i) { while (eltern[i] !== i) { eltern[i] = eltern[eltern[i]]; i = eltern[i]; } return i; }
function vereinige(a, b) { const ra = find(a), rb = find(b); if (ra !== rb) eltern[ra] = rb; }
for (let i = 0; i < ampelnRoh.length; i++) {
  for (let j = i + 1; j < ampelnRoh.length; j++) {
    const radius = (anker[i].echt && anker[j].echt) ? ECHTE_KREUZUNG_RADIUS : KREUZUNGS_RADIUS;
    const d = Math.hypot(anker[i].x - anker[j].x, anker[i].z - anker[j].z);
    if (d <= radius) vereinige(i, j);
  }
}
// Gruppen-IDs zu 0..n-1 verdichten, damit sie sich als seed fuer den
// Kreuzungs-Zeitversatz eignen (siehe LaLaBergAmpel.cpp).
const gruppenIndex = new Map();
const gruppe = ampelnRoh.map((_, i) => {
  const wurzel = find(i);
  if (!gruppenIndex.has(wurzel)) gruppenIndex.set(wurzel, gruppenIndex.size);
  return gruppenIndex.get(wurzel);
});
// Phase je Ampel: 0 fuer die Achse mit dem ersten "gier"-Winkel in einer
// Gruppe (auf 180 Grad reduziert), 1 fuer alles, was um rund 90 Grad
// dagegen verdreht ist.
const gruppenAchse = new Map();
const phase = ampelnRoh.map((a, i) => {
  const g = gruppe[i];
  const achse = ((a.gier % 180) + 180) % 180;
  if (!gruppenAchse.has(g)) { gruppenAchse.set(g, achse); return 0; }
  const bezug = gruppenAchse.get(g);
  let diff = Math.abs(achse - bezug); if (diff > 90) diff = 180 - diff;
  return diff > 45 ? 1 : 0;
});
const ampeln = ampelnRoh.map((s, i) => ({
  x: ux(s.x), y: uz(s.z), z: Math.round(boden(s.x, s.z) * M), gier: Math.round(s.gier),
  gruppe: gruppe[i], phase: phase[i],
}));

// Geparkte Autos: dieselben amtlichen Stellplaetze, dieselbe Filterung wie
// vormals in prepare-stadt.cjs (jeder zweite Platz, kein Wasser) - die dort
// als Kasten-Geometrie ins Stadt-Mesh gebackenen Wagen sind jetzt entfernt
// (siehe Commit-Nachricht: nicht einsteigbar, nicht das CarConcept-Modell).
// Stattdessen hier eine Positionsliste fuer echte, stehende KI-Auto-Akteure
// (ALaLaBergVerkehrsauto ohne Route - siehe LadeVerkehr).
function hash(x, z) {
  const s = Math.sin(x * 12.9898 + z * 78.233) * 43758.5453;
  return s - Math.floor(s);
}
const LACKE = [0xB9BCC0, 0x8E9296, 0x2E3236, 0xE8E9EA, 0x6E7276, 0x1F3A5C,
               0x7A2A24, 0x2C4A32, 0xC8C2B4, 0x4A5058];
const stellplaetze = city.parking || [];
const geparkt = [];
for (let i = 0; i + 2 < stellplaetze.length; i += 3) {
  if ((i / 3) % 2 !== 0) continue;
  const px = stellplaetze[i], pz = stellplaetze[i + 1], winkel = stellplaetze[i + 2] || 0;
  if (Terrain.isWater(px, pz)) continue;
  const t = hash(px, pz);
  const gierGrad = (winkel + (t - 0.5) * 0.06) * 180 / Math.PI;
  const rgb = LACKE[Math.floor(t * 997) % LACKE.length];
  geparkt.push({
    x: ux(px), y: uz(pz), z: Math.round(boden(px, pz) * M), gier: Math.round(gierGrad),
    lack: [((rgb >> 16) & 0xFF) / 255, ((rgb >> 8) & 0xFF) / 255, (rgb & 0xFF) / 255],
  });
}

const kreuzungen = kreuzungsPunkte.map(k => ({ x: ux(k.x), y: uz(k.z), klasse: k.klasse }));
const ergebnis = { schema: 1, autos, passanten, ampeln, geparkt, kreuzungen };
const out = path.resolve(REPO, 'Content/SourceData/Verkehr/verkehr.json');
fs.mkdirSync(path.dirname(out), { recursive: true });
fs.writeFileSync(out, JSON.stringify(ergebnis));
console.log(JSON.stringify({ autos: autos.length, passanten: passanten.length, ampeln: ampeln.length, geparkt: geparkt.length, kreuzungen: kreuzungen.length, bytes: fs.statSync(out).size }));
