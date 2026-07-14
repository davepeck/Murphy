// Sound events emitted by the engine; the audio layer maps them to files.
// prox is Manhattan distance in tiles from Oliver (already halved for thud);
// pan is -10..+10 (positive = right of Oliver). Non-positional events omit them.

export type SoundEventKind =
  | "gotInfotron"
  | "gotLastInfotron"
  | "gotRedDisk"
  | "redDiskReady"
  | "oliverDied"
  | "endLevel"
  | "push"
  | "termUse"
  | "termBad"
  | "portUse"
  | "thud"
  | "explode"
  | "quarkExplode"
  | "bugZap"
  | "noMoreTime";

export interface SoundEvent {
  kind: SoundEventKind;
  prox?: number;
  pan?: number;
}

export type SoundSink = (e: SoundEvent) => void;
