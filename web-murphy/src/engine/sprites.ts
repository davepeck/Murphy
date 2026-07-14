// Sprites: pieces in motion. Faithful port of MRPlaySprite.c. Sprites hold
// pixel positions; MR decision logic runs only when a sprite is exactly
// grid-aligned (every 4 frames at walking speed), then the sprite glides.

import { T, Dir, Facing, OF, OLIVER_TILE_BASE, TILE_SIZE, dirDelta, facingDelta, rotateCW, rotateCCW } from "./tiles";
import { getPieceKind, isPortTile, isRDBlinkTile } from "./kinds";
import type { Game, FrameInput, Loc } from "./game";
import { ZapChore, RDBlinkChore } from "./game";

const MOVE_DELTA = 8; // px/frame: Oliver walking, falls, rolls
const PUSH_DELTA = 4; // px/frame while pushing
const MOVER_DELTA = 4; // px/frame: quarks & scissors

const OLIVER_FRAME_TICKS = 6; // ~100 ms
const MOVER_FRAME_TICKS = 4; // ~66 ms
const ROLL_FRAME_TICKS = 4; // ~66 ms

const ZAP_FREEZE_TICKS = 15;
const RED_DISK_DROP_DELAY = 30;
const RED_DISK_DROP_FINISH = 90;

// Boredom thresholds (ticks since last key activity).
const SAD_START = 5400;
const SAD_END = 5430;
const FU_END = 5490;
const EUREKA_END = 5520;

export abstract class Sprite {
  x: number;
  y: number;
  dx = 0;
  dy = 0;
  loc: Loc;
  newLoc: Loc;
  disposed = false;
  /** Skip the creation frame entirely (SpriteWorld splice semantics). */
  skipFrame = false;
  /** Atlas tile ID to render at (x, y). */
  image: number;
  protected lastFrameTick = 0;

  constructor(loc: Loc, image: number) {
    this.loc = { ...loc };
    this.newLoc = { ...loc };
    this.x = loc.h * TILE_SIZE;
    this.y = loc.v * TILE_SIZE;
    this.image = image;
  }

  aligned(): boolean {
    return this.x % TILE_SIZE === 0 && this.y % TILE_SIZE === 0;
  }

  // Canonical SpriteWorld per-sprite order: frame proc immediately before the
  // sprite's own move proc, then the pixel offset.
  update(g: Game, input: FrameInput): void {
    if (this.skipFrame) {
      this.skipFrame = false;
      return;
    }
    this.frameProc(g);
    if (this.aligned()) this.moveProc(g, input);
    if (this.disposed) return;
    this.x += this.dx;
    this.y += this.dy;
  }

  protected abstract moveProc(g: Game, input: FrameInput): void;
  protected frameProc(_g: Game): void {}
}

// --- falling pieces -----------------------------------------------------------

export class FallSprite extends Sprite {
  private firstCall = true;
  constructor(
    from: Loc,
    to: Loc,
    readonly tile: number,
    private fallDelta: 1 | -1
  ) {
    super(from, tile);
    this.newLoc = { ...to };
    this.dy = fallDelta * MOVE_DELTA;
  }

  protected moveProc(g: Game): void {
    if (this.firstCall) {
      this.firstCall = false;
      return;
    }
    // Arrived at newLoc: blank the cell we left (cascades — columns follow).
    const old = this.loc;
    this.loc = { ...this.newLoc };
    g.drawLevelTile(old.h, old.v, T.Blank);

    const nh = this.loc.h;
    const nv = this.loc.v + this.fallDelta;
    if (g.getTile(nh, nv) === T.Blank) {
      g.drawLevelTile(nh, nv, T.Phantom);
      this.newLoc = { h: nh, v: nv };
      return;
    }
    // Land.
    const s = g.spatial(this.loc.h, this.loc.v);
    g.sound({ kind: "thud", prox: Math.floor(s.prox / 2), pan: s.pan });
    g.drawLevelTile(this.loc.h, this.loc.v, this.tile);
    const blockerTile = g.getTile(nh, nv);
    const blocker = g.pieceKindAt(nh, nv);
    const self = getPieceKind(this.tile);
    if (blocker.isIgnitable) g.startExplosion(nh, nv, true);
    if (self.isIgnitable) g.startExplosion(this.loc.h, this.loc.v, true);
    const o = g.oliver;
    if (o && !o.dead && o.loc.h === nh && o.loc.v === nv) g.killOliver(false);
    if (blockerTile === T.Phantom) g.hitBottomCheck(nh, nv);
    if (self.canRoll && blocker.isRounded) {
      g.tryChoiceRoll(this.loc.h, this.loc.v, this.fallDelta);
    }
    this.disposed = true;
  }
}

