import { Assets } from "./assets";
import { AudioEngine } from "./audio";
import { Input, Pointer } from "./input";
import type { GameContext, Screen } from "./screen";

export const WIDTH = 640;
export const HEIGHT = 480;
export const TICK_HZ = 60;

const canvas = document.getElementById("game") as HTMLCanvasElement;
const g = canvas.getContext("2d");
if (!g) throw new Error("no 2d context");
g.imageSmoothingEnabled = false;

function fitCanvas(): void {
  const scale = Math.max(1, Math.floor(Math.min(window.innerWidth / WIDTH, window.innerHeight / HEIGHT)));
  canvas.style.width = `${WIDTH * scale}px`;
  canvas.style.height = `${HEIGHT * scale}px`;
}
window.addEventListener("resize", fitCanvas);
fitCanvas();

const assets = new Assets();
const input = new Input();
const pointer = new Pointer();
const audio = new AudioEngine();
input.attach(window);
pointer.attach(canvas);
window.addEventListener("keydown", () => audio.ensureContext(), { once: true });
window.addEventListener("pointerdown", () => audio.ensureContext(), { once: true });

let current: Screen | null = null;
let pendingSwitch: Screen | null = null;
let switching = false;

const ctx: GameContext = {
  assets,
  input,
  pointer,
  audio,
  width: WIDTH,
  height: HEIGHT,
  switchScreen(next: Screen) {
    pendingSwitch = next;
  },
};

async function applySwitch(): Promise<void> {
  if (switching) return;
  switching = true;
  while (pendingSwitch) {
    const next = pendingSwitch;
    pendingSwitch = null;
    current?.exit?.(ctx);
    current = null;
    await next.enter(ctx);
    current = next;
  }
  switching = false;
}

const TICK_MS = 1000 / TICK_HZ;
let last = performance.now();
let acc = 0;

function frame(now: number): void {
  acc += Math.min(now - last, 250);
  last = now;
  while (acc >= TICK_MS) {
    acc -= TICK_MS;
    if (current) {
      input.beginTick();
      pointer.beginTick();
      current.tick(ctx);
    }
  }
  if (pendingSwitch) void applySwitch();
  if (current && g) current.render(ctx, g);
  requestAnimationFrame(frame);
}

async function boot(): Promise<void> {
  await assets.loadCore();
  const { TitleScreen } = await import("./ui/title");
  ctx.switchScreen(new TitleScreen());
  await applySwitch();
  requestAnimationFrame((t) => {
    last = t;
    requestAnimationFrame(frame);
  });
}

void boot();
