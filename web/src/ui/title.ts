// Boot/title: Q Software banner + Murphy's Revenge logo, then the main menu.

import type { GameContext, Screen } from "../screen";
import { INTERFACE_MUSIC } from "../audio";

export class TitleScreen implements Screen {
  private t = 0;

  async enter(ctx: GameContext): Promise<void> {
    await Promise.all([
      ctx.assets.image("interface/bar-q-software-presents.png"),
      ctx.assets.image("interface/murphys-revenge-logo.png"),
    ]);
  }

  tick(ctx: GameContext): void {
    this.t += 1;
    if (this.t > 10 && (ctx.input.anyKeyPressed || ctx.pointer.wentDown)) {
      ctx.audio.ensureContext();
      void ctx.audio.playMusic(INTERFACE_MUSIC);
      void import("./menu").then(({ MenuScreen }) => ctx.switchScreen(new MenuScreen()));
    }
  }

  render(ctx: GameContext, g: CanvasRenderingContext2D): void {
    g.fillStyle = "#000";
    g.fillRect(0, 0, ctx.width, ctx.height);
    const fade = Math.min(1, this.t / 60);
    g.globalAlpha = fade;
    const banner = ctx.assets.img("interface/bar-q-software-presents.png");
    g.drawImage(banner, (ctx.width - banner.width) / 2, 40);
    const logo = ctx.assets.img("interface/murphys-revenge-logo.png");
    g.drawImage(logo, (ctx.width - logo.width) / 2, 110);
    if (this.t > 90 && Math.floor(this.t / 40) % 2 === 0) {
      g.fillStyle = "#98CCCC";
      g.font = "12px Helvetica, Arial, sans-serif";
      g.textAlign = "center";
      g.fillText("click or press any key", ctx.width / 2, 430);
      g.textAlign = "left";
    }
    g.globalAlpha = 1;
  }
}
