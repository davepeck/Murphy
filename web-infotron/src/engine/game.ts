// The Infotron engine, ported from the final "NO OOP" Think Pascal sources
// (main.p, everything.p, TheChores.p, TheGridPieces.p, dataIntf.p).
//
// The board is a grid of integers; each value is simultaneously the icl8 icon
// resource ID and the logical piece. There are no sprites: pieces "move" by
// rewriting cells, one whole tile at a time, at ~12 game frames per second
// (one frame per 5 Mac ticks). Chores are per-cell tasks performed once per
// frame, with additions/removals deferred to the next frame.

import {
  IC,
  Kind,
  Dir,
  classify,
  flagsOf,
  isMurphyIcon,
  isExplosionIcon,
  dirDX,
  dirDY,
  turnLeft,
  turnRight,
  oppositeRoll,
} from "./icons";

export interface InfotronLevelData {
  id: number;
  name: string;
  sizeX: number;
  sizeY: number;
  /** 0-based Murphy start */
  murphy: [number, number];
  quota: number;
  /** row-major sizeX*sizeY, 0-based */
  board: number[];
}

export interface FrameInput {
  up: boolean;
  right: boolean;
  down: boolean;
  left: boolean;
  space: boolean;
}

export type SoundSink = (id: number) => void;
export type Rng = (min: number, max: number) => number;

const TICKS_PER_FRAME = 5; // kTicksBetween: ~12 fps
const BUG_SOUND_RANGE = 4;

// Sound resource IDs (InfoSounds; see infotron-reference §13 and the
// graphics spec §8 corrections: 130/136/143 never shipped; v1.1 ships an
// unreferenced 151 "endLevel" which we play on winning — unverified but
// almost certainly its purpose).
export const SND = {
  gotInfotron: 128,
  thud: 129,
  win: 151,
  explosion: 131,
  quarkExplosion: 132,
  usedRedDisk: 133,
  redDiskReady: 134,
  gotLastInfotron: 135,
  died: 137,
  pause: 138,
  unpause: 139,
  bugZap: 140,
  push: 141,
} as const;

interface Chore {
  removed: boolean;
  /** Perform one frame; call g.removeChore(this) to finish. */
  perform(g: InfotronGame): void;
}

export class InfotronGame {
  readonly sizeX: number;
  readonly sizeY: number;
  readonly board: Int32Array;
  private fallingInfo: Uint8Array; // 1 = free to move into
  private fallChoresMap: Uint8Array; // 1 = a fall/move chore owns this cell

  private chores: Chore[] = [];
  private toBeAdded: Chore[] = [];
  private toBeRidded: Chore[] = [];

  tickCount = 0;
  /** true after any frame in which chores ran (renderer repaint hint) */
  boardDirty = true;

  // Murphy state
  murphyX: number;
  murphyY: number;
  numInfos: number;
  numBombs = 0;
  lastDir: Dir = Dir.Right;
  private lastFacingLR: Dir = Dir.Right;
  private gPhase = 4; // walk-mouth phase: 0 big, 2 med, 4 small
  private fromMed = 0; // next after med: 0 big / 4 small
  private lastMoveTick = 0;
  dying = false; // gAARDVARK_PLATYPUSS_WOMBAT
  frozen = false; // freezeMurphy (input ignored, world keeps running)
  wonLevel = false;
  gameOver = false; // KillGameChore fired

  // portal transit
  private whatToBlank: number = IC.Blank;
  private lastWasPortal = false;
  private portalKeys: FrameInput | null = null;

  // red-disk drop (space held with no direction)
  private spaceWentDownTick = -1;
  private diskReadySoundPlayed = false;

  private nextRollDir: Dir = Dir.Right;

  readonly rand: Rng;
  readonly sound: SoundSink;

  constructor(level: InfotronLevelData, opts: { sound?: SoundSink; rand?: Rng } = {}) {
    this.sizeX = level.sizeX;
    this.sizeY = level.sizeY;
    this.board = Int32Array.from(level.board);
    this.fallingInfo = new Uint8Array(this.board.length);
    this.fallChoresMap = new Uint8Array(this.board.length);
    this.rand = opts.rand ?? ((min, max) => min + Math.floor(Math.random() * (max - min + 1)));
    this.sound = opts.sound ?? (() => {});
    this.numInfos = level.quota;
    this.murphyX = level.murphy[0];
    this.murphyY = level.murphy[1];

    // StartNewLevel: place Murphy, then scan every cell for initial state.
    this.setDataRaw(this.murphyX, this.murphyY, IC.WalkBase + 1); // facing right
    for (let x = 0; x < this.sizeX; x++) {
      for (let y = 0; y < this.sizeY; y++) {
        const v = this.data(x, y);
        this.fallingInfo[this.idx(x, y)] = v === IC.Blank ? 1 : 0;
        const f = flagsOf(v);
        if (f.falls) {
          if (this.data(x, y + 1) === IC.Blank) {
            this.addChoreNow(new SuicideChore(x, y));
          } else if (flagsOf(this.data(x, y + 1)).rounded) {
            const leftOK = this.data(x - 1, y) === IC.Blank && this.data(x - 1, y + 1) === IC.Blank;
            const rightOK = this.data(x + 1, y) === IC.Blank && this.data(x + 1, y + 1) === IC.Blank;
            if (leftOK || rightOK) this.addChoreNow(new SuicideChore(x, y));
          }
        } else if (v >= IC.BugFirst && v <= IC.BugFirst + 4) {
          this.addChoreNow(new BugChore(x, y, this));
          this.setDataRaw(x, y, IC.Base); // dormant bug looks and acts like base
        } else if (v >= IC.TerminalFirst && v <= IC.TerminalLast) {
          this.addChoreNow(new TerminalChore(x, y));
        } else if (f.moves) {
          if (classify(v) === Kind.Quark) this.addChoreNow(new QuarkMover(x, y));
          else this.addChoreNow(new ScissorMover(x, y, (v % 8) as Dir));
        }
      }
    }
  }

