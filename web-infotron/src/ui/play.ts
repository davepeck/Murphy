// Gameplay: the original's 640x419 viewport onto a 21x15 tile window drawn at
// a (-16,-16) half-tile offset, over the 61px status bar (PICT 129) with its
// exact field boxes (app-flow-infotron.md §1.3/§2).

import type { GameContext, Screen } from "../screen";
import { InfotronGame, SND } from "../engine/game";
import { classify, Kind } from "../engine/icons";
import type { InfotronLevelData } from "../engine/game";
import { setCurrentLevel } from "../save";

const TILE = 32;
const BOARD_H = 419;
const STATUS_Y = 419;
const ENGINE_DIVIDER = 5; // one engine frame per 5 ticks (~12 fps)

export class PlayScreen implements Screen {
  private game: InfotronGame | null = null;
  private level: InfotronLevelData | null = null;
  private tickN = 0;
  private paused = false;
  private left = 0; // 0-based upperLeft of the 21x15 window
  private top = 0;

  constructor(private levelNumber: number) {}

  async enter(ctx: GameContext): Promise<void> {
    const level = await ctx.assets.level(this.levelNumber);
    this.level = level;
    await ctx.assets.picture(129);
    this.game = new InfotronGame(level, {
      sound: (id) => ctx.audio.play(id),
    });
    ctx.audio.preloadSounds(Object.values(SND));
    this.scrollTo();
    if (ctx.audio.ready) void ctx.audio.startMusic();
    setCurrentLevel(this.levelNumber);
  }

  exit(ctx: GameContext): void {
    ctx.audio.stopMusic();
  }

  /** GotNewBoard's viewport formula (1-based in the original). */
  private scrollTo(): void {
    const g = this.game;
    if (!g) return;
    const mx1 = g.murphyX + 1;
    const my1 = g.murphyY + 1;
    let lh = mx1 - 10;
    let lv = my1 - 7;
    if (g.sizeX - mx1 < 10) lh = g.sizeX - 20;
    if (g.sizeY - my1 < 7) lv = g.sizeY - 13;
    this.left = Math.max(1, lh) - 1;
    this.top = Math.max(1, lv) - 1;
  }

  tick(ctx: GameContext): void {
    const game = this.game;
    if (!game) return;
    const input = ctx.input;

    if (input.wasPressed("pause") && !game.gameOver && !game.dying) {
      this.paused = !this.paused;
      ctx.audio.play(this.paused ? SND.pause : SND.unpause);
      // The original stops the music on pause and restarts it on unpause.
      if (this.paused) ctx.audio.stopMusic();
      else void ctx.audio.startMusic();
    }
    if (this.paused) return;

    if (input.wasPressed("escape") && !game.dying && !game.frozen) {
      game.killMurphy(); // the suicide key
    }
    // v1.1 fast restart during the death freeze: r (or y on the prompt).
    if (
      game.dying &&
      (input.keysPressed.includes("KeyR") || input.keysPressed.includes("KeyY"))
    ) {
      ctx.switchScreen(new PlayScreen(this.levelNumber));
      return;
    }

    this.tickN += 1;
    if (this.tickN % ENGINE_DIVIDER !== 0) return;

    game.frame({
      up: input.isHeld("up"),
      right: input.isHeld("right"),
      down: input.isHeld("down"),
      left: input.isHeld("left"),
      space: input.isHeld("special"),
    });
    this.scrollTo();

    if (game.gameOver) {
      ctx.audio.stopMusic(); // CKillGameChore stops the music
      if (game.wonLevel) {
        const next = Math.min(this.levelNumber + 1, ctx.assets.levelIndex.length);
        setCurrentLevel(next);
        ctx.switchScreen(new PlayScreen(next));
      } else {
        ctx.switchScreen(new PlayScreen(this.levelNumber));
      }
    }
  }

