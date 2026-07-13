// The "bubbling Oliver" idle widget used on the menu, stat bars, and dialogs.
// Art: oliver-basics.png, a 608x32 strip of 32x32 cells; cell 0 = neutral,
// then for each of 4 axes a 2-frame bubble at offsets 32+64a and 64+64a.

const OLIVER_TICK_DELAY = 8;

export class BubbleOliver {
  private step = 0;
  private axis = 0;
  private lastTick = 0;

  constructor(private strip: HTMLImageElement) {}

  update(tick: number, rand: (min: number, max: number) => number): void {
    if (tick - this.lastTick < OLIVER_TICK_DELAY) return;
    this.lastTick = tick;
    this.step = (this.step + 1) % 4;
    if (this.step === 1) this.axis = rand(0, 3);
  }

  draw(g: CanvasRenderingContext2D, x: number, y: number): void {
    let srcX = 0;
    if (this.step === 1 || this.step === 3) srcX = 32 + 64 * this.axis;
    else if (this.step === 2) srcX = 64 + 64 * this.axis;
    g.drawImage(this.strip, srcX, 0, 32, 32, x, y, 32, 32);
  }
}
