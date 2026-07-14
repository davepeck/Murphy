// Builds web/public/assets from the original game data elsewhere in this repo.
//
//   pnpm assets
//
// Sources:
//   ../osx/Murphy/{infotron,aquatron,savanatron}.atlas/  305 named 32x32 tile PNGs per world
//   ../legacy/murphy/levels/*.lvl                        authoritative level dumps (keep
//                                                        hints + time limits that .mbl lost)
//   ../legacy/murphy/sounds/                             WAV sfx + music per world (kept as
//                                                        WAV: music segments chain gaplessly)
//   ../legacy/murphy/artwork/Interface,Misc,World/       interface BMPs

import { cpSync, mkdirSync, readdirSync, readFileSync, writeFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import Jimp from "jimp";

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, "..", "..");
const out = path.resolve(here, "..", "public", "assets");

const WORLDS = ["infotron", "aquatron", "savanatron"];
const ATLAS_COLUMNS = 20;
const TILE = 32;

function outDir(...parts) {
  const p = path.join(out, ...parts);
  mkdirSync(p, { recursive: true });
  return p;
}

// --- tile atlases -----------------------------------------------------------

async function buildAtlases() {
  const atlasOut = outDir("atlas");
  let names = null;
  for (const world of WORLDS) {
    const srcDir = path.join(repo, "osx", "Murphy", `${world}.atlas`);
    const files = readdirSync(srcDir)
      .filter((f) => f.endsWith(".png"))
      .sort();
    const worldNames = files.map((f) => f.replace(/\.png$/, ""));
    if (names === null) {
      names = worldNames;
    } else if (JSON.stringify(names) !== JSON.stringify(worldNames)) {
      throw new Error(`${world} atlas tile names differ from ${WORLDS[0]}`);
    }
    const rows = Math.ceil(files.length / ATLAS_COLUMNS);
    const sheet = new Jimp(ATLAS_COLUMNS * TILE, rows * TILE, 0x00000000);
    for (let i = 0; i < files.length; i++) {
      const tile = await Jimp.read(path.join(srcDir, files[i]));
      sheet.composite(tile, (i % ATLAS_COLUMNS) * TILE, Math.floor(i / ATLAS_COLUMNS) * TILE);
    }
    await sheet.writeAsync(path.join(atlasOut, `${world}.png`));
    console.log(`atlas/${world}.png  (${files.length} tiles)`);
  }
  writeFileSync(
    path.join(atlasOut, "tiles.json"),
    JSON.stringify({ tileSize: TILE, columns: ATLAS_COLUMNS, names }, null, 2)
  );
  console.log(`atlas/tiles.json  (${names.length} names)`);
}

// --- levels (.lvl -> .json) -------------------------------------------------

const FULL = 120; // declared grid size in every .lvl
const TOP_LEFT_BORDER = 30;
const TOP_RIGHT_BORDER = 32;
const BOTTOM_RIGHT_BORDER = 37;
const OLIVER = 9;

// Original campaign order, recovered from the 'MRlv' resources (IDs 301-330)
// inside legacy/murphy/MR-Dev2.cpt. File base names, in levelID order; the
// tail entries after the marker are bonus/cut content not reachable on the
// original world map.
const CAMPAIGN_ORDER = [
  "And So It Begins", // 1
  "Bolder", // 2
  "Achtung!", // 3
  "Inflamatory", // 4
  "Robots & Plasmoids", // 5
  "Excavation", // 6
  "Thriller", // 7
  "Golden Rule", // 8
  "Going Up", // 9
  "Gold Rush", // 10
  "Daylight", // 11 (hidden, switch-revealed)
  "Stress", // 12
  "Catacombs-Aquatron", // 13
  "Trapped Inside", // 14
  "It's Alive!!", // 15
  "Falling", // 16
  "Crystalline", // 18
  "Dot Dot", // 22
  "Dash Dash", // 23
  "Love Boat", // 24
  "Wizard", // 25 (hidden, switch-revealed)
  "Out In Out", // 26
  "In Out In", // 27 (recovered from MR-Dev2.cpt)
  "Labyrinth", // 28
  "No More Secrets", // 30
  // --- bonus: on no world-map node in the original ---
  "Short Circuit", // 20 (vestigial map reference)
  "Combinations", // 21 (vestigial map reference)
  "Insane Infotrons (tm)", // 17 (cut: replaced by Wizard)
  "test level", // 19 (dev laser test)
];

