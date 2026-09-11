'use strict';

const fs = require('node:fs');
const path = require('node:path');

const project = path.resolve(__dirname, '..', '..');
const city = JSON.parse(fs.readFileSync(path.join(project, 'Content', 'SourceData', 'stadt.json')));
const cellSize = 2000;
const roadSections = city.sections.filter(section => /^(Road|Plaza|Rail)/.test(section.name));
const groundSections = city.sections.filter(section => section.name.startsWith('Ground'));
const cells = new Map();

const cellKey = (x, y) => `${x},${y}`;
function readPoint(section, index) {
  return [section.positions[index * 3], section.positions[index * 3 + 1], section.positions[index * 3 + 2]];
}

for (const section of roadSections) {
  for (let offset = 0; offset < section.indices.length; offset += 3) {
    const points = section.indices.slice(offset, offset + 3).map(index => readPoint(section, index));
    const minX = Math.floor(Math.min(...points.map(point => point[0])) / cellSize);
    const maxX = Math.floor(Math.max(...points.map(point => point[0])) / cellSize);
    const minY = Math.floor(Math.min(...points.map(point => point[1])) / cellSize);
    const maxY = Math.floor(Math.max(...points.map(point => point[1])) / cellSize);
    const triangle = { section: section.name, points };
    for (let x = minX; x <= maxX; x++) for (let y = minY; y <= maxY; y++) {
      const key = cellKey(x, y);
      if (!cells.has(key)) cells.set(key, []);
      cells.get(key).push(triangle);
    }
  }
}

function heightAt(x, y) {
  const candidates = cells.get(cellKey(Math.floor(x / cellSize), Math.floor(y / cellSize))) || [];
  let result = null;
  for (const triangle of candidates) {
    const [a, b, c] = triangle.points;
    const denominator = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1]);
    if (Math.abs(denominator) < 0.0001) continue;
    const u = ((b[1] - c[1]) * (x - c[0]) + (c[0] - b[0]) * (y - c[1])) / denominator;
    const v = ((c[1] - a[1]) * (x - c[0]) + (a[0] - c[0]) * (y - c[1])) / denominator;
    const w = 1 - u - v;
    if (u < -0.0001 || v < -0.0001 || w < -0.0001) continue;
    const z = u * a[2] + v * b[2] + w * c[2];
    if (!result || z > result.z) result = { z, section: triangle.section };
  }
  return result;
}

const differences = [];
const nearStreet = [];
const unique = new Set();
for (const section of groundSections) {
  for (let index = 0; index < section.positions.length / 3; index++) {
    const point = readPoint(section, index);
    const id = point.join(',');
    if (unique.has(id)) continue;
    unique.add(id);
    const road = heightAt(point[0], point[1]);
    if (!road) continue;
    const item = { ground: section.name, road: road.section, x: point[0], y: point[1], groundZ: point[2], roadZ: road.z, difference: point[2] - road.z };
    differences.push(item);
    if (Math.hypot(point[0] - 2200, point[1] + 9700) < 20000) nearStreet.push(item);
  }
}

function summarize(items) {
  const sorted = items.map(item => item.difference).sort((a, b) => a - b);
  const percentile = value => sorted.length ? sorted[Math.min(sorted.length - 1, Math.floor(sorted.length * value))] : null;
  return {
    samples: sorted.length,
    p01: percentile(0.01), p10: percentile(0.10), median: percentile(0.50), p90: percentile(0.90), p99: percentile(0.99),
    minimum: sorted[0] ?? null, maximum: sorted[sorted.length - 1] ?? null,
    groundAbove5cm: sorted.filter(value => value > 5).length,
    groundAbove20cm: sorted.filter(value => value > 20).length,
    groundAbove50cm: sorted.filter(value => value > 50).length,
    groundAbove100cm: sorted.filter(value => value > 100).length,
  };
}

console.log('CITY', JSON.stringify(summarize(differences)));
console.log('STREET_PHOTO', JSON.stringify(summarize(nearStreet)));
console.log('WORST_STREET_PHOTO');
for (const item of nearStreet.sort((a, b) => b.difference - a.difference).slice(0, 30)) console.log(JSON.stringify(item));
console.log('WORST_CITY');
for (const item of differences.sort((a, b) => b.difference - a.difference).slice(0, 30)) console.log(JSON.stringify(item));
