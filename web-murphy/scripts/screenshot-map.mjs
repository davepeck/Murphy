// Drives the map room in headless Chromium: BFS-computes the path from the
// start to the level-1 node, walks it, opens the info popup, enters the level.
//   node scripts/screenshot-map.mjs <outdir>
import { chromium } from "playwright";
import { mkdirSync, readFileSync } from "node:fs";

const outDir = process.argv[2] ?? "shots-map";
mkdirSync(outDir, { recursive: true });

// --- plan the walk offline from map.json --------------------------------------
const map = JSON.parse(readFileSync("public/assets/world/map.json", "utf8"));
const W = map.width;
function classify(id) {
  const col = id % 18;
  const row = Math.floor(id / 18);
  if (col === 1 && row <= 2) return "level";
  if (col === 1 && row >= 3 && row <= 5) return "passed";
  if (row === 6 && col <= 8) return "switch";
  if ((row === 7 || row === 8) && col <= 8) return "path";
  return "nopass";
}
const start = map.start;
const startIdx = start[1] * W + start[0];
const tiles = map.tiles.slice();
tiles[startIdx] = map.trigTiles[startIdx]; // start marker shows its trigP
const target = [61, 50]; // level 1: And So It Begins
const prev = new Map();
const q = [start];
prev.set(start[1] * W + start[0], null);
while (q.length) {
  const [x, y] = q.shift();
  if (x === target[0] && y === target[1]) break;
  for (const [dx, dy] of [[0, -1], [0, 1], [-1, 0], [1, 0]]) {
    const nx = x + dx;
    const ny = y + dy;
    if (nx < 0 || ny < 0 || nx >= W || ny >= W) continue;
    const k = ny * W + nx;
    if (prev.has(k)) continue;
    if (classify(tiles[k]) === "nopass") continue;
    prev.set(k, [x, y]);
    q.push([nx, ny]);
  }
}
const path = [];
let cur = target;
while (cur && !(cur[0] === start[0] && cur[1] === start[1])) {
  path.unshift(cur);
  cur = prev.get(cur[1] * W + cur[0]);
  if (cur === undefined) {
    console.log("NO PATH to level 1 node");
    process.exit(1);
  }
}
console.log(`path: ${path.length} cells`);

// --- drive the browser ---------------------------------------------------------
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 660, height: 500 } });
const errors = [];
page.on("pageerror", (e) => errors.push(`pageerror: ${e.message}`));
page.on("console", (m) => {
  if (m.type() === "error") errors.push(`console: ${m.text()}`);
});

await page.goto("http://localhost:5173/");
await page.evaluate(() => localStorage.clear());
await page.waitForTimeout(1200);
await page.keyboard.press("Enter");
await page.waitForTimeout(600);
await page.keyboard.press("KeyS");
await page.waitForTimeout(1000);
await page.screenshot({ path: `${outDir}/1-map-start.png` });

const KEY = { "0,-1": "ArrowUp", "0,1": "ArrowDown", "-1,0": "ArrowLeft", "1,0": "ArrowRight" };
const nextStep = (from) => {
  // walk the BFS parents from target back to `from`
  let cur = target;
  let step = null;
  while (cur) {
    const p = prev.get(cur[1] * W + cur[0]);
    if (p && p[0] === from[0] && p[1] === from[1]) {
      step = cur;
      break;
    }
    cur = p;
  }
  return step;
};
const getLoc = () =>
  page.evaluate(() => JSON.parse(localStorage.getItem("murphy.progress") ?? "{}").mapLocation);

for (let guard = 0; guard < 40; guard++) {
  const loc = (await getLoc()) ?? start;
  if (loc[0] === target[0] && loc[1] === target[1]) break;
  const step = nextStep(loc);
  if (!step) {
    console.log("off the planned path at", loc);
    break;
  }
  const key = KEY[`${step[0] - loc[0]},${step[1] - loc[1]}`];
  await page.keyboard.down(key);
  await page.waitForTimeout(120); // release before the next cell decision
  await page.keyboard.up(key);
  await page.waitForTimeout(300); // let the move finish
}
console.log("arrived at:", await getLoc(), "(want", target, ")");
await page.waitForTimeout(700); // let the info popup appear
await page.screenshot({ path: `${outDir}/3-map-popup.png` });

await page.keyboard.press("Enter");
await page.waitForTimeout(1500);
await page.screenshot({ path: `${outDir}/4-in-level.png` });

await page.keyboard.press("Escape"); // suicide
await page.waitForTimeout(2500);
await page.keyboard.press("KeyN"); // back to map
await page.waitForTimeout(1500);
await page.screenshot({ path: `${outDir}/5-back-on-map.png` });

console.log(errors.length ? "PAGE ERRORS:\n  " + errors.join("\n  ") : "no page errors");
await browser.close();
