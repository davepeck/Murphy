// The main menu: primary-interface art with hover/pressed variants, exactly
// the original hotspot rects, plus story/credits/help sub-screens and a
// volume-only preferences dialog.

import type { GameContext, Screen } from "../screen";
import { BubbleOliver } from "../render/bubble";
import { UI_SOUNDS, VOLUME_STEPS, VOLUME_NAMES, INTERFACE_MUSIC } from "../audio";
import { defaultRng } from "../engine/rng";

const PIC_X = 80; // 480x480 menu picture centered on the 640-wide screen

interface Hot {
  id: string;
  x: number;
  y: number;
  w: number;
  h: number;
  key: string;
}

const HOTSPOTS: Hot[] = [
  { id: "intro", x: 32, y: 96, w: 120, h: 40, key: "KeyI" },
  { id: "credits", x: 32, y: 156, w: 120, h: 40, key: "KeyC" },
  { id: "help", x: 32, y: 216, w: 120, h: 40, key: "KeyH" },
  { id: "quit", x: 32, y: 276, w: 120, h: 40, key: "KeyQ" },
  { id: "start", x: 258, y: 343, w: 222, h: 97, key: "KeyS" },
  { id: "prefs", x: 9, y: 400, w: 182, h: 32, key: "KeyP" },
];

type SubScreen = "menu" | "intro" | "credits" | "help";

const SUB_ART: Record<Exclude<SubScreen, "menu">, { bar: string; text: string }> = {
  intro: { bar: "interface/bar-intro.png", text: "interface/story.png" },
  credits: { bar: "interface/bar-about.png", text: "interface/credits.png" },
  help: { bar: "interface/bar-help.png", text: "interface/help-text.png" },
};

export class MenuScreen implements Screen {
  private sub: SubScreen = "menu";
  private hover: string | null = null;
  private pressed: string | null = null;
  private bubble: BubbleOliver | null = null;
  private t = 0;

  // prefs dialog state
  private prefsOpen = false;
  private prefsSnapshot = { music: 0, sound: 0 };

  async enter(ctx: GameContext): Promise<void> {
    const a = ctx.assets;
    await Promise.all([
      a.image("interface/primary-interface.png"),
      a.image("interface/primary-interface-rollover.png"),
      a.image("interface/primary-interface-select.png"),
      a.image("interface/oliver-basics.png"),
      a.image("interface/preferences.png"),
      a.image("interface/preferences-select.png"),
      ...VOLUME_NAMES.map((n) => a.image(`interface/audio-${n}.png`)),
      ...Object.values(SUB_ART).flatMap((s) => [a.image(s.bar), a.image(s.text)]),
    ]);
    this.bubble = new BubbleOliver(a.img("interface/oliver-basics.png"));
  }

  tick(ctx: GameContext): void {
    this.t += 1;
    this.bubble?.update(this.t, defaultRng);

    if (this.prefsOpen) {
      this.tickPrefs(ctx);
      return;
    }
    if (this.sub !== "menu") {
      if (ctx.input.anyKeyPressed || ctx.pointer.wentDown) {
        ctx.audio.play(UI_SOUNDS.click);
        this.sub = "menu";
      }
      return;
    }

    // Hover / press tracking.
    const p = ctx.pointer;
    this.hover = null;
    for (const h of HOTSPOTS) {
      if (p.inside(PIC_X + h.x, h.y, h.w, h.h)) this.hover = h.id;
    }
    if (p.wentDown && this.hover) {
      this.pressed = this.hover;
      ctx.audio.play(UI_SOUNDS.tick);
    }
    if (p.wentUp) {
      if (this.pressed && this.pressed === this.hover) this.activate(ctx, this.pressed);
      this.pressed = null;
    }
    for (const h of HOTSPOTS) {
      if (ctx.input.keysPressed.includes(h.key)) this.activate(ctx, h.id);
    }
    if (ctx.input.wasPressed("confirm")) this.activate(ctx, "start");
  }

  private activate(ctx: GameContext, id: string): void {
    ctx.audio.play(UI_SOUNDS.click);
    switch (id) {
      case "start":
        ctx.audio.stopMusic();
        void import("./map").then(({ MapScreen }) => ctx.switchScreen(new MapScreen()));
        break;
      case "intro":
      case "credits":
      case "help":
        this.sub = id;
        break;
      case "quit":
        ctx.audio.stopMusic();
        void import("./title").then(({ TitleScreen }) => ctx.switchScreen(new TitleScreen()));
        break;
      case "prefs":
        this.prefsOpen = true;
        this.prefsSnapshot = { music: ctx.audio.musicVolume, sound: ctx.audio.soundVolume };
        ctx.audio.play(UI_SOUNDS.dialogOpen);
        break;
    }
  }

  // --- preferences dialog ----------------------------------------------------

  private prefRects(): Record<string, [number, number, number, number]> {
    const dx = (640 - 200) / 2;
    const dy = 0;
    return {
      musicDown: [dx + 27, dy + 317, 14, 15],
      musicUp: [dx + 161, dy + 317, 14, 15],
      soundDown: [dx + 27, dy + 363, 14, 15],
      soundUp: [dx + 161, dy + 363, 14, 15],
      cancel: [dx + 11, dy + 440, 81, 28],
      okay: [dx + 108, dy + 440, 81, 28],
    };
  }

