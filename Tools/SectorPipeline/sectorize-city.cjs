'use strict';

const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');

const project = path.resolve(__dirname, '..', '..');
const inputPath = path.join(project, 'Content', 'SourceData', 'stadt.json');
const outputRoot = path.join(project, 'Content', 'SourceData', 'Sectors');
const cellSizeCm = Number(process.argv[2] || 50000);

if (!Number.isFinite(cellSizeCm) || cellSizeCm < 10000 || cellSizeCm > 200000) {
  throw new Error('Sector size must be between 10000 and 200000 cm');
}
const sourceBuffer = fs.readFileSync(inputPath);
const city = JSON.parse(sourceBuffer);
if (city.schema !== 1 || city.units !== 'centimeters' || !Array.isArray(city.sections)) {
  throw new Error('Unsupported city data contract');
}

const sectors = new Map();
let sourceTriangles = 0;
let keptTriangles = 0;
let removedTriangles = 0;
const removedBySection = {};
const maximumArchitectureEdgeCm = 15000;
const architecturePattern = /^(Wall|Sockel|Gesims|Roof|Laden|Kamin|Tuer)/;
function sectorFor(x, y) {
  const sx = Math.floor(x / cellSizeCm);
  const sy = Math.floor(y / cellSizeCm);
  const id = `S_${sx < 0 ? 'N' + -sx : 'P' + sx}_${sy < 0 ? 'N' + -sy : 'P' + sy}`;
  if (!sectors.has(id)) sectors.set(id, { id, sx, sy, sections: new Map(), triangles: 0 });
  return sectors.get(id);
}
function sectionFor(sector, source) {
  if (!sector.sections.has(source.name)) {
    sector.sections.set(source.name, {
      name: source.name,
      color: source.color,
      positions: [],
      indices: [],
      uvs: source.uvs ? [] : undefined,
      sourceToLocal: new Map(),
    });
  }
  return sector.sections.get(source.name);
}
function copyVertex(target, source, sourceIndex) {
  if (target.sourceToLocal.has(sourceIndex)) return target.sourceToLocal.get(sourceIndex);
  const local = target.positions.length / 3;
  target.sourceToLocal.set(sourceIndex, local);
  target.positions.push(
    source.positions[sourceIndex * 3],
    source.positions[sourceIndex * 3 + 1],
    source.positions[sourceIndex * 3 + 2],
  );
  if (target.uvs) target.uvs.push(source.uvs[sourceIndex * 2], source.uvs[sourceIndex * 2 + 1]);
  return local;
}

