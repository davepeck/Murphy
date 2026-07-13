// The gameplay screen: 600x480 scrolling viewport + the original 40px status
// bar, pause, and the death/replay dialog. The engine steps at 30 Hz (every
// other UI tick), exactly like the original's SpriteWorld maxFPS cap.

import type { GameContext, Screen } from "../screen";
import { Game } from "../engine/game";
import { Dir } from "../engine/tiles";
import type { LevelData } from "../engine/level";
import { Camera, renderLevel } from "../render/levelRenderer";
import { BitmapFont } from "../render/font";
import { BubbleOliver } from "../render/bubble";
import { GameSounds, gameplayMusic, UI_SOUNDS } from "../audio";
import { defaultRng } from "../engine/rng";
import { markPassed } from "../save";
import type { TileAtlas } from "../assets";

const VIEW_W = 600;
const VIEW_H = 480;
const STAT_X = 600;
const HUD_TEXT = "#98CCCC";

const WORLD_INDEX: Record<string, number> = {
  infotron: 0,
  aquatron: 1,
  savanatron: 2,
  deathtron: 3,
};

type DirAction = "up" | "down" | "left" | "right";
const DIR_OF: Record<DirAction, Dir> = {
  up: Dir.Up,
  down: Dir.Down,
  left: Dir.Left,
  right: Dir.Right,
};

export class PlayScreen implements Screen {
  private game: Game | null = null;
  private level: LevelData | null = null;
  private atlas: TileAtlas | null = null;
  private cam: Camera | null = null;
  private sounds: GameSounds | null = null;
  private font: BitmapFont | null = null;
  private bubble: BubbleOliver | null = null;

  private keyStack: DirAction[] = [];
  private engineParity = 0;
  private paused = false;
  private showDialog = false;
  private winTick: number | null = null;
  private dimPattern: CanvasPattern | null = null;

  constructor(private file: string) {}

  async enter(ctx: GameContext): Promise<void> {
    const a = ctx.assets;
    const level = await a.level(this.file);
    this.level = level;
    this.atlas = await a.atlas(level.world);
    await Promise.all([
      a.image("interface/status-bar-game.png"),
      a.image("interface/level-stats-time-left.png"),
      a.image("interface/infotron-minis.png"),
      a.image("interface/red-disk-minis.png"),
      a.image("interface/game-paused.png"),
      a.image("interface/replay-level.png"),
      a.image("interface/replay-level-select.png"),
      a.image("interface/font-small.png"),
      a.image("interface/oliver-basics.png"),
    ]);
    this.font = new BitmapFont(a.img("interface/font-small.png"));
    this.bubble = new BubbleOliver(a.img("interface/oliver-basics.png"));

    this.sounds = new GameSounds(ctx.audio, level.world);
    this.sounds.preload();
    this.game = new Game(level, { sound: (e) => this.sounds?.handle(e) });
    const o = this.game.oliver;
    this.cam = new Camera(VIEW_W, VIEW_H, level.width * 32, level.height * 32);
    if (o) this.cam.centerOn(o.x, o.y);
    if (ctx.audio.ready) void ctx.audio.playMusic(gameplayMusic(level.world));
  }

  exit(ctx: GameContext): void {
    ctx.audio.stopMusic();
  }

  private exitToSelect(ctx: GameContext): void {
    void import("./select").then(({ SelectScreen }) => ctx.switchScreen(new SelectScreen()));
  }

  private restart(ctx: GameContext): void {
    ctx.switchScreen(new PlayScreen(this.file));
  }

  tick(ctx: GameContext): void {
    const game = this.game;
    if (!game) return;
    const input = ctx.input;

    // Most-recent-wins key stack, like the original's theKeys[4].
    for (const d of ["up", "down", "left", "right"] as DirAction[]) {
      if (input.wasPressed(d)) {
        this.keyStack = this.keyStack.filter((k) => k !== d);
        this.keyStack.unshift(d);
      }
      if (!input.isHeld(d)) this.keyStack = this.keyStack.filter((k) => k !== d);
    }
    if (input.anyKeyPressed) game.lastPressTick = game.tickCount;

    // Pause (only while Oliver is alive and no dialog).
    if (input.wasPressed("pause") && game.oliver && !this.showDialog) {
      this.paused = !this.paused;
      if (this.paused) {
        this.sounds?.pause();
        ctx.audio.fadeMusicOut();
      } else {
        this.sounds?.unpause();
        ctx.audio.fadeMusicIn();
        this.keyStack = [];
      }
    }
    if (this.paused) return;

    // Death / replay dialog.
    if (this.showDialog) {
      const p = ctx.pointer;
      const dx = VIEW_W / 2 - 100;
      const dy = VIEW_H / 2 - 73;
      const yes =
        input.keysPressed.includes("KeyY") ||
        input.wasPressed("confirm") ||
        (p.wentDown && p.inside(dx + 108, dy + 111, 66, 20));
      const no =
        input.keysPressed.includes("KeyN") ||
        input.wasPressed("escape") ||
        (p.wentDown && p.inside(dx + 26, dy + 111, 66, 20));
      if (yes) {
        ctx.audio.play(UI_SOUNDS.click);
        this.restart(ctx);
        return;
      }
      if (no) {
        ctx.audio.play(UI_SOUNDS.click);
        this.exitToSelect(ctx);
        return;
      }
    } else if (input.wasPressed("escape") && game.oliver) {
      game.killOliver(false); // the suicide key
    }

    // The engine runs at 30 Hz.
    this.engineParity ^= 1;
    if (this.engineParity === 1) return;

    const dir = this.keyStack[0];
    game.frame({
      dir: dir ? DIR_OF[dir] : Dir.None,
      special: input.isHeld("special"),
    });

    const o = game.oliver;
    if (o && !o.dead && this.cam) this.cam.follow(o.x, o.y);

    if (game.wonLevel && !game.oliver) {
      if (this.winTick === null) this.winTick = game.tickCount;
      else if (game.tickCount - this.winTick > 60) {
        markPassed(this.file);
        this.exitToSelect(ctx);
      }
    } else if (!game.oliver && !this.showDialog) {
      this.showDialog = true; // death animation finished
    }
  }

