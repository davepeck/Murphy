// The Murphy's Revenge engine: authoritative tile grid + sprites in motion +
// a FIFO chore queue, faithfully ported from the 1996 Mac C code (MRPlay.c,
// MRPlayCommon.c, MRChore.c). One frame() call = one 30 fps logic frame; the
// tick counter advances 2 per frame (1 tick = 1/60 s, like Mac TickCount).

import { T, Dir, TILE_SIZE } from "./tiles";
import { getPieceKind, isPortTile, isRDBlinkTile, isExplosionTile, type PieceKind } from "./kinds";
import type { LevelData } from "./level";
import type { SoundSink } from "./events";
import { defaultRng, type Rng } from "./rng";
import {
  FallSprite,
  MoverSprite,
  OliverSprite,
  PushSprite,
  RollSprite,
  type Sprite,
} from "./sprites";

export interface Loc {
  h: number; // column
  v: number; // row
}

export interface FrameInput {
  /** current movement direction (top of the most-recent-wins key stack) */
  dir: Dir;
  /** special ("snap"/red-disk) key held */
  special: boolean;
}

export interface Chore {
  /** Returns true to stay queued for the next frame. */
  process(g: Game): boolean;
}

export interface PieceKindAt extends PieceKind {
  moverSprite: MoverSprite | null;
  isQuark: boolean;
  isOliver: boolean;
}

const MAX_PROXIMITY = 22;

export class Game {
  readonly level: LevelData;
  readonly width: number;
  readonly height: number;
  readonly tiles: Int16Array;
  readonly isInfotronWorld: boolean;

  sprites: Sprite[] = [];
  oliver: OliverSprite | null = null;
  private chores: Chore[] = [];

  tickCount = 0;
  infotronCount: number;
  redDiskCount = 0;

  timerEnabled: boolean;
  hours = 0;
  mins = 0;
  secs = 0;
  private lastTimerTick = 0;

  wonLevel = false;
  private doneInitialSetup = false;

  /** -1 = left first, +1 = right first; flips on each successful choice roll. */
  nextRollDirection = -1;
  private lastTermSoundTick = -1000;

  private phaseBlocks: { h: number; v: number; phasedIn: boolean }[] = [];
  private lastPhaseTick = -1000;

  // Global cycling tile images for idle infotrons and terminals.
  infotronImage: number = T.FirstInfotron;
  terminalImage: number = T.FirstTerminal;
  private lastTileAnimTick = 0;

  /** tick of the most recent key activity (drives Oliver's boredom animations) */
  lastPressTick = 0;

  readonly rand: Rng;
  readonly sound: SoundSink;

  constructor(level: LevelData, opts: { sound?: SoundSink; rand?: Rng } = {}) {
    this.level = level;
    this.width = level.width;
    this.height = level.height;
    this.tiles = Int16Array.from(level.grid);
    this.isInfotronWorld = level.world === "infotron";
    this.rand = opts.rand ?? defaultRng;
    this.sound = opts.sound ?? (() => {});
    this.infotronCount = level.quota;
    this.timerEnabled = level.time !== null;
    if (level.time) {
      this.hours = level.time.hours;
      this.mins = level.time.mins;
      this.secs = level.time.secs;
    }

    // Load-time tile conversions (InitialTileAction + InfoWorld_PhaseTerm).
    let start: Loc | null = null;
    for (let v = 0; v < this.height; v++) {
      for (let h = 0; h < this.width; h++) {
        const t = this.getTile(h, v);
        if (t === T.Oliver) {
          start = { h, v };
          this.setTileRaw(h, v, T.Blank);
        } else if (t === T.Bug) {
          this.setTileRaw(h, v, T.Circuit);
          this.enqueueChore(new BugChore({ h, v }, this));
        } else if (this.isInfotronWorld && t === T.EmptyPhase) {
          this.phaseBlocks.push({ h, v, phasedIn: false });
          this.setTileRaw(h, v, T.Blank);
        } else if (this.isInfotronWorld && t === T.FullPhase) {
          this.phaseBlocks.push({ h, v, phasedIn: true });
        }
      }
    }
    if (!start) throw new Error(`${level.name}: no player start`);
    this.oliver = new OliverSprite(start);
    this.sprites.push(this.oliver);
  }

  // --- grid access ----------------------------------------------------------

  inBounds(h: number, v: number): boolean {
    return h >= 0 && v >= 0 && h < this.width && v < this.height;
  }

