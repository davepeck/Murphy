// Cell values ARE icl8 icon resource IDs (the original's central conceit).
// All game logic is numeric range checks on the cell value; a classifier maps
// values to one of 17 singleton behavior kinds. Ported from dataIntf.p /
// everything.p GetGridObj.

export const enum IC {
  Base = 128, // "circuit": edible dirt; also the board's default fill
  Blank = 129,
  Exit = 130,
  Chip = 131,
  Portal4Way = 132,
  GreenDisk = 133,
  YellowDisk = 134,
  RedDisk = 135,
  MadLeft = 139, // idle-annoyed Murphy frames 139-144
  CorpseA = 145,
  CorpseB = 147,
  SnapRight = 194,
  SnapLeft = 195,
  PushLeft = 196,
  PushRight = 197,
  Standing = 198,
  SnapUp = 199,
  SnapDown = 200,
  WalkBase = 201, // 201/203/205 left big/med/small; 202/204/206 right
  BugFirst = 300,
  BugLast = 305,
  InfoSnapFirst = 350, // infotron being snapped
  InfoSnapLast = 358,
  RedSnapFirst = 400,
  RedSnapLast = 406,
  BaseSnapFirst = 450,
  BaseSnapLast = 456,
  SleepFirst = 499, // yawn/sleep transition 499-511
  SleepLast = 511,
  TerminalFirst = 550,
  TerminalAnimLast = 560, // anim cycles 550-560; classifier accepts ..561
  TerminalLast = 561,
  ExplFirst = 600,
  ExplLast = 609,
  QuarkFirst = 650,
  QuarkAnimLast = 657,
  QuarkLast = 666, // classifier range
  Zonk = 700,
  ZonkLast = 707, // 701-703 are rotation frames
  HardwareFirst = 750,
  HardwareLast = 757,
  Infotron = 850,
  InfotronLast = 853, // 851-853 roll frames
  PortalFirst = 900,
  PortalVertical = 950,
  PortalHorizontal = 952,
  PortalLast = 952,
  ScissorFirst = 998,
  ScissorSmallLast = 1009, // 1000+dir small; +10 open; +20 medium
  ScissorMediumLast = 1027, // 1010-1027 classify as INERT (shipped quirk)
  Hardware2First = 1050,
  Hardware2Last = 1057,
  ChipFirst = 1100,
  DroppedRedDisk = 1105, // shares the Chip class
  ChipLast = 1106,
  ScissorTurnFirst = 1148,
  ScissorTurnLast = 1160,
  WinFirst = 1200,
  WinAnimLast = 1210, // in the murphy-kill range
  WinLast = 1211,
}

export const enum Kind {
  Inert, // unclassified: indestructible, non-round, blocks everything
  Blank,
  Base,
  Chip,
  Hardware,
  Exit,
  Portal,
  Terminal,
  Bug,
  Zonk,
  Infotron,
  GreenDisk,
  RedDisk,
  YellowDisk,
  Scissor,
  Quark,
  Murphy,
}

/** GetGridObj's exact mapping (final "NO OOP" version), first match wins. */
export function classify(v: number): Kind {
  if (v === IC.Base) return Kind.Base;
  if (v === IC.Blank) return Kind.Blank;
  if (v >= IC.Zonk && v <= IC.ZonkLast) return Kind.Zonk;
  if (v >= IC.Infotron && v <= IC.InfotronLast) return Kind.Infotron;
  if (v === IC.Chip || (v >= IC.ChipFirst && v <= IC.ChipLast)) return Kind.Chip;
  if (
    (v >= IC.HardwareFirst && v <= IC.HardwareLast) ||
    (v >= IC.Hardware2First && v <= IC.Hardware2Last)
  ) {
    return Kind.Hardware;
  }
  if (
    (v >= IC.ScissorFirst && v <= IC.ScissorSmallLast) ||
    (v >= IC.ScissorTurnFirst && v <= IC.ScissorTurnLast)
  ) {
    return Kind.Scissor;
  }
  if (v >= IC.QuarkFirst && v <= IC.QuarkLast) return Kind.Quark;
  if (v >= IC.BugFirst && v <= IC.BugLast) return Kind.Bug;
  if (v === IC.Portal4Way || (v >= IC.PortalFirst && v <= IC.PortalLast)) return Kind.Portal;
  if (v === IC.GreenDisk) return Kind.GreenDisk;
  if (v === IC.RedDisk) return Kind.RedDisk;
  if (v === IC.YellowDisk) return Kind.YellowDisk;
  if (v >= IC.TerminalFirst && v <= IC.TerminalLast) return Kind.Terminal;
  if (v === IC.Exit) return Kind.Exit;
  if (isMurphyIcon(v)) return Kind.Murphy;
  return Kind.Inert;
}