  // --- board access -----------------------------------------------------------

  idx(x: number, y: number): number {
    return y * this.sizeX + x;
  }

  inBounds(x: number, y: number): boolean {
    return x >= 0 && y >= 0 && x < this.sizeX && y < this.sizeY;
  }

  data(x: number, y: number): number {
    if (!this.inBounds(x, y)) return IC.HardwareFirst; // treat outside as wall
    return this.board[this.idx(x, y)] as number;
  }

  private setDataRaw(x: number, y: number, v: number): void {
    if (!this.inBounds(x, y)) return;
    this.board[this.idx(x, y)] = v;
    this.boardDirty = true;
  }

  /** SetData: the reactive update rule — blanking a cell triggers gravity. */
  setData(x: number, y: number, v: number): void {
    if (!this.inBounds(x, y)) return;
    this.setDataRaw(x, y, v);
    if (v === IC.Blank) {
      this.fallingInfo[this.idx(x, y)] = 1;
      this.checkFall(x, y - 1);
      this.checkRoll(x - 1, y - 1, Dir.Right);
      this.checkRoll(x + 1, y - 1, Dir.Left);
      this.checkRoll(x - 1, y, Dir.Right);
      this.checkRoll(x + 1, y, Dir.Left);
    } else {
      this.fallingInfo[this.idx(x, y)] = 0;
    }
  }

  moveInfo(x: number, y: number): boolean {
    if (!this.inBounds(x, y)) return false;
    return this.fallingInfo[this.idx(x, y)] === 1;
  }

  moveThere(x: number, y: number, free: boolean): void {
    if (!this.inBounds(x, y)) return;
    this.fallingInfo[this.idx(x, y)] = free ? 1 : 0;
  }

  fallChoreAt(x: number, y: number): boolean {
    if (!this.inBounds(x, y)) return false;
    return this.fallChoresMap[this.idx(x, y)] === 1;
  }

  setFallChore(x: number, y: number, on: boolean): void {
    if (!this.inBounds(x, y)) return;
    this.fallChoresMap[this.idx(x, y)] = on ? 1 : 0;
  }

  // --- chores -------------------------------------------------------------------

  /** FudADD: fall/move chores are deduped per cell via the fallChores map. */
  addChore(c: Chore): void {
    if (c instanceof FallChore || c instanceof MoveChore || c instanceof FallRollChore) {
      const { x, y } = c as unknown as { x: number; y: number };
      if (this.fallChoreAt(x, y)) return; // discarded
      this.setFallChore(x, y, true);
    }
    this.toBeAdded.push(c);
  }

  private addChoreNow(c: Chore): void {
    if (c instanceof FallChore || c instanceof MoveChore || c instanceof FallRollChore) {
      const { x, y } = c as unknown as { x: number; y: number };
      if (this.fallChoreAt(x, y)) return;
      this.setFallChore(x, y, true);
    }
    this.chores.push(c);
  }

  removeChore(c: Chore): void {
    if (c.removed) return; // idempotent
    c.removed = true;
    this.toBeRidded.push(c);
    if (c instanceof FallChore || c instanceof MoveChore || c instanceof FallRollChore) {
      const { x, y } = c as unknown as { x: number; y: number };
      this.setFallChore(x, y, false);
    }
  }

  private doAllChores(): void {
    // Removals first (swap-with-last), then additions, then perform in order.
    for (const c of this.toBeRidded) {
      const i = this.chores.indexOf(c);
      if (i >= 0) {
        this.chores[i] = this.chores[this.chores.length - 1] as Chore;
        this.chores.pop();
      }
    }
    this.toBeRidded = [];
    for (const c of this.toBeAdded) this.chores.push(c);
    this.toBeAdded = [];
    const n = this.chores.length;
    for (let i = 0; i < n && i < this.chores.length; i++) {
      const c = this.chores[i] as Chore;
      if (!c.removed) c.perform(this);
    }
    if (this.chores.length > 0) this.boardDirty = true;
  }

  // --- the frame ------------------------------------------------------------------

  frame(input: FrameInput): void {
    this.tickCount += TICKS_PER_FRAME;
    this.doAllChores();
    if (!this.frozen && !this.dying) this.murphyAct(input);
  }

  // --- gravity: CheckFall / CheckRoll / Roll ----------------------------------------

  checkFall(x: number, y: number): void {
    const v = this.data(x, y);
    if (!flagsOf(v).falls) return;
    if (!this.moveInfo(x, y + 1)) return;
    this.fall(x, y);
  }

  fall(x: number, y: number): void {
    const kind = classify(this.data(x, y));
    if (kind === Kind.GreenDisk) this.addChore(new FallChore(x, y, true));
    else this.addChore(new FallChore(x, y, false));
  }

  checkRoll(x: number, y: number, dir: Dir): boolean {
    const v = this.data(x, y);
    if (!flagsOf(v).falls) return false; // only zonks/infotrons/green fall...
    if (classify(v) === Kind.GreenDisk) return false; // ...and green can't roll
    const dx = dirDX(dir);
    if (this.fallChoreAt(x, y)) return false;
    if (this.fallChoreAt(x, y + 1)) return false;
    if (!this.moveInfo(x + dx, y)) return false;
    if (!this.moveInfo(x + dx, y + 1)) return false;
    if (!flagsOf(this.data(x, y + 1)).rounded) return false;
    this.roll(x, y, dir);
    return true;
  }