// --- rolling pieces -----------------------------------------------------------

const ZONK_ROLL_RIGHT = [T.FirstZonk, T.FirstZonk + 1, T.FirstZonk + 2, T.LastZonk];
const ZONK_ROLL_LEFT = [T.FirstZonk, T.LastZonk, T.LastZonk - 1, T.LastZonk - 2];
const UPZONK_ROLL_RIGHT = [T.FirstUpZonk, T.FirstUpZonk + 1, T.FirstUpZonk + 2, T.LastUpZonk];
const UPZONK_ROLL_LEFT = [T.FirstUpZonk, T.LastUpZonk, T.LastUpZonk - 1, T.LastUpZonk - 2];

function rollFrames(tile: number, dCol: number): number[] | null {
  if (tile === T.Zonk) return dCol > 0 ? ZONK_ROLL_RIGHT : ZONK_ROLL_LEFT;
  if (tile === T.UpZonk) return dCol > 0 ? UPZONK_ROLL_RIGHT : UPZONK_ROLL_LEFT;
  return null;
}

export class RollSprite extends Sprite {
  private firstCall = true;
  private from: Loc;
  private frames: number[] | null;
  private frameIdx = 0;

  constructor(
    from: Loc,
    to: Loc,
    readonly tile: number,
    dCol: number
  ) {
    super(from, tile);
    this.from = { ...from };
    this.newLoc = { ...to };
    this.dx = dCol * MOVE_DELTA;
    this.frames = rollFrames(tile, dCol);
  }

  protected moveProc(g: Game): void {
    if (this.firstCall) {
      this.firstCall = false;
      return;
    }
    this.loc = { ...this.newLoc };
    g.drawLevelTile(this.loc.h, this.loc.v, this.tile);
    // Deferred cascade for the vacated cell.
    if (g.getTile(this.from.h, this.from.v) === T.Blank) {
      g.drawLevelTile(this.from.h, this.from.v, T.Blank);
    }
    const k = getPieceKind(this.tile);
    const belowV = this.loc.v + k.fallRowDelta;
    const below = g.getTile(this.loc.h, belowV);
    if (below === T.Blank) {
      g.makeFallSprite(this.loc.h, this.loc.v);
    } else if (below === T.Phantom) {
      g.hitBottomCheck(this.loc.h, belowV);
    }
    this.disposed = true;
  }

  protected override frameProc(g: Game): void {
    if (!this.frames) return;
    if (g.tickCount - this.lastFrameTick < ROLL_FRAME_TICKS) return;
    this.lastFrameTick = g.tickCount;
    this.frameIdx = (this.frameIdx + 1) % this.frames.length;
    this.image = this.frames[this.frameIdx] as number;
  }
}

// --- pushed pieces --------------------------------------------------------------

export class PushSprite extends Sprite {
  private firstCall = true;
  private dh: number;
  private dv: number;
  private frames: number[] | null;
  private frameIdx = 0;
  readonly isGold: boolean;

  constructor(
    from: Loc,
    dh: number,
    dv: number,
    readonly tile: number
  ) {
    super(from, tile);
    this.dh = dh;
    this.dv = dv;
    this.isGold = tile === T.GoldDisk;
    this.newLoc = { h: from.h + dh, v: from.v + dv };
    const speed = this.isGold ? MOVE_DELTA : PUSH_DELTA;
    this.dx = dh * speed;
    this.dy = dv * speed;
    this.frames = dv === 0 ? rollFrames(tile, dh) : null;
    // Push sprites are spliced in after Oliver mid-iteration; the original's
    // captured next-pointer means they never run on their creation frame.
    this.skipFrame = true;
  }

