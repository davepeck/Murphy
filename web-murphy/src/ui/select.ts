// Level select (stands in for the original's unfinished map room): the level
// list with the original level-stats info card, map-room side panels + music.

import type { GameContext, Screen } from "../screen";
import { BubbleOliver } from "../render/bubble";
import { MAP_ROOM_MUSIC, UI_SOUNDS } from "../audio";
import { defaultRng } from "../engine/rng";
import { passedLevels } from "../save";

const LIST_X = 64;
const LIST_Y = 22;
const ROW_H = 15;
const CARD_X = 372;
const CARD_Y = 160;
const TEXT_COLOR = "#98CCCC";

export class SelectScreen implements Screen {
  private selected = 0;
  private bubble: BubbleOliver | null = null;
  private t = 0;
  private passed = new Set<string>();

  async enter(ctx: GameContext): Promise<void> {
    const a = ctx.assets;
    await Promise.all([
      a.image("interface/bar-map-room.png"),
      a.image("interface/status-bar-map.png"),
      a.image("interface/back.png"),
      a.image("interface/back-select.png"),
      a.image("interface/level-stats.png"),
      a.image("interface/level-stats-select.png"),
      a.image("interface/oliver-basics.png"),
    ]);
    this.bubble = new BubbleOliver(a.img("interface/oliver-basics.png"));
    this.passed = passedLevels();
    const last = localStorage.getItem("murphy.lastLevel");
    if (last) {
      const i = ctx.assets.levelIndex.findIndex((l) => l.file === last);
      if (i >= 0) this.selected = i;
    }
    if (ctx.audio.ready) void ctx.audio.playMusic(MAP_ROOM_MUSIC);
  }

  private select(ctx: GameContext, i: number): void {
    const n = ctx.assets.levelIndex.length;
    const next = Math.max(0, Math.min(n - 1, i));
    if (next !== this.selected) {
      this.selected = next;
      ctx.audio.play(UI_SOUNDS.levelInfoUp);
    }
  }

  private play(ctx: GameContext): void {
    const entry = ctx.assets.levelIndex[this.selected];
    if (!entry) return;
    ctx.audio.play(UI_SOUNDS.click);
    ctx.audio.stopMusic();
    localStorage.setItem("murphy.lastLevel", entry.file);
    void import("./play").then(({ PlayScreen }) =>
      ctx.switchScreen(new PlayScreen(entry.file, "select"))
    );
  }

  private back(ctx: GameContext): void {
    ctx.audio.play(UI_SOUNDS.click);
    ctx.audio.stopMusic();
    void import("./map").then(({ MapScreen }) => ctx.switchScreen(new MapScreen()));
  }

  tick(ctx: GameContext): void {
    this.t += 1;
    this.bubble?.update(this.t, defaultRng);
    const input = ctx.input;
    if (input.wasPressed("up")) this.select(ctx, this.selected - 1);
    if (input.wasPressed("down")) this.select(ctx, this.selected + 1);
    if (input.wasPressed("confirm")) this.play(ctx);
    if (input.wasPressed("escape")) this.back(ctx);

    const p = ctx.pointer;
    if (p.wentDown) {
      // list rows
      const n = ctx.assets.levelIndex.length;
      for (let i = 0; i < n; i++) {
        if (p.inside(LIST_X - 8, LIST_Y + i * ROW_H - 11, 280, ROW_H)) {
          if (i === this.selected) this.play(ctx);
          else this.select(ctx, i);
          return;
        }
      }
      // play button on the card: (125,116) 64x20
      if (p.inside(CARD_X + 125, CARD_Y + 116, 64, 20)) {
        this.play(ctx);
        return;
      }
      // back arrow, bottom-left
      if (p.inside(0, 440, 40, 40)) this.back(ctx);
    }
  }

  render(ctx: GameContext, g: CanvasRenderingContext2D): void {
    const a = ctx.assets;
    g.fillStyle = "#000";
    g.fillRect(0, 0, ctx.width, ctx.height);
    g.drawImage(a.img("interface/bar-map-room.png"), 0, 0);
    g.drawImage(a.img("interface/status-bar-map.png"), 600, 0);
    const backHover = ctx.pointer.inside(0, 440, 40, 40);
    g.drawImage(a.img(backHover ? "interface/back-select.png" : "interface/back.png"), 0, 440);
    this.bubble?.draw(g, 604, 4);

    // Level list.
    g.font = "12px Helvetica, Arial, sans-serif";
    const levels = ctx.assets.levelIndex;
    for (let i = 0; i < levels.length; i++) {
      const l = levels[i];
      if (!l) continue;
      const y = LIST_Y + i * ROW_H;
      const done = this.passed.has(l.file);
      if (i === this.selected) {
        g.fillStyle = "#1a3a3a";
        g.fillRect(LIST_X - 8, y - 11, 280, ROW_H - 1);
      }
      g.fillStyle = done ? "#7CE87C" : i === this.selected ? "#FFFFFF" : TEXT_COLOR;
      g.fillText(`${done ? "✓ " : "   "}${l.name}`, LIST_X, y);
      g.fillStyle = "#5A8A8A";
      g.textAlign = "right";
      g.fillText(l.world, LIST_X + 268, y);
      g.textAlign = "left";
    }

    // Info card for the selected level.
    const entry = levels[this.selected];
    if (entry) {
      const hoverPlay = ctx.pointer.inside(CARD_X + 125, CARD_Y + 116, 64, 20);
      g.drawImage(
        a.img(hoverPlay ? "interface/level-stats-select.png" : "interface/level-stats.png"),
        CARD_X,
        CARD_Y
      );
      g.fillStyle = TEXT_COLOR;
      g.font = "9px Helvetica, Arial, sans-serif";
      g.fillText(entry.name, CARD_X + 74, CARD_Y + 47, 118);
      g.fillText(`${String(entry.quota).padStart(3, "0")} infotrons${entry.timed ? ", timed" : ""}`, CARD_X + 74, CARD_Y + 60, 118);
      g.fillText(this.passed.has(entry.file) ? "Passed" : "Undefeated", CARD_X + 74, CARD_Y + 72, 118);
      // Hint, word-wrapped like FlowWordsInRect (y 85..110 within the card).
      let hy = CARD_Y + 85;
      let line = "";
      for (const word of entry.hint.split(/\s+/)) {
        const trial = line ? `${line} ${word}` : word;
        if (g.measureText(trial).width > 118 && line) {
          g.fillText(line, CARD_X + 74, hy);
          hy += 9;
          line = word;
          if (hy > CARD_Y + 110) break;
        } else {
          line = trial;
        }
      }
      if (line && hy <= CARD_Y + 110) g.fillText(line, CARD_X + 74, hy);
      g.font = "11px Helvetica, Arial, sans-serif";
      g.fillStyle = "#5A8A8A";
      g.fillText("Enter / Play to begin — Esc for menu", CARD_X - 4, CARD_Y + 165);
    }
  }
}