  roll(x: number, y: number, dir: Dir): void {
    const dx = dirDX(dir);
    this.moveThere(x + dx, y, false);
    this.moveThere(x + dx, y + 1, false);
    this.addChore(new MoveChore(x, y, dir));
  }

  toggleRollDir(): void {
    this.nextRollDir = this.rand(0, 1) === 0 ? Dir.Left : Dir.Right;
  }

  get rollDir(): Dir {
    return this.nextRollDir;
  }

  // --- explosions --------------------------------------------------------------------

  /** Explode the piece at (x,y) according to its kind (CanExplode dispatch). */
  explodePiece(x: number, y: number): void {
    const kind = classify(this.data(x, y));
    if (kind === Kind.Murphy) {
      this.killMurphy();
    } else if (kind === Kind.Quark) {
      this.quarkExplode(x, y);
    } else {
      this.startExplosion(x, y);
    }
  }

  /** A full 3x3 explosion centered here (CExplChore). */
  startExplosion(x: number, y: number): void {
    this.addChore(new ExplChore(x, y));
    this.setDataRaw(x, y, IC.ExplFirst);
  }

  /** Quark death: 3x3 that leaves a block of infotrons (CQuark.Explode). */
  quarkExplode(x: number, y: number): void {
    this.sound(SND.quarkExplosion);
    this.setDataRaw(x, y, IC.InfoSnapLast); // 358
    const seq = [358, 357, 356, 355, 354, 353, 352, 351, IC.Infotron];
    for (let dy = -1; dy <= 1; dy++) {
      for (let dx = -1; dx <= 1; dx++) {
        const cx = x + dx;
        const cy = y + dy;
        if (!this.inBounds(cx, cy)) continue;
        if (classify(this.data(cx, cy)) === Kind.Hardware) continue;
        if (dx !== 0 || dy !== 0) {
          const k = classify(this.data(cx, cy));
          if (flagsOf(this.data(cx, cy)).explodes || k === Kind.Murphy) {
            this.explodePiece(cx, cy);
          }
        }
        this.addChore(new AnimChore(cx, cy, seq, true));
        this.addChore(new WaitInsChore(9, () => this.addChore(new SuicideChore(cx, cy))));
      }
    }
  }

  /** Destroy (no chain): the 600-609->129 animation, if destructable. */
  destroyPiece(x: number, y: number): void {
    if (!flagsOf(this.data(x, y)).destructable) return;
    this.addChore(new AnimChore(x, y, [600, 601, 602, 603, 604, 605, 606, 607, 608, 609, IC.Blank], false));
    this.setDataRaw(x, y, IC.ExplFirst);
  }

  // --- Murphy ---------------------------------------------------------------------------

  private murphyAct(input: FrameInput): void {
    // Forced continuation out of a portal.
    if (this.lastWasPortal && this.portalKeys) {
      const keys = this.portalKeys;
      this.lastWasPortal = false;
      this.portalKeys = null;
      this.doMurphyInput(keys, true);
      return;
    }
    this.doMurphyInput(input, false);
  }

  private prepareDir(input: FrameInput): Dir {
    if (input.up) return Dir.Up;
    if (input.right) return Dir.Right;
    if (input.down) return Dir.Down;
    if (input.left) return Dir.Left;
    return Dir.None;
  }

  private doMurphyInput(input: FrameInput, fromPortal: boolean): void {
    const dir = this.prepareDir(input);

    // Space with no direction: red-disk drop timer (release to drop).
    if (input.space && dir === Dir.None) {
      if (this.spaceWentDownTick < 0) {
        this.spaceWentDownTick = this.tickCount;
        this.diskReadySoundPlayed = false;
      } else if (
        this.tickCount - this.spaceWentDownTick >= 90 &&
        this.numBombs > 0 &&
        !this.diskReadySoundPlayed
      ) {
        this.diskReadySoundPlayed = true;
        this.sound(SND.redDiskReady);
      }
      this.idleTick();
      return;
    }
    if (!input.space && this.spaceWentDownTick >= 0) {
      // Space released: drop a red disk if it was held long enough.
      const held = this.tickCount - this.spaceWentDownTick;
      this.spaceWentDownTick = -1;
      if (held >= 90 && this.numBombs > 0) {
        this.numBombs -= 1;
        this.sound(SND.usedRedDisk);
        this.whatToBlank = IC.DroppedRedDisk;
        const bx = this.murphyX;
        const by = this.murphyY;
        this.addChore(new WaitTimeChore(2, () => this.startExplosion(bx, by)));
      }
    }

    if (dir === Dir.None) {
      this.idleTick();
      return;
    }
    this.lastMoveTick = this.tickCount;

    if (input.space && !fromPortal) {
      this.spaceWentDownTick = -1; // a snap cancels a pending drop
      this.snap(dir);
      return;
    }

    this.lastDir = dir;
    if (dir === Dir.Left || dir === Dir.Right) this.lastFacingLR = dir;
    const nx = this.murphyX + dirDX(dir);
    const ny = this.murphyY + dirDY(dir);
    this.murphyMove(nx, ny, dir, input);
  }

  private idleTick(): void {
    // Idle: standing icon after 15 ticks without a move.
    if (this.tickCount - this.lastMoveTick >= 15) {
      const cur = this.data(this.murphyX, this.murphyY);
      if (isMurphyIcon(cur) && cur !== IC.Standing && cur < IC.SleepFirst) {
        this.setDataRaw(this.murphyX, this.murphyY, IC.Standing);
        this.gPhase = 2;
        this.fromMed = 0;
      }
    }
  }

