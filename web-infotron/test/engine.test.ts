import { describe, it, expect } from "vitest";
import { InfotronGame, type InfotronLevelData, type FrameInput } from "../src/engine/game";
import { IC, classify, Kind } from "../src/engine/icons";

const CH: Record<string, number> = {
  "#": 750, // hardware
  ".": 129, // blank
  ":": 128, // base
  Z: 700, // zonk
  I: 850, // infotron
  E: 130, // exit
  C: 131, // chip
  R: 135, // red disk
  Y: 134, // yellow disk
  G: 133, // green disk
  T: 550, // terminal
  B: 300, // bug
  Q: 650, // quark
  S: 1002, // scissor facing right
  P: 132, // 4-way portal
  ">": 906, // one-way portal, enter moving right (906 % 8 == 2)
};

function level(rows: string[], quota = 99): InfotronLevelData {
  const sizeY = rows.length;
  const sizeX = (rows[0] as string).length;
  const board: number[] = [];
  let murphy: [number, number] = [1, 1];
  rows.forEach((row, y) =>
    [...row].forEach((ch, x) => {
      if (ch === "M") {
        murphy = [x, y];
        board.push(129);
      } else {
        const v = CH[ch];
        if (v === undefined) throw new Error(`bad char ${ch}`);
        board.push(v);
      }
    })
  );
  return { id: 1, name: "test", sizeX, sizeY, murphy, quota, board };
}

const IDLE: FrameInput = { up: false, right: false, down: false, left: false, space: false };
const dir = (d: Partial<FrameInput>): FrameInput => ({ ...IDLE, ...d });
const rngMin = (min: number) => min;

function run(g: InfotronGame, frames: number, input: FrameInput = IDLE): void {
  for (let i = 0; i < frames; i++) g.frame(input);
}

describe("classification", () => {
  it("maps the icon legend to kinds", () => {
    expect(classify(128)).toBe(Kind.Base);
    expect(classify(129)).toBe(Kind.Blank);
    expect(classify(703)).toBe(Kind.Zonk);
    expect(classify(853)).toBe(Kind.Infotron);
    expect(classify(1105)).toBe(Kind.Chip); // dropped red disk = chip
    expect(classify(1010)).toBe(Kind.Inert); // scissor medium frames: the shipped gap
    expect(classify(999)).toBe(Kind.Scissor);
    expect(classify(660)).toBe(Kind.Quark);
    expect(classify(202)).toBe(Kind.Murphy);
    expect(classify(1205)).toBe(Kind.Murphy); // win anim is in the kill range
  });
});

describe("falling", () => {
  it("a zonk falls one cell per frame and thuds", () => {
    const sounds: number[] = [];
    const g = new InfotronGame(level(["#####", "#Z..#", "#...#", "#...#", "#:M:#", "#####"]), {
      rand: rngMin,
      sound: (id) => sounds.push(id),
    });
    // support was never there: initial SuicideChore starts the fall
    run(g, 2);
    expect(g.data(1, 2)).toBe(700); // fell one cell after chore warmup
    run(g, 6);
    expect(g.data(1, 3)).toBe(700); // resting on the base floor
    expect(sounds).toContain(129); // thud
  });

  it("a zonk rolls off a chip", () => {
    const g = new InfotronGame(level(["#####", "#.Z.#", "#.C.#", "#...#", "#M..#", "#####"]), {
      rand: rngMin,
    });
    run(g, 20);
    // first roll try uses the initial roll direction (Right)
    const settled = [g.data(1, 4), g.data(3, 4)];
    expect(settled).toContain(700);
    expect(g.data(2, 1)).toBe(129);
  });

  it("a falling zonk crushes and kills Murphy", () => {
    const sounds: number[] = [];
    const g = new InfotronGame(level(["###", "#Z#", "#.#", "#M#", "###"]), {
      rand: rngMin,
      sound: (id) => sounds.push(id),
    });
    run(g, 10);
    expect(sounds).toContain(137); // died
    expect(g.dying).toBe(true);
  });

  it("a green disk explodes on landing", () => {
    const sounds: number[] = [];
    const g = new InfotronGame(level(["#####", "#G..#", "#...#", "#:::#", "#M::#", "#####"]), {
      rand: rngMin,
      sound: (id) => sounds.push(id),
    });
    run(g, 12);
    expect(sounds).toContain(131); // explosion
  });
});