for (const section of city.sections) {
  if (!Array.isArray(section.positions) || section.positions.length % 3 !== 0 ||
      !Array.isArray(section.indices) || section.indices.length % 3 !== 0 ||
      (section.uvs && section.uvs.length !== section.positions.length / 3 * 2)) {
    throw new Error(`Invalid section ${section.name}`);
  }
  const vertexCount = section.positions.length / 3;
  for (let i = 0; i < section.indices.length; i += 3) {
    const tri = section.indices.slice(i, i + 3);
    sourceTriangles++;
    if (tri.some(index => !Number.isInteger(index) || index < 0 || index >= vertexCount)) {
      throw new Error(`Invalid index in ${section.name}`);
    }
    const points = tri.map(index => section.positions.slice(index * 3, index * 3 + 3));
    const edge = (a, b) => Math.hypot(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
    const longestEdge = Math.max(edge(points[0], points[1]), edge(points[1], points[2]), edge(points[2], points[0]));
    // Die Stadtquelle enthaelt einzelne Architekturpolygone, die zwei 175 bis
    // 316 Meter entfernte Gebaeude verbinden. Solche Bruecken sind in dieser
    // Detailklasse unmoeglich und erzeugen schwebende Waende quer durch Stadtteile.
    if (architecturePattern.test(section.name) && longestEdge > maximumArchitectureEdgeCm) {
      removedTriangles++;
      removedBySection[section.name] = (removedBySection[section.name] || 0) + 1;
      continue;
    }
    const cx = tri.reduce((sum, index) => sum + section.positions[index * 3], 0) / 3;
    const cy = tri.reduce((sum, index) => sum + section.positions[index * 3 + 1], 0) / 3;
    const sector = sectorFor(cx, cy);
    const target = sectionFor(sector, section);
    target.indices.push(...tri.map(index => copyVertex(target, section, index)));
    sector.triangles++;
    keptTriangles++;
  }
}

const tempRoot = outputRoot + '.next';
fs.rmSync(tempRoot, { recursive: true, force: true });
fs.mkdirSync(tempRoot, { recursive: true });
const manifestSectors = [];
let outputTriangles = 0;
for (const sector of [...sectors.values()].sort((a, b) => a.id.localeCompare(b.id))) {
  const payload = {
    schema: 1,
    units: city.units,
    originGameXZ: city.originGameXZ,
    terrainBaseMeters: city.terrainBaseMeters,
    sector: {
      id: sector.id,
      index: [sector.sx, sector.sy],
      sizeCm: cellSizeCm,
      boundsCm: [sector.sx * cellSizeCm, sector.sy * cellSizeCm,
        (sector.sx + 1) * cellSizeCm, (sector.sy + 1) * cellSizeCm],
    },
    sections: [...sector.sections.values()].sort((a, b) => a.name.localeCompare(b.name)).map(s => {
      const out = { name: s.name, color: s.color, positions: s.positions, indices: s.indices };
      if (s.uvs) out.uvs = s.uvs;
      outputTriangles += s.indices.length / 3;
      return out;
    }),
  };
  const json = JSON.stringify(payload);
  const file = sector.id + '.json';
  fs.writeFileSync(path.join(tempRoot, file), json);
  manifestSectors.push({
    id: sector.id,
    file,
    index: [sector.sx, sector.sy],
    boundsCm: payload.sector.boundsCm,
    triangles: sector.triangles,
    sections: payload.sections.map(s => s.name),
    bytes: Buffer.byteLength(json),
    sha256: crypto.createHash('sha256').update(json).digest('hex'),
  });
}
if (outputTriangles !== keptTriangles || sourceTriangles !== keptTriangles + removedTriangles) {
  throw new Error(`Triangle conservation failed: ${sourceTriangles} -> ${keptTriangles} + ${removedTriangles}`);
}

const manifest = {
  schema: 1,
  generator: 'Tools/SectorPipeline/sectorize-city.cjs',
  source: {
    file: 'Content/SourceData/stadt.json',
    bytes: sourceBuffer.length,
    sha256: crypto.createHash('sha256').update(sourceBuffer).digest('hex'),
  },
  units: city.units,
  originGameXZ: city.originGameXZ,
  terrainBaseMeters: city.terrainBaseMeters,
  cellSizeCm,
  buildingCount: city.buildingCount,
  faceCount: city.faceCount,
  sourceTriangleCount: sourceTriangles,
  triangleCount: keptTriangles,
  spawnHeightCm: city.spawnHeightCm,
  spawn: city.spawn,
  geometryRepair: {
    maximumArchitectureEdgeCm,
    removedTriangles,
    removedBySection,
  },
  sectorCount: manifestSectors.length,
  sectors: manifestSectors,
};
fs.writeFileSync(path.join(tempRoot, 'manifest.json'), JSON.stringify(manifest, null, 2) + '\n');

const priorRoot = outputRoot + '.previous';
fs.rmSync(priorRoot, { recursive: true, force: true });
if (fs.existsSync(outputRoot)) fs.renameSync(outputRoot, priorRoot);
try {
  fs.renameSync(tempRoot, outputRoot);
  fs.rmSync(priorRoot, { recursive: true, force: true });
} catch (error) {
  if (!fs.existsSync(outputRoot) && fs.existsSync(priorRoot)) fs.renameSync(priorRoot, outputRoot);
  throw error;
}

console.log(JSON.stringify({
  sourceBytes: sourceBuffer.length,
  cellSizeCm,
  sectors: manifest.sectorCount,
  sourceTriangles: manifest.sourceTriangleCount,
  triangles: manifest.triangleCount,
  removedTriangles: manifest.geometryRepair.removedTriangles,
  outputBytes: manifestSectors.reduce((sum, s) => sum + s.bytes, 0),
  largestSectorBytes: Math.max(...manifestSectors.map(s => s.bytes)),
  sourceSha256: manifest.source.sha256,
}));