  private walkIcon(dir: Dir): number {
    // 201 + (0 left | 1 right) + phase (0 big, 2 med, 4 small)
    const lr = dir === Dir.Left || dir === Dir.Right ? dir : this.lastFacingLR;
    const facing = lr === Dir.Right ? 1 : 0;
    // advance mouth phase: big/small -> med; med -> alternate big/small
    if (this.gPhase === 2) {
      this.gPhase = this.fromMed;
      this.fromMed = 4 - this.fromMed;
    } else {
      this.gPhase = 2;
    }
    return IC.WalkBase + facing + this.gPhase;
  }

  private murphyMove(nx: number, ny: number, dir: Dir, input: FrameInput): void {
    const num = this.data(nx, ny);
    if (isExplosionIcon(num) || (num >= IC.InfoSnapFirst && num <= IC.InfoSnapLast)) {
      this.killMurphy();
      return;
    }
    const kind = classify(num);
    let walked = false;
    let pushed = false;

    switch (kind) {
      case Kind.Blank:
      case Kind.Base:
        walked = true;
        break;
      case Kind.Infotron:
        this.gotInfotron();
        walked = true;
        break;
      case Kind.RedDisk:
        this.gotRedDisk();
        walked = true;
        break;
      case Kind.Zonk:
      case Kind.GreenDisk:
        if (dir === Dir.Left || dir === Dir.Right) {
          pushed = this.push(nx, ny, dir, kind);
          walked = pushed;
        }
        break;
      case Kind.YellowDisk:
        pushed = this.pushYellow(nx, ny, dir);
        walked = pushed;
        break;
      case Kind.Bug:
        this.explodeTwo(nx, ny);
        return;
      case Kind.Scissor:
      case Kind.Quark:
        this.killMurphy();
        return;
      case Kind.Exit:
        if (this.numInfos < 1) this.winGame();
        return;
      case Kind.Portal:
        this.tryPortal(nx, ny, dir, input);
        return;
      case Kind.Terminal:
        this.touchTerminal(); // detonates every yellow disk; Murphy stays put
        return;
      default:
        return; // chip/hardware/inert: blocked
    }

    if (!walked || this.dying) return;

    // Move: destination becomes Murphy, origin becomes whatToBlank.
    const ox = this.murphyX;
    const oy = this.murphyY;
    this.murphyX = nx;
    this.murphyY = ny;
    if (pushed) {
      this.setDataRaw(nx, ny, dir === Dir.Left ? IC.PushLeft : dir === Dir.Right ? IC.PushRight : this.walkIcon(dir));
      this.sound(SND.push);
    } else {
      this.setDataRaw(nx, ny, this.walkIcon(dir));
    }
    const blankTo = this.whatToBlank;
    this.whatToBlank = IC.Blank;
    this.setData(ox, oy, blankTo);
  }

  /** Terminal bump: detonate every yellow disk on the board (§10). */
  touchTerminal(): void {
    for (let x = 0; x < this.sizeX; x++) {
      for (let y = 0; y < this.sizeY; y++) {
        if (this.data(x, y) === IC.YellowDisk) this.startExplosion(x, y);
      }
    }
  }

  private tryPortal(px: number, py: number, dir: Dir, input: FrameInput): void {
    const icon = this.data(px, py);
    let allowed = false;
    if (icon === IC.Portal4Way) allowed = true;
    else if (icon === IC.PortalVertical) allowed = dir === Dir.Up || dir === Dir.Down;
    else if (icon === IC.PortalHorizontal) allowed = dir === Dir.Left || dir === Dir.Right;
    else allowed = icon % 8 === dir;
    if (!allowed) return;
    const bx = px + dirDX(dir);
    const by = py + dirDY(dir);
    if (this.data(bx, by) !== IC.Blank) return;
    // Transit: Murphy conceptually crosses the portal; the far-side move is
    // forced next frame with the currently held keys.
    const ox = this.murphyX;
    const oy = this.murphyY;
    this.murphyX = px;
    this.murphyY = py;
    this.whatToBlank = icon; // leaving restores the portal
    this.lastWasPortal = true;
    this.portalKeys = { ...input, space: false };
    this.setData(ox, oy, IC.Blank);
    // The board keeps the PORTAL icon during transit (the original draws
    // Murphy half-through at pixel offsets); the renderer overlays Murphy at
    // (murphyX, murphyY) when the cell there is a portal.
  }

  private push(px: number, py: number, dir: Dir, kind: Kind): boolean {
    const dx = dirDX(dir);
    const bx = px + dx;
    if (this.data(bx, py) !== IC.Blank) return false;
    if (kind === Kind.Zonk) {
      if (this.fallChoreAt(px, py)) {
        // pushing a currently-falling zonk kills Murphy
        this.killMurphy();
        return false;
      }
      // move the zonk with a rotation step
      const cur = this.data(px, py);
      let rot = cur === IC.Zonk ? IC.Zonk : cur;
      rot = IC.Zonk + ((((rot - IC.Zonk + (dir === Dir.Right ? 1 : -1)) % 4) + 4) % 4);
      this.setDataRaw(bx, py, rot);
      this.moveThere(bx, py, false);
      if (this.moveInfo(bx, py + 1)) {
        this.addChore(new FallRollChore(bx, py, dir));
      }
      this.addChore(new WaitInsChore(3, () => this.addChore(new RollAfterChore(bx, py, dir))));
      return true;
    }
    // green disk: move, then it may fall (and explode on landing)
    this.setDataRaw(bx, py, IC.GreenDisk);
    this.moveThere(bx, py, false);
    this.checkFall(bx, py);
    return true;
  }