// The .lvl dump tool truncated hints at the first ':'; originals recovered
// from the archived resources.
const HINT_FIXES = {
  "And So It Begins": "Take heed: the puzzles have only just begun...",
  Crystalline: "T3: (try trance techno)",
  Combinations: "Pay attention: the correct drops will get you out.",
};

function parseLvl(text) {
  const lines = text.split("\n").map((l) => l.trim());
  const header = {
    name: lines[0],
    hint: lines[1],
    quota: parseInt(lines[2], 10),
    world: lines[3].toLowerCase(),
    hours: parseInt(lines[4], 10),
    mins: parseInt(lines[5], 10),
    secs: parseInt(lines[6], 10),
  };
  // 8 fields per cell, row-major (y outer, x inner); we need iconX/iconY only.
  const grid = new Array(FULL * FULL);
  let line = 9;
  for (let i = 0; i < FULL * FULL; i++, line += 8) {
    const iconX = parseInt(lines[line], 10);
    const iconY = parseInt(lines[line + 1], 10);
    grid[i] = iconY * 10 + iconX;
  }
  return { header, grid };
}

// The original game's GetLevelSize (MRPlay.c): scan row 0 for the top-right
// border, then down that column for the bottom-right border.
function levelSize(grid) {
  if (grid[0] !== TOP_LEFT_BORDER) return { width: FULL, height: FULL };
  let width = FULL;
  for (let x = 1; x < FULL; x++) {
    if (grid[x] === TOP_RIGHT_BORDER) {
      width = x + 1;
      break;
    }
  }
  let height = FULL;
  for (let y = 1; y < FULL; y++) {
    if (grid[y * FULL + (width - 1)] === BOTTOM_RIGHT_BORDER) {
      height = y + 1;
      break;
    }
  }
  return { width, height };
}

function buildLevels() {
  const levelsOut = outDir("levels");
  const sources = [
    path.join(repo, "legacy", "murphy", "levels"),
    path.join(here, "..", "recovered-levels"),
  ];
  const files = new Map(); // base name -> full path
  for (const src of sources) {
    for (const f of readdirSync(src).filter((f) => f.endsWith(".lvl"))) {
      const base = f.replace(/\.lvl$/, "");
      if (base === "World") continue; // the map room, not a playable level
      files.set(base, path.join(src, f));
    }
  }
  const order = new Map(CAMPAIGN_ORDER.map((name, i) => [name, i]));
  const bases = [...files.keys()].sort((a, b) => {
    const ia = order.get(a) ?? 999;
    const ib = order.get(b) ?? 999;
    return ia - ib || a.localeCompare(b);
  });
  const index = [];
  for (const base of bases) {
    // Original dumps are pure ASCII; the recovered levels are UTF-8 ("™").
    const { header, grid } = parseLvl(readFileSync(files.get(base), "utf8"));
    if (HINT_FIXES[base]) header.hint = HINT_FIXES[base];
    const { width, height } = levelSize(grid);
    const cropped = new Array(width * height);
    let start = null;
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const t = grid[y * FULL + x];
        cropped[y * width + x] = t;
        if (t === OLIVER) start = [x, y];
      }
    }
    if (!start) throw new Error(`${base}: no player start (tile 9)`);
    const timed = header.hours > 0 || header.mins > 0 || header.secs > 0;
    const level = {
      name: header.name,
      hint: header.hint,
      quota: header.quota,
      world: header.world,
      time: timed ? { hours: header.hours, mins: header.mins, secs: header.secs } : null,
      width,
      height,
      start,
      grid: cropped,
    };
    writeFileSync(path.join(levelsOut, `${base}.json`), JSON.stringify(level));
    index.push({ file: base, name: header.name, world: header.world, quota: header.quota, timed, time: level.time, hint: header.hint });
    console.log(`levels/${base}.json  (${width}x${height} ${header.world}${timed ? " timed" : ""})`);
  }
  writeFileSync(path.join(levelsOut, "levels.json"), JSON.stringify(index, null, 2));
}

// --- audio ------------------------------------------------------------------

function buildAudio() {
  const soundsSrc = path.join(repo, "legacy", "murphy", "sounds");
  for (const world of readdirSync(soundsSrc)) {
    let files;
    try {
      files = readdirSync(path.join(soundsSrc, world)).filter((f) => f.endsWith(".wav"));
    } catch {
      continue; // not a directory
    }
    const dest = outDir("sounds", world.toLowerCase());
    for (const f of files) cpSync(path.join(soundsSrc, world, f), path.join(dest, f));
    console.log(`sounds/${world.toLowerCase()}/  (${files.length} files)`);
  }
}