/** The "Murphy kill range": what enemies/fallers recognize as the player. */
export function isMurphyIcon(v: number): boolean {
  return (
    (v >= IC.SleepFirst && v <= IC.SleepLast) ||
    (v >= IC.MadLeft && v <= 214) ||
    (v >= IC.WinFirst && v <= IC.WinAnimLast)
  );
}

export function isExplosionIcon(v: number): boolean {
  return v >= IC.ExplFirst && v <= 610; // original checks 600..610
}

export interface KindFlags {
  destructable: boolean;
  rounded: boolean;
  falls: boolean;
  explodes: boolean; // chains a full 3x3 when destroyed
  moves: boolean;
}

const FLAGS: Record<Kind, KindFlags> = {
  [Kind.Inert]: { destructable: false, rounded: false, falls: false, explodes: false, moves: false },
  [Kind.Blank]: { destructable: true, rounded: false, falls: false, explodes: false, moves: false },
  [Kind.Base]: { destructable: true, rounded: false, falls: false, explodes: false, moves: false },
  [Kind.Chip]: { destructable: true, rounded: true, falls: false, explodes: false, moves: false },
  [Kind.Hardware]: { destructable: false, rounded: false, falls: false, explodes: false, moves: false },
  [Kind.Exit]: { destructable: true, rounded: false, falls: false, explodes: false, moves: false },
  [Kind.Portal]: { destructable: true, rounded: false, falls: false, explodes: false, moves: false },
  [Kind.Terminal]: { destructable: true, rounded: false, falls: false, explodes: false, moves: false },
  [Kind.Bug]: { destructable: true, rounded: false, falls: false, explodes: false, moves: false },
  [Kind.Zonk]: { destructable: true, rounded: true, falls: true, explodes: false, moves: false },
  [Kind.Infotron]: { destructable: true, rounded: true, falls: true, explodes: false, moves: false },
  [Kind.GreenDisk]: { destructable: true, rounded: false, falls: true, explodes: true, moves: false },
  [Kind.RedDisk]: { destructable: true, rounded: false, falls: false, explodes: false, moves: false },
  [Kind.YellowDisk]: { destructable: true, rounded: false, falls: false, explodes: false, moves: false },
  [Kind.Scissor]: { destructable: true, rounded: false, falls: false, explodes: true, moves: true },
  [Kind.Quark]: { destructable: true, rounded: false, falls: false, explodes: true, moves: true },
  [Kind.Murphy]: { destructable: true, rounded: false, falls: false, explodes: true, moves: false },
};

export function flagsOf(v: number): KindFlags {
  return FLAGS[classify(v)];
}

// Directions (dataIntf.p): 8-way compass, kNoMove = 8.
export const enum Dir {
  Up = 0,
  UpRight = 1,
  Right = 2,
  DownRight = 3,
  Down = 4,
  DownLeft = 5,
  Left = 6,
  UpLeft = 7,
  None = 8,
}

const DX = [0, 1, 1, 1, 0, -1, -1, -1, 0];
const DY = [-1, -1, 0, 1, 1, 1, 0, -1, 0];

export function dirDX(d: Dir): number {
  return DX[d] as number;
}

export function dirDY(d: Dir): number {
  return DY[d] as number;
}

export function turnLeft(d: Dir): Dir {
  return ((d + 6) % 8) as Dir;
}

export function turnRight(d: Dir): Dir {
  return ((d + 2) % 8) as Dir;
}

/** The roll code's "opposite" (valid for Left/Right, as used). */
export function oppositeRoll(d: Dir): Dir {
  return (8 - d) as Dir;
}
