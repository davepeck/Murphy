// Loads and caches the converted original game assets from /assets.

import type { LevelData, LevelIndexEntry } from "./engine/level";
import { validateLevel } from "./engine/level";

export interface TileAtlas {
  image: HTMLImageElement;
  tileSize: number;
  columns: number;
  /** tile name -> index into the sheet grid */
  index: Map<string, number>;
}

export interface AtlasMeta {
  tileSize: number;
  columns: number;
  names: string[];
}

const BASE = import.meta.env.BASE_URL + "assets";

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

export class Assets {
  private atlases = new Map<string, TileAtlas>();
  private atlasMeta: AtlasMeta | null = null;
  private images = new Map<string, HTMLImageElement>();
  private levels = new Map<string, LevelData>();
  levelIndex: LevelIndexEntry[] = [];

  async loadCore(): Promise<void> {
    this.atlasMeta = await loadJSON<AtlasMeta>(`${BASE}/atlas/tiles.json`);
    this.levelIndex = await loadJSON<LevelIndexEntry[]>(`${BASE}/levels/levels.json`);
  }

  async atlas(world: string): Promise<TileAtlas> {
    const cached = this.atlases.get(world);
    if (cached) return cached;
    if (!this.atlasMeta) throw new Error("loadCore() first");
    const image = await loadImage(`${BASE}/atlas/${world}.png`);
    const index = new Map<string, number>();
    this.atlasMeta.names.forEach((n, i) => index.set(n, i));
    const atlas: TileAtlas = {
      image,
      tileSize: this.atlasMeta.tileSize,
      columns: this.atlasMeta.columns,
      index,
    };
    this.atlases.set(world, atlas);
    return atlas;
  }

  /** Interface / misc / world art by path relative to assets root, e.g. "interface/font-large.png". */
  async image(rel: string): Promise<HTMLImageElement> {
    const cached = this.images.get(rel);
    if (cached) return cached;
    const img = await loadImage(`${BASE}/${rel}`);
    this.images.set(rel, img);
    return img;
  }

  /** A previously loaded image (throws if not loaded yet). */
  img(rel: string): HTMLImageElement {
    const i = this.images.get(rel);
    if (!i) throw new Error(`image not preloaded: ${rel}`);
    return i;
  }

  async level(file: string): Promise<LevelData> {
    const cached = this.levels.get(file);
    if (cached) return cached;
    const data = validateLevel(
      await loadJSON<LevelData>(`${BASE}/levels/${encodeURIComponent(file)}.json`)
    );
    this.levels.set(file, data);
    return data;
  }
}

export function drawTile(
  ctx: CanvasRenderingContext2D,
  atlas: TileAtlas,
  name: string,
  x: number,
  y: number
): void {
  const i = atlas.index.get(name);
  if (i === undefined) throw new Error(`unknown tile: ${name}`);
  const s = atlas.tileSize;
  ctx.drawImage(atlas.image, (i % atlas.columns) * s, Math.floor(i / atlas.columns) * s, s, s, x, y, s, s);
}