  protected moveProc(g: Game): void {
    if (this.firstCall) {
      this.firstCall = false;
      return;
    }
    this.loc = { ...this.newLoc };
    if (this.isGold) {
      this.goldStep(g);
      return;
    }
    // Ordinary pushed piece: comes to rest at the target cell.
    g.drawLevelTile(this.loc.h, this.loc.v, this.tile);
    if (this.tile === T.LaserPower && g.laserBatteryCheck(this.loc.h, this.loc.v)) {
      g.setTileRaw(this.loc.h, this.loc.v, T.LaserEmptyPiece);
    }
    const k = getPieceKind(this.tile);
    if (k.canFall && g.getTile(this.loc.h, this.loc.v + k.fallRowDelta) === T.Blank) {
      g.makeFallSprite(this.loc.h, this.loc.v);
    } else if (k.canRoll && this.dv === 0) {
      const below = getPieceKind(g.getTile(this.loc.h, this.loc.v + k.fallRowDelta));
      if (below.isRounded && g.canRollTo(this.loc.h, this.loc.v, this.dh, k.fallRowDelta, false)) {
        g.makeRollSprite(this.loc.h, this.loc.v, this.dh);
      }
    }
    g.oliver?.endPush();
    this.disposed = true;
  }

  /** The gold disk is a fire torpedo: it plows through explodable material. */
  private goldStep(g: Game): void {
    const nh = this.loc.h + this.dh;
    const nv = this.loc.v + this.dv;
    const next = g.pieceKindAt(nh, nv);
    if (next.isExplodable && !next.isIgnitable) {
      g.startExplosion(this.loc.h, this.loc.v, false);
      const s = g.spatial(this.loc.h, this.loc.v);
      g.sound({ kind: "explode", ...s });
      this.newLoc = { h: nh, v: nv };
    } else {
      g.startExplosion(this.loc.h, this.loc.v, true);
      g.oliver?.endPush();
      this.disposed = true;
    }
  }

  protected override frameProc(g: Game): void {
    if (!this.frames) return;
    if (g.tickCount - this.lastFrameTick < ROLL_FRAME_TICKS) return;
    this.lastFrameTick = g.tickCount;
    this.frameIdx = (this.frameIdx + 1) % this.frames.length;
    this.image = this.frames[this.frameIdx] as number;
  }
}

// --- enemies (quarks & scissors) -------------------------------------------------

export class MoverSprite extends Sprite {
  facing: Facing;
  private turning = false;
  private turningTo: Facing = Facing.Up;
  private clockwiseTurn = false;
  private turnStep = 0;
  private animInternal = 0;
  private animDir = 1;

  constructor(
    loc: Loc,
    readonly isQuark: boolean,
    facing: Facing
  ) {
    super(loc, isQuark ? T.FirstQuark : T.FirstUpScissor + (facing - 1) * 4);
    this.facing = facing;
  }

  protected moveProc(g: Game, _input: FrameInput): void {
    // Arrived at the reserved cell: blank the one we left (cascade).
    if (this.newLoc.h !== this.loc.h || this.newLoc.v !== this.loc.v) {
      const old = this.loc;
      this.loc = { ...this.newLoc };
      g.drawLevelTile(old.h, old.v, T.Blank);
    }
    this.dx = 0;
    this.dy = 0;

    if (!this.isQuark && this.turning) {
      this.advanceTurn(g);
      return;
    }

    // Wall-following scan: relative Left, Straight, Right, Back.
    let d = rotateCCW(this.facing);
    let chosen: Facing | null = null;
    for (let i = 0; i < 4; i++) {
      const { dh, dv } = facingDelta(d);
      const t = g.getTile(this.loc.h + dh, this.loc.v + dv);
      if (t === T.Phantom) {
        const k = g.pieceKindAt(this.loc.h + dh, this.loc.v + dv);
        if (k.isOliver) {
          g.killOliver(false);
          if (this.isQuark) g.startQuarkExplosion(this.loc.h, this.loc.v, true);
          else g.startExplosion(this.loc.h, this.loc.v, true);
          return;
        }
      }
      if (t === T.Blank) {
        chosen = d;
        break;
      }
      d = rotateCW(d);
    }

    if (chosen === null) {
      if (this.isQuark) this.facing = rotateCW(this.facing);
      return;
    }
    if (this.isQuark || chosen === this.facing) {
      this.startMove(g, chosen);
    } else {
      this.beginTurn(chosen);
    }
  }

