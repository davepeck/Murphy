# Infotron! — web port

The 1994 Macintosh shareware game, ported to TypeScript at its original
640×480. The engine is a faithful port of the final "NO OOP" Think Pascal
sources in `../legacy/infotron/src/` (`main.p`, `everything.p`, `TheChores.p`,
`TheGridPieces.p`, `dataIntf.p`).

## Run it

```sh
pnpm install
pnpm assets   # one-time: extracts everything from ../legacy/infotron/infotron-11.hqx
pnpm dev
```

`pnpm test` runs the engine tests; `pnpm build` makes a production build.

## How the original works (and this port too)

The board is a grid of integers where each cell value **is** the icl8 icon
resource ID (128 = base, 129 = empty, 700 = zonk, 850 = infotron, …); all game
logic is numeric range checks over those values. There are no sprites — pieces
move a whole tile at a time at ~12 game frames/second (one frame per 5 Mac
ticks), driven by per-cell "chores" with additions/removals deferred one
frame. Blanking a cell reactively triggers the gravity checks around it
(`SetData`). Murphy walks/eats/pushes once per frame, enemies act every other
frame, roll direction tie-breaks are a 50/50 coin flip per attempt, quarks
explode into a 3×3 block of infotrons, and red disks arm on *releasing* the
space bar after a 1.5-second hold, detonating 2 seconds later.

Shipped-engine quirks are preserved where they shaped gameplay — e.g. the
scissor classifier gap (scissors showing their "medium/open" animation frames
are briefly immune to explosions and crushes), walking into a *falling* zonk
from the side is fatal, the exit is destructible (blow it up and the level is
unwinnable), and hardware icons 758–760 aren't in the classifier's hardware
range so quark explosions can transmute those walls into infotrons.

## Assets

`scripts/build-assets.mjs` extracts the shipped v1.1 archive
(`infotron-11.hqx`, BinHex → resource forks) and decodes everything:

- **icons/** — every icl8 (8-bit Mac system palette) + its ICN# mask → RGBA PNG
- **levels/** — all 51 `lüVl` CompressedBoard resources → JSON (RLE-decoded,
  byte-exactly, including the original decoder's skip-one-int quirk)
- **sounds/**, **music/** — every `snd ` resource → WAV (including the one
  stereo extSH sound, the explosion)
- **pictures/** — all 28 PICTs → PNG via a purpose-written QuickDraw PICT v2
  decoder (ImageMagick silently renders 17 of them as solid black)

Fun shipped-audio facts this port preserves: the music rotation picks a random
track in 1000–1002 every 120 seconds, but only two tracks were ever shipped —
so a third of rotations are silence. There is no win jingle (sound 130 was
never shipped); v1.1 ships an unreferenced "endLevel" sound (151) which this
port plays on winning.

Reverse-engineering specs live in `docs/original-specs/`.

## Adapted for the web

Arrow keys/WASD move, Space = snap/drop, P or Tab = pause (the original used
Caps Lock!), Esc = suicide. Progress and audio settings persist in
localStorage. The title screen has a simple level selector in place of the
original's Picker dialog.