  getTile(h: number, v: number): number {
    if (!this.inBounds(h, v)) return T.TopBorder; // levels are border-enclosed
    return this.tiles[v * this.width + h] as number;
  }

  /** Raw write: no cascade (the original's direct SWDrawTile). */
  setTileRaw(h: number, v: number, tile: number): void {
    if (!this.inBounds(h, v)) return;
    this.tiles[v * this.width + h] = tile;
  }

  /**
   * DrawLevelTile: the single cascading write hook. Writing kBlankTile checks
   * whether neighbors should start falling or rolling into the hole.
   */
  drawLevelTile(h: number, v: number, tile: number): void {
    this.setTileRaw(h, v, tile);
    if (tile === T.Blank) this.blankCascade(h, v);
  }

  // --- piece kind lookups ---------------------------------------------------

  pieceKindAt(h: number, v: number): PieceKindAt {
    const tile = this.getTile(h, v);
    const k: PieceKindAt = {
      ...getPieceKind(tile),
      moverSprite: null,
      isQuark: false,
      isOliver: false,
    };
    if (tile === T.Phantom) {
      const m = this.findMoverAt(h, v);
      if (m) {
        k.moverSprite = m;
        k.isQuark = m.isQuark;
        k.isIgnitable = true; // movers chain-explode
      } else if (this.oliver && !this.oliver.dead && this.oliver.loc.h === h && this.oliver.loc.v === v) {
        k.isOliver = true;
      }
    }
    return k;
  }

  findMoverAt(h: number, v: number): MoverSprite | null {
    for (const s of this.sprites) {
      if (s instanceof MoverSprite && !s.disposed) {
        if ((s.loc.h === h && s.loc.v === v) || (s.newLoc.h === h && s.newLoc.v === v)) return s;
      }
    }
    return null;
  }

  // --- the frame ------------------------------------------------------------

  frame(input: FrameInput): void {
    this.tickCount += 2;

    if (!this.doneInitialSetup) {
      this.doneInitialSetup = true;
      this.initialFalls();
      this.makeMovers();
      this.lastTimerTick = this.tickCount;
    }

    // Ambient tile-image cycling (idle infotrons & terminals, ~15 Hz).
    if (this.tickCount - this.lastTileAnimTick > 2) {
      this.lastTileAnimTick = this.tickCount;
      this.infotronImage = this.infotronImage >= T.LastInfotron ? T.FirstInfotron : this.infotronImage + 1;
      this.terminalImage = this.terminalImage >= T.LastTerminal ? T.FirstTerminal : this.terminalImage + 1;
    }

    // Sprites, in list order over the LIVE list: sprites appended during the
    // walk (falls/rolls/movers) run this same frame, exactly like the
    // original's captured-next-pointer traversal; push sprites carry a
    // skipFrame flag instead.
    for (let i = 0; i < this.sprites.length; i++) {
      const s = this.sprites[i];
      if (s && !s.disposed) s.update(this, input);
    }
    this.sprites = this.sprites.filter((s) => !s.disposed);
    if (this.oliver && this.oliver.disposed) this.oliver = null;

    // Chores: process the queue snapshot once, FIFO, re-queue if still alive.
    const n = this.chores.length;
    for (let i = 0; i < n; i++) {
      const c = this.chores.shift();
      if (c && c.process(this)) this.chores.push(c);
    }

    // Countdown timer (timed levels only).
    if (this.timerEnabled && this.tickCount - this.lastTimerTick >= 60) {
      this.lastTimerTick += 60;
      this.secs -= 1;
      if (this.secs < 0) {
        if (this.mins > 0) {
          this.mins -= 1;
          this.secs = 59;
        } else if (this.hours > 0) {
          this.hours -= 1;
          this.mins = 59;
          this.secs = 59;
        } else {
          this.secs = 0;
          this.timerEnabled = false;
          if (this.oliver) this.killOliver(false);
        }
      }
      if (this.timerEnabled && this.hours === 0 && this.mins === 0 && this.secs <= 10) {
        this.sound({ kind: "noMoreTime", prox: Math.floor(this.secs * 1.5) });
      }
    }
  }

  enqueueChore(c: Chore): void {
    this.chores.push(c);
  }

