// Draws the scrolling level: tile grid, sprites, port overlay, with the
// original's proportional follow camera.

import type { Game } from "../engine/game";
import { T, TILE_SIZE, tileName } from "../engine/tiles";
import { isPortTile } from "../engine/kinds";
import { drawTile, type TileAtlas } from "../assets";

const SPRITE_MOVE_DELTA = 8;
const SPRITE_MOVE_DISTANCE = 100;

export class Camera {
  x = 0;
  y = 0;

  constructor(
    private viewW: number,
    private viewH: number,
    private mapW: number,
    private mapH: number
  ) {}

  centerOn(px: number, py: number): void {
    this.x = px + TILE_SIZE / 2 - this.viewW / 2;
    this.y = py + TILE_SIZE / 2 - this.viewH / 2;
    this.clamp();
  }

  /** Original proportional catch-up scroll (Scroll_LevWorldMove). */
  follow(px: number, py: number): void {
    const midX = this.x + (this.viewW - TILE_SIZE) / 2;
    const midY = this.y + (this.viewH - TILE_SIZE) / 2;
    this.x += Math.trunc((SPRITE_MOVE_DELTA * (px - midX)) / SPRITE_MOVE_DISTANCE);
    this.y += Math.trunc((SPRITE_MOVE_DELTA * (py - midY)) / SPRITE_MOVE_DISTANCE);
    this.clamp();
  }

  private clamp(): void {
    const maxX = Math.max(0, this.mapW - this.viewW);
    const maxY = Math.max(0, this.mapH - this.viewH);
    this.x = Math.min(Math.max(0, Math.round(this.x)), maxX);
    this.y = Math.min(Math.max(0, Math.round(this.y)), maxY);
  }
}

/** Substitutions for tiles whose idle image cycles globally. */
function displayTileId(g: Game, tile: number): number {
  if (tile === T.Infotron) return g.infotronImage;
  if (tile === T.Terminal) return g.terminalImage;
  return tile;
}

export function renderLevel(
  g2d: CanvasRenderingContext2D,
  atlas: TileAtlas,
  game: Game,
  cam: Camera,
  viewX: number,
  viewY: number,
  viewW: number,
  viewH: number
): void {
  g2d.save();
  g2d.beginPath();
  g2d.rect(viewX, viewY, viewW, viewH);
  g2d.clip();
  g2d.fillStyle = "#000";
  g2d.fillRect(viewX, viewY, viewW, viewH);
  g2d.translate(viewX - cam.x, viewY - cam.y);

  const h0 = Math.max(0, Math.floor(cam.x / TILE_SIZE));
  const v0 = Math.max(0, Math.floor(cam.y / TILE_SIZE));
  const h1 = Math.min(game.width - 1, Math.ceil((cam.x + viewW) / TILE_SIZE));
  const v1 = Math.min(game.height - 1, Math.ceil((cam.y + viewH) / TILE_SIZE));

  // Tile layer.
  for (let v = v0; v <= v1; v++) {
    for (let h = h0; h <= h1; h++) {
      const t = displayTileId(game, game.getTile(h, v));
      drawTile(g2d, atlas, tileName(t), h * TILE_SIZE, v * TILE_SIZE);
    }
  }

  // Sprites (Oliver first in list; drawn in list order).
  for (const s of game.sprites) {
    const img = displayTileId(game, s.image);
    drawTile(g2d, atlas, tileName(img), s.x, s.y);
  }

  // Port tiles draw over sprites (the original loads them with a solid mask
  // and puts Oliver "under tiles", so he passes beneath ports).
  for (let v = v0; v <= v1; v++) {
    for (let h = h0; h <= h1; h++) {
      const t = game.getTile(h, v);
      if (isPortTile(t)) {
        drawTile(g2d, atlas, tileName(t), h * TILE_SIZE, v * TILE_SIZE);
      }
    }
  }

  g2d.restore();
}