  private startMove(g: Game, d: Facing): void {
    this.facing = d;
    const { dh, dv } = facingDelta(d);
    this.newLoc = { h: this.loc.h + dh, v: this.loc.v + dv };
    g.drawLevelTile(this.newLoc.h, this.newLoc.v, T.Phantom);
    this.dx = dh * MOVER_DELTA;
    this.dy = dv * MOVER_DELTA;
  }

  private opposite(f: Facing): Facing {
    return rotateCW(rotateCW(f));
  }

  private beginTurn(chosen: Facing): void {
    this.turning = true;
    this.clockwiseTurn = chosen === rotateCW(this.facing);
    this.turningTo = chosen === this.opposite(this.facing) ? rotateCCW(this.facing) : chosen;
    this.turnStep = 0;
    this.image = this.diagonalFrame();
  }

  private diagonalFrame(): number {
    // CW: facing*4-1; CCW: (facing-1)*4-1, wrapping -1 -> 15 (relative to 198).
    let idx = this.clockwiseTurn ? this.facing * 4 - 1 : (this.facing - 1) * 4 - 1;
    if (idx < 0) idx = 15;
    return T.FirstUpScissor + idx;
  }

  private advanceTurn(g: Game): void {
    if (this.turnStep === 0) {
      this.turnStep = 1;
      this.image = this.diagonalFrame();
      return;
    }
    this.facing = this.turningTo;
    const { dh, dv } = facingDelta(this.facing);
    if (g.getTile(this.loc.h + dh, this.loc.v + dv) === T.Blank) {
      this.turning = false;
      this.animInternal = 0;
      this.startMove(g, this.facing);
      this.image = T.FirstUpScissor + (this.facing - 1) * 4;
    } else {
      this.turningTo = this.clockwiseTurn ? rotateCW(this.turningTo) : rotateCCW(this.turningTo);
      this.turnStep = 0;
      this.image = T.FirstUpScissor + (this.facing - 1) * 4; // closed frame mid-turn
    }
  }

  protected override frameProc(g: Game): void {
    if (g.tickCount - this.lastFrameTick < MOVER_FRAME_TICKS) return;
    this.lastFrameTick = g.tickCount;
    if (this.isQuark) {
      this.image = this.image >= T.LastQuark ? T.FirstQuark : this.image + 1;
      return;
    }
    if (this.turning) return; // turn code owns the image
    // Ping-pong the 3 open/close frames of the current facing.
    this.animInternal += this.animDir;
    if (this.animInternal >= 2) this.animDir = -1;
    if (this.animInternal <= 0) this.animDir = 1;
    this.image = T.FirstUpScissor + (this.facing - 1) * 4 + this.animInternal;
  }
}

// --- Oliver ----------------------------------------------------------------------

const enum AnimState {
  Static,
  Moving,
  Death,
  Zap,
  RD,
  Push,
  Yawn,
}

export class OliverSprite extends Sprite {
  dead = false;
  frame = OF.Bub; // sprite-local 0..84

  private animState = AnimState.Static;
  private lastHoriz: "left" | "right" = "right";
  private lastWasPort = false;
  private portDh = 0;
  private portDv = 0;
  private isPushing = false;
  private pushHack = false;
  private pushDir: Dir = Dir.None;

  // Special-key (snap / red-disk) machine
  private specialActive = false;
  private specialStartTick = 0;
  private zapFreezeUntil = -1;
  private deniedUntilRelease = false;
  private allowDiskDrop = false;
  private droppedThisPress = false;

  // Death animation
  private deathFirst = 0;
  private deathLast = 0;

  // Idle bubbling
  private bubAxis = 0;
  private bubStep = 0;
  private walkStep = 0;
  private yawnDone = false;

  constructor(loc: Loc) {
    super(loc, OLIVER_TILE_BASE);
  }

  endPush(): void {
    this.isPushing = false;
    this.pushHack = true;
  }

