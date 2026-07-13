// Keyboard + pointer state with per-tick edge detection.

export type Action =
  | "up"
  | "down"
  | "left"
  | "right"
  | "special"
  | "pause"
  | "escape"
  | "confirm";

const BINDINGS: Record<string, Action> = {
  ArrowUp: "up",
  ArrowDown: "down",
  ArrowLeft: "left",
  ArrowRight: "right",
  KeyW: "up",
  KeyS: "down",
  KeyA: "left",
  KeyD: "right",
  Space: "special",
  ShiftLeft: "special",
  ShiftRight: "special",
  KeyP: "pause",
  Tab: "pause",
  Escape: "escape",
  Enter: "confirm",
};

export class Input {
  private held = new Set<Action>();
  private actionQueue: Action[] = [];
  private keyQueue: string[] = [];
  private pressedThisTick = new Set<Action>();
  /** KeyboardEvent.code values pressed since the last tick. */
  keysPressed: string[] = [];
  /** true if any key went down since the last tick */
  anyKeyPressed = false;

  attach(target: Window): void {
    target.addEventListener("keydown", (e) => {
      if (e.metaKey || e.ctrlKey) return; // don't eat browser shortcuts
      this.keyQueue.push(e.code);
      const action = BINDINGS[e.code];
      if (action) {
        e.preventDefault();
        if (!this.held.has(action)) {
          this.held.add(action);
          this.actionQueue.push(action);
        }
      }
    });
    target.addEventListener("keyup", (e) => {
      const action = BINDINGS[e.code];
      if (action) {
        e.preventDefault();
        this.held.delete(action);
      }
    });
    target.addEventListener("blur", () => this.held.clear());
  }

  beginTick(): void {
    this.pressedThisTick = new Set(this.actionQueue);
    this.actionQueue.length = 0;
    this.keysPressed = this.keyQueue.slice();
    this.keyQueue.length = 0;
    this.anyKeyPressed = this.keysPressed.length > 0;
  }

  isHeld(a: Action): boolean {
    return this.held.has(a);
  }

  wasPressed(a: Action): boolean {
    return this.pressedThisTick.has(a);
  }
}

export class Pointer {
  x = -1;
  y = -1;
  isDown = false;
  private downQueue = 0;
  private upQueue = 0;
  /** edges within the last tick */
  wentDown = false;
  wentUp = false;

  attach(canvas: HTMLCanvasElement): void {
    const toCanvas = (e: MouseEvent) => {
      const r = canvas.getBoundingClientRect();
      this.x = ((e.clientX - r.left) / r.width) * canvas.width;
      this.y = ((e.clientY - r.top) / r.height) * canvas.height;
    };
    canvas.addEventListener("pointermove", toCanvas);
    canvas.addEventListener("pointerdown", (e) => {
      toCanvas(e);
      this.isDown = true;
      this.downQueue += 1;
    });
    window.addEventListener("pointerup", (e) => {
      if (e.target === canvas) toCanvas(e as MouseEvent);
      this.isDown = false;
      this.upQueue += 1;
    });
  }

  beginTick(): void {
    this.wentDown = this.downQueue > 0;
    this.wentUp = this.upQueue > 0;
    this.downQueue = 0;
    this.upQueue = 0;
  }

  inside(x: number, y: number, w: number, h: number): boolean {
    return this.x >= x && this.y >= y && this.x < x + w && this.y < y + h;
  }
}