  render(ctx: GameContext, g: CanvasRenderingContext2D): void {
    const game = this.game;
    const level = this.level;
    if (!game || !level) return;

    g.fillStyle = "#000";
    g.fillRect(0, 0, ctx.width, ctx.height);

    // 21x15 tile window at the (-16,-16) half-tile offset, clipped to 640x419.
    g.save();
    g.beginPath();
    g.rect(0, 0, ctx.width, BOARD_H);
    g.clip();
    for (let row = 0; row < 15; row++) {
      const y = this.top + row;
      if (y >= game.sizeY) break;
      for (let col = 0; col < 21; col++) {
        const x = this.left + col;
        if (x >= game.sizeX) break;
        const v = game.data(x, y);
        if (v !== 129) ctx.assets.drawIcon(g, v, col * TILE - 16, row * TILE - 16);
      }
    }
    // Portal transit: the board keeps the portal icon; overlay Murphy.
    if (!game.dying && classify(game.data(game.murphyX, game.murphyY)) === Kind.Portal) {
      ctx.assets.drawIcon(
        g,
        198,
        (game.murphyX - this.left) * TILE - 16,
        (game.murphyY - this.top) * TILE - 16
      );
    }
    g.restore();

    this.renderStatus(ctx, g, game, level);

    if (game.dying && !game.gameOver) {
      // v1.1's "Care To Try Again?" during the 4s death freeze
      g.fillStyle = "rgba(0,0,0,0.6)";
      g.fillRect(160, 180, 320, 52);
      g.fillStyle = "#ff4444";
      g.font = "bold 16px Chicago, Monaco, monospace";
      g.textAlign = "center";
      g.fillText("Care To Try Again?", 320, 202);
      g.fillStyle = "#dddddd";
      g.font = "11px Monaco, monospace";
      g.fillText("R to restart now", 320, 222);
      g.textAlign = "left";
    }

    if (this.paused) {
      g.fillStyle = "rgba(0,0,0,0.55)";
      g.fillRect(0, 0, ctx.width, BOARD_H);
      g.fillStyle = "#33ff44";
      g.font = "bold 18px Chicago, Monaco, monospace";
      g.textAlign = "center";
      g.fillText("— PAUSED —", ctx.width / 2, BOARD_H / 2);
      g.textAlign = "left";
    }
  }

  /** The status bar's exact field boxes (app-flow spec §2). */
  private renderStatus(
    ctx: GameContext,
    g: CanvasRenderingContext2D,
    game: InfotronGame,
    level: InfotronLevelData
  ): void {
    g.drawImage(ctx.assets.img("pictures/129.png"), 1, STATUS_Y);
    const text = (s: string, x: number, baseline: number, color: string, center = false) => {
      g.fillStyle = color;
      g.font = "bold 12px Chicago, Monaco, monospace";
      g.textAlign = center ? "center" : "left";
      g.fillText(s, x, baseline);
      g.textAlign = "left";
    };
    // Player name, red, centered between x 191..334, baseline 442.
    text("OLIVER", (191 + 334) / 2, 442, "#ee2222", true);
    // Clock (blue): elapsed engine ticks -> h/m/s like convertTime.
    const t = game.tickCount;
    text(String(Math.floor(t / 216000)).padStart(2, "0"), 359, 442, "#4466ff");
    text(String(Math.floor((t % 216000) / 3600)).padStart(2, "0"), 397, 442, "#4466ff");
    text(String(Math.floor(((t % 216000) % 3600) / 60)).padStart(2, "0"), 435, 442, "#4466ff");
    // Infotrons still needed: green when 0, else red.
    text(String(game.numInfos).padStart(3, "0"), 495, 442, game.numInfos === 0 ? "#22cc33" : "#ee2222");
    // Level name, green, centered between x 191..409, baseline 466.
    text(level.name, (191 + 409) / 2, 466, "#22cc33", true);
    // Level number, green.
    text(String(this.levelNumber).padStart(3, "0"), 427, 466, "#22cc33");
    // Red disks: blue when 0, red when carrying.
    text(String(game.numBombs).padStart(3, "0"), 495, 466, game.numBombs > 0 ? "#ee2222" : "#4466ff");
  }
}
