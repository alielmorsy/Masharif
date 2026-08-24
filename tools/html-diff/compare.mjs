// Joins browser.jsonl (ground truth) against masharif.jsonl (engine output) on (case,id)
// and reports per-field deltas. Two cases are pre-declared "known gaps" (documented,
// reasoned-about limitations found during code review) rather than raw failures.
import { readFileSync, writeFileSync, existsSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const OUT_DIR = path.join(__dirname, 'out');
const TOL = 0.5; // px -- sub-pixel float rounding tolerance, not a real mismatch

const KNOWN_GAPS = new Set([
  'relative_offset_dead_field:shifted',
  'block_normal_flow:c2',
]);

function loadJsonl(p) {
  const map = new Map();
  for (const line of readFileSync(p, 'utf8').split('\n')) {
    if (!line.trim()) continue;
    const o = JSON.parse(line);
    map.set(`${o.case}:${o.id}`, o);
  }
  return map;
}

const browserPath = path.join(OUT_DIR, 'browser.jsonl');
const masharifPath = path.join(OUT_DIR, 'masharif.jsonl');
if (!existsSync(browserPath) || !existsSync(masharifPath)) {
  console.error('Missing out/browser.jsonl or out/masharif.jsonl -- run extract.mjs and the harness first.');
  process.exit(1);
}
const browser = loadJsonl(browserPath);
const masharif = loadJsonl(masharifPath);

const rows = [];
const allKeys = new Set([...browser.keys(), ...masharif.keys()]);
for (const key of allKeys) {
  const b = browser.get(key);
  const m = masharif.get(key);
  const [caseName, id] = key.split(':');
  if (!b || !m) {
    rows.push({ case: caseName, id, status: 'MISSING', detail: !b ? 'no browser rect' : 'no masharif rect' });
    continue;
  }
  const dx = m.x - b.x, dy = m.y - b.y, dw = m.w - b.w, dh = m.h - b.h;
  const maxDelta = Math.max(Math.abs(dx), Math.abs(dy), Math.abs(dw), Math.abs(dh));
  const isKnownGap = KNOWN_GAPS.has(key);
  let status;
  if (maxDelta <= TOL) status = 'MATCH';
  else if (isKnownGap) status = 'KNOWN GAP';
  else status = 'MISMATCH';
  rows.push({
    case: caseName, id, status,
    browser: `(${b.x.toFixed(1)}, ${b.y.toFixed(1)}, ${b.w.toFixed(1)}x${b.h.toFixed(1)})`,
    masharif: `(${m.x.toFixed(1)}, ${m.y.toFixed(1)}, ${m.w.toFixed(1)}x${m.h.toFixed(1)})`,
    delta: `(${dx.toFixed(1)}, ${dy.toFixed(1)}, ${dw.toFixed(1)}, ${dh.toFixed(1)})`,
    maxDelta,
  });
}

rows.sort((a, b2) => a.case.localeCompare(b2.case) || a.id.localeCompare(b2.id));

const counts = rows.reduce((acc, r) => { acc[r.status] = (acc[r.status] ?? 0) + 1; return acc; }, {});
console.log('Summary:', counts);

const lines = [];
lines.push('| case | id | status | browser (x,y,wxh) | masharif (x,y,wxh) | delta (dx,dy,dw,dh) |');
lines.push('|---|---|---|---|---|---|');
for (const r of rows) {
  if (r.status === 'MISSING') { lines.push(`| ${r.case} | ${r.id} | **MISSING** | - | - | ${r.detail} |`); continue; }
  const mark = r.status === 'MATCH' ? 'MATCH' : r.status === 'KNOWN GAP' ? '**KNOWN GAP**' : '**MISMATCH**';
  lines.push(`| ${r.case} | ${r.id} | ${mark} | ${r.browser} | ${r.masharif} | ${r.delta} |`);
}
writeFileSync(path.join(OUT_DIR, 'results.md'), lines.join('\n') + '\n', 'utf8');
console.log(`Wrote ${rows.length} rows to out/results.md`);

const unexpected = rows.filter((r) => r.status === 'MISMATCH' || r.status === 'MISSING');
if (unexpected.length) {
  console.log('\nUnexpected mismatches (not pre-declared known gaps):');
  for (const r of unexpected) console.log(`  ${r.case}:${r.id} -> ${r.status} ${r.delta ?? r.detail}`);
}
