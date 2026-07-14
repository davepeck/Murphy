// Tile IDs exactly as in the original MRPlayConstants.h `enum TileTypes`
// (verified against osx/Murphy/MurphyTiles.swift). Atlas tile names are the
// enum names; TILE_NAMES maps ID -> name for rendering.

export const enum T {
  Circuit = 0,
  Blank = 1,
  Exit = 2,
  GreenDisk = 3,
  YellowDisk = 4,
  RedDisk = 5,
  GoldDisk = 6,
  Terminal = 7,
  Bug = 8,
  Oliver = 9,
  LeftPort = 10,
  DownPort = 11,
  RightPort = 12,
  UpPort = 13,
  LeftRightPort = 14,
  UpDownPort = 15,
  FourWayPort = 16,
  Zonk = 17,
  UpZonk = 18,
  Infotron = 19,
  Quark = 20,
  UpScissor = 21,
  RightScissor = 22,
  DownScissor = 23,
  LeftScissor = 24,
  Chip = 25,
  TopChip = 26,
  BottomChip = 27,
  LeftChip = 28,
  RightChip = 29,
  TopLeftBorder = 30,
  TopBorder = 31,
  TopRightBorder = 32,
  RightBorder = 33,
  LeftBorder = 34,
  BottomLeftBorder = 35,
  BottomBorder = 36,
  BottomRightBorder = 37,
  BlankChip = 38,
  FirstHardware = 39,
  LastHardware = 65,
  Phantom = 66,
  Laser = 67,
  LaserPower = 68,
  ForeMirror = 69,
  BackMirror = 70,
  EmptyPhase = 71,
  FullPhase = 72,
  PhaseTerm = 73,
  BlankLaserPiece = 80,
  LaserEmptyPiece = 81,
  LaserPowerPiece = 82,
  LaserPiece = 83,
  ForeMirrorPiece = 84,
  BackMirrorPiece = 85,
  TLBRLaserForeMirrorLight = 86,
  RLLaserLight = 87,
  UDLaserLight = 88,
  TLLaserForeMirrorLight = 89,
  BRLaserForeMirrorLight = 90,
  BLLaserBackMirrorLight = 91,
  TRLaserBackMirrorLight = 92,
  BLTRLaserBackMirrorLight = 93,
  FirstInfotron = 94,
  LastInfotron = 109,
  FirstTerminal = 110,
  LastTerminal = 121,
  FirstBug = 122,
  LastBug = 127,
  FirstExpl = 128,
  IgniteExpl = 131,
  LastExpl = 137,
  FirstExit = 138,
  LastExit = 145, // the open, passable exit
  FirstZapRedDisk = 146,
  LastZapRedDisk = 153,
  FirstZapCircuit = 156,
  LastZapCircuit = 162,
  FirstZapInfotron = 166,
  ZapInfotron25 = 171, // quark-explosion ignite frame
  LastZapInfotron = 178,
  FirstRDBlink = 179,
  LastRDBlink = 180,
  FirstZonk = 181,
  LastZonk = 184,
  FirstUpZonk = 185,
  LastUpZonk = 188,
  FirstQuark = 189,
  LastQuark = 197,
  FirstUpScissor = 198,
  LastUpScissor = 200,
  UpRightScissor = 201,
  FirstRightScissor = 202,
  LastRightScissor = 204,
  DownRightScissor = 205,
  FirstDownScissor = 206,
  LastDownScissor = 208,
  DownLeftScissor = 209,
  FirstLeftScissor = 210,
  LastLeftScissor = 212,
  UpLeftScissor = 213,
  Max = 214,
}

/** Oliver sprite frames live at atlas IDs 220..304; frame index is 0..84. */
export const OLIVER_TILE_BASE = 220;

// Oliver frame indices (sprite-local, 0-based; add OLIVER_TILE_BASE for atlas).
export const enum OF {
  Bub = 0, // idle base; bubble axes use 1+2a / 2+2a for a in 0..3
  LeftFirst = 9, // walk left 9,10,11
  RightFirst = 12, // walk right 12,13,14
  PushLeftFirst = 16,
  PushRightFirst = 18,
  PushDownFirst = 20,
  PushUpFirst = 22,
  ZapLeft = 24,
  ZapRight = 25,
  ZapDown = 26,
  ZapUp = 27,
  FadeDeathFirst = 30, // ..39
  BubbleDeathFirst = 40, // ..49
  StaticDeathFirst = 50, // ..59
  YawnFirst = 60, // ..71
  ShockDeathFirst = 72, // ..78
  Eureka = 80,
  Sad = 81,
  FingerYou = 82,
  ExLaxFirst = 83, // 83,84
  ExLaxLast = 84,
}

function range(prefix: string, first: string, mids: string[], last: string): string[] {
  return [first, ...mids.map((m) => `${prefix}${m}`), last];
}

