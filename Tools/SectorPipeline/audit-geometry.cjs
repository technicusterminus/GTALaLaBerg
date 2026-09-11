'use strict';

const fs = require('node:fs');
const path = require('node:path');

const project = path.resolve(__dirname, '..', '..');
const source = JSON.parse(fs.readFileSync(path.join(project, 'Content', 'SourceData', 'stadt.json')));
const focus = { x: Number(process.argv[2] || -1700), y: Number(process.argv[3] || -2600) };
const focusRadius = Number(process.argv[4] || 30000);
const top = [];
const summaries = [];
const focusRoofComponents = [];

function distance(a, b) {
  return Math.hypot(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
}

for (const section of source.sections) {
  let degenerate = 0;
  let over100m = 0;
  let over150m = 0;
  let over200m = 0;
  let steep = 0;
  let maxEdge = 0;
  let maxDz = 0;
  const read = index => section.positions.slice(index * 3, index * 3 + 3);
  for (let offset = 0; offset < section.indices.length; offset += 3) {
    const indices = section.indices.slice(offset, offset + 3);
    const points = indices.map(read);
    const edges = [distance(points[0], points[1]), distance(points[1], points[2]), distance(points[2], points[0])];
    const dz = Math.max(
      Math.abs(points[0][2] - points[1][2]),
      Math.abs(points[1][2] - points[2][2]),
      Math.abs(points[2][2] - points[0][2]),
    );
    const longest = Math.max(...edges);
    const cx = (points[0][0] + points[1][0] + points[2][0]) / 3;
    const cy = (points[0][1] + points[1][1] + points[2][1]) / 3;
    const nearFocus = Math.hypot(cx - focus.x, cy - focus.y) <= focusRadius;
    const ax = points[1][0] - points[0][0], ay = points[1][1] - points[0][1], az = points[1][2] - points[0][2];
    const bx = points[2][0] - points[0][0], by = points[2][1] - points[0][1], bz = points[2][2] - points[0][2];
    const area2 = Math.hypot(ay * bz - az * by, az * bx - ax * bz, ax * by - ay * bx);
    if (area2 < 0.01) degenerate++;
    if (longest > 10000) over100m++;
    if (longest > 15000) over150m++;
    if (longest > 20000) over200m++;
    if (dz > 2000 && longest > 2500) steep++;
    maxEdge = Math.max(maxEdge, longest);
    maxDz = Math.max(maxDz, dz);
    if (longest > 7000 || dz > 3000) {
      top.push({ section: section.name, triangle: offset / 3, longest, dz, cx, cy, nearFocus, points });
    }
  }
  summaries.push({ section: section.name, triangles: section.indices.length / 3, degenerate, over100m, over150m, over200m, steep, maxEdge, maxDz });

  if (section.name.startsWith('Roof')) {
    const candidates = [];
    for (let offset = 0; offset < section.indices.length; offset += 3) {
      const indices = section.indices.slice(offset, offset + 3);
      const points = indices.map(read);
      const cx = (points[0][0] + points[1][0] + points[2][0]) / 3;
      const cy = (points[0][1] + points[1][1] + points[2][1]) / 3;
      if (Math.hypot(cx - focus.x, cy - focus.y) <= focusRadius) candidates.push({ offset, points });
    }
    const parent = candidates.map((_, index) => index);
    const find = index => parent[index] === index ? index : (parent[index] = find(parent[index]));
    const join = (left, right) => {
      left = find(left); right = find(right);
      if (left !== right) parent[right] = left;
    };
    const owners = new Map();
    candidates.forEach((triangle, index) => triangle.points.forEach(point => {
      const key = point.join(',');
      if (owners.has(key)) join(index, owners.get(key));
      else owners.set(key, index);
    }));
    const components = new Map();
    candidates.forEach((triangle, index) => {
      const root = find(index);
      if (!components.has(root)) components.set(root, []);
      components.get(root).push(triangle);
    });
    for (const triangles of components.values()) {
      const points = triangles.flatMap(triangle => triangle.points);
      const mins = [0, 1, 2].map(axis => Math.min(...points.map(point => point[axis])));
      const maxs = [0, 1, 2].map(axis => Math.max(...points.map(point => point[axis])));
      focusRoofComponents.push({
        section: section.name,
        triangles: triangles.length,
        firstTriangle: triangles[0].offset / 3,
        bounds: [...mins, ...maxs],
        diagonalXY: Math.hypot(maxs[0] - mins[0], maxs[1] - mins[1]),
        height: maxs[2] - mins[2],
      });
    }
  }
}

const important = summaries.filter(item => item.degenerate || item.over100m || item.steep)
  .sort((a, b) => b.maxEdge - a.maxEdge);
console.log('SECTIONS_WITH_RISK');
for (const item of important) console.log(JSON.stringify(item));
console.log('TOP_GLOBAL');
for (const item of top.sort((a, b) => Math.max(b.longest, b.dz * 2) - Math.max(a.longest, a.dz * 2)).slice(0, 40)) console.log(JSON.stringify(item));
console.log('TOP_FOCUS');
for (const item of top.filter(item => item.nearFocus).sort((a, b) => Math.max(b.longest, b.dz * 2) - Math.max(a.longest, a.dz * 2)).slice(0, 60)) console.log(JSON.stringify(item));
console.log('ROOF_COMPONENTS_FOCUS');
for (const item of focusRoofComponents.sort((a, b) => b.diagonalXY - a.diagonalXY).slice(0, 80)) console.log(JSON.stringify(item));
