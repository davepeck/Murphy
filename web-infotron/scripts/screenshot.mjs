// Drives Infotron headless for verification screenshots.
//   node scripts/screenshot.mjs <outdir> [level]
import { chromium } from "playwright";
import { mkdirSync } from "node:fs";

const outDir = process.argv[2] ?? "shots";
const level = parseInt(process.argv[3] ?? "1", 10);
mkdirSync(outDir, { recursive: true });

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 660, height: 500 } });
const errors = [];
page.on("pageerror", (e) => errors.push(`pageerror: ${e.message}`));
page.on("console", (m) => { if (m.type() === "error") errors.push(`console: ${m.text()}`); });

await page.goto("http://localhost:5174/");
await page.evaluate((lv) => {
  localStorage.clear();
  localStorage.setItem("infotron.progress", JSON.stringify({ current: lv, highest: lv }));
  localStorage.setItem("infotron.audio", JSON.stringify({ sound: true, music: false }));
}, level);
await page.reload();
await page.waitForTimeout(1500);
await page.screenshot({ path: `${outDir}/1-title.png` });

await page.keyboard.press("Enter");
await page.waitForTimeout(1500);
await page.screenshot({ path: `${outDir}/2-level-start.png` });

for (const [key, ms] of [["ArrowLeft", 700], ["ArrowUp", 500], ["ArrowRight", 900], ["ArrowDown", 500]]) {
  await page.keyboard.down(key);
  await page.waitForTimeout(ms);
  await page.keyboard.up(key);
  await page.waitForTimeout(150);
}
await page.screenshot({ path: `${outDir}/3-after-walk.png` });

await page.keyboard.press("KeyP");
await page.waitForTimeout(300);
await page.screenshot({ path: `${outDir}/4-paused.png` });
await page.keyboard.press("KeyP");

await page.keyboard.press("Escape"); // suicide
await page.waitForTimeout(1000);
await page.screenshot({ path: `${outDir}/5-death.png` });
await page.waitForTimeout(4500); // end-freeze, then auto-retry
await page.screenshot({ path: `${outDir}/6-retried.png` });

console.log(errors.length ? "PAGE ERRORS:\n  " + errors.join("\n  ") : "no page errors");
await browser.close();