/** ID -> atlas tile name for IDs 0..213 (grid tiles) and 220..304 (Oliver). */
export const TILE_NAMES: readonly string[] = [
  // 0-9
  "CircuitTile", "BlankTile", "ExitTile", "GreenDiskTile", "YellowDiskTile",
  "RedDiskTile", "GoldDiskTile", "TerminalTile", "BugTile", "OliverTile",
  // 10-16 ports
  "LeftPortTile", "DownPortTile", "RightPortTile", "UpPortTile",
  "LeftRightPortTile", "UpDownPortTile", "FourWayPortTile",
  // 17-19
  "ZonkTile", "UpZonkTile", "InfotronTile",
  // 20-24
  "QuarkTile", "UpScissorTile", "RightScissorTile", "DownScissorTile", "LeftScissorTile",
  // 25-29 chips
  "ChipTile", "TopChipTile", "BottomChipTile", "LeftChipTile", "RightChipTile",
  // 30-37 borders
  "TopLeftBorderTile", "TopBorderTile", "TopRightBorderTile", "RightBorderTile",
  "LeftBorderTile", "BottomLeftBorderTile", "BottomBorderTile", "BottomRightBorderTile",
  // 38
  "BlankChipTile",
  // 39-65 hardware
  ...range("HardwareTile", "FirstHardwareTile",
    Array.from({ length: 25 }, (_, i) => `${i + 2}`), "LastHardwareTile"),
  // 66-73
  "PhantomTile", "LaserTile", "LaserPowerTile", "ForeMirrorTile", "BackMirrorTile",
  "EmptyPhaseTile", "FullPhaseTile", "PhaseTermTile",
  // 74-79
  "EmptyTile4", "EmptyTile5", "EmptyTile6", "EmptyTile7", "EmptyTile8", "EmptyTile9",
  // 80-93 laser pieces
  "BlankLaserPiece", "LaserEmptyPiece", "LaserPowerPiece", "LaserPiece",
  "ForeMirrorPiece", "BackMirrorPiece", "TLBRLaserForeMirrorLightPiece",
  "RLLaserLightPiece", "UDLaserLightPiece", "TLLaserForeMirrorLightPiece",
  "BRLaserForeMirrorLightPiece", "BLLaserBackMirrorLightPiece",
  "TRLaserBackMirrorLightPiece", "BLTRLaserBackMirrorLightPiece",
  // 94-109 infotron animation
  ...range("InfoTile", "FirstInfotronTile",
    Array.from({ length: 14 }, (_, i) => `${i + 2}`), "LastInfotronTile"),
  // 110-121 terminal animation
  ...range("TermTile", "FirstTerminalTile",
    Array.from({ length: 10 }, (_, i) => `${i + 2}`), "LastTerminalTile"),
  // 122-127 bug animation
  ...range("BugTile", "FirstBugTile",
    Array.from({ length: 4 }, (_, i) => `${i + 2}`), "LastBugTile"),
  // 128-137 explosion
  "FirstExplTile", "ExplTile2", "ExplTile3", "IgniteExplTile", "ExplTile5",
  "ExplTile6", "ExplTile7", "ExplTile8", "ExplTile9", "LastExplTile",
  // 138-145 exit opening
  ...range("ExitTile", "FirstExitTile",
    Array.from({ length: 6 }, (_, i) => `${i + 2}`), "LastExitTile"),
  // 146-153 zap red disk
  ...range("ZapRedDiskTile", "FirstZapRedDiskTile",
    Array.from({ length: 6 }, (_, i) => `${i + 1}`), "LastZapRedDiskTile"),
  // 154-155
  "EmptyZapTile8", "EmptyZapTile9",
  // 156-162 zap circuit
  ...range("ZapCircuitTile", "FirstZapCircuitTile",
    Array.from({ length: 5 }, (_, i) => `${i + 11}`), "LastZapCircuitTile"),
  // 163-165
  "EmptyZapTile17", "EmptyZapTile18", "EmptyZapTile19",
  // 166-178 zap infotron
  ...range("ZapInfotronTile", "FirstZapInfotronTile",
    Array.from({ length: 11 }, (_, i) => `${i + 21}`), "LastZapInfotronTile"),
  // 179-180 red-disk blink
  "FirstRDBlinkTile", "LastRDBlinkTile",
  // 181-184 zonk roll
  "FirstZonkTile", "ZonkTile1", "ZonkTile2", "LastZonkTile",
  // 185-188 up-zonk roll
  "FirstUpZonkTile", "UpZonkTile1", "UpZonkTile2", "LastUpZonkTile",
  // 189-197 quark
  ...range("QuarkTile", "FirstQuarkTile",
    Array.from({ length: 7 }, (_, i) => `${i + 1}`), "LastQuarkTile"),
  // 198-213 scissors
  "FirstUpScissorTile", "UpScissorTile1", "LastUpScissorTile", "UpRightScissorTile",
  "FirstRightScissorTile", "RightScissorTile1", "LastRightScissorTile", "DownRightScissorTile",
  "FirstDownScissorTile", "DownScissorTile1", "LastDownScissorTile", "DownLeftScissorTile",
  "FirstLeftScissorTile", "LeftScissorTile1", "LastLeftScissorTile", "UpLeftScissorTile",
  // 214-219 unused padding
  "UnusedTile1", "UnusedTile2", "UnusedTile3", "UnusedTile4", "UnusedTile5", "UnusedTile6",
  // 220-304 Oliver frames
  "BubFrame", "ABubFrame1", "ABubFrame2", "BBubFrame1", "BBubFrame2",
  "CBubFrame1", "CBubFrame2", "DBubFrame1", "DBubFrame2",
  "LeftBubFirstFrame", "LeftBubFrame2", "LeftBubLastFrame",
  "RightBubFirstFrame", "RightBubFrame2", "RightBubLastFrame",
  "EmptyBubFrame1",
  "PushLeftBubFirstFrame", "PushLeftBubLastFrame",
  "PushRightBubFirstFrame", "PushRightBubLastFrame",
  "PushDownBubFirstFrame", "PushDownBubLastFrame",
  "PushUpBubFirstFrame", "PushUpBubLastFrame",
  "ZapLeftBubFrame", "ZapRightBubFrame", "ZapDownBubFrame", "ZapUpBubFrame",
  "EmptyBubFrame2", "EmptyBubFrame3",
  ...range("FadeDeathBubFrame", "FadeDeathBubFirstFrame",
    Array.from({ length: 8 }, (_, i) => `${i + 2}`), "FadeDeathBubLastFrame"),
  ...range("BubbleDeathBubFrame", "BubbleDeathBubFirstFrame",
    Array.from({ length: 8 }, (_, i) => `${i + 2}`), "BubbleDeathBubLastFrame"),
  ...range("StaticDeathBubFrame", "StaticDeathBubFirstFrame",
    Array.from({ length: 8 }, (_, i) => `${i + 2}`), "StaticDeathBubLastFrame"),
  ...range("YawnBubFrame", "YawnBubFirstFrame",
    Array.from({ length: 10 }, (_, i) => `${i + 2}`), "YawnBubLastFrame"),
  ...range("ShockDeathBubFrame", "ShockDeathBubFirstFrame",
    Array.from({ length: 5 }, (_, i) => `${i + 2}`), "ShockDeathBubLastFrame"),
  "EmptyBubFrame4", "EurekaBubFrame", "SadBubFrame", "FuckYouBubFrame",
  "ExLaxBubFirstFrame", "ExLaxBubLastFrame",
];

