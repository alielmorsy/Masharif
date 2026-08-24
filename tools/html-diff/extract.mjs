// Loads every fixtures/*.html in headless Chromium and extracts each [data-id] element's
// border-box rect relative to the #root element's own border-box origin -- the same frame
// Masharif's ComputedX/Y uses (root always sits at local 0,0).
import { chromium } from 'playwright';
import { readdirSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const FIXTURES_DIR = path.join(__dirname, 'fixtures');
const OUT = path.join(__dirname, 'out', 'browser.jsonl');

const files = readdirSync(FIXTURES_DIR).filter((f) => f.endsWith('.html'));

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1400, height: 1000 } });
const lines = [];

for (const file of files) {
  const caseName = file.replace(/\.html$/, '');
  await page.goto('file://' + path.join(FIXTURES_DIR, file));
  const rects = await page.evaluate(() => {
    const root = document.querySelector('[data-id="root"]');
    const rootRect = root.getBoundingClientRect();
    const out = [];
    document.querySelectorAll('[data-id]').forEach((el) => {
      const r = el.getBoundingClientRect();
      out.push({
        id: el.getAttribute('data-id'),
        x: r.left - rootRect.left,
        y: r.top - rootRect.top,
        w: r.width,
        h: r.height,
      });
    });
    return out;
  });
  for (const r of rects) {
    lines.push(JSON.stringify({ case: caseName, id: r.id, x: r.x, y: r.y, w: r.w, h: r.h }));
  }
  console.log(`extracted ${caseName}: ${rects.length} nodes`);
}

await browser.close();
writeFileSync(OUT, lines.join('\n') + '\n', 'utf8');
console.log(`Wrote ${lines.length} rects to ${OUT}`);