  addSprite(s: Sprite): void {
    // Push sprites go right after Oliver; everything else appends (original order).
    if (s instanceof PushSprite && this.sprites.length > 0) {
      this.sprites.splice(1, 0, s);
    } else {
      this.sprites.push(s);
    }
  }

  private initialFalls(): void {
    for (let v = 0; v < this.height; v++) {
      for (let h = 0; h < this.width; h++) {
        if (this.getTile(h, v) === T.Blank) this.blankCascade(h, v);
      }
    }
  }

  private makeMovers(): void {
    for (let v = 0; v < this.height; v++) {
      for (let h = 0; h < this.width; h++) {
        const t = this.getTile(h, v);
        if (t === T.Quark) {
          this.addSprite(new MoverSprite({ h, v }, true, 1 /* Facing.Up */));
        } else if (t >= T.UpScissor && t <= T.LeftScissor) {
          // 21..24 -> facing up/right/down/left = 1..4
          this.addSprite(new MoverSprite({ h, v }, false, t - T.UpScissor + 1));
        }
      }
    }
  }

  // --- gravity cascades -----------------------------------------------------

  /** Checks run when (h,v) becomes blank; starts at most one motion. */
  private blankCascade(h: number, v: number): void {
    const o = this.oliver;
    if (o && !o.dead && o.loc.h === h && o.loc.v === v) return;
    if (this.checkFall(h, v - 1, 1)) return;
    if (this.checkFall(h, v + 1, -1)) return;
    if (this.checkRollOver(h, v - 1, 1)) return;
    if (this.checkRollOver(h, v + 1, -1)) return;
    if (this.checkSideRoll(h, v, -1)) return;
    if (this.checkSideRoll(h, v, 1)) return;
  }

  private checkFall(h: number, v: number, wantDelta: 1 | -1): boolean {
    const k = getPieceKind(this.getTile(h, v));
    if (k.canFall && k.fallRowDelta === wantDelta) {
      this.makeFallSprite(h, v);
      return true;
    }
    return false;
  }

  /** A piece diagonally beside (h,v) rolls into (h,v), then falls past it. */
  private checkRollOver(h: number, v: number, fallDelta: 1 | -1): boolean {
    if (this.getTile(h, v) !== T.Blank) return false;
    // Left neighbor gets priority, then right.
    for (const dCol of [-1, 1]) {
      const ph = h + dCol;
      const k = getPieceKind(this.getTile(ph, v));
      if (!k.canRoll || k.fallRowDelta !== fallDelta) continue;
      const below = getPieceKind(this.getTile(ph, v + fallDelta));
      if (!below.isRounded) continue;
      if (this.canRollTo(ph, v, -dCol, fallDelta, true)) {
        this.makeRollSprite(ph, v, -dCol);
        return true;
      }
    }
    return false;
  }

  /** The piece directly beside the hole rolls sideways into it. */
  private checkSideRoll(h: number, v: number, dCol: number): boolean {
    const ph = h + dCol;
    const k = getPieceKind(this.getTile(ph, v));
    if (!k.canRoll) return false;
    const fd = k.fallRowDelta;
    const below = getPieceKind(this.getTile(ph, v + fd));
    if (!below.isRounded) return false;
    if (this.canRollTo(ph, v, -dCol, fd, true)) {
      this.makeRollSprite(ph, v, -dCol);
      return true;
    }
    return false;
  }

  /** _sprite_CanRoll: target side cell and the cell past it (fall dir) are blank. */
  canRollTo(h: number, v: number, dCol: number, fallDelta: 1 | -1, checkOliver: boolean): boolean {
    if (this.getTile(h + dCol, v) !== T.Blank) return false;
    if (this.getTile(h + dCol, v + fallDelta) !== T.Blank) return false;
    if (checkOliver && this.oliver && !this.oliver.dead) {
      const o = this.oliver.loc;
      if (o.h === h + dCol && o.v === v) return false;
    }
    return true;
  }

  makeFallSprite(h: number, v: number): void {
    const tile = this.getTile(h, v);
    const k = getPieceKind(tile);
    const dest = { h, v: v + k.fallRowDelta };
    this.drawLevelTile(dest.h, dest.v, T.Phantom);
    this.setTileRaw(h, v, T.Phantom);
    this.addSprite(new FallSprite({ h, v }, dest, tile, k.fallRowDelta));
  }