  private cycle(v: number, delta: number): number {
    const i = VOLUME_STEPS.indexOf(v as (typeof VOLUME_STEPS)[number]);
    const n = (i < 0 ? 3 : i + delta + VOLUME_STEPS.length) % VOLUME_STEPS.length;
    return VOLUME_STEPS[n] as number;
  }

  private tickPrefs(ctx: GameContext): void {
    const p = ctx.pointer;
    if (!p.wentDown) {
      if (ctx.input.wasPressed("confirm")) this.closePrefs(ctx, true);
      else if (ctx.input.wasPressed("escape")) this.closePrefs(ctx, false);
      return;
    }
    const r = this.prefRects();
    const hit = (k: string) => {
      const [x, y, w, h] = r[k] as [number, number, number, number];
      return p.inside(x, y, w, h);
    };
    if (hit("musicDown")) ctx.audio.musicVolume = this.cycle(ctx.audio.musicVolume, -1);
    else if (hit("musicUp")) ctx.audio.musicVolume = this.cycle(ctx.audio.musicVolume, 1);
    else if (hit("soundDown")) ctx.audio.soundVolume = this.cycle(ctx.audio.soundVolume, -1);
    else if (hit("soundUp")) ctx.audio.soundVolume = this.cycle(ctx.audio.soundVolume, 1);
    else if (hit("okay")) {
      this.closePrefs(ctx, true);
      return;
    } else if (hit("cancel")) {
      this.closePrefs(ctx, false);
      return;
    }
    ctx.audio.play(UI_SOUNDS.tick);
  }

  private closePrefs(ctx: GameContext, save: boolean): void {
    if (save) {
      ctx.audio.saveVolumes();
      // The original restarts the music only if its volume actually changed.
      if (ctx.audio.musicVolume !== this.prefsSnapshot.music) {
        if (ctx.audio.musicVolume === 0) ctx.audio.stopMusic();
        else void ctx.audio.playMusic(INTERFACE_MUSIC);
      }
    } else {
      ctx.audio.musicVolume = this.prefsSnapshot.music;
      ctx.audio.soundVolume = this.prefsSnapshot.sound;
    }
    ctx.audio.play(UI_SOUNDS.click);
    this.prefsOpen = false;
  }

  // --- rendering ---------------------------------------------------------------

  render(ctx: GameContext, g: CanvasRenderingContext2D): void {
    g.fillStyle = "#000";
    g.fillRect(0, 0, ctx.width, ctx.height);

    if (this.sub !== "menu") {
      const art = SUB_ART[this.sub];
      g.drawImage(ctx.assets.img(art.bar), 0, 0); // flush left, like WipePictIntoWindow
      g.drawImage(ctx.assets.img(art.text), 120, 0);
      return;
    }

    const a = ctx.assets;
    g.drawImage(a.img("interface/primary-interface.png"), PIC_X, 0);
    const overlayFor = (state: "hover" | "pressed") =>
      state === "hover"
        ? a.img("interface/primary-interface-rollover.png")
        : a.img("interface/primary-interface-select.png");
    for (const h of HOTSPOTS) {
      const state = this.pressed === h.id && this.hover === h.id ? "pressed" : this.hover === h.id ? "hover" : null;
      if (state) {
        g.drawImage(overlayFor(state), h.x, h.y, h.w, h.h, PIC_X + h.x, h.y, h.w, h.h);
      }
    }
    this.bubble?.draw(g, PIC_X + 304, 198);

    if (this.prefsOpen) this.renderPrefs(ctx, g);
  }

  private renderPrefs(ctx: GameContext, g: CanvasRenderingContext2D): void {
    // Screen-door dim, like DimRectangle.
    g.fillStyle = "rgba(0,0,0,0.6)";
    g.fillRect(0, 0, ctx.width, ctx.height);
    const dx = (640 - 200) / 2;
    g.drawImage(ctx.assets.img("interface/preferences.png"), dx, 0);
    this.bubble?.draw(g, dx + 83, 120);

    const label = (vol: number) =>
      ctx.assets.img(`interface/audio-${VOLUME_NAMES[Math.max(0, VOLUME_STEPS.indexOf(vol as never))]}.png`);
    g.drawImage(label(ctx.audio.musicVolume), dx + 37, 313);
    g.drawImage(label(ctx.audio.soundVolume), dx + 37, 359);

    // Show our fixed key bindings in the key boxes.
    g.fillStyle = "#fff";
    g.font = "bold 11px Helvetica, Arial, sans-serif";
    g.textAlign = "center";
    const box = (x: number, y: number, w: number, h: number, s: string) =>
      g.fillText(s, dx + x + w / 2, y + h / 2 + 4);
    box(77, 68, 44, 42, "↑");
    box(31, 114, 44, 44, "←");
    box(125, 114, 42, 44, "→");
    box(77, 162, 44, 42, "↓");
    box(129, 213, 34, 17, "Spc");
    box(129, 238, 34, 17, "P");
    box(129, 263, 34, 17, "Esc");
    g.textAlign = "left";
  }
}
