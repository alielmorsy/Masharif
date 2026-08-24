// Diffs masharif_results.json (from harness.exe) against web_results.json (from extract.mjs).
// Prints a per-fixture, per-node, per-field table and a final PASS/FAIL summary.
import { readFileSync } from 'fs';

const EPS = 0.6; // px tolerance: sub-pixel rounding/rendering differences are not bugs

const masharif = JSON.parse(readFileSync('masharif_results.json', 'utf8'));
const web = JSON.parse(readFileSync('web_results.json', 'utf8'));

let totalNodes = 0, mismatchedNodes = 0;
const report = [];

for (const fixture of Object.keys(web)) {
    const wNodes = web[fixture];
    const mNodes = masharif[fixture];
    if (!mNodes) {
        report.push(`FIXTURE ${fixture}: MISSING from masharif_results.json`);
        continue;
    }
    for (const id of Object.keys(wNodes)) {
        totalNodes++;
        const w = wNodes[id];
        const m = mNodes[id];
        if (!m) {
            mismatchedNodes++;
            report.push(`${fixture}.${id}: MISSING in masharif output`);
            continue;
        }
        const diffs = [];
        for (const field of ['x', 'y', 'w', 'h']) {
            const dv = Math.abs(w[field] - m[field]);
            if (dv > EPS) diffs.push(`${field}: web=${w[field]} masharif=${m[field]} (Δ${dv.toFixed(2)})`);
        }
        if (diffs.length) {
            mismatchedNodes++;
            report.push(`${fixture}.${id}: ${diffs.join(', ')}`);
        } else {
            report.push(`${fixture}.${id}: OK (x=${m.x},y=${m.y},w=${m.w},h=${m.h})`);
        }
    }
}

console.log(report.join('\n'));
console.log('\n--- SUMMARY ---');
console.log(`${totalNodes} nodes compared, ${mismatchedNodes} mismatched, ${totalNodes - mismatchedNodes} matched (tolerance ${EPS}px)`);
