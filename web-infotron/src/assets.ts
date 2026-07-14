// Loads the extracted original resources: icl8 icons (by resource ID),
// levels, and PICT art. Icons are blitted into one runtime atlas canvas.

import type { InfotronLevelData } from "./engine/game";

const BASE = import.meta.env.BASE_URL + "assets";
const ICON = 32;

export async function loadImage(url: string): Promise<HTMLImageElement> {
  const img = new Image();
  img.src = url;
  await img.decode();
  return img;
}

export async function loadJSON<T>(url: string): Promise<T> {
  const res = await fetch(url);
  if (!res.ok) throw new Error(`fetch ${url}: ${res.status}`);
  return (await res.json()) as T;
}

export interface LevelIndexEntry {
  levelNumber: number;
  file: string;
  name: string;
  quota: number;
  sizeX: number;
  sizeY: number;
}

interface LevelJSON {
  levelNumber: number;
  name: string;
  sizeX: number;
  sizeY: number;
  murphy: { x: number; y: number };
  infotronQuota: number;
  board: number[][];
}

export class Assets {
  /** one big atlas canvas holding every icon */
  private atlas: HTMLCanvasElement | null = null;
  private atlasSlot = new Map<number, number>();
  private atlasCols = 16;
  private images = new Map<string, HTMLImageElement>();
  private levels = new Map<number, InfotronLevelData>();
  levelIndex: LevelIndexEntry[] = [];

  async loadCore(): Promise<void> {
    const iconIndex = await loadJSON<{ ids: number[] }>(`${BASE}/icons/index.json`);
    const ids = iconIndex.ids;
    this.levelIndex = await loadJSON<LevelIndexEntry[]>(`${BASE}/levels/index.json`);

    const imgs = await Promise.all(ids.map((id) => loadImage(`${BASE}/icons/${id}.png`)));
    const rows = Math.ceil(ids.length / this.atlasCols);
    const cv = document.createElement("canvas");
    cv.width = this.atlasCols * ICON;
    cv.height = rows * ICON;
    const g = cv.getContext("2d");
    if (!g) throw new Error("no 2d context for icon atlas");
    ids.forEach((id, i) => {
      g.drawImage(imgs[i] as HTMLImageElement, (i % this.atlasCols) * ICON, Math.floor(i / this.atlasCols) * ICON);
      this.atlasSlot.set(id, i);
    });
    this.atlas = cv;
  }

  hasIcon(id: number): boolean {
    return this.atlasSlot.has(id);
  }

  /** Draw the icon with the given resource ID at (x, y). */
  drawIcon(g: CanvasRenderingContext2D, id: number, x: number, y: number): void {
    const slot = this.atlasSlot.get(id);
    if (slot === undefined || !this.atlas) return; // unknown icon: leave black
    g.drawImage(
      this.atlas,
      (slot % this.atlasCols) * ICON,
      Math.floor(slot / this.atlasCols) * ICON,
      ICON,
      ICON,
      x,
      y,
      ICON,
      ICON
    );
  }

  async picture(id: number): Promise<HTMLImageElement> {
    return this.image(`pictures/${id}.png`);
  }

  async image(rel: string): Promise<HTMLImageElement> {
    const cached = this.images.get(rel);
    if (cached) return cached;
    const img = await loadImage(`${BASE}/${rel}`);
    this.images.set(rel, img);
    return img;
  }

  img(rel: string): HTMLImageElement {
    const i = this.images.get(rel);
    if (!i) throw new Error(`image not preloaded: ${rel}`);
    return i;
  }

  async level(levelNumber: number): Promise<InfotronLevelData> {
    const cached = this.levels.get(levelNumber);
    if (cached) return cached;
    const entry = this.levelIndex.find((l) => l.levelNumber === levelNumber);
    if (!entry) throw new Error(`no level ${levelNumber}`);
    const json = await loadJSON<LevelJSON>(`${BASE}/levels/${entry.file}`);
    const board: number[] = [];
    for (const row of json.board) board.push(...row);
    const data: InfotronLevelData = {
      id: json.levelNumber,
      name: json.name,
      sizeX: json.sizeX,
      sizeY: json.sizeY,
      murphy: [json.murphy.x - 1, json.murphy.y - 1], // 1-based -> 0-based
      quota: json.infotronQuota,
      board,
    };
    this.levels.set(levelNumber, data);
    return data;
  }
}
