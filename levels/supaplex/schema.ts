/** TODO: separate visual appearance from behavior */

/** The top-level supaplex level data format. */
export interface SupaplexLevel {
  /** The name of the level. */
  name: string;

  /** The number of infotrons required. Must be > 0. */
  infotrons: number;

  /**
   * The level's primary contents.
   *
   * Supaplex levels were all 60x24, but Infotron and Murphy's Revenge allowed
   * for arbitrary size levels. A border should be drawn around the level;
   * this data format does not contain border tiles.
   */
  board: Tile[][];

  /** Whether to freeze zonks initially. Default false if not provided. */
  freezeZonks?: boolean;

  /** Optional "gravity changing" port entries. Default none if not provided. */
  specialPorts?: SpecialPort[];
}

/** Special port metadata (rare, but defined in SPFIX63a). */
export interface SpecialPort {
  /** Where the special port is located in the level. */
  location: Point;

  /** If true, gravity is on post-port. Otherwise, off. */
  hasGravity: boolean;

  /** If true, freeze (or unfreeze) zonks post-port. */
  freezeZonks: boolean;

  /** If true, freeze (or unfreeze) enemies post-port. */
  freezeEnemies: boolean;
}

/** An x, y coordinate */
export type Point = [number, number];

/** All known tile types. */
export type Tile = SupaplexTile | InfotronTile | MurphysRevengeTile;

/** Known tile types from supaplex, defined in SPFIX63a. */
export enum SupaplexTile {
  /** A blank space. */
  Space = 0,

  /** A standard zonk. */
  Zonk = 1,

  /** The default 'edible circuit' base. */
  Base = 2,

  /** Murphy's starting position. */
  Murphy = 3,

  /** An infotron. */
  Infotron = 4,

  /** RAM chip (pins on all 4 sides, standard shape) Dull */
  RamChipSingleSquare = 5,

  /** Hardware (gray dented pyramid, standard shape) Dull */
  HardwareStandard = 6,

  /** The exit. */
  Exit = 7,

  /** An orange utility disk. (In Infotron for Mac, a 'Green' Disk). */
  OrangeDisk = 8,

  /**
   * A left-to-right port.
   *
   * Unlike the supaplex format, there is no need to distinguish between
   * a normal port and a "special" port. That's because
   * code can simply look at the specialPorts array, if present, in the JSON.
   */
  PortLeftToRight = 9,

  /** An up-to-down port. */
  PortUpToDown = 10,

  /** A right-to-left port. */
  PortRightToLeft = 11,

  /** A down-to-up port. */
  PortDownToUp = 12,

  /** A Snik Snak (initially pointing upward) */
  SnikSnakUp = 17,

  /** A yellow utility disk. */
  YellowDisk = 18,

  /** Terminal for exploding yellow disks. */
  Terminal = 19,

  /** A red utility disk. */
  RedDisk = 20,

  /** A vertical bi-directional port. (up-to-down and down-to-up) */
  PortVertical = 21,

  /** A horizontal bi-directional port. (left-to-right and right-to-left) */
  PortHorizontal = 22,

  /** A cross port. */
  PortCross = 23,

  /** An electron (initially pointing upward). */
  ElectronUp = 24,

  /** A bug. */
  Bug = 25,

  /** RAM chip (horizontal left 'pin 1') Fancy */
  RamChipHorizontalLeft = 26,

  /** RAM chip (horizontal right) Fancy */
  RamChipHorizontalRight = 27,

  /** Hardware (radial blue circular + colored shapes) */
  HardwareBlueCircular = 28,

  /** Hardware (green signal lamp) */
  HardwareGreenLamp = 29,

  /** Hardware (blue signal lamp) */
  HardwareBlueLamp = 30,

  /** Hardware (red signal lamp) */
  HardwareRedLamp = 31,

  /**
   * Hardware (yellow/black diagonal stripes)
   *
   * Infotron for Mac actually had two vaiants of this.
   */
  HardwareDiagonalStripes = 32,