// --- the overworld map --------------------------------------------------------

// World.lvl is the map room: 120x120 cells, 8 fields per cell. Tile IDs use the
// 18-wide world sheet (iconY*18 + iconX), and the trigger/kind/levelID fields
// are live data (unlike game levels, where they are all zero).
const WORLD_TILES_WIDE = 18;
const OLIVER_START_TILE = 18; // icon (0,1): the map start marker

function buildWorld() {
  const worldOut = outDir("world");
  const text = readFileSync(path.join(repo, "legacy", "murphy", "levels", "World.lvl"), "utf8");
  const lines = text.split("\n").map((l) => l.trim());
  const n = FULL * FULL;
  const tiles = new Array(n);
  const trig = new Array(n);
  const trigTiles = new Array(n);
  const kind = new Array(n);
  const levelID = new Array(n);
  let start = null;
  let line = 9;
  for (let i = 0; i < n; i++, line += 8) {
    const iconX = parseInt(lines[line], 10);
    const iconY = parseInt(lines[line + 1], 10);
    tiles[i] = iconY * WORLD_TILES_WIDE + iconX;
    trig[i] = parseInt(lines[line + 2], 10);
    const trigX = parseInt(lines[line + 3], 10);
    const trigY = parseInt(lines[line + 4], 10);
    trigTiles[i] = trigY * WORLD_TILES_WIDE + trigX;
    kind[i] = parseInt(lines[line + 5], 10);
    levelID[i] = parseInt(lines[line + 7], 10);
    if (tiles[i] === OLIVER_START_TILE) start = [i % FULL, Math.floor(i / FULL)];
  }
  if (!start) throw new Error("World.lvl: no start marker (tile 18)");
  writeFileSync(
    path.join(worldOut, "map.json"),
    JSON.stringify({
      width: FULL,
      height: FULL,
      tilesWide: WORLD_TILES_WIDE,
      start,
      tiles,
      trig,
      trigTiles,
      kind,
      levelID,
    })
  );
  console.log(`world/map.json  (start at ${start})`);
}

// Bake the map Oliver's transparency: oliver-mini.bmp is two 24x24 frames
// stacked with a 1px gutter; oliver-mini-masks.bmp is black=opaque on white.
async function buildMiniOliver() {
  const worldOut = outDir("world");
  const art = path.join(repo, "legacy", "murphy", "artwork", "World");
  const img = await Jimp.read(path.join(art, "oliver-mini.bmp"));
  const mask = await Jimp.read(path.join(art, "oliver-mini-masks.bmp"));
  for (let frame = 0; frame < 2; frame++) {
    const out = new Jimp(24, 24, 0x00000000);
    const srcY = frame * 25; // 24px frame + 1px gutter
    for (let y = 0; y < 24; y++) {
      for (let x = 0; x < 24; x++) {
        const m = Jimp.intToRGBA(mask.getPixelColor(x, srcY + y));
        if (m.r < 128) out.setPixelColor((img.getPixelColor(x, srcY + y) | 0xff) >>> 0, x, y);
      }
    }
    await out.writeAsync(path.join(worldOut, `oliver-mini-${frame}.png`));
  }
  console.log("world/oliver-mini-{0,1}.png  (masked)");
}

// --- interface art ----------------------------------------------------------

async function buildInterface() {
  const groups = [
    ["artwork/Interface", "interface"],
    ["artwork/Misc", "misc"],
    ["artwork/World", "world"],
  ];
  for (const [srcRel, destRel] of groups) {
    const srcDir = path.join(repo, "legacy", "murphy", srcRel);
    const dest = outDir(destRel);
    const bmps = readdirSync(srcDir).filter((f) => f.endsWith(".bmp"));
    for (const f of bmps) {
      const img = await Jimp.read(path.join(srcDir, f));
      await img.writeAsync(path.join(dest, f.replace(/\.bmp$/, ".png")));
    }
    console.log(`${destRel}/  (${bmps.length} images)`);
  }
}

const only = process.argv[2];
if (!only || only === "atlas") await buildAtlases();
if (!only || only === "levels") buildLevels();
if (!only || only === "world") {
  buildWorld();
  await buildMiniOliver();
}
if (!only || only === "audio") buildAudio();
if (!only || only === "interface") await buildInterface();
console.log("assets done.");
