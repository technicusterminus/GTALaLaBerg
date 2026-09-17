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
// Durchschnittliche Fahrbahnbreite je Strassenklasse (Meter, empirisch aus
// city.roads[].w berechnet) - der Strassengraph traegt selbst keine Breite
// je Kante, nur die Klasse. Die wichtigste angeschlossene Klasse
// (kreuzungsKlasse oben, niedrigste Zahl = breiteste Strasse) liefert damit
// eine Naeherung dafuer, wie breit die Kreuzung selbst ungefaehr ist - vor
// allem an einer breiten oder mehrarmigen Kreuzung sichtbar falsch, wenn
// LaLaBergVerkehrsauto::BremseVorKreuzung stattdessen fuer jede Kreuzung
// denselben festen Anhalteabstand (3 m) verwendet, egal ob eine schmale
// Nebenstrasse oder eine breite Hauptkreuzung mit fuenf Armen gemeint ist.
const BREITE_JE_KLASSE = { 0: 7.5, 1: 6.0, 2: 5.8, 3: 3.8, 4: 6.5, 5: 2.0 };
const kreuzungsPunkte = [...kreuzungsKlasse.keys()].map(n => ({
  x: graph.p[2 * n], z: graph.p[2 * n + 1], klasse: kreuzungsKlasse.get(n),
  breite: BREITE_JE_KLASSE[kreuzungsKlasse.get(n)] ?? 3.8,
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

// Eine Fahrt verkettet jetzt mehrere Strassen ueber den Strassengraphen,
// statt je Auto eine einzelne Strasse vor und zurueck zu fahren - erst
// dadurch biegt ein Auto an einer Kreuzung ueberhaupt ab.
//
// Jede Fahrt ist ein geschlossener Rundkurs, kein Hin und Zurueck. Der
// Wegfolger im Spiel haengt am Ende eines Rundkurses wieder an den Anfang an
// (siehe FLaLaBergWegfolger::bRund), statt die Route rueckwaerts
// zurueckzufahren. Das loest zwei Dinge auf einmal: das Auto dreht sich nicht
// mehr am Routenende auf der Stelle um, und Einbahnstrassen sind nutzbar,
// weil die Route nie gegen die Fahrtrichtung durchlaufen wird.
//
// Der Graph wird deshalb gerichtet gelesen (Einbahnregel wie im Webprojekt,
// js/world.js buildGraph: ed[3] >= 0 erlaubt a->b, <= 0 erlaubt b->a) und auf
// die groesste stark zusammenhaengende Komponente beschraenkt: nur dort ist
// garantiert, dass es von jedem Knoten aus einen gerichteten Rueckweg gibt.
// Die umfasst 6023 der 8192 Knoten, 100,6 km Strasse und 406 der 574
// Einbahnkanten.
const nachbarn = new Map();
{
  const anzahl = graph.p.length / 2;
  const vor = Array.from({ length: anzahl }, () => []);
  const rueck = Array.from({ length: anzahl }, () => []);
  const kanten = [];
  for (const [a, b, klasse, einbahn] of graph.e) {
    const weite = Math.hypot(graph.p[2 * b] - graph.p[2 * a], graph.p[2 * b + 1] - graph.p[2 * a + 1]);
    if (weite < 0.5) continue;                 // entartete Kante
    if (einbahn >= 0) { vor[a].push(b); rueck[b].push(a); kanten.push([a, b, klasse, weite]); }
    if (einbahn <= 0) { vor[b].push(a); rueck[a].push(b); kanten.push([b, a, klasse, weite]); }
  }
  // Starke Zusammenhangskomponenten nach Kosaraju, iterativ (rekursiv waere
  // bei 8192 Knoten ein Stapelueberlauf).
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
  for (const [von, nach, klasse, weite] of kanten) {
    if (komponente[von] !== beste || komponente[nach] !== beste) continue;
    if (!nachbarn.has(von)) nachbarn.set(von, []);
    nachbarn.get(von).push({ nach, klasse, weite });
  }
}
const knotenOrt = n => [graph.p[2 * n], graph.p[2 * n + 1]];
const kantenName = (a, b) => (a < b ? a + ':' + b : b + ':' + a);

// Zufallsfahrt von Kante zu Kante. Geradeaus ist wahrscheinlicher als
// Abbiegen (sonst irrt jedes Auto im Karree herum), eine wichtigere oder
// gleich wichtige Strasse wird bevorzugt (Verkehr folgt Hauptstrassen),
// und eine in dieser Fahrt schon befahrene Kante wird gemieden - ohne das
// pendelt die Fahrt zwischen denselben zwei Kreuzungen.
// Kalibriert: mit 1.5/0.4 (starke Geradeaus-Praeferenz) bog der Median aller
// 70 Routen kein einziges Mal ab - die Fahrten sahen aus wie vorher, nur
// laenger. 0.9/0.8 ergibt an einer T-Kreuzung rund ein Drittel Abbiegungen,
// an einer Vierfachkreuzung etwa die Haelfte.
const GERADE_GEWICHT = 0.9, ABBIEGE_GEWICHT = 0.8;
let abbiegungenGesamt = 0, kreuzungsWahlen = 0;
function baueFahrt(startVon, startNach, zielLaenge) {
  const kette = [{ n: startVon, klasse: null }, { n: startNach, klasse: null }];
  const benutzt = new Set([kantenName(startVon, startNach)]);
  let vorher = startVon, jetzt = startNach;
  const [sx, sz] = knotenOrt(startVon), [zx, zz] = knotenOrt(startNach);
  let laenge = Math.hypot(zx - sx, zz - sz);
  let klasseJetzt = 3;
  for (const o of nachbarn.get(startVon) || []) if (o.nach === startNach) klasseJetzt = o.klasse;
  kette[0].klasse = kette[1].klasse = klasseJetzt;
  while (laenge < zielLaenge) {
    const moeglich = (nachbarn.get(jetzt) || []).filter(o => o.nach !== vorher);
    if (!moeglich.length) break;                          // Sackgasse
    const [vx, vz] = knotenOrt(vorher), [jx, jz] = knotenOrt(jetzt);
    const rein = Math.hypot(jx - vx, jz - vz) || 1;
    const gewichte = moeglich.map(o => {
      const [nx, nz] = knotenOrt(o.nach);
      const raus = Math.hypot(nx - jx, nz - jz) || 1;
      const gerade = ((jx - vx) * (nx - jx) + (jz - vz) * (nz - jz)) / (rein * raus);
      let g = ABBIEGE_GEWICHT + Math.max(0, gerade) ** 2 * GERADE_GEWICHT;
      if (o.klasse <= klasseJetzt) g *= 1.2;
      if (benutzt.has(kantenName(jetzt, o.nach))) g *= 0.15;
      // Sackgasse meiden: dort endet die Fahrt sofort, und je kuerzer die
      // Route, desto oefter dreht das Auto im Spiel am Ende auf der Stelle um.
      if ((nachbarn.get(o.nach) || []).length <= 1) g *= 0.05;
      return g;
    });
    let wurf = Math.random() * gewichte.reduce((a, b) => a + b, 0), i = 0;
    while (i < gewichte.length - 1 && (wurf -= gewichte[i]) > 0) i++;
    const gewaehlt = moeglich[i];
    // Nur echte Wahlmoeglichkeiten zaehlen: ein Knoten mit genau einer
    // Fortsetzung ist ein Stuetzpunkt der Strassenlinie, keine Kreuzung.
    if (moeglich.length > 1) {
      kreuzungsWahlen++;
      const [gx, gz] = knotenOrt(gewaehlt.nach);
      const geradeGewaehlt = ((jx - vx) * (gx - jx) + (jz - vz) * (gz - jz)) /
        (rein * (Math.hypot(gx - jx, gz - jz) || 1));
      if (geradeGewaehlt < Math.cos(35 * Math.PI / 180)) abbiegungenGesamt++;
    }
    const [nx, nz] = knotenOrt(gewaehlt.nach);
    if (Terrain.isWater(nx, nz)) break;
    benutzt.add(kantenName(jetzt, gewaehlt.nach));
    klasseJetzt = gewaehlt.klasse;
    kette.push({ n: gewaehlt.nach, klasse: klasseJetzt });
    laenge += gewaehlt.weite;
    vorher = jetzt; jetzt = gewaehlt.nach;
  }
  return { kette, laenge, benutzt, ende: jetzt, vorletzter: vorher };
}

// Kuerzester gerichteter Rueckweg zum Startknoten, damit die Fahrt ein
// geschlossener Rundkurs wird. Schon befahrene Kanten kosten das Vierfache:
// der Rueckweg soll moeglichst neue Strassen nehmen statt die Hinfahrt
// einfach rueckwaerts abzufahren - erlaubt bleibt es, weil es sonst bei
// Stichstrassen gar keinen Rueckweg gaebe.
// Zwei Kehrtwenden muessen verhindert werden, sonst hat der Ring eine
// 180-Grad-Spitze statt einer Kurve:
//   "verbotenAmStart" ist der Knoten, aus dem die Hinfahrt zuletzt kam - der
//   erste Schritt des Rueckwegs darf nicht dorthin zurueck (Wende am
//   Umkehrpunkt).
//   "verbotenAmZiel" ist der zweite Knoten des Rings - der Rueckweg darf
//   nicht von dort auf den Startknoten einbiegen, sonst trifft er die
//   Hinfahrt frontal an der Naht. Genau das erzeugte zuvor in 33 von 70
//   Ringen eine exakte 180-Grad-Spitze direkt hinter dem Startpunkt.
function rueckweg(von, ziel, benutzt, verbotenAmStart, verbotenAmZiel) {
  const dist = new Map([[von, 0]]), herkunft = new Map();
  // Binaerhaufen: bei 6000 Knoten je Auto ist eine lineare Suche zu langsam.
  const haufen = [[0, von]];
  const hoch = i => { while (i > 0) { const e = (i - 1) >> 1;
    if (haufen[e][0] <= haufen[i][0]) break; [haufen[e], haufen[i]] = [haufen[i], haufen[e]]; i = e; } };
  const runter = i => { for (;;) { const l = 2 * i + 1, r = l + 1; let k = i;
    if (l < haufen.length && haufen[l][0] < haufen[k][0]) k = l;
    if (r < haufen.length && haufen[r][0] < haufen[k][0]) k = r;
    if (k === i) break; [haufen[k], haufen[i]] = [haufen[i], haufen[k]]; i = k; } };
  while (haufen.length) {
    const [d, v] = haufen[0];
    haufen[0] = haufen[haufen.length - 1]; haufen.pop(); if (haufen.length) runter(0);
    if (d > (dist.get(v) ?? Infinity)) continue;
    if (v === ziel) break;
    for (const o of nachbarn.get(v) || []) {
      if (v === von && o.nach === verbotenAmStart) continue;
      if (o.nach === ziel && v === verbotenAmZiel) continue;
      const kosten = o.weite * (benutzt.has(kantenName(v, o.nach)) ? 4 : 1);
      if (d + kosten < (dist.get(o.nach) ?? Infinity)) {
        dist.set(o.nach, d + kosten);
        herkunft.set(o.nach, { von: v, klasse: o.klasse, weite: o.weite });
        haufen.push([d + kosten, o.nach]); hoch(haufen.length - 1);
      }
    }
  }
  if (!herkunft.has(ziel) && von !== ziel) return null;
  const rueck = []; let k = ziel, laenge = 0;
  while (k !== von) {
    const h = herkunft.get(k);
    if (!h) return null;
    rueck.push({ n: k, klasse: h.klasse });
    laenge += h.weite;
    k = h.von;
  }
  rueck.reverse();
  return { kette: rueck, laenge };
}

// Wegpunkte verdichten wie resample(), aber Ecken erhalten: ein Punkt mit
// deutlicher Richtungsaenderung ist eine Kreuzung oder ein echter Knick und
// muss erhalten bleiben, sonst schneidet der gleichmaessige Abstand die
// Kurve auf einer willkuerlichen Sehne ab, statt ihr zu folgen.
// Arbeitet auf einem geschlossenen Ring: der letzte Punkt ist mit dem ersten
// verbunden, ohne dass er doppelt in der Liste steht (siehe Rundkurs oben).
const ECKE_AB = Math.cos((180 - 12) * Math.PI / 180);   // Knick ab 12 Grad
function verdichte(ring, schritt) {
  const n = ring.length;
  const aus = [ring[0]];
  let rest = schritt;
  for (let i = 0; i < n; i++) {
    let a = ring[i];
    const b = ring[(i + 1) % n];
    let seglen = Math.hypot(b.x - a.x, b.z - a.z);
    while (seglen >= rest) {
      const t = rest / seglen;
      a = { x: a.x + (b.x - a.x) * t, z: a.z + (b.z - a.z) * t, klasse: b.klasse };
      aus.push(a); seglen -= rest; rest = schritt;
    }
    rest -= seglen;
    // Ecke am Endpunkt dieses Segments? Dann diesen Punkt selbst behalten.
    // Richtung aus den urspruenglichen Segmenten, nicht aus dem zuletzt
    // ausgegebenen Punkt: der kann genau auf b liegen und ergaebe dann einen
    // Nullvektor statt einer Richtung. Der letzte Durchlauf (i = n-1) endet
    // auf ring[0], dessen Ecke schon als Startpunkt in "aus" steht.
    if (i === n - 1) break;
    const v = ring[i], c = ring[(i + 2) % n];
    const l1 = Math.hypot(b.x - v.x, b.z - v.z), l2 = Math.hypot(c.x - b.x, c.z - b.z);
    if (l1 < 0.2 || l2 < 0.2) continue;
    const cos = ((b.x - v.x) * (c.x - b.x) + (b.z - v.z) * (c.z - b.z)) / (l1 * l2);
    if (cos < ECKE_AB) { aus.push(b); rest = schritt; }
  }
  return aus;
}

// Echte Abbiegekurve statt rechtwinkliger Ecke: der Knick wird durch einen
// quadratischen Bezierbogen ersetzt, dessen Radius mit der Fahrbahnbreite
// waechst (eine Hauptstrasse hat einen weiteren Bogen als eine Gasse) und
// nie mehr als knapp die Haelfte der angrenzenden Segmente verbraucht.
// Die Bogenpunkte erben die wichtigere (niedrigere) der beiden Klassen: wer
// von der Hauptstrasse abbiegt, behaelt in der Kreuzung deren Vorfahrt.
// Auch dies auf einem geschlossenen Ring: die Naht, an der sich der Rundkurs
// schliesst, ist eine Ecke wie jede andere und muss genauso gerundet werden -
// sonst faehrt das Auto genau einmal je Runde einen Knick.
const RUNDUNG_AB = 12 * Math.PI / 180;
function rundeEcken(ring) {
  const n = ring.length;
  if (n < 3) return ring;
  const aus = [];
  for (let i = 0; i < n; i++) {
    const A = ring[(i - 1 + n) % n], B = ring[i], C = ring[(i + 1) % n];
    const v1x = A.x - B.x, v1z = A.z - B.z, v2x = C.x - B.x, v2z = C.z - B.z;
    const l1 = Math.hypot(v1x, v1z), l2 = Math.hypot(v2x, v2z);
    if (l1 < 0.2 || l2 < 0.2) continue;
    const cos = Math.max(-1, Math.min(1, (v1x * v2x + v1z * v2z) / (l1 * l2)));
    const abweichung = Math.PI - Math.acos(cos);          // 0 = geradeaus
    if (abweichung < RUNDUNG_AB) { aus.push(B); continue; }
    const klasse = Math.min(B.klasse, C.klasse);
    const breite = BREITE_JE_KLASSE[klasse] ?? 3.8;
    const r = Math.min(breite * 1.6, 11, l1 * 0.45, l2 * 0.45);
    // Zu kurze Nachbarsegmente lassen keinen sinnvollen Bogen zu - ein Bogen
    // mit Zentimeterradius waere nur eine Punktwolke auf der Ecke. Und bei
    // einer echten Kehrtwende (ueber 150 Grad) liegen Anfang und Ende des
    // Bogens fast auf demselben Strahl: der Bogen wuerde als Spitze
    // hinauslaufen und wieder zurueck. In beiden Faellen die Ecke behalten.
    if (r < 0.5 || abweichung > 150 * Math.PI / 180) { aus.push(B); continue; }
    const p0 = { x: B.x + v1x / l1 * r, z: B.z + v1z / l1 * r };
    const p2 = { x: B.x + v2x / l2 * r, z: B.z + v2z / l2 * r };
    const stufen = 2 + Math.round(abweichung / (Math.PI / 8));
    for (let s = 0; s <= stufen; s++) {
      const t = s / stufen, u = 1 - t;
      aus.push({
        x: u * u * p0.x + 2 * u * t * B.x + t * t * p2.x,
        z: u * u * p0.z + 2 * u * t * B.z + t * t * p2.z,
        klasse,
      });
    }
  }
  // Zu dichte Punkte wegwerfen: an einem sehr kurzen Segment faellt der
  // Bogenradius winzig aus, und die Bogenpunkte liegen dann im Zentimeter-
  // abstand uebereinander. Fuer die Fahrt aendern sie nichts, verfaelschen
  // aber jede Richtungsrechnung (Nullvektor) und blaehen die Datei auf.
  const knapp = [];
  for (const p of aus) {
    const l = knapp[knapp.length - 1];
    if (l && Math.hypot(p.x - l.x, p.z - l.z) < 0.3) continue;
    knapp.push(p);
  }
  // Auch ueber die Naht hinweg, sonst liegt der letzte Punkt auf dem ersten.
  while (knapp.length > 3) {
    const a = knapp[knapp.length - 1], b = knapp[0];
    if (Math.hypot(a.x - b.x, a.z - b.z) >= 0.3) break;
    knapp.pop();
  }
  return knapp;
}

// Startkanten: gewichtet gezogen statt nach Klasse sortiert. Streng sortiert
// begannen alle 70 Fahrten auf Klasse-0-Strassen (Umgehung/Hauptstrassen) -
// dort gibt es kaum Kreuzungen, entsprechend kam auf 487 m Fahrt nur eine
// einzige Abzweigung. Hauptstrassen bleiben bevorzugt, Nebenstrassen kommen
// aber vor; zusaetzlich ein Mindestabstand zwischen den Startpunkten, damit
// der Verkehr ueber die Stadt verteilt ist und nicht 70 Autos auf derselben
// Strasse stehen.
// Laenge der Hinfahrt; der Rueckweg zum Startknoten kommt etwa in derselben
// Groessenordnung dazu, ein Rundkurs wird also rund doppelt so lang.
const FAHRT_LAENGE = 600;
const FAHRT_VERSUCHE = 4;
const START_GEWICHT = { 0: 2.2, 1: 1.8, 2: 1.4, 3: 1.0, 4: 0.8 };
const START_ABSTAND = 120;
const startKanten = graph.e
  .filter(([a, b, , einbahn]) => einbahn === 0 && nachbarn.has(a) && nachbarn.has(b) &&
    !Terrain.isWater(graph.p[2 * a], graph.p[2 * a + 1]))
  .map(([a, b, klasse]) => ({ a, b, klasse,
    weite: Math.hypot(graph.p[2 * b] - graph.p[2 * a], graph.p[2 * b + 1] - graph.p[2 * a + 1]) }))
  .filter(k => k.weite >= 8)
  // Gewichtetes Ziehen ohne Zuruecklegen ueber den Exponential-Trick:
  // kleinerer Schluessel = frueher gezogen, Gewicht erhoeht die Chance.
  .map(k => ({ ...k, schluessel: -Math.log(Math.random()) / (START_GEWICHT[k.klasse] ?? 1.0) }))
  .sort((x, y) => x.schluessel - y.schluessel);

const autos = [];
const startOrte = [];
let uebersprungen = 0;
for (const k of startKanten) {
  if (autos.length >= AUTO_ROUTEN) break;
  const [kx, kz] = knotenOrt(k.a);
  if (startOrte.some(([x, z]) => Math.hypot(x - kx, z - kz) < START_ABSTAND)) continue;
  // Mehrere Versuche je Startkante, der laengste Rundkurs gewinnt: eine
  // Zufallsfahrt kann frueh in einem Stichweg enden, ein zweiter Wurf ab
  // derselben Kante findet meist weiter. Jede Hinfahrt wird ueber den
  // kuerzesten gerichteten Rueckweg zum Startknoten zu einem Ring
  // geschlossen - erst dadurch braucht das Auto im Spiel nie zu wenden.
  let kette = null, laenge = 0;
  for (let versuch = 0; versuch < FAHRT_VERSUCHE; versuch++) {
    const ziel = FAHRT_LAENGE * (0.75 + Math.random() * 0.65);
    const fahrt = baueFahrt(k.a, k.b, ziel);
    if (fahrt.kette.length < 3) continue;
    const heim = rueckweg(fahrt.ende, k.a, fahrt.benutzt, fahrt.vorletzter, k.b);
    if (!heim) continue;
    // Der Rueckweg endet auf dem Startknoten, der schon als erstes Glied im
    // Ring steht - sein letztes Glied deshalb weglassen, sonst liegt der
    // Startpunkt doppelt im Ring.
    const ring = fahrt.kette.concat(heim.kette.slice(0, -1));
    const gesamt = fahrt.laenge + heim.laenge;
    if (gesamt > laenge) { kette = ring; laenge = gesamt; }
    if (laenge >= ziel * 1.4) break;
  }
  if (laenge < 260 || !kette || kette.length < 4) { uebersprungen++; continue; }
  const roh = kette.map(({ n, klasse }) => {
    const [x, z] = knotenOrt(n);
    return { x, z, klasse: klasse ?? 3 };
  });
  const punkte = rundeEcken(verdichte(roh, 9));
  if (punkte.length < 3) { uebersprungen++; continue; }
  const weg = punkte.map(p => [ux(p.x), uz(p.z), Math.round(boden(p.x, p.z) * M)]);
  const flach = punkte.flatMap(p => [p.x, p.z]);
  // Klasse und Breite jetzt je Wegpunkt statt je Route: eine Fahrt fuehrt
  // ueber mehrere Strassen, also aendern sich Vorfahrtsrang und Fahrbahn-
  // breite unterwegs. "klasse"/"w" bleiben als Rueckfall fuer aeltere
  // Spielstaende, die kp/bp noch nicht kennen (siehe LadeVerkehr).
  const kp = punkte.map(p => p.klasse);
  const bp = punkte.map(p => BREITE_JE_KLASSE[p.klasse] ?? 3.8);
  autos.push({
    w: Math.max(3.0, Math.min(...bp)), klasse: Math.min(...kp),
    // "rund": geschlossener Ring - der Wegfolger haengt hinter dem letzten
    // Wegpunkt wieder den ersten an, statt die Route rueckwaerts
    // zurueckzufahren (siehe FLaLaBergWegfolger).
    rund: true,
    kp, bp, kreuzungen: findeKreuzungen(flach), p: weg.flat(),
  });
  startOrte.push([kx, kz]);
}
console.log(JSON.stringify({ autoRouten: autos.length, uebersprungen,
  mittlereWegpunkte: Math.round(autos.reduce((s, a) => s + a.p.length / 3, 0) / Math.max(1, autos.length)),
  kreuzungsWahlen, abbiegungenGesamt,
  abbiegeAnteil: +(abbiegungenGesamt / Math.max(1, kreuzungsWahlen)).toFixed(2) }));

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
// Phase je Ampel: eine eigene, sich nie ueberschneidende Phase je
// tatsaechlich unterschiedlicher Fahrbahnachse innerhalb der Gruppe, statt
// nur zwei fester Buckets ("ungefaehr Referenzachse" vs. "alles andere").
// Bei einer gewoehnlichen Kreuzung mit vier Armen (zwei Achsen) ist das
// weiterhin dasselbe Ergebnis wie vorher; bei mehr als vier Armen (mehr als
// zwei tatsaechlich verschiedene Achsen) reichten zwei Buckets nicht mehr,
// um jede Achse sicher von jeder anderen zu trennen - eine dritte Achse
// landete zwangslaeufig im selben Bucket wie eine der beiden ersten, obwohl
// sie mit keiner davon dieselbe Fahrtrichtung teilt. Jede neue Achse
// bekommt hier stattdessen ihre eigene Phase, bis sie nah genug (<= 40 Grad,
// modulo 180 wegen Hin-/Rueckrichtung derselben Strasse) an einer bereits
// vorhandenen liegt. LaLaBergAmpel.cpp teilt den Zyklus in ebenso viele
// gleich lange, nicht ueberlappende Zeitfenster wie es Phasen gibt.
const ACHSEN_TOLERANZ = 40;
const gruppenAchsen = new Map(); // Gruppe -> [{achse, phase}]
const phase = ampelnRoh.map((a, i) => {
  const g = gruppe[i];
  const achse = ((a.gier % 180) + 180) % 180;
  if (!gruppenAchsen.has(g)) gruppenAchsen.set(g, []);
  const liste = gruppenAchsen.get(g);
  for (const eintrag of liste) {
    let diff = Math.abs(achse - eintrag.achse); if (diff > 90) diff = 180 - diff;
    if (diff <= ACHSEN_TOLERANZ) return eintrag.phase;
  }
  const neuePhase = liste.length;
  liste.push({ achse, phase: neuePhase });
  return neuePhase;
});
const anzahlPhasenJeGruppe = new Map();
for (const [g, liste] of gruppenAchsen) anzahlPhasenJeGruppe.set(g, liste.length);
const ampeln = ampelnRoh.map((s, i) => ({
  x: ux(s.x), y: uz(s.z), z: Math.round(boden(s.x, s.z) * M), gier: Math.round(s.gier),
  gruppe: gruppe[i], phase: phase[i], phasen: anzahlPhasenJeGruppe.get(gruppe[i]),
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

const kreuzungen = kreuzungsPunkte.map(k => ({ x: ux(k.x), y: uz(k.z), klasse: k.klasse, breite: Math.round(k.breite * 10) / 10 }));
const ergebnis = { schema: 1, autos, passanten, ampeln, geparkt, kreuzungen };
const out = path.resolve(REPO, 'Content/SourceData/Verkehr/verkehr.json');
fs.mkdirSync(path.dirname(out), { recursive: true });
fs.writeFileSync(out, JSON.stringify(ergebnis));
console.log(JSON.stringify({ autos: autos.length, passanten: passanten.length, ampeln: ampeln.length, geparkt: geparkt.length, kreuzungen: kreuzungen.length, bytes: fs.statSync(out).size }));