  makeRollSprite(h: number, v: number, dCol: number): void {
    const tile = this.getTile(h, v);
    const dest = { h: h + dCol, v };
    this.drawLevelTile(dest.h, dest.v, T.Phantom);
    this.setTileRaw(h, v, T.Blank); // raw: cascade deferred until roll completes
    this.addSprite(new RollSprite({ h, v }, dest, tile, dCol));
  }

  /** Try both roll directions for a landed piece; flips the global preference. */
  tryChoiceRoll(h: number, v: number, fallDelta: 1 | -1): boolean {
    for (const dCol of [this.nextRollDirection, -this.nextRollDirection]) {
      if (this.canRollTo(h, v, dCol, fallDelta, true)) {
        this.makeRollSprite(h, v, dCol);
        this.nextRollDirection = -this.nextRollDirection;
        return true;
      }
    }
    return false;
  }

  /** A moving piece has landed against a phantom: crush a mover under it. */
  hitBottomCheck(h: number, v: number): void {
    const m = this.findMoverAt(h, v);
    if (!m) return;
    if (m.isQuark) this.startQuarkExplosion(m.loc.h, m.loc.v, true);
    else this.startExplosion(m.loc.h, m.loc.v, true);
  }

  // --- eating, exits, death -------------------------------------------------

  eatPiece(tile: number): void {
    if (tile === T.Infotron) {
      if (this.infotronCount > 0) {
        this.infotronCount -= 1;
        if (this.infotronCount === 0) {
          this.sound({ kind: "gotLastInfotron" });
          this.openAllExits();
        } else {
          this.sound({ kind: "gotInfotron" });
        }
      } else {
        this.sound({ kind: "gotInfotron" });
      }
    } else if (tile === T.RedDisk) {
      this.redDiskCount += 1;
      this.sound({ kind: "gotRedDisk" });
      if (this.redDiskCount > 9 && this.oliver) this.killOliver(false);
    }
    // Circuit: silent, no counter.
  }

  private openAllExits(): void {
    for (let v = 0; v < this.height; v++) {
      for (let h = 0; h < this.width; h++) {
        if (this.getTile(h, v) === T.Exit) {
          this.enqueueChore(new ExitChore({ h, v }, this.tickCount));
        }
      }
    }
  }

  winLevel(): void {
    this.sound({ kind: "endLevel" });
    this.wonLevel = true;
    this.timerEnabled = false;
    if (this.oliver) {
      this.oliver.disposed = true;
      this.oliver = null;
    }
  }

  killOliver(electric: boolean): void {
    const o = this.oliver;
    if (!o || o.dead) return;
    o.startDeath(this, electric);
    this.timerEnabled = false;
    const { h, v } = o.loc;
    if (isExplosionTile(this.getTile(h, v))) {
      this.igniteExplosion(h, v);
    } else {
      this.startExplosion(h, v, true);
    }
    this.sound({ kind: "oliverDied" });
  }

  // --- explosions -----------------------------------------------------------

  startExplosion(h: number, v: number, shouldIgnite: boolean): void {
    const k = this.pieceKindAt(h, v);
    if (!k.isExplodable) return;
    if (k.moverSprite && k.isQuark && shouldIgnite) {
      this.startQuarkExplosion(h, v, true);
      return;
    }
    this.enqueueChore(new ExplChore({ h, v }, shouldIgnite, this.tickCount));
    this.setTileRaw(h, v, T.FirstExpl);
    if (shouldIgnite) this.sound({ kind: "explode", ...this.spatial(h, v) });
    if (k.moverSprite) k.moverSprite.disposed = true;
    if (k.isOliver) this.killOliver(false);
  }

  startQuarkExplosion(h: number, v: number, shouldIgnite: boolean): void {
    const k = this.pieceKindAt(h, v);
    if (!k.isExplodable) return;
    this.enqueueChore(new QuarkExplChore({ h, v }, shouldIgnite, this.tickCount));
    this.setTileRaw(h, v, T.LastZapInfotron);
    this.sound({ kind: "quarkExplode", ...this.spatial(h, v) });
    if (k.moverSprite) k.moverSprite.disposed = true;
    if (k.isOliver) this.killOliver(false);
  }

  /** 3x3 blast around a center (center excluded). */
  igniteExplosion(h: number, v: number): void {
    for (let dv = -1; dv <= 1; dv++) {
      for (let dh = -1; dh <= 1; dh++) {
        if (dh === 0 && dv === 0) continue;
        const k = this.pieceKindAt(h + dh, v + dv);
        this.startExplosion(h + dh, v + dv, k.isIgnitable);
      }
    }
  }