  private pushYellow(px: number, py: number, dir: Dir): boolean {
    const bx = px + dirDX(dir);
    const by = py + dirDY(dir);
    if (this.data(bx, by) !== IC.Blank) return false;
    this.setDataRaw(bx, by, IC.YellowDisk);
    this.moveThere(bx, by, false);
    return true;
  }

  /** Snap: space + direction eats an adjacent cell without moving (§6.4). */
  private snap(dir: Dir): void {
    const tx = this.murphyX + dirDX(dir);
    const ty = this.murphyY + dirDY(dir);
    const v = this.data(tx, ty);
    const pose =
      dir === Dir.Left ? IC.SnapLeft : dir === Dir.Right ? IC.SnapRight : dir === Dir.Up ? IC.SnapUp : IC.SnapDown;
    if (v >= IC.BugFirst && v <= IC.BugLast) {
      this.explodeTwo(this.murphyX, this.murphyY);
      return;
    }
    if (v === IC.Infotron) {
      this.setDataRaw(this.murphyX, this.murphyY, pose);
      this.gotInfotron();
      this.setData(tx, ty, IC.Blank);
    } else if (v === IC.RedDisk) {
      this.setDataRaw(this.murphyX, this.murphyY, pose);
      this.gotRedDisk();
      this.setData(tx, ty, IC.Blank);
    } else if (v === IC.Base) {
      this.setDataRaw(this.murphyX, this.murphyY, pose);
      this.setData(tx, ty, IC.Blank);
    }
    // anything else: no effect
  }

  gotInfotron(): void {
    if (this.numInfos > 0) {
      this.numInfos -= 1;
      this.sound(this.numInfos === 0 ? SND.gotLastInfotron : SND.gotInfotron);
    } else {
      this.sound(SND.gotInfotron);
    }
  }

  gotRedDisk(): void {
    this.numBombs += 1;
    this.sound(SND.usedRedDisk);
    if (this.numBombs > 99) this.killMurphy(); // the >99 easter-egg overflow
  }

  /** Murphy dies at his own cell (KillMurphy). */
  killMurphy(): void {
    if (this.dying) return;
    this.dying = true;
    this.startExplosion(this.murphyX, this.murphyY);
    this.setDataRaw(this.murphyX, this.murphyY, IC.CorpseA);
    this.endLevel();
    this.sound(SND.died);
  }

  /** Bug contact: Murphy dies at the bug's cell (ExplodeTWO). */
  explodeTwo(x: number, y: number): void {
    if (this.dying) return;
    this.dying = true;
    this.setData(this.murphyX, this.murphyY, IC.Blank);
    this.murphyX = x;
    this.murphyY = y;
    this.startExplosion(x, y);
    this.setDataRaw(x, y, IC.CorpseB);
    this.endLevel();
    this.sound(SND.died);
  }

  private winGame(): void {
    this.wonLevel = true;
    this.sound(SND.win);
    this.addChore(
      new AnimChore(this.murphyX, this.murphyY, [1200, 1201, 1202, 1203, 1204, 1205, 1206, 1207, 1208, 1209, 1210, 1211], false)
    );
    this.endLevel();
  }

  /** EndLevel: freeze input, keep simulating 4 s, then the game is over. */
  private endLevel(): void {
    this.frozen = true;
    this.addChore(new WaitTimeChore(4, () => (this.gameOver = true)));
  }

  murphyIsAt(x: number, y: number): boolean {
    return this.murphyX === x && this.murphyY === y && !this.dying;
  }

  bugSoundIfNear(x: number, y: number): void {
    const d = Math.abs(this.murphyX - x) + Math.abs(this.murphyY - y);
    if (d < BUG_SOUND_RANGE) this.sound(SND.bugZap);
  }
}

// --- chores ------------------------------------------------------------------------

/** Base helper: chores are per-cell tasks performed once per frame. */
abstract class BaseChore implements Chore {
  removed = false;
  abstract perform(g: InfotronGame): void;
}

/** Watches a cell that could not fall/roll yet and keeps trying (settle). */
export class SuicideChore extends BaseChore {
  private uselessCount = 0;
  constructor(
    private x: number,
    private y: number
  ) {
    super();
  }

  perform(g: InfotronGame): void {
    const v = g.data(this.x, this.y);
    if (!flagsOf(v).falls) {
      g.removeChore(this);
      return;
    }
    if (g.moveInfo(this.x, this.y + 1)) {
      g.setFallChore(this.x, this.y, false);
      g.checkFall(this.x, this.y);
      g.removeChore(this);
    } else if (
      g.checkRoll(this.x, this.y, g.rollDir) ||
      g.checkRoll(this.x, this.y, oppositeRoll(g.rollDir))
    ) {
      g.removeChore(this);
    } else {
      this.uselessCount += 1;
      if (this.uselessCount > 30) g.removeChore(this);
    }
    g.toggleRollDir();
  }
}

/** Plain falling: one cell per frame (CFallChore); green disks explode on landing. */
export class FallChore extends BaseChore {
  constructor(
    public x: number,
    public y: number,
    private explodeOnLand: boolean
  ) {
    super();
  }