  render(ctx: GameContext, g: CanvasRenderingContext2D): void {
    const game = this.game;
    const atlas = this.atlas;
    const cam = this.cam;
    const level = this.level;
    if (!game || !atlas || !cam || !level) return;

    renderLevel(g, atlas, game, cam, 0, 0, VIEW_W, VIEW_H);
    this.renderHUD(ctx, g, game, level);

    if (this.showDialog) this.renderDialog(ctx, g);
    if (this.paused) this.renderPause(ctx, g);
  }

  private renderHUD(ctx: GameContext, g: CanvasRenderingContext2D, game: Game, level: LevelData): void {
    const a = ctx.assets;
    const timed = level.time !== null;
    g.drawImage(
      a.img(timed ? "interface/level-stats-time-left.png" : "interface/status-bar-game.png"),
      STAT_X,
      0
    );
    // Bubbling Oliver head.
    this.bubble?.update(game.tickCount, defaultRng);
    this.bubble?.draw(g, STAT_X + 4, 4);
    // Vertical level title in green lettering.
    this.font?.drawVertical(g, level.name, STAT_X + 11, 77, 267);

    // Counters.
    const w = WORLD_INDEX[level.world] ?? 0;
    g.drawImage(a.img("interface/infotron-minis.png"), w * 16, 0, 16, 16, STAT_X + 4, 324, 16, 16);
    g.drawImage(a.img("interface/red-disk-minis.png"), w * 16, 0, 16, 16, STAT_X + 4, 364, 16, 16);
    g.fillStyle = HUD_TEXT;
    g.font = "9px Helvetica, Arial, sans-serif";
    g.textAlign = "center";
    g.fillText(String(game.infotronCount), STAT_X + 28, 336);
    g.fillText(String(game.redDiskCount), STAT_X + 28, 376);
    if (timed) {
      g.fillText(String(game.hours), STAT_X + 7.5, 438);
      g.fillText(String(game.mins).padStart(2, "0"), STAT_X + 17, 438);
      g.fillText(String(game.secs).padStart(2, "0"), STAT_X + 29, 438);
    }
    g.textAlign = "left";
  }

  private dimScreen(g: CanvasRenderingContext2D, w: number, h: number): void {
    // DimRectangle: black lines every 2 px both ways (a 75% screen-door).
    if (!this.dimPattern) {
      const c = document.createElement("canvas");
      c.width = 2;
      c.height = 2;
      const cg = c.getContext("2d");
      if (cg) {
        cg.fillStyle = "#000";
        cg.fillRect(0, 0, 2, 2);
        cg.clearRect(1, 1, 1, 1);
      }
      this.dimPattern = g.createPattern(c, "repeat");
    }
    if (this.dimPattern) {
      g.fillStyle = this.dimPattern;
      g.fillRect(0, 0, w, h);
    }
  }

  private renderPause(ctx: GameContext, g: CanvasRenderingContext2D): void {
    this.dimScreen(g, ctx.width, ctx.height);
    const pic = ctx.assets.img("interface/game-paused.png");
    g.drawImage(pic, (ctx.width - pic.width) / 2, (ctx.height - pic.height) / 2);
  }

  private renderDialog(ctx: GameContext, g: CanvasRenderingContext2D): void {
    const dx = VIEW_W / 2 - 100;
    const dy = VIEW_H / 2 - 73;
    const p = ctx.pointer;
    const hoverYes = p.inside(dx + 108, dy + 111, 66, 20);
    const hoverNo = p.inside(dx + 26, dy + 111, 66, 20);
    g.drawImage(ctx.assets.img("interface/replay-level.png"), dx, dy);
    if (hoverYes || hoverNo) {
      const sel = ctx.assets.img("interface/replay-level-select.png");
      if (hoverYes) g.drawImage(sel, 108, 111, 66, 20, dx + 108, dy + 111, 66, 20);
      if (hoverNo) g.drawImage(sel, 26, 111, 66, 20, dx + 26, dy + 111, 66, 20);
    }
  }
}