  igniteQuarkExplosion(h: number, v: number): void {
    for (let dv = -1; dv <= 1; dv++) {
      for (let dh = -1; dh <= 1; dh++) {
        if (dh === 0 && dv === 0) continue;
        const k = this.pieceKindAt(h + dh, v + dv);
        if (k.moverSprite && k.isQuark) this.startQuarkExplosion(h + dh, v + dv, k.isIgnitable);
        else this.startExplosion(h + dh, v + dv, k.isIgnitable);
      }
    }
  }

  /** Re-trigger fall checks around a cell (after a quark leaves an infotron). */
  checkForMotions(h: number, v: number): void {
    for (let dv = -1; dv <= 1; dv++) {
      for (let dh = -1; dh <= 1; dh++) {
        if (this.getTile(h + dh, v + dv) === T.Blank) this.blankCascade(h + dh, v + dv);
      }
    }
  }

  // --- terminals, phase hardware, lasers -------------------------------------

  explodeYellowDisks(): void {
    let exploded = 0;
    for (let h = 0; h < this.width; h++) {
      for (let v = 0; v < this.height; v++) {
        if (this.getTile(h, v) === T.YellowDisk) {
          this.startExplosion(h, v, true);
          exploded += 1;
        }
      }
    }
    if (this.tickCount - this.lastTermSoundTick >= 60) {
      this.lastTermSoundTick = this.tickCount;
      this.sound({ kind: exploded > 0 ? "termUse" : "termBad" });
    }
  }

  phaseHardware(): void {
    if (!this.isInfotronWorld) return;
    if (this.tickCount - this.lastPhaseTick < 45) return;
    this.lastPhaseTick = this.tickCount;
    for (const b of this.phaseBlocks) {
      if (b.phasedIn) {
        b.phasedIn = false;
        this.drawLevelTile(b.h, b.v, T.Blank);
      } else {
        b.phasedIn = true;
        this.drawLevelTile(b.h, b.v, T.FullPhase);
      }
    }
  }

  /** A pushed battery came to rest: fire every adjacent laser emitter. */
  laserBatteryCheck(h: number, v: number): boolean {
    let fired = false;
    const dirs = [
      { dh: 0, dv: -1 },
      { dh: 0, dv: 1 },
      { dh: -1, dv: 0 },
      { dh: 1, dv: 0 },
    ];
    for (const d of dirs) {
      if (this.getTile(h + d.dh, v + d.dv) === T.Laser) {
        this.activateLaser(h + d.dh, v + d.dv);
        fired = true;
      }
    }
    return fired;
  }

  activateLaser(h: number, v: number): void {
    const dirs = [
      { dh: 0, dv: -1 },
      { dh: 0, dv: 1 },
      { dh: -1, dv: 0 },
      { dh: 1, dv: 0 },
    ];
    for (const d of dirs) {
      this.activateSingleLaser(h + d.dh, v + d.dv, d.dh, d.dv);
    }
  }

  activateSingleLaser(h: number, v: number, dh: number, dv: number): void {
    const t = this.getTile(h, v);
    const k = getPieceKind(t);
    if (!k.isExplodable) return;
    if (t === T.LaserPower || t === T.LaserEmptyPiece) return; // beams stop at batteries
    this.enqueueChore(new LaserChore({ h, v }, dh, dv, this.tickCount));
  }

  // --- ports ------------------------------------------------------------------

  /** GetPortPassable: direction rules + the cell beyond must be exactly blank. */
  portPassable(h: number, v: number, dh: number, dv: number): boolean {
    if (this.getTile(h + dh, v + dv) !== T.Blank) return false;
    switch (this.getTile(h, v)) {
      case T.LeftPort: return dh === -1;
      case T.DownPort: return dv === 1;
      case T.RightPort: return dh === 1;
      case T.UpPort: return dv === -1;
      case T.LeftRightPort: return dv === 0;
      case T.UpDownPort: return dh === 0;
      case T.FourWayPort: return true;
      default: return false;
    }
  }

  // --- sound spatialization ---------------------------------------------------

  spatial(h: number, v: number): { prox: number; pan: number } {
    const o = this.oliver?.loc ?? { h, v };
    const prox = Math.abs(o.h - h) + Math.abs(o.v - v);
    let pan = (h - o.h) >> 1;
    if (pan > 10) pan = 10;
    if (pan < -10) pan = -10;
    return { prox: Math.min(prox, MAX_PROXIMITY), pan };
  }
}

