/**
 * @file converters/infotron.ts
 *
 * Implementation of a converter from Infotron levels in their custom JSON format
 * to the new Supaplex JSON format defined in ../schema.json.
 */

import {LevelConverter, gridSize, trimEdges} from './shared';
import {Tile, SupaplexTile, MurphysRevengeTile, InfotronTile} from '../schema';

const TILE_MAP: {[key: number]: Tile} = {
  128: SupaplexTile.Base,
  129: SupaplexTile.Space,
  130: SupaplexTile.Exit,
  131: SupaplexTile.RamChipSingleSquare,
  132: SupaplexTile.PortCross,
  133: SupaplexTile.OrangeDisk,
  134: SupaplexTile.YellowDisk,
  135: SupaplexTile.RedDisk,
  198: SupaplexTile.Murphy,
  300: SupaplexTile.Bug,
  550: SupaplexTile.Terminal,
  650: SupaplexTile.ElectronUp,
  700: SupaplexTile.Zonk,
  750: SupaplexTile.HardwareStandard,
  751: SupaplexTile.HardwareDiagonalStripes,
  752: SupaplexTile.HardwareRedYellowBlueHorizontalResistors,
  753: SupaplexTile.HardwareDiagonalStripes,
  754: SupaplexTile.HardwareRedLamp,
  755: SupaplexTile.HardwareBlueLamp,
  756: SupaplexTile.HardwareGreenLamp,
  757: SupaplexTile.HardwareRedVerticalResistors,
  758: SupaplexTile.HardwareHorizontalRedCapacitor,
  759: SupaplexTile.HardwareBlueCircular,
  760: SupaplexTile.HardwareYellowHorizontalResistors,
  850: SupaplexTile.Infotron,
  900: SupaplexTile.PortUpToDown,
  902: SupaplexTile.PortRightToLeft,
  904: SupaplexTile.PortDownToUp,
  906: SupaplexTile.PortLeftToRight,
  950: SupaplexTile.PortVertical,
  952: SupaplexTile.PortHorizontal,
  1000: SupaplexTile.SnikSnakUp,
  1002: InfotronTile.SnikSnakRight,
  1004: InfotronTile.SnikSnakDown,
  1006: InfotronTile.SnikSnakLeft,
  1050: InfotronTile.BorderTop,
  1051: InfotronTile.BorderTopRight,
  1052: InfotronTile.BorderRight,
  1053: InfotronTile.BorderBottomRight,
  1054: InfotronTile.BorderBottom,
  1055: InfotronTile.BorderBottomLeft,
  1056: InfotronTile.BorderLeft,
  1057: InfotronTile.BorderTopLeft,
  1100: SupaplexTile.RamChipVerticalTop,
  1102: SupaplexTile.RamChipHorizontalRight,
  1104: SupaplexTile.RamChipVerticalBottom,
  1106: SupaplexTile.RamChipHorizontalLeft,
};

const mapTile = (infoTile: number): Tile => {
  const tile: Tile = TILE_MAP[infoTile];
  if (tile === undefined) {
    throw new Error(`Unknown infoTile: ${infoTile}`);
  }
  return tile;
};

const mapRow = (infoTileRow: number[]): Tile[] => infoTileRow.map(mapTile);

/** The private Infotron JSON format. */
export interface InfotronLevel {
  name: string;
  infotrons: number;
  size: [number, number];
  board: number[][];
}

/** The primary data converter */
export const convert: LevelConverter<InfotronLevel> = (infoLevel) => {
  const [sizeX, sizeY] = infoLevel.size;
  const [boardSizeX, boardSizeY] = gridSize(infoLevel.board);

  if (sizeX != boardSizeX || sizeY != boardSizeY) {
    throw new Error(
      `Mismatched size: [${sizeX}, ${sizeY}] vs boardSize: [${boardSizeX}, ${boardSizeY}]`
    );
  }

  return {
    name: infoLevel.name,
    infotrons: infoLevel.infotrons,
    board: trimEdges(infoLevel.board).map(mapRow),
  };
};