  perform(g: InfotronGame): void {
    const v = g.data(this.x, this.y);
    if (isExplosionIcon(v)) {
      g.removeChore(this);
      return;
    }
    if (isMurphyIcon(v)) {
      // Murphy stepped into the falling piece's cell: he dies.
      g.killMurphy();
      g.removeChore(this);
      return;
    }
    if (v >= IC.InfoSnapFirst && v <= IC.InfoSnapLast) {
      g.removeChore(this); // snap stole the falling piece
      return;
    }
    const below = g.data(this.x, this.y + 1);
    const belowKind = classify(below);
    if (flagsOf(below).explodes || belowKind === Kind.Murphy) {
      // crush: detonate the victim
      g.explodePiece(this.x, this.y + 1);
      g.removeChore(this);
      return;
    }
    if (!g.moveInfo(this.x, this.y + 1)) {
      this.hitBottom(g);
      return;
    }
    // move down one cell
    g.setData(this.x, this.y + 1, v);
    g.setFallChore(this.x, this.y, false);
    g.setFallChore(this.x, this.y + 1, true);
    g.setData(this.x, this.y, IC.Blank);
    this.y += 1;
  }

  private hitBottom(g: InfotronGame): void {
    g.removeChore(this);
    if (this.explodeOnLand) {
      g.startExplosion(this.x, this.y);
      return;
    }
    if (!g.checkRoll(this.x, this.y, g.rollDir)) {
      g.checkRoll(this.x, this.y, oppositeRoll(g.rollDir));
    }
    g.toggleRollDir();
    g.sound(SND.thud);
  }
}

/** Sideways roll: 5-frame animation, occupancy transfers at frame 3 (CMoveChore). */
export class MoveChore extends BaseChore {
  private frames: number[];
  private frame = 0;
  private isInfotron: boolean;

  constructor(
    public x: number,
    public y: number,
    private dir: Dir
  ) {
    super();
    const v = 0; // decided at first perform from the live cell
    void v;
    this.isInfotron = false;
    this.frames = [];
  }

  perform(g: InfotronGame): void {
    const v = g.data(this.x, this.y);
    if (this.frame === 0) {
      this.isInfotron = classify(v) === Kind.Infotron;
      this.frames = this.isInfotron
        ? [850, 851, 852, 853, 850]
        : this.dir === Dir.Right
          ? [700, 701, 702, 703, 700]
          : [700, 703, 702, 701, 700];
    }
    if (isExplosionIcon(v) || (v >= IC.InfoSnapFirst && v <= IC.InfoSnapLast)) {
      g.removeChore(this);
      return;
    }
    if (this.isInfotron && isMurphyIcon(v)) {
      g.killMurphy();
      g.removeChore(this);
      return;
    }
    this.frame += 1;
    if (this.frame > this.frames.length) {
      this.sequenceDone(g);
      return;
    }
    g.setData(this.x, this.y, this.frames[this.frame - 1] as number);
    if (this.frame === 3) {
      // occupancy transfers to the destination
      const dx = dirDX(this.dir);
      const mtx = this.x + dx;
      if (g.data(mtx, this.y) === IC.Blank) {
        const cur = g.data(this.x, this.y);
        g.setData(mtx, this.y, cur);
        g.setFallChore(this.x, this.y, false);
        g.setFallChore(mtx, this.y, true);
        g.setData(this.x, this.y, IC.Blank);
        this.x = mtx;
      } else {
        // destination got blocked: un-reserve, stay put
        g.moveThere(this.x + dx, this.y, true);
        g.moveThere(this.x + dx, this.y + 1, true);
      }
    }
  }

  private sequenceDone(g: InfotronGame): void {
    g.removeChore(this);
    const below = g.data(this.x, this.y + 1);
    if (below === IC.Blank) {
      // The original calls Fall directly and un-reserves the cell below
      // (its own roll reserved it): "MoveThere(xq, yq + 1, TRUE); {? is this ok}"
      g.fall(this.x, this.y);
      g.moveThere(this.x, this.y + 1, true);
    } else if (isMurphyIcon(below)) {
      g.killMurphy();
    } else if (!g.checkRoll(this.x, this.y, g.rollDir)) {
      g.checkRoll(this.x, this.y, oppositeRoll(g.rollDir));
    }
    g.toggleRollDir();
  }
}

/** Pushed-zonk fall: keeps rotating the graphic while falling (CFallRollChore). */
export class FallRollChore extends BaseChore {
  constructor(
    public x: number,
    public y: number,
    private dir: Dir
  ) {
    super();
  }

  perform(g: InfotronGame): void {
    const v = g.data(this.x, this.y);
    if (isExplosionIcon(v) || isMurphyIcon(v)) {
      g.removeChore(this);
      return;
    }
    const below = g.data(this.x, this.y + 1);
    if (flagsOf(below).explodes || classify(below) === Kind.Murphy) {
      g.explodePiece(this.x, this.y + 1);
      g.removeChore(this);
      return;
    }
    if (!g.moveInfo(this.x, this.y + 1)) {
      g.setData(this.x, this.y, IC.Zonk); // normalize on landing
      g.removeChore(this);
      if (!g.checkRoll(this.x, this.y, g.rollDir)) {
        g.checkRoll(this.x, this.y, oppositeRoll(g.rollDir));
      }
      g.toggleRollDir();
      g.sound(SND.thud);
      return;
    }
    // rotate one step while falling
    let rot = v;
    if (rot >= IC.Zonk && rot <= IC.Zonk + 3) {
      rot = IC.Zonk + ((((rot - IC.Zonk + (this.dir === Dir.Right ? 1 : -1)) % 4) + 4) % 4);
    }
    g.setData(this.x, this.y + 1, rot);
    g.setFallChore(this.x, this.y, false);
    g.setFallChore(this.x, this.y + 1, true);
    g.setData(this.x, this.y, IC.Blank);
    this.y += 1;
  }
}

/** After a push: rotate the zonk graphic back to canonical, then settle. */
export class RollAfterChore extends BaseChore {
  private uselessCount = 0;
  constructor(
    private x: number,
    private y: number,
    private dir: Dir
  ) {
    super();
  }

