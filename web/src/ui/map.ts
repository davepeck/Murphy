// The map room: walk the overworld, throw switches to open routes, stand on a
// level beacon to get its info card, and enter levels. Ported from MRMap.c.

import type { GameContext, Screen } from "../screen";
import type { LevelIndexEntry } from "../engine/level";
import { WorldMap, MapPiece, WORLD_TILE_SIZE, type WorldData } from "../engine/world";
import { Camera } from "../render/levelRenderer";
import { BubbleOliver } from "../render/bubble";
import { BitmapFont } from "../render/font";
import { MAP_ROOM_MUSIC, UI_SOUNDS } from "../audio";
import { defaultRng } from "../engine/rng";
import { loadJSON } from "../assets";
import { mapLocation, passedLevels, setMapLocation, thrownSwitches, toggleThrow } from "../save";

const VIEW_X = 40;
const VIEW_W = 560;
const VIEW_H = 480;
const MOVE_DELTA = 4; // px/frame at 30 fps; 6 frames per 24px cell
const INFO_DELAY_TICKS = 15;
const SPRITE_FRAME_TICKS = 30; // mini-Oliver: 500 ms/frame
const TEXT_COLOR = "#98CCCC";

type DirAction = "up" | "down" | "left" | "right";
const DIRS: Record<DirAction, [number, number]> = {
  up: [0, -1],
  down: [0, 1],
  left: [-1, 0],
  right: [1, 0],
};

/** Ambient world-tile animations (Tile_MapChangeProc). */
interface AnimGroup {
  first: number;
  last: number;
  delay: number; // ticks per step
  step: number;
  lastTick: number;
}

const ANIM_GROUPS: Omit<AnimGroup, "step" | "lastTick">[] = [
  { first: 238, last: 241, delay: 12 }, // ocean
  { first: 1, last: 8, delay: 10 }, // level beacons, rows 0-5 of the sheet
  { first: 19, last: 26, delay: 10 },
  { first: 37, last: 44, delay: 10 },
  { first: 55, last: 62, delay: 10 },
  { first: 73, last: 80, delay: 10 },
  { first: 91, last: 98, delay: 10 },
  { first: 256, last: 259, delay: 17 }, // whirlpool
  { first: 186, last: 188, delay: 15 }, // "thing 2"
  { first: 288, last: 293, delay: 15 }, // machine
];

export class MapScreen implements Screen {
  private world: WorldMap | null = null;
  private tileSheet: HTMLImageElement | null = null;
  private mini: [HTMLImageElement, HTMLImageElement] | null = null;
  private cam: Camera | null = null;
  private font: BitmapFont | null = null;
  private bubble: BubbleOliver | null = null;

  private tickCount = 0;
  private parity = 0;
  private keyStack: DirAction[] = [];

  // player position in world pixels; cell = px/24
  private px = 0;
  private py = 0;
  private dx = 0;
  private dy = 0;
  private cellX = 0;
  private cellY = 0;
  private prevCell: [number, number] | null = null;
  private cameFrom: [number, number] | null = null; // for unpassed-level gating

  private onLevelTicks = -1;
  private popupVisible = false;
  private passed = new Set<string>();

  private anims: AnimGroup[] = ANIM_GROUPS.map((g) => ({ ...g, step: 0, lastTick: 0 }));
  private grabThing = { step: 0, lastTick: 0 }; // tiles 168-170, random trigger
  private spriteFrame = 0;
  private lastSpriteTick = 0;