  startDeath(g: Game, electric: boolean): void {
    this.dead = true;
    this.dx = 0;
    this.dy = 0;
    this.animState = AnimState.Death;
    if (electric) {
      this.deathFirst = OF.ShockDeathFirst;
      this.deathLast = OF.ShockDeathFirst + 6;
    } else if (g.rand(1, 9) <= 3) {
      this.deathFirst = OF.FadeDeathFirst;
      this.deathLast = OF.FadeDeathFirst + 9;
    } else if (g.rand(0, 9) < 5) {
      this.deathFirst = OF.BubbleDeathFirst;
      this.deathLast = OF.BubbleDeathFirst + 9;
    } else {
      this.deathFirst = OF.StaticDeathFirst;
      this.deathLast = OF.StaticDeathFirst + 9;
    }
    this.frame = this.deathFirst;
    this.image = OLIVER_TILE_BASE + this.frame;
  }

  protected moveProc(g: Game, input: FrameInput): void {
    if (this.dead) return;

    if (!input.special) {
      this.specialActive = false;
      this.deniedUntilRelease = false;
      this.droppedThisPress = false;
    }

    const curTile = g.getTile(this.loc.h, this.loc.v);
    const curK = getPieceKind(curTile);
    if (curK.isDeadly) {
      g.killOliver(curK.isElectric);
      return;
    }

    // Determine movement deltas.
    let dh = 0;
    let dv = 0;
    let frozen = false;
    if (this.lastWasPort) {
      dh = this.portDh;
      dv = this.portDv;
      this.lastWasPort = false;
    } else if (input.special) {
      frozen = this.handleSpecial(g, input, curTile);
    } else {
      const d = dirDelta(input.dir);
      dh = d.dh;
      dv = d.dv;
    }
    if (frozen) {
      dh = 0;
      dv = 0;
    }

    // Mark occupancy (skipped on ports and blinking red disks).
    if (!isPortTile(curTile) && !isRDBlinkTile(curTile)) {
      g.setTileRaw(this.loc.h, this.loc.v, T.Phantom);
    }

    const nh = this.loc.h + dh;
    const nv = this.loc.v + dv;
    const newTile = g.getTile(nh, nv);
    const newK = g.pieceKindAt(nh, nv);
    let passable = newK.isPassable;
    if (isPortTile(newTile)) passable = g.portPassable(nh, nv, dh, dv);

    if (newK.isDeadly) {
      g.killOliver(newK.isElectric);
      return;
    }
    if ((dh !== 0 || dv !== 0) && newK.isPushable && this.safeToPush(g, dh, dv, newTile)) {
      this.push(g, dh, dv, newTile);
    } else if ((dh !== 0 || dv !== 0) && passable) {
      this.passMove(g, dh, dv, newTile, newK.isEdible);
    } else {
      this.blocked(g, nh, nv, newTile, newK.moverSprite !== null, frozen);
    }
  }

  private handleSpecial(g: Game, input: FrameInput, curTile: number): boolean {
    if (!this.specialActive) {
      this.specialActive = true;
      this.specialStartTick = g.tickCount;
      this.allowDiskDrop = curTile === T.Blank || curTile === T.Phantom;
    }
    if (this.deniedUntilRelease) return true;

    if (input.dir !== Dir.None) {
      if (g.tickCount < this.zapFreezeUntil) return true;
      const d = dirDelta(input.dir);
      const th = this.loc.h + d.dh;
      const tv = this.loc.v + d.dv;
      const t = g.getTile(th, tv);
      if (t === T.Circuit || t === T.Infotron || t === T.RedDisk) {
        this.animState = AnimState.Zap;
        this.frame = this.zapPose(input.dir);
        this.image = OLIVER_TILE_BASE + this.frame;
        this.zapFreezeUntil = g.tickCount + ZAP_FREEZE_TICKS;
        g.enqueueChore(new ZapChore({ h: th, v: tv }, t, g.tickCount));
      } else {
        // "you shouldn't move after a zap if your special key is still held down"
        this.deniedUntilRelease = true;
      }
      return true;
    }

    if (this.droppedThisPress) return true;
    const held = g.tickCount - this.specialStartTick;
    if (held > RED_DISK_DROP_DELAY && g.redDiskCount > 0 && this.allowDiskDrop) {
      this.animState = AnimState.RD;
      if (held > RED_DISK_DROP_FINISH) {
        g.redDiskCount -= 1;
        g.sound({ kind: "redDiskReady" });
        g.enqueueChore(new RDBlinkChore({ ...this.loc }, g.tickCount));
        this.droppedThisPress = true;
        this.animState = AnimState.Static;
      }
    } else if (this.animState === AnimState.RD) {
      this.animState = AnimState.Static;
    }
    return true;
  }