  perform(g: InfotronGame): void {
    const v = g.data(this.x, this.y);
    if (v > IC.Zonk && v <= IC.Zonk + 3) {
      const rot = IC.Zonk + ((((v - IC.Zonk + (this.dir === Dir.Right ? 1 : -1)) % 4) + 4) % 4);
      g.setData(this.x, this.y, rot);
      return;
    }
    if (v !== IC.Zonk) {
      g.removeChore(this);
      return;
    }
    if (g.moveInfo(this.x, this.y + 1)) {
      g.setFallChore(this.x, this.y, false);
      g.checkFall(this.x, this.y);
      g.removeChore(this);
    } else if (
      g.checkRoll(this.x, this.y, g.rollDir) ||
      g.checkRoll(this.x, this.y, oppositeRoll(g.rollDir))
    ) {
      g.removeChore(this);
    } else {
      this.uselessCount += 1;
      if (this.uselessCount > 30) g.removeChore(this);
    }
    g.toggleRollDir();
  }
}

/** The canonical 3x3 explosion (CExplChore). */
export class ExplChore extends BaseChore {
  private frame = 0;
  private static readonly ANIM = [600, 601, 602, 603, 604, 605, 606, 607, 608, 609, IC.Blank];

  constructor(
    private x: number,
    private y: number
  ) {
    super();
  }

  perform(g: InfotronGame): void {
    this.frame += 1;
    if (this.frame > ExplChore.ANIM.length) {
      g.removeChore(this);
      return;
    }
    if (this.frame === 1 && g.murphyIsAt(this.x, this.y)) {
      g.killMurphy();
    }
    if (this.frame === 3) {
      g.sound(SND.explosion);
      for (let dy = -1; dy <= 1; dy++) {
        for (let dx = -1; dx <= 1; dx++) {
          if (dx === 0 && dy === 0) continue;
          const cx = this.x + dx;
          const cy = this.y + dy;
          const v = g.data(cx, cy);
          const kind = classify(v);
          if (flagsOf(v).explodes || kind === Kind.Murphy) {
            g.explodePiece(cx, cy);
          } else {
            g.destroyPiece(cx, cy);
          }
        }
      }
    }
    const icon = ExplChore.ANIM[this.frame - 1] as number;
    if (icon === IC.Blank) g.setData(this.x, this.y, IC.Blank);
    else g.setData(this.x, this.y, icon);
    if (this.frame === ExplChore.ANIM.length) g.removeChore(this);
  }
}

/** Generic per-frame cell animation; abortIfExploding mirrors the quark anim. */
export class AnimChore extends BaseChore {
  private frame = 0;
  constructor(
    private x: number,
    private y: number,
    private seq: number[],
    private abortIfExploding: boolean
  ) {
    super();
  }

  perform(g: InfotronGame): void {
    if (this.abortIfExploding && isExplosionIcon(g.data(this.x, this.y)) && this.frame > 0) {
      g.removeChore(this);
      return;
    }
    if (this.frame >= this.seq.length) {
      g.removeChore(this);
      return;
    }
    const icon = this.seq[this.frame] as number;
    if (icon === IC.Blank) g.setData(this.x, this.y, IC.Blank);
    else g.setData(this.x, this.y, icon);
    this.frame += 1;
    if (this.frame >= this.seq.length) g.removeChore(this);
  }
}

/** Bug: dormant (looks like base) 2-6 s, then a 7-frame deadly zap. */
export class BugChore extends BaseChore {
  private wait: number;
  private animFrame = -1;
  private static readonly ANIM = [300, 301, 302, 303, 304, 305, IC.Base];

  constructor(
    private x: number,
    private y: number,
    g: InfotronGame
  ) {
    super();
    this.wait = g.rand(1, 240) + 120;
  }

  perform(g: InfotronGame): void {
    const v = g.data(this.x, this.y);
    const legal = v === IC.Base || (v >= IC.BugFirst && v <= IC.BugLast) || (v >= IC.TerminalFirst && v <= IC.TerminalLast);
    if (!legal) {
      g.removeChore(this);
      return;
    }
    if (this.animFrame < 0) {
      this.wait -= 5; // ticks per frame
      if (this.wait <= 0) {
        this.animFrame = 0;
        g.bugSoundIfNear(this.x, this.y);
      }
      return;
    }
    g.setData(this.x, this.y, BugChore.ANIM[this.animFrame] as number);
    this.animFrame += 1;
    if (this.animFrame >= BugChore.ANIM.length) {
      this.animFrame = -1;
      this.wait = g.rand(1, 240) + 120;
    }
  }
}

/** Terminal: endless 550-560 cycle. */
export class TerminalChore extends BaseChore {
  private frame = 0;
  constructor(
    private x: number,
    private y: number
  ) {
    super();
  }

  perform(g: InfotronGame): void {
    const v = g.data(this.x, this.y);
    if (!(v >= IC.TerminalFirst && v <= IC.TerminalLast)) {
      g.removeChore(this);
      return;
    }
    g.setData(this.x, this.y, IC.TerminalFirst + this.frame);
    this.frame = (this.frame + 1) % 11; // 550..560
  }
}

/** Waits N frames, then runs an action (CWaitInsChore). */
export class WaitInsChore extends BaseChore {
  constructor(
    private frames: number,
    private action: () => void
  ) {
    super();
  }

  perform(g: InfotronGame): void {
    this.frames -= 1;
    if (this.frames <= 0) {
      g.removeChore(this);
      this.action();
    }
  }
}

/** Waits wall-clock seconds, then runs an action (CWaitTimeChore). */
export class WaitTimeChore extends BaseChore {
  private fireAt = -1;
  constructor(
    private seconds: number,
    private action: () => void
  ) {
    super();
  }