// --- chores -------------------------------------------------------------------

const EXPL_DELAY_TICKS = 3;
const EXIT_DELAY_TICKS = 3;
const ZAP_DELAY_TICKS = 3;
const BUG_ZAP_DELAY = 120;
const BUG_ANIM_DELAY = 5;
const RD_BLINK_DELAY = 20;
const RD_BLINK_COUNT = 12;

export class ExplChore implements Chore {
  private cur = T.FirstExpl;
  constructor(
    private loc: Loc,
    private ignite: boolean,
    private lastTick: number
  ) {}

  process(g: Game): boolean {
    if (g.tickCount - this.lastTick < EXPL_DELAY_TICKS) return true;
    this.lastTick = g.tickCount;
    this.cur += 1;
    if (this.cur > T.LastExpl) {
      g.drawLevelTile(this.loc.h, this.loc.v, T.Blank);
      return false;
    }
    g.setTileRaw(this.loc.h, this.loc.v, this.cur);
    if (this.cur === T.IgniteExpl && this.ignite) {
      g.igniteExplosion(this.loc.h, this.loc.v);
    }
    return true;
  }
}

export class QuarkExplChore implements Chore {
  private cur = T.LastZapInfotron;
  constructor(
    private loc: Loc,
    private ignite: boolean,
    private lastTick: number
  ) {}

  process(g: Game): boolean {
    if (g.tickCount - this.lastTick < EXPL_DELAY_TICKS) return true;
    this.lastTick = g.tickCount;
    this.cur -= 1;
    if (this.cur < T.FirstZapInfotron) {
      g.setTileRaw(this.loc.h, this.loc.v, T.Infotron);
      g.checkForMotions(this.loc.h, this.loc.v);
      return false;
    }
    g.setTileRaw(this.loc.h, this.loc.v, this.cur);
    if (this.cur === T.ZapInfotron25 && this.ignite) {
      g.igniteQuarkExplosion(this.loc.h, this.loc.v);
    }
    return true;
  }
}

export class ExitChore implements Chore {
  private cur = T.FirstExit;
  constructor(
    private loc: Loc,
    private lastTick: number
  ) {}

  process(g: Game): boolean {
    if (g.tickCount - this.lastTick < EXIT_DELAY_TICKS) return true;
    this.lastTick = g.tickCount;
    if (this.cur > T.LastExit) return false;
    g.setTileRaw(this.loc.h, this.loc.v, this.cur);
    this.cur += 1;
    return this.cur <= T.LastExit;
  }
}

/** Oliver's remote "snap" eating a neighboring cell. */
export class ZapChore implements Chore {
  private cur: number;
  private readonly last: number;
  constructor(
    private loc: Loc,
    private origTile: number,
    private lastTick: number
  ) {
    if (origTile === T.Infotron) {
      this.cur = T.FirstZapInfotron;
      this.last = T.LastZapInfotron;
    } else if (origTile === T.RedDisk) {
      this.cur = T.FirstZapRedDisk;
      this.last = T.LastZapRedDisk;
    } else {
      this.cur = T.FirstZapCircuit;
      this.last = T.LastZapCircuit;
    }
  }

  process(g: Game): boolean {
    if (g.tickCount - this.lastTick < ZAP_DELAY_TICKS) return true;
    this.lastTick = g.tickCount;
    if (this.cur > this.last) {
      g.drawLevelTile(this.loc.h, this.loc.v, T.Blank);
      g.eatPiece(this.origTile);
      return false;
    }
    g.setTileRaw(this.loc.h, this.loc.v, this.cur);
    this.cur += 1;
    return true;
  }
}

export class RDBlinkChore implements Chore {
  private blinks = 0;
  private showFirst = true;
  constructor(
    private loc: Loc,
    private lastTick: number
  ) {}

  process(g: Game): boolean {
    if (g.tickCount - this.lastTick < RD_BLINK_DELAY) return true;
    this.lastTick = g.tickCount;
    this.blinks += 1;
    if (this.blinks > RD_BLINK_COUNT) {
      g.startExplosion(this.loc.h, this.loc.v, true);
      return false;
    }
    g.setTileRaw(this.loc.h, this.loc.v, this.showFirst ? T.FirstRDBlink : T.LastRDBlink);
    this.showFirst = !this.showFirst;
    return true;
  }
}

