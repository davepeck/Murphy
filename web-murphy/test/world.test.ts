import { describe, it, expect } from "vitest";
import { readFileSync } from "node:fs";
import path from "node:path";
import {
  WorldMap,
  MapPiece,
  classifyWorldTile,
  LEVEL_FILES,
  type WorldData,
} from "../src/engine/world";

const data = JSON.parse(
  readFileSync(path.resolve(__dirname, "..", "public", "assets", "world", "map.json"), "utf8")
) as WorldData;

function makeWorld(passed: string[] = [], throws: number[] = []): WorldMap {
  return new WorldMap(data, new Set(passed), new Set(throws));
}

describe("world tile classification", () => {
  it("classifies by sheet position", () => {
    expect(classifyWorldTile(1)).toBe(MapPiece.Level); // (1,0)
    expect(classifyWorldTile(19)).toBe(MapPiece.Level); // (1,1)
    expect(classifyWorldTile(55)).toBe(MapPiece.PassedLevel); // (1,3)
    expect(classifyWorldTile(6 * 18 + 0)).toBe(MapPiece.Switch); // (0,6)
    expect(classifyWorldTile(6 * 18 + 9)).toBe(MapPiece.NoPass); // (9,6): beyond usable cols
    expect(classifyWorldTile(7 * 18 + 4)).toBe(MapPiece.Passable); // (4,7)
    expect(classifyWorldTile(0)).toBe(MapPiece.NoPass);
    expect(classifyWorldTile(238)).toBe(MapPiece.NoPass); // ocean
  });
});

describe("world map", () => {
  it("replaces the start marker with its trigger tile", () => {
    const w = makeWorld();
    expect(w.tileAt(58, 54)).toBe(130); // 7*18+4, a walkway
    expect(w.pieceAt(58, 54)).toBe(MapPiece.Passable);
  });

  it("level 1 node is adjacent to the start island", () => {
    const w = makeWorld();
    expect(w.pieceAt(61, 50)).toBe(MapPiece.Level);
    expect(w.levelIdAt(61, 50)).toBe(1);
    expect(LEVEL_FILES[1]).toBe("And So It Begins");
  });

  it("throwing a switch swaps all same-trigger cells and is reversible", () => {
    const w = makeWorld();
    // find any switch cell with a nonzero trig
    let sx = -1;
    let sy = -1;
    outer: for (let y = 0; y < 120; y++) {
      for (let x = 0; x < 120; x++) {
        if (w.pieceAt(x, y) === MapPiece.Switch && w.trigAt(x, y) > 0) {
          sx = x;
          sy = y;
          break outer;
        }
      }
    }
    expect(sx).toBeGreaterThanOrEqual(0);
    const before = Array.from(w.tiles);
    const t = w.triggerSwitch(sx, sy);
    expect(t).toBeGreaterThan(0);
    expect(Array.from(w.tiles)).not.toEqual(before);
    w.triggerSwitch(sx, sy);
    expect(Array.from(w.tiles)).toEqual(before);
  });

  it("declines the trig-0 landmine switch at (43,30)", () => {
    const w = makeWorld();
    expect(w.pieceAt(43, 30)).toBe(MapPiece.Switch);
    expect(w.trigAt(43, 30)).toBe(0);
    const before = Array.from(w.tiles);
    expect(w.triggerSwitch(43, 30)).toBe(0);
    expect(Array.from(w.tiles)).toEqual(before);
  });

  it("switch trig 9 reveals the hidden Daylight node at (91,15)", () => {
    const w = makeWorld();
    expect(w.pieceAt(91, 15)).not.toBe(MapPiece.Level); // hidden initially
    const w2 = makeWorld([], [9]);
    expect(w2.pieceAt(91, 15)).toBe(MapPiece.Level);
    expect(w2.levelIdAt(91, 15)).toBe(11);
    expect(LEVEL_FILES[11]).toBe("Daylight");
  });

  it("passed levels show the passed icon and stay walkable", () => {
    const w = makeWorld(["And So It Begins"]);
    expect(w.pieceAt(61, 50)).toBe(MapPiece.PassedLevel);
  });

  it("every reachable level opens up at switch fixpoint", () => {
    // throw every trigger the map uses, then check all 25 mapped level nodes
    const allTrigs = [1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 17, 18, 20, 21, 55];
    const w = makeWorld([], allTrigs);
    const ids = new Set<number>();
    for (let y = 0; y < 120; y++) {
      for (let x = 0; x < 120; x++) {
        const p = w.pieceAt(x, y);
        if ((p === MapPiece.Level || p === MapPiece.PassedLevel) && w.levelIdAt(x, y) > 0) {
          ids.add(w.levelIdAt(x, y));
        }
      }
    }
    const expected = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 22, 23, 24, 25, 26, 27, 28, 30];
    for (const id of expected) expect(ids.has(id), `levelID ${id}`).toBe(true);
    // and every one of them maps to a real level file
    for (const id of expected) expect(LEVEL_FILES[id], `file for ${id}`).toBeDefined();
  });
});