  async enter(ctx: GameContext): Promise<void> {
    const a = ctx.assets;
    const data = await loadJSON<WorldData>(
      `${import.meta.env.BASE_URL}assets/world/map.json`
    );
    await Promise.all([
      a.image("world/world-tiles.png"),
      a.image("world/oliver-mini-0.png"),
      a.image("world/oliver-mini-1.png"),
      a.image("interface/bar-map-room.png"),
      a.image("interface/status-bar-map.png"),
      a.image("interface/back.png"),
      a.image("interface/back-select.png"),
      a.image("interface/level-stats.png"),
      a.image("interface/level-stats-select.png"),
      a.image("interface/font-large.png"),
      a.image("interface/oliver-basics.png"),
    ]);
    this.tileSheet = a.img("world/world-tiles.png");
    this.mini = [a.img("world/oliver-mini-0.png"), a.img("world/oliver-mini-1.png")];
    this.font = new BitmapFont(a.img("interface/font-large.png"));
    this.bubble = new BubbleOliver(a.img("interface/oliver-basics.png"));

    this.passed = passedLevels();
    this.world = new WorldMap(data, this.passed, thrownSwitches());

    const loc = mapLocation() ?? this.world.start;
    this.cellX = loc[0];
    this.cellY = loc[1];
    this.px = this.cellX * WORLD_TILE_SIZE;
    this.py = this.cellY * WORLD_TILE_SIZE;
    this.cam = new Camera(
      VIEW_W,
      VIEW_H,
      this.world.width * WORLD_TILE_SIZE,
      this.world.height * WORLD_TILE_SIZE
    );
    this.cam.centerOn(this.px, this.py);
    this.enterCell(ctx, this.cellX, this.cellY, true);
    if (ctx.audio.ready) void ctx.audio.playMusic(MAP_ROOM_MUSIC);
  }

  private back(ctx: GameContext): void {
    ctx.audio.play(UI_SOUNDS.click);
    ctx.audio.stopMusic();
    void import("./menu").then(({ MenuScreen }) => ctx.switchScreen(new MenuScreen()));
  }

  private launch(ctx: GameContext): void {
    const w = this.world;
    if (!w || this.onLevelTicks < 0) return;
    const file = w.levelFileAt(this.cellX, this.cellY);
    if (!file) {
      // "This level doesn't exist yet." — the original beeps and stays.
      ctx.audio.play(UI_SOUNDS.tick);
      return;
    }
    ctx.audio.play(UI_SOUNDS.click);
    ctx.audio.stopMusic();
    void import("./play").then(({ PlayScreen }) =>
      ctx.switchScreen(new PlayScreen(file, "map"))
    );
  }

  tick(ctx: GameContext): void {
    this.bubble?.update(this.tickCount, defaultRng);
    const input = ctx.input;

    for (const d of Object.keys(DIRS) as DirAction[]) {
      if (input.wasPressed(d)) {
        this.keyStack = this.keyStack.filter((k) => k !== d);
        this.keyStack.unshift(d);
      }
      if (!input.isHeld(d)) this.keyStack = this.keyStack.filter((k) => k !== d);
    }

    if (input.wasPressed("escape")) {
      this.back(ctx);
      return;
    }
    if (input.wasPressed("confirm")) {
      this.launch(ctx);
      return;
    }
    if (input.keysPressed.includes("KeyL")) {
      // quick level list (web extra)
      ctx.audio.play(UI_SOUNDS.click);
      ctx.audio.stopMusic();
      void import("./select").then(({ SelectScreen }) => ctx.switchScreen(new SelectScreen()));
      return;
    }
    const p = ctx.pointer;
    if (p.wentDown) {
      if (p.inside(0, 440, 40, 40)) {
        this.back(ctx);
        return;
      }
      if (this.popupVisible) {
        const [cx, cy] = this.popupPos();
        if (p.inside(cx + 125, cy + 116, 64, 20)) {
          this.launch(ctx);
          return;
        }
      }
    }

    // Map logic runs at 30 fps.
    this.parity ^= 1;
    if (this.parity === 1) return;
    this.tick2();

    if (this.onLevelTicks >= 0) {
      this.onLevelTicks += 2;
      if (!this.popupVisible && this.onLevelTicks >= INFO_DELAY_TICKS) {
        this.popupVisible = true;
        ctx.audio.play(UI_SOUNDS.levelInfoUp);
      }
    }

    this.moveStep(ctx);
    this.cam?.follow(this.px, this.py, MOVE_DELTA);
  }

  /** advance the 1/60s tick counter by one 30fps frame */
  private tick2(): void {
    this.tickCount += 2;
    for (const g of this.anims) {
      if (this.tickCount - g.lastTick > g.delay) {
        g.lastTick = this.tickCount;
        g.step = (g.step + 1) % (g.last - g.first + 1);
      }
    }
    const gt = this.grabThing;
    if (this.tickCount - gt.lastTick > 15) {
      gt.lastTick = this.tickCount;
      if (gt.step > 0) gt.step = (gt.step + 1) % 3;
      else if (defaultRng(0, 999) < 75) gt.step = 1;
    }
  }