export class BugChore implements Chore {
  private zapping = false;
  private nextZapTick: number;
  private lastAnimTick = 0;

  constructor(private loc: Loc, g: Game) {
    this.nextZapTick = g.tickCount + BUG_ZAP_DELAY + 15 * g.rand(0, 16);
  }

  process(g: Game): boolean {
    const t = g.getTile(this.loc.h, this.loc.v);
    if (!this.zapping) {
      if (t !== T.Circuit) return false; // bug destroyed (eaten/exploded/walked over)
      if (g.tickCount >= this.nextZapTick) {
        g.setTileRaw(this.loc.h, this.loc.v, T.FirstBug);
        g.sound({ kind: "bugZap", ...g.spatial(this.loc.h, this.loc.v) });
        this.zapping = true;
        this.lastAnimTick = g.tickCount;
      }
      return true;
    }
    if (t < T.FirstBug || t > T.LastBug) return false; // exploded mid-zap
    if (g.tickCount - this.lastAnimTick >= BUG_ANIM_DELAY) {
      this.lastAnimTick = g.tickCount;
      if (t >= T.LastBug) {
        g.setTileRaw(this.loc.h, this.loc.v, T.Circuit);
        this.zapping = false;
        this.nextZapTick = g.tickCount + BUG_ZAP_DELAY + 15 * g.rand(0, 16);
      } else {
        g.setTileRaw(this.loc.h, this.loc.v, t + 1);
      }
    }
    return true;
  }
}

/** A laser beam propagating cell by cell (Infotron terrain only; see Wizard). */
export class LaserChore implements Chore {
  private mode: "init" | "drawn" | "spread" = "init";
  private savedTile = T.Blank;
  private beamDh: number;
  private beamDv: number;

  constructor(
    private loc: Loc,
    dh: number,
    dv: number,
    private lastTick: number
  ) {
    this.beamDh = dh;
    this.beamDv = dv;
  }

  process(g: Game): boolean {
    const { h, v } = this.loc;
    if (this.mode === "init") {
      const t = g.getTile(h, v);
      const k = g.pieceKindAt(h, v);
      if (k.isIgnitable) {
        g.startExplosion(h, v, true);
        return false;
      }
      if (k.isOliver) {
        g.killOliver(false);
      } else if (
        k.isExplodable &&
        t !== T.Blank &&
        !(t > T.Laser && t <= T.LaserPower) &&
        !(t > T.LaserEmptyPiece && t <= T.BLTRLaserBackMirrorLight)
      ) {
        g.startExplosion(h, v, false);
        return false;
      }
      // Draw the beam, reflecting off mirrors.
      if (t === T.ForeMirror || t === T.TLLaserForeMirrorLight || t === T.BRLaserForeMirrorLight) {
        g.setTileRaw(h, v, this.beamDh === 1 || this.beamDv === 1 ? T.TLLaserForeMirrorLight : T.BRLaserForeMirrorLight);
        this.savedTile = T.ForeMirror;
        const { beamDh, beamDv } = this;
        this.beamDh = -beamDv;
        this.beamDv = -beamDh;
      } else if (t === T.BackMirror || t === T.BLLaserBackMirrorLight || t === T.TRLaserBackMirrorLight) {
        g.setTileRaw(h, v, this.beamDh === -1 || this.beamDv === 1 ? T.TRLaserBackMirrorLight : T.BLLaserBackMirrorLight);
        this.savedTile = T.BackMirror;
        const { beamDh, beamDv } = this;
        this.beamDh = beamDv;
        this.beamDv = beamDh;
      } else {
        this.savedTile = T.Blank;
        g.setTileRaw(h, v, this.beamDh === 0 ? T.UDLaserLight : T.RLLaserLight);
      }
      this.mode = "drawn";
      this.lastTick = g.tickCount;
      return true;
    }
    if (this.mode === "drawn") {
      if (g.tickCount - this.lastTick <= 1) return true;
      g.activateSingleLaser(h + this.beamDh, v + this.beamDv, this.beamDh, this.beamDv);
      this.mode = "spread";
      this.lastTick = g.tickCount;
      return true;
    }
    // spread -> cleanup
    if (g.tickCount - this.lastTick <= 1) return true;
    g.drawLevelTile(h, v, this.savedTile);
    return false;
  }
}
