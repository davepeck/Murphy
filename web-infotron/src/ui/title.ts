// Title: the original 640x480 splash (PICT 3000) with a level selector.

import type { GameContext, Screen } from "../screen";
import { progress } from "../save";

export class TitleScreen implements Screen {
  private t = 0;
  private level = 1;
  private maxLevel = 51;

  async enter(ctx: GameContext): Promise<void> {
    await ctx.assets.picture(3000);
    const p = progress();
    this.level = p.current;
    this.maxLevel = ctx.assets.levelIndex.length;
  }

  tick(ctx: GameContext): void {
    this.t += 1;
    const input = ctx.input;
    if (input.wasPressed("up") || input.wasPressed("right")) {
      this.level = Math.min(this.maxLevel, this.level + 1);
    }
    if (input.wasPressed("down") || input.wasPressed("left")) {
      this.level = Math.max(1, this.level - 1);
    }
    if (input.wasPressed("confirm") || input.keysPressed.includes("Space") || ctx.pointer.wentDown) {
      ctx.audio.ensureContext();
      void import("./play").then(({ PlayScreen }) => ctx.switchScreen(new PlayScreen(this.level)));
    }
  }

  render(ctx: GameContext, g: CanvasRenderingContext2D): void {
    g.fillStyle = "#000";
    g.fillRect(0, 0, ctx.width, ctx.height);
    g.drawImage(ctx.assets.img("pictures/3000.png"), 0, 0);
    const entry = ctx.assets.levelIndex.find((l) => l.levelNumber === this.level);
    if (this.t > 30) {
      g.fillStyle = "rgba(0,0,0,0.72)";
      g.fillRect(140, 420, 360, 44);
      g.fillStyle = "#33ff44";
      g.font = "bold 14px Monaco, monospace";
      g.textAlign = "center";
      g.fillText(`Level ${String(this.level).padStart(3, "0")}: ${entry?.name ?? "?"}`, 320, 438);
      g.fillStyle = "#ffffff";
      g.font = "11px Monaco, monospace";
      g.fillText("←/→ choose level — Enter to play", 320, 456);
      g.textAlign = "left";
    }
  }
}