if (TILE_NAMES.length !== 305) {
  throw new Error(`TILE_NAMES has ${TILE_NAMES.length} entries, expected 305`);
}

export function tileName(id: number): string {
  const n = TILE_NAMES[id];
  if (!n) throw new Error(`no tile name for id ${id}`);
  return n;
}

export function oliverFrameName(frame: number): string {
  return tileName(OLIVER_TILE_BASE + frame);
}

// Directions, as in the original.
export const enum Dir {
  None = 0,
  Up = 1,
  Down = 2,
  Left = 3,
  Right = 4,
}

// Facings for movers (quark/scissor), as in the original.
export const enum Facing {
  Static = 0,
  Up = 1,
  Right = 2,
  Down = 3,
  Left = 4,
}

export function dirDelta(d: Dir): { dh: number; dv: number } {
  switch (d) {
    case Dir.Up: return { dh: 0, dv: -1 };
    case Dir.Down: return { dh: 0, dv: 1 };
    case Dir.Left: return { dh: -1, dv: 0 };
    case Dir.Right: return { dh: 1, dv: 0 };
    default: return { dh: 0, dv: 0 };
  }
}

export function facingDelta(f: Facing): { dh: number; dv: number } {
  switch (f) {
    case Facing.Up: return { dh: 0, dv: -1 };
    case Facing.Right: return { dh: 1, dv: 0 };
    case Facing.Down: return { dh: 0, dv: 1 };
    case Facing.Left: return { dh: -1, dv: 0 };
    default: return { dh: 0, dv: 0 };
  }
}

export function rotateCW(f: Facing): Facing {
  switch (f) {
    case Facing.Up: return Facing.Right;
    case Facing.Right: return Facing.Down;
    case Facing.Down: return Facing.Left;
    default: return Facing.Up;
  }
}

export function rotateCCW(f: Facing): Facing {
  switch (f) {
    case Facing.Up: return Facing.Left;
    case Facing.Left: return Facing.Down;
    case Facing.Down: return Facing.Right;
    default: return Facing.Up;
  }
}

export const TILE_SIZE = 32;