  private aligned(): boolean {
    return this.px % WORLD_TILE_SIZE === 0 && this.py % WORLD_TILE_SIZE === 0;
  }

  private moveStep(ctx: GameContext): void {
    const w = this.world;
    if (!w) return;
    if (this.aligned()) {
      // arrived / deciding
      const newCellX = this.px / WORLD_TILE_SIZE;
      const newCellY = this.py / WORLD_TILE_SIZE;
      if (newCellX !== this.cellX || newCellY !== this.cellY) {
        this.prevCell = [this.cellX, this.cellY];
        this.cellX = newCellX;
        this.cellY = newCellY;
        this.enterCell(ctx, newCellX, newCellY, false);
      }
      this.dx = 0;
      this.dy = 0;
      const dir = this.keyStack[0];
      if (dir) {
        const [dh, dv] = DIRS[dir];
        const tx = this.cellX + dh;
        const ty = this.cellY + dv;
        if (this.canMoveTo(tx, ty)) {
          this.dx = dh * MOVE_DELTA;
          this.dy = dv * MOVE_DELTA;
        }
      }
    }
    this.px += this.dx;
    this.py += this.dy;
    if (this.tickCount - this.lastSpriteTick >= SPRITE_FRAME_TICKS) {
      this.lastSpriteTick = this.tickCount;
      this.spriteFrame ^= 1;
    }
  }

  private canMoveTo(x: number, y: number): boolean {
    const w = this.world;
    if (!w) return false;
    const piece = w.pieceAt(x, y);
    if (piece === MapPiece.NoPass) return false;
    // From an unpassed level node you may only go back the way you came.
    const here = w.pieceAt(this.cellX, this.cellY);
    if (here === MapPiece.Level && this.cameFrom) {
      return x === this.cameFrom[0] && y === this.cameFrom[1];
    }
    return true;
  }

  private enterCell(ctx: GameContext, x: number, y: number, initial: boolean): void {
    const w = this.world;
    if (!w) return;
    setMapLocation([x, y]);
    const piece = w.pieceAt(x, y);

    const wasOnLevel = this.onLevelTicks >= 0;
    if (piece === MapPiece.Level || piece === MapPiece.PassedLevel) {
      this.onLevelTicks = 0;
      this.cameFrom = this.prevCell;
    } else {
      if (wasOnLevel && this.popupVisible) ctx.audio.play(UI_SOUNDS.levelInfoDown);
      this.onLevelTicks = -1;
      this.popupVisible = false;
      this.cameFrom = null;
    }

    if (piece === MapPiece.Switch && !initial) {
      const t = w.triggerSwitch(x, y);
      if (t > 0) {
        ctx.audio.play(UI_SOUNDS.mapSwitch);
        toggleThrow(t);
      }
    }
  }

  // --- rendering --------------------------------------------------------------

  /** Displayed image for a tile ID, with ambient animation substitution. */
  private displayTile(id: number): number {
    for (const g of this.anims) {
      if (id >= g.first && id <= g.last) {
        return g.first + ((id - g.first + g.step) % (g.last - g.first + 1));
      }
    }
    if (id >= 168 && id <= 170) {
      return 168 + ((id - 168 + this.grabThing.step) % 3);
    }
    return id;
  }

  private popupPos(): [number, number] {
    // Bottom-right of the viewport when the sprite is in the top half,
    // else top-left; inset 10 px (kInfoBufferSpace).
    const cam = this.cam;
    const inTopHalf = cam ? this.py - cam.y < VIEW_H / 2 : true;
    if (inTopHalf) return [VIEW_X + VIEW_W - 200 - 10, VIEW_H - 147 - 10];
    return [VIEW_X + 10, 10];
  }

