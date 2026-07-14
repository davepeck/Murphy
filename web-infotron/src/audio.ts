// WebAudio for Infotron: sounds by resource ID, and the background-music
// rotation (the original rotated its tracks every 120 seconds).

const BASE = import.meta.env.BASE_URL + "assets";
const MUSIC_ROTATE_SECS = 120;

export class AudioEngine {
  private ctx: AudioContext | null = null;
  private gain: GainNode | null = null;
  private musicGain: GainNode | null = null;
  private buffers = new Map<string, AudioBuffer>();
  private musicSource: AudioBufferSourceNode | null = null;
  private musicTimer: number | null = null;
  private musicTrackIdx = 0;
  private musicTracks: number[] = [];

  soundOn: boolean;
  musicOn: boolean;

  constructor() {
    let sound = true;
    let music = true;
    try {
      const saved = JSON.parse(localStorage.getItem("infotron.audio") ?? "{}");
      if (typeof saved.sound === "boolean") sound = saved.sound;
      if (typeof saved.music === "boolean") music = saved.music;
    } catch {
      // defaults stand
    }
    this.soundOn = sound;
    this.musicOn = music;
  }

  save(): void {
    localStorage.setItem("infotron.audio", JSON.stringify({ sound: this.soundOn, music: this.musicOn }));
  }

  ensureContext(): AudioContext {
    if (!this.ctx) {
      this.ctx = new AudioContext();
      this.gain = this.ctx.createGain();
      this.gain.gain.value = 0.8;
      this.gain.connect(this.ctx.destination);
      this.musicGain = this.ctx.createGain();
      this.musicGain.gain.value = 0.5;
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
      const buf = await this.ensureContext().decodeAudioData(await res.arrayBuffer());
      this.buffers.set(rel, buf);
      return buf;
    } catch {
      return null;
    }
  }

  preloadSounds(ids: number[]): void {
    if (!this.ctx) return;
    for (const id of ids) void this.buffer(`sounds/${id}.wav`);
  }

  play(id: number): void {
    if (!this.ctx || !this.soundOn) return;
    void this.buffer(`sounds/${id}.wav`).then((buf) => {
      if (!buf || !this.ctx || !this.gain) return;
      const src = this.ctx.createBufferSource();
      src.buffer = buf;
      src.connect(this.gain);
      src.start();
    });
  }

  /**
   * Background music, exactly as shipped: every 120s the original picks a
   * random track in 1000..1002, but only 1000 ("music Intro") and 1001
   * ("m.loop") exist — so a third of rotations are silence. Each track loops
   * seamlessly until the next rotation.
   */
  async startMusic(): Promise<void> {
    this.stopMusic();
    if (!this.ctx || !this.musicOn) return;
    this.musicTracks = [1000, 1001, 0]; // 0 = the missing track 1002: silence
    this.musicTrackIdx = 0; // start on a real track
    await this.playCurrentTrack();
    this.musicTimer = window.setInterval(() => {
      this.musicTrackIdx = Math.floor(Math.random() * this.musicTracks.length);
      void this.playCurrentTrack();
    }, MUSIC_ROTATE_SECS * 1000);
  }

  private async playCurrentTrack(): Promise<void> {
    if (this.musicSource) {
      try {
        this.musicSource.stop();
      } catch {
        // fine
      }
      this.musicSource = null;
    }
    const id = this.musicTracks[this.musicTrackIdx];
    if (!id) return; // the shipped game's missing-track silence
    const buf = await this.buffer(`music/${id}.wav`);
    if (!buf || !this.ctx || !this.musicGain || !this.musicOn) return;
    const src = this.ctx.createBufferSource();
    src.buffer = buf;
    src.loop = true;
    src.connect(this.musicGain);
    src.start();
    this.musicSource = src;
  }

  stopMusic(): void {
    if (this.musicTimer !== null) {
      clearInterval(this.musicTimer);
      this.musicTimer = null;
    }
    if (this.musicSource) {
      try {
        this.musicSource.stop();
      } catch {
        // fine
      }
      this.musicSource = null;
    }
  }

  setMusicOn(on: boolean): void {
    this.musicOn = on;
    this.save();
    if (!on) this.stopMusic();
  }
}
