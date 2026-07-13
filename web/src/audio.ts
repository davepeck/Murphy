// WebAudio: positional sound effects and gapless segment-sequenced music,
// modeled on MRSound.c. Volumes use the original 0..255 scale with the
// four preset steps (Silent/Audible/Pleasant/Excruciating).

import type { SoundEvent } from "./engine/events";

const BASE = import.meta.env.BASE_URL + "assets/sounds";
const MAX_PROXIMITY = 22;

export const VOLUME_STEPS = [0, 85, 171, 255] as const;
export const VOLUME_NAMES = ["silent", "audible", "pleasant", "excruciating"] as const;

export interface MusicSpec {
  /** URLs relative to assets/sounds */
  files: string[];
  /** play order: indices into files */
  order: number[];
  /** index in order[] to wrap back to */
  loopAt: number;
  /** index in order[] to start at */
  startAt: number;
}

function letterSequence(letters: string, loopAt: number, startAt: number, dir: string, count: number): MusicSpec {
  const files = Array.from({ length: count }, (_, i) => `${dir}/music-${i + 1}.wav`);
  return {
    files,
    order: [...letters].map((c) => c.charCodeAt(0) - 65),
    loopAt,
    startAt,
  };
}

/** Per-world gameplay music (sequences from sounds/music-info.txt). */
export function gameplayMusic(world: string): MusicSpec {
  switch (world) {
    case "infotron":
      // 'AxABBCCDDEEEECCBBA' -> post-x-removal 'AABBCCDDEEEECCBBA', loop 1, start 1
      return letterSequence("AABBCCDDEEEECCBBA", 1, 1, "infotron", 5);
    case "savanatron":
      // 'xAABBBBCCCCBB' -> 'AABBBBCCCCBB', loop 0, start 1
      return letterSequence("AABBBBCCCCBB", 0, 1, "savanatron", 3);
    case "aquatron":
      return { files: ["aquatron/music.wav"], order: [0], loopAt: 0, startAt: 0 };
    default:
      return { files: ["deathtron/music.wav"], order: [0], loopAt: 0, startAt: 0 };
  }
}

export const INTERFACE_MUSIC: MusicSpec = {
  files: ["interface/intro-music.wav"],
  order: [0],
  loopAt: 0,
  startAt: 0,
};

export const MAP_ROOM_MUSIC: MusicSpec = {
  files: ["interface/map-room-music.wav"],
  order: [0],
  loopAt: 0,
  startAt: 0,
};

export class AudioEngine {
  private ctx: AudioContext | null = null;
  private buffers = new Map<string, AudioBuffer>();
  private sfxGain: GainNode | null = null;
  private musicGain: GainNode | null = null;

  musicVolume: number;
  soundVolume: number;

  private musicSources: AudioBufferSourceNode[] = [];
  private musicPump: number | null = null;
  private musicSpec: MusicSpec | null = null;
  private musicIdx = 0;
  private musicNextTime = 0;
  private musicToken = 0;

  constructor() {
    this.musicVolume = 171;
    this.soundVolume = 171;
    try {
      const saved = JSON.parse(localStorage.getItem("murphy.audio") ?? "{}");
      if (typeof saved.music === "number") this.musicVolume = saved.music;
      if (typeof saved.sound === "number") this.soundVolume = saved.sound;
    } catch {
      // defaults stand
    }
  }

  saveVolumes(): void {
    localStorage.setItem("murphy.audio", JSON.stringify({ music: this.musicVolume, sound: this.soundVolume }));
    if (this.musicGain) this.musicGain.gain.value = this.musicVolume / 255;
  }

  /** Must first be called during a user gesture. */
  ensureContext(): AudioContext {
    if (!this.ctx) {
      this.ctx = new AudioContext();
      this.sfxGain = this.ctx.createGain();
      this.sfxGain.connect(this.ctx.destination);
      this.musicGain = this.ctx.createGain();
      this.musicGain.gain.value = this.musicVolume / 255;
      this.musicGain.connect(this.ctx.destination);
    }
    if (this.ctx.state === "suspended") void this.ctx.resume();
    return this.ctx;
  }

  get ready(): boolean {
    return this.ctx !== null;
  }

  private async buffer(rel: string): Promise<AudioBuffer | null> {
    const cached = this.buffers.get(rel);
    if (cached) return cached;
    try {
      const res = await fetch(`${BASE}/${rel}`);
      if (!res.ok) return null;
      const data = await res.arrayBuffer();
      const buf = await this.ensureContext().decodeAudioData(data);
      this.buffers.set(rel, buf);
      return buf;
    } catch {
      return null;
    }
  }

  async preload(rels: string[]): Promise<void> {
    if (!this.ctx) return;
    await Promise.all(rels.map((r) => this.buffer(r)));
  }

  /**
   * Fire-and-forget SFX. vol is 0..1 (proximity already applied);
   * pan is -10..10 as in the original.
   */
  play(rel: string, vol = 1, pan = 0): void {
    if (!this.ctx || this.soundVolume === 0 || vol <= 0) return;
    void this.buffer(rel).then((buf) => {
      if (!buf || !this.ctx || !this.sfxGain) return;
      const src = this.ctx.createBufferSource();
      src.buffer = buf;
      const gain = this.ctx.createGain();
      gain.gain.value = (this.soundVolume / 255) * Math.min(1, vol);
      src.connect(gain);
      const p = Math.max(-10, Math.min(10, pan)) / 10;
      if (p !== 0 && typeof this.ctx.createStereoPanner === "function") {
        const panner = this.ctx.createStereoPanner();
        panner.pan.value = p;
        gain.connect(panner);
        panner.connect(this.sfxGain);
      } else {
        gain.connect(this.sfxGain);
      }
      src.start();
    });
  }

