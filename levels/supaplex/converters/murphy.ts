/**
 * @file converters/murphy.ts
 *
 * Convert Murphy's Revenge levels into the new Supaplex JSON format
 * defined in schema.json.
 */

import {LevelConverter, gridSize, trimEdges} from './shared';
import {Tile, SupaplexTile, MurphysRevengeTile, InfotronTile} from '../schema';

const TILE_MAP: {[key: string]: Tile} = {
  '0,0': SupaplexTile.Base,
  '1,0': SupaplexTile.Space,
  '2,0': SupaplexTile.Exit,
  '3,0': SupaplexTile.OrangeDisk,
  '4,0': SupaplexTile.YellowDisk,
  '5,0': SupaplexTile.RedDisk,
  '6,0': MurphysRevengeTile.GoldDisk,
  '7,0': SupaplexTile.Terminal,
  '8,0': SupaplexTile.Bug,
  '9,0': SupaplexTile.Murphy,
  '0,1': SupaplexTile.PortRightToLeft,
  '1,1': SupaplexTile.PortUpToDown,
  '2,1': SupaplexTile.PortLeftToRight,
  '3,1': SupaplexTile.PortDownToUp,
  '4,1': SupaplexTile.PortHorizontal,
  '5,1': SupaplexTile.PortVertical,
  '6,1': SupaplexTile.PortCross,
  '7,1': SupaplexTile.Zonk,
  '8,1': MurphysRevengeTile.UpZonk,
  '9,1': SupaplexTile.Infotron,
  '0,2': SupaplexTile.ElectronUp,
  '1,2': SupaplexTile.SnikSnakUp,
  '2,2': InfotronTile.SnikSnakRight,
  '3,2': InfotronTile.SnikSnakDown,
  '4,2': InfotronTile.SnikSnakLeft,
  '5,2': SupaplexTile.RamChipSingleSquare,
  '6,2': SupaplexTile.RamChipVerticalTop,
  '7,2': SupaplexTile.RamChipVerticalBottom,
  '8,2': SupaplexTile.RamChipHorizontalLeft,
  '9,2': SupaplexTile.RamChipHorizontalRight,
  '0,3': InfotronTile.BorderTopLeft,
  '1,3': InfotronTile.BorderTop,
  '2,3': InfotronTile.BorderTopRight,
  '3,3': InfotronTile.BorderLeft,
  '4,3': InfotronTile.BorderRight,
  '5,3': InfotronTile.BorderBottomLeft,
  '6,3': InfotronTile.BorderBottom,
  '7,3': InfotronTile.BorderBottomRight,
  '8,3': MurphysRevengeTile.MovableRamChip,
  '9,3': SupaplexTile.HardwareStandard,
  '0,4': SupaplexTile.HardwareDiagonalStripes,
  '1,4': SupaplexTile.HardwareRedYellowBlueHorizontalResistors,
  '2,4': SupaplexTile.HardwareDiagonalStripes /* Reversed, actually */,
  '3,4': SupaplexTile.HardwareRedLamp,
  '4,4': SupaplexTile.HardwareBlueLamp,
  '5,4': SupaplexTile.HardwareGreenLamp,
  '6,4': SupaplexTile.HardwareYellowResistorBlueRed,
  '7,4': SupaplexTile.HardwareHorizontalRedCapacitor,
  '8,4': SupaplexTile.HardwareBlueCircular,
  '9,4': SupaplexTile.HardwareYellowHorizontalResistors,
  /* What follows are many MR-specific hardware tile images */
  '0,5': SupaplexTile.HardwareStandard,
  '1,5': SupaplexTile.HardwareStandard,
  '2,5': SupaplexTile.HardwareStandard,
  '3,5': SupaplexTile.HardwareStandard,
  '4,5': SupaplexTile.HardwareStandard,
  '5,5': SupaplexTile.HardwareStandard,
  '6,5': SupaplexTile.HardwareStandard,
  '7,5': SupaplexTile.HardwareStandard,
  '8,5': SupaplexTile.HardwareStandard,
  '9,5': SupaplexTile.HardwareStandard,
  /* Even more! */
  '0,6': SupaplexTile.HardwareStandard,
  '1,6': SupaplexTile.HardwareStandard,
  '2,6': SupaplexTile.HardwareStandard,
  '3,6': SupaplexTile.HardwareStandard,
  '4,6': SupaplexTile.HardwareStandard,
  '5,6': SupaplexTile.HardwareStandard,
};

const mapTile = (mrTile: [number, number]): Tile => {
  const tile: Tile = TILE_MAP[String(mrTile)];
  if (tile === undefined) {
    throw new Error(`Unknown mrTile: ${mrTile}`);
  }
  return tile;
};

const mapRow = (mrTileRow: [number, number][]): Tile[] =>
  mrTileRow.map(mapTile);

/** The private Murphys Revenge JSON format */
export interface MurphysRevengeLevel {
  name: string;
  infotrons: number;
  size: [number, number];
  board: [number, number][][];
}

/** The primary data converter */
export const convert: LevelConverter<MurphysRevengeLevel> = (mrLevel) => {
  const [sizeY, sizeX] = mrLevel.size;
  const [boardSizeX, boardSizeY] = gridSize(mrLevel.board);

  if (sizeX != boardSizeX || sizeY != boardSizeY) {
    throw new Error(
      `Mismatched size: [${sizeX}, ${sizeY}] vs boardSize: [${boardSizeX}, ${boardSizeY}]`
    );
  }

  return {
    name: mrLevel.name,
    infotrons: mrLevel.infotrons,
    board: trimEdges(mrLevel.board).map(mapRow),
  };
};
