import {SupaplexLevel} from '../schema';

export type LevelConverter<T extends {}> = (level: T) => SupaplexLevel;

/** Get the size of a rectangular 2D array. */
export const gridSize = (a: any[][]): [number, number] => {
  return [a.length, a[0].length];
};

/** Get rid of all edges of a 2D array. */
export const trimEdges = <T>(a: T[][]): T[][] => {
  return a.slice(1, -1).map((r) => r.slice(1, -1));
};
