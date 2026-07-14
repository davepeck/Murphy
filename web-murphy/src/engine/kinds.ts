// Piece property table, a faithful port of GetPieceKind (MRPlayCommon.c).

import { T } from "./tiles";

export interface PieceKind {
  isPassable: boolean;
  isRounded: boolean;
  isExplodable: boolean;
  isIgnitable: boolean;
  isPort: boolean;
  isDeadly: boolean;
  isElectric: boolean;
  isEdible: boolean;
  isPushable: boolean;
  canFall: boolean;
  canRoll: boolean;
  /** +1 falls down, -1 falls up (up-zonk) */
  fallRowDelta: 1 | -1;
}

function base(): PieceKind {
  return {
    isPassable: false,
    isRounded: false,
    isExplodable: true,
    isIgnitable: false,
    isPort: false,
    isDeadly: false,
    isElectric: false,
    isEdible: false,
    isPushable: false,
    canFall: false,
    canRoll: false,
    fallRowDelta: 1,
  };
}

const table: PieceKind[] = [];
for (let id = 0; id < T.Max; id++) {
  const k = base();
  switch (id) {
    case T.Circuit:
      k.isEdible = true;
      k.isPassable = true;
      break;
    case T.Blank:
      k.isPassable = true;
      break;
    case T.BlankChip:
      k.isPushable = true;
      k.isRounded = true;
      break;
    case T.Chip:
    case T.TopChip:
    case T.BottomChip:
    case T.LeftChip:
    case T.RightChip:
      k.isRounded = true;
      break;
    case T.Infotron:
      k.isPassable = true;
      k.isEdible = true;
      k.canFall = true;
      k.canRoll = true;
      k.isRounded = true;
      break;
    case T.GreenDisk:
      k.isIgnitable = true;
      k.isPushable = true;
      k.canFall = true;
      break;
    case T.RedDisk:
      k.isEdible = true;
      k.isPassable = true;
      break;
    case T.GoldDisk:
      k.isPushable = true;
      break;
    case T.FullPhase:
      k.isExplodable = false;
      break;
    case T.Terminal:
      k.isExplodable = false;
      break;
    // The original switch falls through UpZonk -> Zonk -> YellowDisk, so
    // zonks are pushable too.
    case T.UpZonk:
      k.fallRowDelta = -1;
      k.canFall = true;
      k.canRoll = true;
      k.isRounded = true;
      k.isPushable = true;
      break;
    case T.Zonk:
      k.canFall = true;
      k.canRoll = true;
      k.isRounded = true;
      k.isPushable = true;
      break;
    case T.YellowDisk:
      k.isPushable = true;
      break;
    case T.LastExit:
      k.isPassable = true;
      break;
    case T.LaserPower:
    case T.ForeMirror:
    case T.BackMirror:
    case T.LaserEmptyPiece:
      k.isPushable = true;
      break;
    default:
      if (id >= T.FirstZapInfotron && id <= T.LastZapInfotron) {
        k.isExplodable = false;
      } else if (id >= T.FirstExpl && id <= T.LastExpl) {
        k.isExplodable = false;
        k.isDeadly = true;
      } else if (id >= T.TopLeftBorder && id <= T.BottomRightBorder) {
        k.isExplodable = false;
      } else if (id >= T.FirstHardware && id <= T.LastHardware) {
        k.isExplodable = false;
      } else if (id >= T.LeftPort && id <= T.FourWayPort) {
        k.isPort = true;
      } else if (id >= T.FirstBug && id <= T.LastBug) {
        k.isPassable = true;
        k.isDeadly = true;
        k.isElectric = true;
      }
      break;
  }
  table.push(k);
}

export function getPieceKind(tile: number): PieceKind {
  const k = table[tile];
  if (!k) return base(); // out-of-range: inert default
  return k;
}

export function isPortTile(tile: number): boolean {
  return tile >= T.LeftPort && tile <= T.FourWayPort;
}

export function isRDBlinkTile(tile: number): boolean {
  return tile === T.FirstRDBlink || tile === T.LastRDBlink;
}

export function isExplosionTile(tile: number): boolean {
  return tile >= T.FirstExpl && tile <= T.LastExpl;
}