  render(ctx: GameContext, g: CanvasRenderingContext2D): void {
    const w = this.world;
    const sheet = this.tileSheet;
    const cam = this.cam;
    if (!w || !sheet || !cam) return;

    g.fillStyle = "#000";
    g.fillRect(0, 0, ctx.width, ctx.height);

    // Map viewport.
    g.save();
    g.beginPath();
    g.rect(VIEW_X, 0, VIEW_W, VIEW_H);
    g.clip();
    g.translate(VIEW_X - cam.x, -cam.y);
    const s = WORLD_TILE_SIZE;
    const x0 = Math.max(0, Math.floor(cam.x / s));
    const y0 = Math.max(0, Math.floor(cam.y / s));
    const x1 = Math.min(w.width - 1, Math.ceil((cam.x + VIEW_W) / s));
    const y1 = Math.min(w.height - 1, Math.ceil((cam.y + VIEW_H) / s));
    for (let y = y0; y <= y1; y++) {
      for (let x = x0; x <= x1; x++) {
        const id = this.displayTile(w.tileAt(x, y));
        const sx = (id % 18) * s;
        const sy = Math.floor(id / 18) * s;
        g.drawImage(sheet, sx, sy, s, s, x * s, y * s, s, s);
      }
    }
    if (this.mini) {
      g.drawImage(this.mini[this.spriteFrame] as HTMLImageElement, this.px, this.py);
    }
    g.restore();

    // Side panels.
    g.drawImage(ctx.assets.img("interface/bar-map-room.png"), 0, 0);
    g.drawImage(ctx.assets.img("interface/status-bar-map.png"), 600, 0);
    const backHover = ctx.pointer.inside(0, 440, 40, 40);
    g.drawImage(ctx.assets.img(backHover ? "interface/back-select.png" : "interface/back.png"), 0, 440);
    this.bubble?.draw(g, 604, 4);
    this.font?.drawVertical(g, "oliver", 611, 77, 267);

    if (this.popupVisible) this.renderPopup(ctx, g);
  }

  private renderPopup(ctx: GameContext, g: CanvasRenderingContext2D): void {
    const w = this.world;
    if (!w) return;
    const [cx, cy] = this.popupPos();
    const hoverPlay = ctx.pointer.inside(cx + 125, cy + 116, 64, 20);
    g.drawImage(
      ctx.assets.img(hoverPlay ? "interface/level-stats-select.png" : "interface/level-stats.png"),
      cx,
      cy
    );
    const file = w.levelFileAt(this.cellX, this.cellY);
    const entry = file ? ctx.assets.levelIndex.find((l) => l.file === file) : undefined;
    g.fillStyle = TEXT_COLOR;
    g.font = "9px Helvetica, Arial, sans-serif";
    if (!entry) {
      g.fillText("Nonexistant Level", cx + 74, cy + 47, 118);
      g.fillText("This level doesn't exist yet.", cx + 74, cy + 60, 118);
      return;
    }
    g.fillText(entry.name, cx + 74, cy + 47, 118);
    g.fillText(goalLine(entry), cx + 74, cy + 60, 118);
    g.fillText(this.passed.has(entry.file) ? "Passed" : "Undefeated", cx + 74, cy + 72, 118);
    let hy = cy + 85;
    let line = "";
    for (const word of entry.hint.split(/\s+/)) {
      const trial = line ? `${line} ${word}` : word;
      if (g.measureText(trial).width > 118 && line) {
        g.fillText(line, cx + 74, hy);
        hy += 9;
        line = word;
        if (hy > cy + 110) break;
      } else {
        line = trial;
      }
    }
    if (line && hy <= cy + 110) g.fillText(line, cx + 74, hy);
  }

}

/** The info card's goal line, in the original's formats. */
function goalLine(entry: LevelIndexEntry): string {
  const q = String(entry.quota).padStart(3, "0");
  const t = entry.time;
  if (!t) return q;
  if (t.hours > 0) return `${q} in ${t.hours} hr. ${String(t.mins).padStart(2, "0")} min.`;
  if (t.mins > 0) {
    return t.secs > 0
      ? `${q} in ${t.mins} min., ${String(t.secs).padStart(2, "0")} sec.`
      : `${q} in ${t.mins} min.`;
  }
  return `${q} in ${t.secs} sec.`;
}