  perform(g: InfotronGame): void {
    if (this.fireAt < 0) this.fireAt = g.tickCount + this.seconds * 60;
    if (g.tickCount >= this.fireAt) {
      g.removeChore(this);
      this.action();
    }
  }
}

// --- enemies -------------------------------------------------------------------------

/** Both movers act every other frame: one movement pass, one animation pass. */
abstract class Mover extends BaseChore {
  protected shouldMove = false;
  constructor(
    protected x: number,
    protected y: number,
    protected dir: Dir
  ) {
    super();
  }

  protected abstract myIcons(v: number): boolean;
  protected abstract animPass(g: InfotronGame): void;
  protected abstract movePass(g: InfotronGame): void;

  perform(g: InfotronGame): void {
    const v = g.data(this.x, this.y);
    if (isExplosionIcon(v)) return; // idle until the explosion resolves
    if (!this.myIcons(v)) {
      g.removeChore(this);
      return;
    }
    this.shouldMove = !this.shouldMove;
    if (this.shouldMove) this.movePass(g);
    else this.animPass(g);
  }

  /** Left-wall follow: check Left, Straight, Right; kill Murphy on contact. */
  protected scan(g: InfotronGame): Dir | null {
    for (const d of [turnLeft(this.dir), this.dir, turnRight(this.dir)]) {
      const tx = this.x + dirDX(d);
      const ty = this.y + dirDY(d);
      const t = g.data(tx, ty);
      if (isMurphyIcon(t)) {
        g.killMurphy();
        return null;
      }
      if (t === IC.Blank) return d;
    }
    return null;
  }

  protected moveTo(g: InfotronGame, d: Dir, icon: number): void {
    const tx = this.x + dirDX(d);
    const ty = this.y + dirDY(d);
    g.setData(tx, ty, icon);
    g.setData(this.x, this.y, IC.Blank);
    this.x = tx;
    this.y = ty;
    this.dir = d;
  }
}

export class QuarkMover extends Mover {
  private anim = 0;
  constructor(x: number, y: number) {
    super(x, y, Dir.Right);
  }

  protected myIcons(v: number): boolean {
    return v >= IC.QuarkFirst && v <= IC.QuarkLast;
  }

  protected animPass(g: InfotronGame): void {
    this.anim = (this.anim + 1) % 8;
    // The original's anim pass writes 129 then the new frame; the momentary
    // blank fires the reactive fall checks — that is how a zonk resting on a
    // stationary quark starts falling and crush-detonates it.
    g.setData(this.x, this.y, IC.Blank);
    g.setData(this.x, this.y, IC.QuarkFirst + this.anim);
  }

  protected movePass(g: InfotronGame): void {
    const d = this.scan(g);
    this.anim = (this.anim + 1) % 8;
    if (d !== null) {
      this.moveTo(g, d, IC.QuarkFirst + this.anim);
    } else {
      this.dir = turnLeft(this.dir); // blocked: rotate in place (CCW)
      g.setData(this.x, this.y, IC.QuarkFirst + this.anim);
    }
  }
}

export class ScissorMover extends Mover {
  private size = 0; // 0 small, 10 open, 20 medium
  private afterMed = 10;
  private turnAround = false;
  private forceNextMove = false;
  private goingValue = 1;

  constructor(x: number, y: number, dir: Dir) {
    super(x, y, (dir % 8) as Dir);
  }

  protected myIcons(v: number): boolean {
    return (
      (v >= IC.ScissorFirst && v <= IC.ScissorMediumLast) ||
      (v >= IC.ScissorTurnFirst && v <= IC.ScissorTurnLast)
    );
  }

  private advanceSize(): void {
    if (this.size === 20) {
      this.size = this.afterMed;
      this.afterMed = 10 - this.afterMed;
    } else {
      this.size = 20;
    }
  }

  protected animPass(g: InfotronGame): void {
    this.advanceSize();
    // blank-then-rewrite, like the quark: lets resting zonks crush a
    // stationary scissor (emergent shipped behavior)
    g.setData(this.x, this.y, IC.Blank);
    g.setData(this.x, this.y, 1000 + this.dir + this.size);
  }

  protected movePass(g: InfotronGame): void {
    this.advanceSize();
    if (this.turnAround) {
      // second half of a blocked 180°: turn left once more
      this.turnAround = false;
      this.doTurn(g);
      return;
    }
    if (this.forceNextMove) {
      // quirk kept: moves straight ahead without re-checking the target
      this.forceNextMove = false;
      this.moveTo(g, this.dir, 1000 + this.dir + this.size);
      return;
    }
    for (const d of [turnLeft(this.dir), this.dir, turnRight(this.dir)]) {
      const tx = this.x + dirDX(d);
      const ty = this.y + dirDY(d);
      const t = g.data(tx, ty);
      if (isMurphyIcon(t)) {
        g.killMurphy();
        return;
      }
      if (t === IC.Blank) {
        if (d === this.dir) {
          this.moveTo(g, d, 1000 + d + this.size);
        } else {
          // turning costs a beat: show a turn graphic, move next pass
          this.goingValue = d === turnLeft(this.dir) ? -1 : 1;
          this.dir = d;
          this.forceNextMove = true;
          let icon = this.dir + 1000 + this.goingValue;
          if (icon === 999) icon = 1007;
          g.setData(this.x, this.y, icon);
        }
        return;
      }
    }
    // all three directions blocked: 90° now, another 90° next movement pass
    this.doTurn(g);
    this.turnAround = true;
  }

  private doTurn(g: InfotronGame): void {
    this.dir = turnLeft(this.dir);
    g.setData(this.x, this.y, this.dir + 1000);
  }
}