  // --- music ---------------------------------------------------------------

  async playMusic(spec: MusicSpec): Promise<void> {
    this.stopMusic();
    if (!this.ctx || this.musicVolume === 0) return;
    const token = ++this.musicToken;
    const bufs: AudioBuffer[] = [];
    for (const f of spec.files) {
      const b = await this.buffer(f);
      if (!b) return; // missing/undecodable: silence rather than crash
      bufs.push(b);
    }
    if (token !== this.musicToken || !this.ctx || !this.musicGain) return;
    this.musicSpec = spec;
    this.musicIdx = spec.startAt;
    this.musicNextTime = this.ctx.currentTime + 0.05;
    this.musicGain.gain.value = this.musicVolume / 255;

    const pump = () => {
      if (!this.ctx || !this.musicGain || !this.musicSpec || token !== this.musicToken) return;
      while (this.musicNextTime < this.ctx.currentTime + 1.5) {
        const fileIdx = this.musicSpec.order[this.musicIdx] ?? 0;
        const buf = bufs[fileIdx];
        if (!buf) return;
        const src = this.ctx.createBufferSource();
        src.buffer = buf;
        src.connect(this.musicGain);
        src.start(this.musicNextTime);
        this.musicSources.push(src);
        if (this.musicSources.length > 8) this.musicSources.shift();
        this.musicNextTime += buf.duration;
        this.musicIdx += 1;
        if (this.musicIdx >= this.musicSpec.order.length) this.musicIdx = this.musicSpec.loopAt;
      }
    };
    pump();
    this.musicPump = window.setInterval(pump, 250);
  }

  stopMusic(): void {
    this.musicToken += 1;
    if (this.musicPump !== null) {
      clearInterval(this.musicPump);
      this.musicPump = null;
    }
    for (const s of this.musicSources) {
      try {
        s.stop();
      } catch {
        // never started or already ended
      }
    }
    this.musicSources = [];
    this.musicSpec = null;
  }

  /** Mute (keep playing) — used for pause, like the original's FadeMusicOut. */
  fadeMusicOut(): void {
    if (this.ctx && this.musicGain) {
      this.musicGain.gain.linearRampToValueAtTime(0, this.ctx.currentTime + 0.5);
    }
  }

  fadeMusicIn(): void {
    if (this.ctx && this.musicGain) {
      this.musicGain.gain.linearRampToValueAtTime(this.musicVolume / 255, this.ctx.currentTime + 0.5);
    }
  }
}

// --- gameplay SFX mapping ------------------------------------------------------

const EVENT_FILES: Record<string, string> = {
  gotInfotron: "got-infotron.wav",
  gotLastInfotron: "last-infotron.wav",
  gotRedDisk: "got-red-disk.wav",
  redDiskReady: "red-disk-ready.wav",
  oliverDied: "oliver-died.wav",
  endLevel: "exit-level.wav",
  push: "push.wav",
  termUse: "terminal-use.wav",
  termBad: "terminal-no-yellow.wav",
  portUse: "use-port.wav",
  thud: "thud.wav",
  explode: "explosion.wav",
  quarkExplode: "plasmoid-explosion.wav",
  bugZap: "bugzap.wav",
};

const AQUATRON_OVERRIDES: Record<string, string> = {
  thud: "water-thud.wav",
  push: "water-push.wav",
};

/** Worlds with no sfx of their own fall back to the Infotron bank. */
const SFX_WORLD: Record<string, string> = {
  infotron: "infotron",
  aquatron: "aquatron",
  savanatron: "infotron",
  deathtron: "infotron",
};

export class GameSounds {
  private world: string;

  constructor(
    private engine: AudioEngine,
    world: string
  ) {
    this.world = SFX_WORLD[world] ?? "infotron";
  }

  private resolve(kind: string): string | null {
    if (kind === "noMoreTime") return "interface/time-is-low.wav";
    const file =
      this.world === "aquatron" && AQUATRON_OVERRIDES[kind]
        ? AQUATRON_OVERRIDES[kind]
        : EVENT_FILES[kind];
    return file ? `${this.world}/${file}` : null;
  }

  preload(): void {
    const rels = Object.keys(EVENT_FILES).map((k) => this.resolve(k)).filter((r): r is string => !!r);
    rels.push("interface/time-is-low.wav", `${this.world}/pause.wav`, `${this.world}/unpause.wav`);
    void this.engine.preload(rels);
  }

  handle(e: SoundEvent): void {
    const rel = this.resolve(e.kind);
    if (!rel) return;
    let vol = 1;
    if (e.prox !== undefined) {
      if (e.prox > MAX_PROXIMITY) return;
      vol = (MAX_PROXIMITY - e.prox) / MAX_PROXIMITY;
    }
    this.engine.play(rel, vol, e.pan ?? 0);
  }

  pause(): void {
    this.engine.play(`${this.world}/pause.wav`);
  }

  unpause(): void {
    this.engine.play(`${this.world}/unpause.wav`);
  }
}

/** Interface bank helpers (menu clicks etc.). */
export const UI_SOUNDS = {
  tick: "interface/tick.wav",
  click: "interface/click.wav",
  levelInfoUp: "interface/level-info-pop-up.wav",
  levelInfoDown: "interface/level-info-pop-down.wav",
  mapSwitch: "interface/map-switch.wav",
  dialogOpen: "interface/player-select-dialog-open.wav",
  qLogo: "interface/q-logo-sound.wav",
} as const;
