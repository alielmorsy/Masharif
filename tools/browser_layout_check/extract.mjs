// Loads each fixtures/*.html in headless Chromium via Playwright and extracts, for every
// element with an id, its box relative to #root's box (x,y,w,h in CSS px). Writes one
// combined JSON object (fixture name -> {id -> {x,y,w,h}}) to web_results.json, matching
// the shape harness.exe prints for the Masharif side.
import { chromium } from 'playwright';
import { readdirSync } from 'fs';
import { writeFileSync } from 'fs';
import path from 'path';
import { fileURLToPath, pathToFileURL } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const fixturesDir = path.join(__dirname, 'fixtures');
const files = readdirSync(fixturesDir).filter(f => f.endsWith('.html')).sort();

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1400, height: 1400 } });

const results = {};
for (const file of files) {
    const name = file.replace(/\.html$/, '');
    const url = pathToFileURL(path.join(fixturesDir, file)).href;
    await page.goto(url);
    const rects = await page.evaluate(() => {
        const root = document.getElementById('root');
        const rootRect = root.getBoundingClientRect();
        const out = {};
        document.querySelectorAll('[id]').forEach(el => {
            const r = el.getBoundingClientRect();
            out[el.id] = {
                x: Math.round((r.left - rootRect.left) * 100) / 100,
                y: Math.round((r.top - rootRect.top) * 100) / 100,
                w: Math.round(r.width * 100) / 100,
                h: Math.round(r.height * 100) / 100,
            };
        });
        return out;
    });
    results[name] = rects;
    console.log(`extracted ${name}: ${Object.keys(rects).length} nodes`);
}

await browser.close();
writeFileSync(path.join(__dirname, 'web_results.json'), JSON.stringify(results, null, 2));
console.log('wrote web_results.json');
