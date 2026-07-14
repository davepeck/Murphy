// Drives the game in headless Chromium for verification screenshots.
//   node scripts/screenshot.mjs <outdir> [level-file]
import { chromium } from "playwright";
import { mkdirSync } from "node:fs";

const outDir = process.argv[2] ?? "shots";
const levelFile = process.argv[3] ?? "Bolder";
mkdirSync(outDir, { recursive: true });

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 660, height: 500 } });
const errors = [];
page.on("pageerror", (e) => errors.push(`pageerror: ${e.message}`));
page.on("console", (m) => {
  if (m.type() === "error") errors.push(`console: ${m.text()}`);
});

await page.goto("http://localhost:5173/");
await page.waitForTimeout(1500);
await page.screenshot({ path: `${outDir}/1-title.png` });

// title -> menu
await page.keyboard.press("Enter");
await page.waitForTimeout(800);
await page.screenshot({ path: `${outDir}/2-menu.png` });

// menu -> level select
await page.keyboard.press("KeyS");
await page.waitForTimeout(800);
await page.screenshot({ path: `${outDir}/3-select.png` });

// pick the requested level by walking down the list
const idx = await page.evaluate(async (file) => {
  const res = await fetch("/assets/levels/levels.json");
  const list = await res.json();
  return list.findIndex((l) => l.file === file);
}, levelFile);
for (let i = 0; i < idx; i++) {
  await page.keyboard.press("ArrowDown");
  await page.waitForTimeout(30);
}
await page.waitForTimeout(300);
await page.screenshot({ path: `${outDir}/4-select-level.png` });

// start the level
await page.keyboard.press("Enter");
await page.waitForTimeout(1200);
await page.screenshot({ path: `${outDir}/5-level-start.png` });

// walk around: right, down, left, up
for (const [key, ms] of [
  ["ArrowRight", 900],
  ["ArrowDown", 900],
  ["ArrowLeft", 600],
  ["ArrowUp", 600],
]) {
  await page.keyboard.down(key);
  await page.waitForTimeout(ms);
  await page.keyboard.up(key);
}
await page.screenshot({ path: `${outDir}/6-after-walk.png` });

// pause
await page.keyboard.press("KeyP");
await page.waitForTimeout(300);
await page.screenshot({ path: `${outDir}/7-paused.png` });
await page.keyboard.press("KeyP");

// suicide -> death dialog
await page.keyboard.press("Escape");
await page.waitForTimeout(2500);
await page.screenshot({ path: `${outDir}/8-death-dialog.png` });

if (errors.length) {
  console.log("PAGE ERRORS:");
  for (const e of errors) console.log("  " + e);
} else {
  console.log("no page errors");
}
await browser.close();