  private zapPose(dir: Dir): number {
    switch (dir) {
      case Dir.Left: return OF.ZapLeft;
      case Dir.Right: return OF.ZapRight;
      case Dir.Down: return OF.ZapDown;
      default: return OF.ZapUp;
    }
  }

  private safeToPush(g: Game, dh: number, dv: number, pieceTile: number): boolean {
    if (
      (pieceTile === T.Zonk || pieceTile === T.UpZonk || pieceTile === T.GreenDisk) &&
      dv !== 0
    ) {
      return false;
    }
    const beyond = g.getTile(this.loc.h + 2 * dh, this.loc.v + 2 * dv);
    if (pieceTile === T.GoldDisk) return beyond === T.Blank || beyond === T.Circuit;
    return beyond === T.Blank;
  }

  private push(g: Game, dh: number, dv: number, pieceTile: number): void {
    const pLoc = { h: this.loc.h + dh, v: this.loc.v + dv };
    const bLoc = { h: this.loc.h + 2 * dh, v: this.loc.v + 2 * dv };
    const gold = pieceTile === T.GoldDisk;
    if (!gold || g.getTile(bLoc.h, bLoc.v) === T.Blank) {
      g.drawLevelTile(bLoc.h, bLoc.v, T.Phantom);
    }
    if (gold || pieceTile === T.Zonk || pieceTile === T.UpZonk) {
      g.setTileRaw(pLoc.h, pLoc.v, T.Blank);
    }
    const origin = { ...this.loc };
    const originTile = g.getTile(origin.h, origin.v);
    this.loc = pLoc;
    if (!isPortTile(originTile) && !isRDBlinkTile(originTile)) {
      g.drawLevelTile(origin.h, origin.v, T.Blank);
    }
    this.dx = dh * PUSH_DELTA;
    this.dy = dv * PUSH_DELTA;
    g.addSprite(new PushSprite(pLoc, dh, dv, pieceTile));
    g.sound({ kind: "push" });
    this.isPushing = true;
    this.animState = AnimState.Push;
    this.pushDir = dh < 0 ? Dir.Left : dh > 0 ? Dir.Right : dv > 0 ? Dir.Down : Dir.Up;
    if (dh !== 0) this.lastHoriz = dh < 0 ? "left" : "right";
    this.frame = this.pushBase();
    this.image = OLIVER_TILE_BASE + this.frame;
  }

  private passMove(g: Game, dh: number, dv: number, newTile: number, edible: boolean): void {
    this.dx = dh * MOVE_DELTA;
    this.dy = dv * MOVE_DELTA;
    const origin = { ...this.loc };
    this.loc = { h: this.loc.h + dh, v: this.loc.v + dv };

    if (newTile === T.Blank || newTile === T.Infotron) {
      g.setTileRaw(this.loc.h, this.loc.v, T.Phantom);
    }
    if (isPortTile(newTile)) {
      this.lastWasPort = true;
      this.portDh = dh;
      this.portDv = dv;
      g.sound({ kind: "portUse", prox: 8, pan: 0 });
    } else if (edible) {
      g.eatPiece(newTile);
    } else if (newTile === T.LastExit) {
      g.winLevel();
      return;
    }
    const originTile = g.getTile(origin.h, origin.v);
    if (!isPortTile(originTile) && !isRDBlinkTile(originTile)) {
      g.drawLevelTile(origin.h, origin.v, T.Blank);
    }
    if (dh !== 0) this.lastHoriz = dh < 0 ? "left" : "right";
    if (this.animState !== AnimState.Zap || g.tickCount >= this.zapFreezeUntil) {
      this.animState = AnimState.Moving;
    }
  }

