// The overworld map model, ported from MRMap.c: an 120x120 grid of 24px tiles
// on an 18-wide tile sheet. Passability is a pure function of a tile's
// position in the sheet (MakePieceKinds — verified as the shipped mapping);
// switches swap every same-trigger cell between its two icons.

export interface WorldData {
  width: number;
  height: number;
  tilesWide: number;
  start: [number, number];
  tiles: number[];
  trig: number[];
  trigTiles: number[];
  kind: number[];
  levelID: number[];
}

export const enum MapPiece {
  Passable = 1,
  Switch = 2,
  Level = 3,
  PassedLevel = 4,
  NoPass = 5,
}

export const WORLD_TILE_SIZE = 24;
const TILES_WIDE = 18;
const OLIVER_START_TILE = 18;
const PASSED_ROW_OFFSET = 3 * TILES_WIDE; // passed level icons live 3 rows down

/**
 * levelID -> level file, recovered from the 'MRlv' resources (301-330) in
 * MR-Dev2.cpt. IDs missing here (and levelID 0) are "Nonexistant Level"
 * markers of the unfinished second map area.
 */
export const LEVEL_FILES: Readonly<Record<number, string>> = {
  1: "And So It Begins",
  2: "Bolder",
  3: "Achtung!",
  4: "Inflamatory",
  5: "Robots & Plasmoids",
  6: "Excavation",
  7: "Thriller",
  8: "Golden Rule",
  9: "Going Up",
  10: "Gold Rush",
  11: "Daylight",
  12: "Stress",
  13: "Catacombs-Aquatron",
  14: "Trapped Inside",
  15: "It's Alive!!",
  16: "Falling",
  17: "Insane Infotrons (tm)",
  18: "Crystalline",
  19: "test level",
  20: "Short Circuit",
  21: "Combinations",
  22: "Dot Dot",
  23: "Dash Dash",
  24: "Love Boat",
  25: "Wizard",
  26: "Out In Out",
  27: "In Out In",
  28: "Labyrinth",
  30: "No More Secrets",
};

/** Tile class from sheet position (MakePieceKinds; kUsableTilesWide = 9). */
export function classifyWorldTile(tileID: number): MapPiece {
  const col = tileID % TILES_WIDE;
  const row = Math.floor(tileID / TILES_WIDE);
  if (col === 1 && row <= 2) return MapPiece.Level;
  if (col === 1 && row >= 3 && row <= 5) return MapPiece.PassedLevel;
  if (row === 6 && col <= 8) return MapPiece.Switch;
  if ((row === 7 || row === 8) && col <= 8) return MapPiece.Passable;
  return MapPiece.NoPass;
}

export class WorldMap {
  readonly width: number;
  readonly height: number;
  readonly start: [number, number];
  readonly tiles: Int16Array;
  readonly trigTiles: Int16Array;
  private readonly trig: Int32Array;
  private readonly levelID: Int32Array;

  constructor(data: WorldData, passedFiles: Set<string>, thrownTrigs: Set<number>) {
    this.width = data.width;
    this.height = data.height;
    this.start = data.start;
    this.tiles = Int16Array.from(data.tiles);
    this.trigTiles = Int16Array.from(data.trigTiles);
    this.trig = Int32Array.from(data.trig);
    this.levelID = Int32Array.from(data.levelID);

    // The start marker displays its trigger icon instead (SetUpTileData).
    const si = data.start[1] * this.width + data.start[0];
    if (this.tiles[si] === OLIVER_START_TILE) {
      this.tiles[si] = this.trigTiles[si] as number;
    }

    // Passed levels show the icon 3 sheet-rows down (AdjustLevelID) — applied
    // to both icon layers so switch swaps preserve the marking.
    for (let i = 0; i < this.tiles.length; i++) {
      if (this.isPassedLevelCell(i, passedFiles)) {
        if (classifyWorldTile(this.tiles[i] as number) === MapPiece.Level) {
          this.tiles[i] = (this.tiles[i] as number) + PASSED_ROW_OFFSET;
        }
        if (classifyWorldTile(this.trigTiles[i] as number) === MapPiece.Level) {
          this.trigTiles[i] = (this.trigTiles[i] as number) + PASSED_ROW_OFFSET;
        }
      }
    }

    // Re-apply the player's persisted switch throws.
    for (const t of thrownTrigs) this.swapTrigger(t);
  }

  private isPassedLevelCell(i: number, passedFiles: Set<string>): boolean {
    const id = this.levelID[i] as number;
    if (id <= 0) return false;
    const file = LEVEL_FILES[id];
    return file !== undefined && passedFiles.has(file);
  }

  index(x: number, y: number): number {
    return y * this.width + x;
  }

  tileAt(x: number, y: number): number {
    if (x < 0 || y < 0 || x >= this.width || y >= this.height) return 0;
    return this.tiles[this.index(x, y)] as number;
  }

  pieceAt(x: number, y: number): MapPiece {
    if (x < 0 || y < 0 || x >= this.width || y >= this.height) return MapPiece.NoPass;
    return classifyWorldTile(this.tileAt(x, y));
  }

  trigAt(x: number, y: number): number {
    return this.trig[this.index(x, y)] as number;
  }

  levelIdAt(x: number, y: number): number {
    return this.levelID[this.index(x, y)] as number;
  }

  levelFileAt(x: number, y: number): string | null {
    return LEVEL_FILES[this.levelIdAt(x, y)] ?? null;
  }

  /**
   * Throw the switch at (x,y): swap p <-> trigP on every cell sharing its
   * trigger ID. Returns the trigger ID, or 0 if this switch is inert
   * (the original had no trig==0 guard; swapping 14k trig-0 cells would
   * destroy the map, so the port declines).
   */
  triggerSwitch(x: number, y: number): number {
    const t = this.trigAt(x, y);
    if (t === 0) return 0;
    this.swapTrigger(t);
    return t;
  }

  private swapTrigger(t: number): void {
    for (let i = 0; i < this.tiles.length; i++) {
      if ((this.trig[i] as number) === t) {
        const a = this.tiles[i] as number;
        this.tiles[i] = this.trigTiles[i] as number;
        this.trigTiles[i] = a;
      }
    }
  }
}
