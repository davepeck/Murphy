// Level data as produced by scripts/build-assets.mjs from the original .lvl dumps.

export interface LevelTime {
  hours: number;
  mins: number;
  secs: number;
}

export interface LevelData {
  name: string;
  hint: string;
  quota: number;
  world: string; // "infotron" | "aquatron" | "savanatron"
  time: LevelTime | null;
  width: number;
  height: number;
  start: [number, number]; // [x, y] of tile 9
  grid: number[]; // row-major tile IDs, width*height
}

export interface LevelIndexEntry {
  file: string;
  name: string;
  world: string;
  quota: number;
  timed: boolean;
  hint: string;
}

export function validateLevel(l: LevelData): LevelData {
  if (l.grid.length !== l.width * l.height) {
    throw new Error(`${l.name}: grid size mismatch`);
  }
  return l;
}
