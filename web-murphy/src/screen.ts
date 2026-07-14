// Screen state machine: title, menu, level select, gameplay.

import type { Assets } from "./assets";
import type { Input, Pointer } from "./input";
import type { AudioEngine } from "./audio";

export interface GameContext {
  assets: Assets;
  input: Input;
  pointer: Pointer;
  audio: AudioEngine;
  switchScreen(next: Screen): void;
  readonly width: number;
  readonly height: number;
}

export interface Screen {
  enter(ctx: GameContext): Promise<void> | void;
  exit?(ctx: GameContext): void;
  /** One fixed 60 Hz logic tick. */
  tick(ctx: GameContext): void;
  render(ctx: GameContext, g: CanvasRenderingContext2D): void;
}