  private blocked(
    g: Game,
    nh: number,
    nv: number,
    newTile: number,
    isMoverPhantom: boolean,
    frozen: boolean
  ): void {
    this.dx = 0;
    this.dy = 0;
    if (newTile === T.Terminal) {
      g.explodeYellowDisks();
    } else if (newTile === T.PhaseTerm && g.isInfotronWorld) {
      g.phaseHardware();
    } else if (newTile === T.Phantom && isMoverPhantom) {
      const m = g.findMoverAt(nh, nv);
      g.killOliver(false);
      if (m) {
        if (m.isQuark) g.startQuarkExplosion(m.loc.h, m.loc.v, true);
        else g.startExplosion(m.loc.h, m.loc.v, true);
      }
      return;
    }
    if (!frozen && this.animState !== AnimState.Death) {
      if (this.animState === AnimState.Zap && g.tickCount < this.zapFreezeUntil) {
        // hold the zap pose
      } else if (this.pushHack) {
        // one extra push frame handled in frameProc
      } else if (this.animState !== AnimState.Static) {
        this.animState = AnimState.Static;
        this.frame = OF.Bub;
        this.bubStep = 0;
      }
    }
  }

  protected override frameProc(g: Game): void {
    if (g.tickCount - this.lastFrameTick < OLIVER_FRAME_TICKS) return;
    this.lastFrameTick = g.tickCount;

    switch (this.animState) {
      case AnimState.Death: {
        if (this.frame < this.deathLast) {
          this.frame += 1;
        } else {
          this.disposed = true;
        }
        break;
      }
      case AnimState.Zap: {
        if (g.tickCount >= this.zapFreezeUntil) {
          this.animState = AnimState.Static;
          this.frame = OF.Bub;
          this.bubStep = 0;
        }
        break;
      }
      case AnimState.RD: {
        this.frame = this.frame === OF.ExLaxFirst ? OF.ExLaxLast : OF.ExLaxFirst;
        break;
      }
      case AnimState.Push: {
        const base = this.pushBase();
        this.frame = this.frame === base ? base + 1 : base;
        if (!this.isPushing) {
          if (this.pushHack) {
            this.pushHack = false;
          } else {
            this.animState = AnimState.Static;
            this.frame = OF.Bub;
            this.bubStep = 0;
          }
        }
        break;
      }
      case AnimState.Moving: {
        const base = this.lastHoriz === "left" ? OF.LeftFirst : OF.RightFirst;
        this.walkStep = (this.walkStep + 1) % 3;
        this.frame = base + this.walkStep;
        if (this.dx === 0 && this.dy === 0) {
          this.animState = AnimState.Static;
          this.frame = OF.Bub;
          this.bubStep = 0;
        }
        break;
      }
      case AnimState.Yawn: {
        if (g.tickCount - g.lastPressTick < SAD_START) {
          // key activity woke him up
          this.animState = AnimState.Static;
          this.frame = OF.Bub;
          this.yawnDone = false;
        } else if (this.frame < OF.YawnFirst + 11) {
          this.frame += 1;
        } else {
          this.yawnDone = true; // hold the last yawn frame
        }
        break;
      }
      case AnimState.Static: {
        const idle = g.tickCount - g.lastPressTick;
        if (idle >= SAD_START && idle < SAD_END) {
          this.frame = OF.Sad;
        } else if (idle >= SAD_END && idle < FU_END) {
          this.frame = OF.FingerYou;
        } else if (idle >= FU_END && idle < EUREKA_END) {
          this.frame = OF.Eureka;
        } else if (idle >= EUREKA_END) {
          this.animState = AnimState.Yawn;
          this.frame = OF.YawnFirst;
          this.yawnDone = false;
        } else {
          // 4-direction bubble idle: 0 -> 1+2a -> 2+2a -> 1+2a -> 0 ...
          this.bubStep = (this.bubStep + 1) % 4;
          if (this.bubStep === 1) this.bubAxis = g.rand(0, 3);
          switch (this.bubStep) {
            case 1:
            case 3:
              this.frame = 1 + 2 * this.bubAxis;
              break;
            case 2:
              this.frame = 2 + 2 * this.bubAxis;
              break;
            default:
              this.frame = OF.Bub;
          }
        }
        break;
      }
    }
    this.image = OLIVER_TILE_BASE + this.frame;
  }

  private pushBase(): number {
    switch (this.pushDir) {
      case Dir.Left: return OF.PushLeftFirst;
      case Dir.Right: return OF.PushRightFirst;
      case Dir.Down: return OF.PushDownFirst;
      default: return OF.PushUpFirst;
    }
  }
}
