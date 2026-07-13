/** Inclusive uniform random integer, like the original GetRandom(min, max). */
export type Rng = (min: number, max: number) => number;

export const defaultRng: Rng = (min, max) =>
  min + Math.floor(Math.random() * (max - min + 1));
