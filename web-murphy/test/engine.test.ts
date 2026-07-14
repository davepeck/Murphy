import { describe, it, expect } from "vitest";
import { readFileSync } from "node:fs";
import path from "node:path";
import { Game } from "../src/engine/game";
import type { LevelData } from "../src/engine/level";
import { T, Dir, TILE_NAMES } from "../src/engine/tiles";
import type { SoundEvent } from "../src/engine/events";

const ASSETS = path.resolve(__dirname, "..", "public", "assets");

// Deterministic rng for tests.
const rngMin = (min: number) => min;

const CHARS: Record<string, number> = {
  "#": T.TopBorder,
  ".": T.Blank,
  ":": T.Circuit,
  Z: T.Zonk,
  U: T.UpZonk,
  I: T.Infotron,
  O: T.Oliver,
  E: T.Exit,
  Q: T.Quark,
  S: T.UpScissor,
  R: T.RedDisk,
  Y: T.YellowDisk,
  G: T.GreenDisk,
  D: T.GoldDisk,
  T: T.Terminal,
  C: T.Chip,
  ">": T.RightPort,
  "<": T.LeftPort,
  B: T.Bug,
};

function makeLevel(rows: string[], quota = 99): LevelData {
  const height = rows.length;
  const width = (rows[0] as string).length;
  const grid: number[] = [];
  let start: [number, number] = [1, 1];
  rows.forEach((row, v) => {
    [...row].forEach((ch, h) => {
      const t = CHARS[ch];
      if (t === undefined) throw new Error(`bad char ${ch}`);
      if (t === T.Oliver) start = [h, v];
      grid.push(t);
    });
  });
  return { name: "test", hint: "", quota, world: "infotron", time: null, width, height, start, grid };
}

function run(g: Game, frames: number, dir: Dir = Dir.None, special = false): void {
  for (let i = 0; i < frames; i++) g.frame({ dir, special });
}

describe("tile name table", () => {
  it("matches the packed atlas exactly", () => {
    const meta = JSON.parse(readFileSync(path.join(ASSETS, "atlas", "tiles.json"), "utf8"));
    const atlasNames = new Set<string>(meta.names);
    for (const n of TILE_NAMES) expect(atlasNames.has(n), `missing ${n}`).toBe(true);
    expect(meta.names.length).toBe(TILE_NAMES.length);
  });
});

describe("level loading", () => {
  it("loads Bolder with the right header and start", () => {
    const bolder = JSON.parse(
      readFileSync(path.join(ASSETS, "levels", "Bolder.json"), "utf8")
    ) as LevelData;
    expect(bolder.quota).toBe(20);
    expect(bolder.width).toBe(31);
    expect(bolder.height).toBe(29);
    expect(bolder.start).toEqual([15, 14]);
    expect(bolder.time).toEqual({ hours: 0, mins: 8, secs: 0 });
    const g = new Game(bolder, { rand: rngMin });
    expect(g.getTile(15, 14)).toBe(T.Blank); // start marker converted
    run(g, 10);
    expect(g.oliver).not.toBeNull();
  });
});

describe("falling", () => {
  it("a zonk falls into a blank column and lands with a thud", () => {
    const events: SoundEvent[] = [];
    const g = new Game(
      makeLevel(["#####", "#.Z.#", "#...#", "#...#", "#O..#", "#####"]),
      { rand: rngMin, sound: (e) => events.push(e) }
    );
    run(g, 20);
    expect(g.getTile(2, 1)).toBe(T.Blank);
    expect(g.getTile(2, 4)).toBe(T.Zonk); // rests on the bottom border
    expect(events.some((e) => e.kind === "thud")).toBe(true);
  });

  it("an up-zonk floats upward", () => {
    const g = new Game(makeLevel(["#####", "#...#", "#...#", "#U..#", "#O..#", "#####"]), {
      rand: rngMin,
    });
    run(g, 20);
    expect(g.getTile(1, 1)).toBe(T.UpZonk);
    expect(g.getTile(1, 3)).toBe(T.Blank);
  });

  it("a zonk rolls off a rounded piece (left first)", () => {
    const g = new Game(makeLevel(["#####", "#.Z.#", "#.C.#", "#...#", "#..O#", "#####"]), {
      rand: rngMin,
    });
    run(g, 40);
    // rolled left off the chip, then fell to the floor
    expect(g.getTile(1, 4)).toBe(T.Zonk);
    expect(g.getTile(2, 1)).toBe(T.Blank);
  });

  it("a falling zonk kills Oliver beneath it", () => {
    const events: SoundEvent[] = [];
    const g = new Game(makeLevel(["#####", "#Z..#", "#...#", "#O..#", "#####"]), {
      rand: rngMin,
      sound: (e) => events.push(e),
    });
    run(g, 30);
    expect(events.some((e) => e.kind === "oliverDied")).toBe(true);
  });
});