describe("Murphy", () => {
  it("walks one tile per frame, eating base silently", () => {
    const sounds: number[] = [];
    const g = new InfotronGame(level(["#####", "#M::#", "#####"]), {
      rand: rngMin,
      sound: (id) => sounds.push(id),
    });
    g.frame(dir({ right: true }));
    expect(g.murphyX).toBe(2);
    expect(g.data(1, 1)).toBe(129);
    g.frame(dir({ right: true }));
    expect(g.murphyX).toBe(3);
    expect(sounds.length).toBe(0); // base is silent
  });

  it("collects infotrons and opens the way to winning", () => {
    const sounds: number[] = [];
    const g = new InfotronGame(level(["#####", "#MIE#", "#####"]), {
      rand: rngMin,
      sound: (id) => sounds.push(id),
    });
    (g as unknown as { numInfos: number }).numInfos = 1;
    g.frame(dir({ right: true }));
    expect(g.numInfos).toBe(0);
    expect(sounds).toContain(135); // last infotron
    g.frame(dir({ right: true })); // walk into the exit
    expect(g.wonLevel).toBe(true);
    expect(g.frozen).toBe(true);
    run(g, 60); // 4 seconds of end-freeze
    expect(g.gameOver).toBe(true);
  });

  it("cannot enter the exit before quota", () => {
    const g = new InfotronGame(level(["####", "#ME#", "####"]), { rand: rngMin });
    g.frame(dir({ right: true }));
    expect(g.murphyX).toBe(1);
    expect(g.wonLevel).toBe(false);
  });

  it("snaps an adjacent infotron without moving", () => {
    const g = new InfotronGame(level(["#####", "#MI:#", "#####"]), { rand: rngMin });
    (g as unknown as { numInfos: number }).numInfos = 5;
    g.frame(dir({ right: true, space: true }));
    expect(g.murphyX).toBe(1); // did not move
    expect(g.data(2, 1)).toBe(129);
    expect(g.numInfos).toBe(4);
  });

  it("pushes a zonk horizontally but not vertically", () => {
    const g = new InfotronGame(level(["#####", "#MZ.#", "#####"]), { rand: rngMin });
    g.frame(dir({ right: true }));
    expect(g.murphyX).toBe(2);
    expect(classify(g.data(3, 1))).toBe(Kind.Zonk);

    const g2 = new InfotronGame(level(["###", "#Z#", "#M#", "###"]), { rand: rngMin });
    g2.frame(dir({ up: true }));
    expect(g2.murphyY).toBe(2); // blocked
  });

  it("collecting a red disk and dropping it explodes after 2s", () => {
    const sounds: number[] = [];
    const g = new InfotronGame(level(["#####", "#MR:#", "#::.#", "#####"]), {
      rand: rngMin,
      sound: (id) => sounds.push(id),
    });
    g.frame(dir({ right: true })); // collect
    expect(g.numBombs).toBe(1);
    // hold space alone >= 90 ticks (18 frames), then release
    run(g, 20, dir({ space: true }));
    expect(sounds).toContain(134); // disk ready
    g.frame(IDLE); // release -> drop armed at current cell
    expect(g.numBombs).toBe(0);
    g.frame(dir({ right: true })); // walk off; cell becomes the dropped disk
    expect(g.data(2, 1)).toBe(1105);
    run(g, 30); // 2 seconds
    expect(sounds).toContain(131); // it blew up
  });

  it("traverses a one-way portal in the right direction only", () => {
    const g = new InfotronGame(level(["#####", "#M>.#", "#####"]), { rand: rngMin });
    g.frame(dir({ right: true })); // step onto portal (transit)
    g.frame(dir({ right: true })); // forced out the far side
    expect(g.murphyX).toBe(3);
    expect(g.data(2, 1)).toBe(906); // portal survives

    // now try to come back left: blocked
    g.frame(dir({ left: true }));
    expect(g.murphyX).toBe(3);
  });

  it("terminal detonates all yellow disks", () => {
    const sounds: number[] = [];
    const g = new InfotronGame(level(["######", "#MT.Y#", "#::::#", "######"]), {
      rand: rngMin,
      sound: (id) => sounds.push(id),
    });
    g.frame(dir({ right: true })); // bump the terminal
    expect(g.murphyX).toBe(1); // stays put
    run(g, 15);
    expect(sounds).toContain(131);
    expect(g.data(4, 1)).toBe(129); // disk gone
  });

  it("walking into an active bug kills at the bug's cell", () => {
    const sounds: number[] = [];
    const g = new InfotronGame(level(["####", "#MB#", "####"]), {
      rand: (min) => min, // shortest dormant wait: 121 ticks
      sound: (id) => sounds.push(id),
    });
    // dormant bug is plain base: walking over it eats it... so wait for a zap.
    run(g, 27); // ~130 ticks: dormant wait (121) elapses, first zap frame draws
    const v = g.data(2, 1);
    expect(v).toBeGreaterThanOrEqual(300); // zapping
    g.frame(dir({ right: true }));
    expect(g.dying).toBe(true);
    expect(sounds).toContain(137);
  });
});

describe("enemies and explosions", () => {
  it("an exploded quark leaves a 3x3 of infotrons", () => {
    const g = new InfotronGame(level(["#####", "#Z:.#", "#:::#", "#Q::#", "#:::#", "#M::#", "######".slice(0, 5)]), {
      rand: rngMin,
    });
    // drop the zonk onto the quark: snap the base under the zonk... simpler:
    // detonate the quark directly.
    g.quarkExplode(1, 3);
    run(g, 15);
    let infotrons = 0;
    for (let y = 2; y <= 4; y++) {
      for (let x = 0; x <= 2; x++) {
        if (g.data(x, y) === 850) infotrons++;
      }
    }
    expect(infotrons).toBeGreaterThanOrEqual(4); // borders/hardware block some
  });

  it("a quark roams empty space without leaving debris", () => {
    const g = new InfotronGame(level(["######", "#....#", "#.Q..#", "#....#", "#M...#", "######"]), {
      rand: rngMin,
    });
    run(g, 40);
    let quarks = 0;
    let junk = 0;
    for (let y = 1; y <= 3; y++) {
      for (let x = 1; x <= 4; x++) {
        const k = classify(g.data(x, y));
        if (k === Kind.Quark) quarks++;
        else if (k !== Kind.Blank) junk++;
      }
    }
    expect(quarks).toBe(1);
    expect(junk).toBe(0);
  });

  it("walking into a scissor kills Murphy", () => {
    const g = new InfotronGame(level(["#####", "#M.S#", "#####"]), { rand: rngMin });
    run(g, 4, dir({ right: true }));
    expect(g.dying).toBe(true);
  });
});