  /** Hardware (yellow resistor + blue + red shapes) */
  HardwareYellowResistorBlueRed = 33,

  /** Hardware (horizontal red capacitor + smd shape) */
  HardwareHorizontalRedCapacitor = 34,

  /** Hardware (red + yellow + blue horizontal resistors) */
  HardwareRedYellowBlueHorizontalResistors = 35,

  /** Hardware (3 red veritcal resistors) */
  HardwareRedVerticalResistors = 36,

  /** Hardware (3 yellow horizontal resistors) */
  HardwareYellowHorizontalResistors = 37,

  /** RAM chip (vertical top) */
  RamChipVerticalTop = 38,

  /** RAM chip (vertical bottom) */
  RamChipVerticalBottom = 39,

  /** Invisible wall (?) */
  InvisibleWall = 40,
}

/** Special tile types only found in Infotron for classic MacOS. */
export enum InfotronTile {
  /**
   * A top-left level border tile.
   *
   * We never include outer borders in the `board` array. However,
   * some Infotron and Murphy's Revenge levels contained inner borders too!
   * Not surprisingly, inner boders should be treated as indestructible
   * hardware.
   */
  BorderTopLeft = 100,

  /** A top level border tile. */
  BorderTop = 101,

  /** The top-right level border tile. */
  BorderTopRight = 102,

  /** The left level border tile. */
  BorderLeft = 103,

  /** The right level border tile. */
  BorderRight = 104,

  /** The bottom-left level border tile. */
  BorderBottomLeft = 105,

  /** The bottom level border tile. */
  BorderBottom = 106,

  /** The bottom-right level border tile. */
  BorderBottomRight = 107,

  /**
   * A Snik Snak (initially pointing leftward)
   *
   * Unlike Supaplex, in Infotron and Murphy's Revenge, Snik Snaks
   * can initially point in any of the four cardinal directions.
   */
  SnikSnakLeft = 108,

  /** A Snik Snak (initially pointing downward) */
  SnikSnakDown = 109,

  /** A Snik Snak (initially pointing rightward) */
  SnikSnakRight = 110,
}

/** Special tile types only found in Murphy's Revenge for classic MacOS. */
export enum MurphysRevengeTile {
  /**
   * A zonk that floats upward, against gravity.
   *
   * Murphy's Revenge allows levels to arbitrarily mix standard zonks
   * that fall down with "up zonks" that, well, float up. Their behavior
   * is otherwise the same.
   *
   * In the MR graphics set, Up Zonks are distinguished by their bright blue
   * color.
   *
   * Levels that mix up+down zonks are tons of fun!
   */
  UpZonk = 200,

  /**
   * A "gold" disk.
   *
   * This special kind of disk acts like an explosive rocket. It is not
   * affected by gravity. Murphy can push the disk exactly once. At that point,
   * it "takes off", continuing to head in the same direction and exploding
   * everything in its path until it hits any indestructible hardware. At
   * that point, it goes away. The explosions along the way are only one
   * tile in size -- not the standard 9-tiles of a typical disk explosion.
   * But, of course, they can set *off* larger chain reactions.
   *
   * Several MR levels make extensive use of this new type of disk.
   */
  GoldDisk = 201,

  /**
   * A "movable" RAM chip.
   *
   * In the MR graphics set, these look like standard 1-square RAM chips,
   * except they are blank on their face.
   *
   * They can be pushed just like a zonk, if there is space behind them.
   * Like standard RAM chips, they never fall. Like standard RAM chips,
   * zonks roll off of them. Like standard RAM chips, they can be destroyed
   * by explosions.
   */
  MovableRamChip = 202,

  /* TODO: There were a few other new pieces that we were experimenting with
   * in MR, most notably lasers and mirrors, but we don't have many levels
   * that use these right now, so we'll hold on defining them here. */
}