describe("Oliver movement", () => {
  it("walks right one tile in 4 frames and eats circuit silently", () => {
    const g = new Game(makeLevel(["#####", "#O:.#", "#####"]), { rand: rngMin });
    run(g, 1); // initial setup frame
    run(g, 4, Dir.Right); // decision frame + 3 gliding frames
    const o = g.oliver;
    expect(o).not.toBeNull();
    expect(o?.loc).toEqual({ h: 2, v: 1 }); // loc updates at move start
    expect(g.getTile(1, 1)).toBe(T.Blank); // origin blanked
  });

  it("collects an infotron and opens the exit on quota", () => {
    const events: SoundEvent[] = [];
    const g = new Game(makeLevel(["#####", "#OIE#", "#####"]), {
      rand: rngMin,
      sound: (e) => events.push(e),
    });
    (g as unknown as { infotronCount: number }).infotronCount = 1;
    run(g, 1);
    run(g, 6, Dir.Right);
    expect(g.infotronCount).toBe(0);
    expect(events.some((e) => e.kind === "gotLastInfotron")).toBe(true);
    run(g, 40); // exit opening animation
    expect(g.getTile(3, 1)).toBe(T.LastExit);
    run(g, 8, Dir.Right);
    expect(g.wonLevel).toBe(true);
    expect(g.oliver).toBeNull();
  });

  it("pushes a zonk horizontally at half speed", () => {
    const g = new Game(makeLevel(["#####", "#OZ.#", "#####"]), { rand: rngMin });
    run(g, 1);
    run(g, 16, Dir.Right); // 8 frames per tile while pushing
    expect(g.getTile(3, 1)).toBe(T.Zonk);
    expect(g.oliver?.loc).toEqual({ h: 2, v: 1 });
  });

  it("cannot push a zonk vertically", () => {
    const g = new Game(makeLevel(["#####", "#Z..#", "#O..#", "#####"]), { rand: rngMin });
    run(g, 1);
    run(g, 10, Dir.Up);
    expect(g.getTile(1, 1)).toBe(T.Zonk);
    expect(g.oliver?.loc).toEqual({ h: 1, v: 2 });
  });

  it("traverses a directional port only the right way", () => {
    const g = new Game(makeLevel(["#####", "#O>.#", "#####"]), { rand: rngMin });
    run(g, 1);
    run(g, 12, Dir.Right);
    expect(g.oliver?.loc).toEqual({ h: 3, v: 1 });
    expect(g.getTile(2, 1)).toBe(T.RightPort); // port survives traversal
    // now try to come back left through the right-port: blocked
    run(g, 12, Dir.Left);
    expect(g.oliver?.loc).toEqual({ h: 3, v: 1 });
  });

  it("snaps (special+direction) an adjacent infotron without moving", () => {
    const events: SoundEvent[] = [];
    const g = new Game(makeLevel(["#####", "#OI.#", "#####"]), {
      rand: rngMin,
      sound: (e) => events.push(e),
    });
    run(g, 1);
    run(g, 40, Dir.Right, true);
    expect(g.oliver?.loc).toEqual({ h: 1, v: 1 }); // did not move
    expect(g.getTile(2, 1)).toBe(T.Blank); // infotron zapped away
    expect(events.some((e) => e.kind === "gotInfotron" || e.kind === "gotLastInfotron")).toBe(true);
  });
});

describe("explosions", () => {
  it("terminal detonates all yellow disks 3x3", () => {
    const g = new Game(makeLevel(["######", "#OT.Y#", "#::::#", "######"]), { rand: rngMin });
    run(g, 1);
    run(g, 4, Dir.Right); // bump the terminal
    run(g, 40);
    expect(g.getTile(4, 1)).toBe(T.Blank); // disk gone
    expect(g.getTile(3, 2)).toBe(T.Blank); // 3x3 took the circuit below-left of disk
    expect(g.getTile(1, 2)).toBe(T.Circuit); // far cell untouched
  });

  it("an exploded quark leaves an infotron", () => {
    const g = new Game(makeLevel(["#####", "#Z.O#", "#...#", "#Q..#", "#:::#", "#####"]), {
      rand: rngMin,
    });
    // zonk falls onto the quark's cell region; quark wanders but in this box
    run(g, 120);
    // eventually a quark explosion leaves an infotron somewhere in the box
    let found = false;
    for (let v = 1; v < 5 && !found; v++) {
      for (let h = 1; h < 4 && !found; h++) {
        if (g.getTile(h, v) === T.Infotron || g.getTile(h, v) === T.Phantom) found = true;
      }
    }
    expect(found).toBe(true);
  });
});

describe("enemies", () => {
  it("a quark wanders without leaving debris", () => {
    const g = new Game(makeLevel(["######", "#....#", "#.Q..#", "#....#", "#O...#", "######"]), {
      rand: rngMin,
    });
    run(g, 100);
    // The quark should still exist as a sprite and the grid should contain
    // only blanks/phantoms in its roaming area.
    const movers = g.sprites.filter((s) => s.constructor.name === "MoverSprite");
    expect(movers.length).toBe(1);
    let weird = 0;
    for (let v = 1; v <= 3; v++) {
      for (let h = 1; h <= 4; h++) {
        const t = g.getTile(h, v);
        if (t !== T.Blank && t !== T.Phantom) weird += 1;
      }
    }
    expect(weird).toBe(0);
  });

  it("timer death fires at 0:00", () => {
    const events: SoundEvent[] = [];
    const level = makeLevel(["####", "#O:#", "####"]);
    level.time = { hours: 0, mins: 0, secs: 2 };
    const g = new Game(level, { rand: rngMin, sound: (e) => events.push(e) });
    run(g, 120); // 4 seconds of frames
    expect(events.some((e) => e.kind === "oliverDied")).toBe(true);
  });
});
