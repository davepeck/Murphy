# Infotron (1994 Mac, shipped) — Graphics, Animation & Audio Mapping Spec

Purpose: the rendering/audio bridge between the rules spec's icon-ID legend
(`infotron-reference.md` §1.2) and what the TypeScript port must actually draw and play.
Everything here was verified against the final ("NO OOP") sources — `main.p`, `everything.p`,
`TheChores.p`, `TheGridPieces.p`, `dataIntf.p`, `InfoSounds.p` (unit QIST), `InfoWindows.p`,
`ColorOffScreen.p`/`RevisedColorOff.p`, `myRGBColors.p` — **and** against the shipped v1.1
resource forks (`Info Datafiles/Infotron Pieces|Sounds|Music|Pictures|Levels`), including a
full decode of all 51 shipped levels to determine which icon IDs actually occur in level data.

Cross-references like "§4.3" refer to `infotron-reference.md`. Corrections to that spec are
collected in §12 here.

---

## 1. How drawing works (the answer to "alpha or opaque?")

**Everything on the board is an opaque 32×32 tile blit. No transparency is used in-game.
The board background never shows through anything; "empty" is itself an opaque solid-black
tile (icl8 129, resource name `black`, all 1024 pixels = index 255 = black).**

Mechanism (from `everything.p`):

1. **Startup icon sheet** (`SetUpIcons`): one 8-bit GWorld `gIconOff` of **482×386 px**.
   For every ID 128..1211, if an `icl8` resource exists it is plotted with the *real*
   Icon Utilities `PlotIconID` trap (`$ABC9`, i.e. WITH its ICN# mask) into the sheet at
   15 icons per row, cell (col,row) → rect (1+32·col, 1+32·row); its source rect is recorded
   in `gIconLocs[id]`. Missing IDs get the invalid rect (-1,-1,-1,-1).
   - The ICN# mask matters only at sheet-build time. **Verified from the shipped v1.1
     Pieces file: all 234 ICN# masks are 100% solid except IDs 136/137 (Finder icons,
     not gameplay).** So even at build time nothing is transparent.
2. **In-game `PlotIconID`** (the game's own function shadowing the trap) is just
   `CopyBits(gIconOff → target, gIconLocs[id], destRect, srcCopy)` — a plain opaque copy.
   `Align`/`Transform` args are ignored. A missing ID (invalid rect) copies nothing:
   the destination cell keeps its previous pixels.
3. GWorlds are 8-bit (`NewGWorld(…, 8, …)`); the game requires a 640×480+ monitor in
   exactly 256 colors and uses the standard system CLUT for icons (a custom palette is set
   only around the intro PICTs, then `RestoreDeviceClut`/`NormalPal` restores the standard
   one before `InitalizeGraphics`).

**Port decision: draw icons as opaque 32×32 tiles (decode icl8 with the standard Mac 8-bit
system palette; ignore ICN# masks). The window/board background is solid black
(`FillRect(gMainWind, black)` at startup; `RedrawBoard` fills black when no level).**

Icon-sheet overflow quirk (original bug, do NOT reproduce): the sheet holds 15×12 = 180
icons but 183 icl8s exist. The last three in ID order — **1209, 1210, 1211** (win-anim
`goodbye10..12`) — land on a 13th row at y=385 in a 386-px-tall GWorld, so only 1 px of them
is real; the original blits undefined memory for those frames (including the resting 1211
tile shown during the 4-second win freeze). The port should just render them correctly.

---

## 2. The offscreen board buffer, viewport geometry

- Offscreen board GWorld `myOff`: rect **(-32,-32) to (672, 451)** (i.e. 480−61+32 = 451).
- Visible playfield on screen: **640×419 px** at window offset (`gMoveH`,`gMoveV`)
  (the 640×480 layout is centered on larger monitors, surrounded by black + a 3-D gray
  frame, `BigMonUpdate`). Status bar occupies y 419..480 (61 px).
- A cell (bx,by) is drawn at `((bx−upperLeft.h)·32 − 16, (by−upperLeft.v)·32 − 16)`
  in `myOff`. **The whole 21×15 tile window is offset by (−16,−16)**, and the screen blit
  takes `myOff` rect (0,0,640,419). Net effect on screen:
  - Columns: half of column 0 (right 16 px), columns 1–19 full, half of column 20 (left 16 px)
    → 19 full + 2 half columns.
  - Rows: bottom 16 px of row 0, rows 1–12 full, top 19 px of row 13; row 14 is drawn into
    the buffer but never visible.
  So Murphy's viewport shows fractional tiles at all four edges. A faithful port draws the
  21×15 tile grid at offset (−16,−16) clipped to 640×419.

Drawing primitives (`everything.p`):

| Routine | What it does |
|---|---|
| `Draw1(x,y)` | plot cell's *current data* icon into `myOff` (no screen blit); no-op if outside the 21×15 window |
| `DrawOne(x,y)` | `Draw1` + `RedrawBoard` (immediate full blit) |
| `DrawOffset(x,y,offX,offY,icn)` | plot arbitrary icon at cell position + pixel offset (used only for the portal half-in/half-out effect) |
| `DrawOffscreen` | repaint all 21×15 cells from data + blit (level load only; **remaps data 300→128** so bugs start hidden) |
| `RedrawBoard` | blit `myOff`(0,0,640,419) → window at (gMoveH,gMoveV); fills black if no level |
| `MoveUp/Down/Left/Right` | scroll: self-CopyBits `myOff` by ±32 px, plot the 21 (or 15) newly exposed edge cells via `getData`, `upperLeft` ±1, `RedrawBoard` |

There is **no smooth pixel scrolling** — the viewport jumps a whole tile within the frame
where Murphy crosses a scroll threshold (§6.2 of the rules spec).

---

## 3. Screen refresh: exactly what repaints when

Per game frame (`HandlePlaying`, ~12 fps):

1. `DoAllChores` runs all chores. Chores draw by writing cell data (`SetData`) + `Draw1`
   (offscreen only). At the end: **`if iuLen > 0 then RedrawBoard`** — one full-board blit
   per frame *iff at least one chore exists*. A fully static board with no chores never
   re-blits (safe: nothing changed).
2. Murphy movement (`MurphyMove` walk path): `Draw1(old)`, `Draw1(new)`, then either a
   scroll routine (which blits) or `RedrawBoard` directly. So a Murphy move causes its own
   blit in addition to the chore blit.
3. Immediate mid-frame blits via `DrawOne` (each = a full 640×419 blit, executed
   back-to-back with **no tick delay**, i.e. sub-frame "flash" animations):
   - snap animations (§5.4 below), win animation 1200–1211, corpse icons 145/147,
     idle-standing icon 198 (`NormalMurphy`), sleep-sequence icons, `ExplodeTWO`'s clearing
     of Murphy's cell.
4. Status bar: `DrawTBoxes(FALSE)` runs **every** `Process1Event` (repaints only h/m/s
   digits whose value changed); `DrawInfoStat`/`DrawDiskStat` repaint immediately on
   pickup/drop events; full `drawStatus` (PICT 129 background + all fields) only on level
   load / win / OS update events / status changes.
5. OS update events → `RedrawBoard` + `DrawStatus` (+ `BigMonUpdate` frame).

Port guidance: a modern always-redraw-every-frame renderer reproduces everything visible.
The only behavior to emulate deliberately is that the "instant" `DrawOne` sequences (snap,
win) are logically instantaneous — the cell's final value is what matters; show the
animation cosmetically for 1 frame or spread it if desired.

---

## 4. Complete icon-ID catalog as actually drawn

Legend for "shipped": `art` = icl8 exists in v1.1 Pieces; `lvl` = occurs in the 51 shipped
levels' cell data; `rt` = written at runtime by the engine.

| ID(s) | Meaning | Shipped | Notes |
|---|---|---|---|
| 128 | base/circuit | art, lvl, rt | default fill; bug cells become 128 |
| 129 | empty (solid black) | art, lvl, rt | |
| 130 | exit | art, lvl | |
| 131 | chip | art, lvl | name `chip` |
| 1100/1102/1104/1106 | chip edge variants (top/rt/bot/lt) | art, lvl | 1101/1103 have **no art** and never occur |
| 1105 | dropped red disk (classifies as chip) | art, rt | never in level data; written on disk drop |
| 132 | 4-way portal | art, lvl | |
| 900/902/904/906 | 1-way portals (enter moving Down/Left/Up/Right = `id mod 8`) | art, lvl | names porttop/portrt/portbot/portlt; **only these four one-way IDs exist** — no diagonal/other ports shipped |
| 950 / 952 | 2-way vertical / horizontal portal | art, lvl | |
| 133 / 134 / 135 | green / yellow / red disk | art, lvl | |
| 139–145,147 | Murphy mad/idea/asleep (L: 139/140/141, R: 142/143/144), corpses 145 & 147 | art, rt | 145 `MurphyScaredShitless` = KillMurphy corpse; 147 = ExplodeTWO (bug) corpse |
| 194/195/199/200 | Murphy snap poses (look right/left/up/down) | art, rt | |
| 196/197 | Murphy push pose (left/right) | art, rt | |
| 198 | Murphy standing | art, lvl(!), rt | **level data marks Murphy's start cell with 198** (one per level) |
| 201–206 | Murphy walk (L1/R1/L2/R2/L3/R3 = big/med/small mouth) | art, rt | |
| 214 | `murphysad` | art | only drawn in the Help/About dialogs, never on the board; inside kill range |
| 146,148–193,207–213,215 | (kill-range gaps) | no icl8 | can never render; never occur |
| 250 | `blackButNothing` | art | debug tile; all writes commented out — unused |
| 300–305 | bug zap frames | art; lvl uses only 300 | anim 300..305 then 128 |
| 350–358 | infotron snap/shrink (`shrinkinfo1..9`) | art, rt | snap draws 350..357; 358 is used as quark-explosion seed frame |
| 400–407 | red-disk snap (`…8` art) | art, rt | snap draws 400..406; 407 exists but only drawn in Help dialog |
| 450–457 | base snap | art, rt | snap draws 450..456; 457 Help-only |
| 499–511 | Murphy yawn (`murphyyawn1..12` = 500..511) | art, rt | 499 exists but is **never displayed** (loop index only) |
| 550–561 | terminal (`comp1..12`) | art; lvl uses only 550 | anim cycles 550..560 (11 frames); 561 renderable but only if hand-placed |
| 600–610 | explosion (`expl1..11`) | art, rt | anim writes 600..609 then 129; **610 never drawn** (only in ≤610 checks) |
| 650–657 | quark spin (`elec1..8`) | art; lvl uses only 650 | 658 has art, never drawn; classifier accepts 650–666 |
| 700–707 | zonk + 8-step rotation art | art; lvl uses only 700 | engine cycles only 700–703; 704–707 render-capable but unused |
| 750–757 | hardware (`indblock1..8`) | art, lvl | |
| **758–760** | hardware look-alikes | **art, lvl** | **classifier gap**: not in 750–757 range → generic piece → indestructible **but Murphy can walk onto/consume them** (see §12.4) |
| 850–853 | infotron + roll frames | art; lvl uses only 850 | |
| 1000–1007 | scissors small, per direction (`id mod 8`) | art; lvl uses 1000/1002/1004/1006 | diagonal art exists (1001/1003/1005/1007) but shipped levels use cardinals only |
| 1010/1012/1014/1016 | scissors "open", cardinal | art, rt | odd/diagonal open frames have **no art** |
| 1020/1022/1024/1026 | scissors "medium", cardinal | art, rt | |
| 1150/1152/1154/1156 | scissors turn frames, cardinal | art | referenced only by dead code (old CMoveChore `MakeSciMove`) and the Help dialog — **never drawn on the board by the final engine** |
| 998/999, 1148–1160 others | scissors legend tails | no art | in classifier ranges but unusable (and would break the mover, §6.3) |
| 1050–1057 | hardware wall edges (top/toprt/rt/…/toplt) | art, lvl | |
| 1200–1211 | win animation (`goodbye1..12`) | art, rt | |

Missing-ID behavior: any cell value with no icl8 (e.g. 138, 998, 1148) draws nothing —
the cell keeps stale pixels. Never happens with shipped levels + final engine except via
the §1 sheet-overflow bug.

---

## 5. Animation sequences, frame by frame

All chore-driven animations advance **one frame per game frame** (~12 fps, `delayBetween=0`).
Each animation frame is *written into the cell data* (`SetData`) then `Draw1`'d — data and
graphic are the same value, which is why other logic reacts to animation frames.

### 5.1 Zonk roll / fall / push (verified §4.3–4.7 of rules spec — correct)

- Roll right (CMoveChore, boulder, dir<kDown): frames **[700,701,702,703,700]**, occupancy
  transfers at frame 3. Roll left (dir>kDown): **[700,703,702,701,700]**, frame 3.
  (A straight-down [700,700] variant exists but is unreachable.)
- Push (CZonk.Push): the zonk's graphic rotates **one step per push-frame**: right push
  `+1` in 700..703 with 704→700 wraparound; left push `−1` with 699→703.
- Pushed-zonk fall (`CFallRollChore`): continues rotating one step per cell fallen (same
  direction as the push); on landing the graphic is normalized to **700** before roll checks.
- Post-push settle (`CRollAfterChore`, installed 3 frames after push): if the cell shows
  701–703 it first rotates one step per frame (push direction) until it reads 700, then
  does fall/roll checks.
- Plain fall (`CFallChore`): no graphic change while falling (stays 700).
- Landing thud sound 129 (see §8).

### 5.2 Infotron roll

`CMoveChore(infotron)`: frames **[850,851,852,853,850]**, move at frame 3, **same order for
both directions**. No rotation while falling; no rotation on push (infotrons can't be pushed).

### 5.3 Murphy — how his icon is chosen each frame

Constants: `kBase=201`, `kLft=0`, `kRt=1`; phases `kBig=0, kMed=2, kSml=4`.
Walk icon = `201 + (0 left | 1 right) + phase` → left 201/203/205, right 202/204/206.

Per direction-key frame (`MurphyMove`, walk path, WalkOnto succeeded):
1. Write walk icon at the **destination** cell: L/R set `lastFacingLR`; U/D reuse
   `lastFacingLR`.
2. Advance mouth phase *after* drawing: from big or small → med; from med → alternate
   big/small. Cycle as drawn: **sml, med, big, med, sml, …** (start of level: first step
   draws sml because `gPhase` inits to kSml; idle resets make the next step med).
3. If the destination *was* a zonk (700–707), green (133) or yellow (134) disk (i.e. a
   successful push), overwrite with push pose **196 (facing left) / 197 (facing right)**
   and play sound 141. (Yellow pushed up/down still shows 196/197 by last horizontal facing.)
4. Old cell := `whatToBlank` (129 normally; the portal icon after a portal transit; 1105
   after a red-disk drop).

Snap (space+dir at bug/infotron/red disk/base): Murphy's own cell gets pose
**195 (left) / 199 (up) / 194 (right) / 200 (down)** via `DrawOne`; the pose persists until
the idle reset or next move. Target cell plays, via back-to-back `SetData`+`DrawOne` with
no delay (all within one game frame):
- infotron: **350→357** (8 frames), then `GotInfotron`, cell := 129
- red disk: **400→406** (7 frames), then `GotRedDisk`, cell := 129
- base: **450→456** (7 frames), cell := 129
(358/407/457 exist in art but are not part of the snap flashes.)

Idle: if `tickCount − lastMove > 15` ticks (and not sleeping/dying), cell := **198**
(standing), phase resets (next move draws med, then big).

Sleep (no input for 45 s → `StartSleep`, then `DoSleepStuff` each frame):
- Yawn 1: draws **501..511**, one icon per game frame (500 skipped the first time because
  the phase variable increments before the first draw; 499 is never displayed).
- 30 s pause, yawn 2: **500..511**; 30 s pause, yawn 3: **500..511**.
- 2 s after yawn 3: **mad** = 139 (facing left) / 142 (right).
- 3 s later: **idea** = 140/143. 45 ticks later: **asleep** = 141/144, stays until any
  direction key (all sleep icons are lethal cell values — kill range).

Death: `KillMurphy` writes corpse **145** at Murphy's cell (+`DrawOne`, then a blocking
9-tick delay); `ExplodeTWO` clears Murphy's cell to 129 and writes corpse **147** at the
bug's cell (blocking 12 ticks). The 3×3 explosion chore then overwrites the corpse cell
with 600..609→129 over the following 11 frames.

Win (`CExit.WalkOnto` with quota met): writes **1200→1211** at Murphy's current cell via
12 immediate `DrawOne`s (sub-frame flash); cell stays **1211** during the 4-second
end-of-level freeze. No sound is played by the final code (see §12.1).

Portal transit (`CoolPortals` + `DoLastWasPortal`): on the entry frame the walk icon is
written at the portal cell, captured as `murphPic`, then the portal cell data is restored
to the portal icon and `murphPic` is drawn twice with 16-px offsets: at the entry-side cell
shifted 16 px *toward* the portal, and at the far-side cell shifted 16 px *back toward* the
portal (offsets per direction: up = −16/+16 y; down = +16/−16 y; left = −16/+16 x;
right = +16/−16 x). Next frame `DoLastWasPortal` repaints the four neighbors of the portal
cell (erasing both half-Murphies) and replays the held keys to push Murphy out the far side.

Cheat: player name `NoRtOfNi42` enables `MajikMurphy` noclip on keys I/J/L/M (icon 198,
swaps cell data under him via `whatToBlank`) and unlocks all levels.

### 5.4 Bug (verified §8 — correct)

`CContAnimChore` with random dormant delay `ARandom(240)+120` ticks (121–360). Active
animation **[300,301,302,303,304,305,128]** — 7 frames at one per game frame, ending as
plain base 128. Dormant bugs are literally 128 (invisible); the initial full-board draw
also remaps any leftover 300 data → 128. Zap sound 140 plays at cycle start only if
Murphy is within Manhattan distance < 4 (volume scaled, §8).

### 5.5 Terminal

Continuous `CContAnimChore(cycleDelay=0)` over **550..560** (11 frames, `termAnim`), i.e.
~0.9 s per loop, no pause between loops. 561 exists in art but is not in the cycle.

### 5.6 Explosion (verified §5.1 — correct)

`CExplChore`/`CAnimChore` anim **[600,601,…,609,129]**, 11 frames, one per game frame;
frame 1 kills Murphy if centered on him, frame 3 plays sound 131 and damages the 8
neighbors, frame 11 writes 129. Plain `Destroy` on destructible neighbors runs the same
11-frame animation without chaining. Icon 610 exists but is never drawn.

### 5.7 Quark spin, move, and explosion

- `CQuarkMover`: keeps `lastAnim` (init = cell's level icon, i.e. 650), increments it every
  Perform (both movement and anim-only passes) with wrap 657→650 — a continuous spin at
  ~6 fps per full loop step (one icon per game frame, one action per 2 frames).
  Anim-only pass rewrites the *same cell* (see the momentary-129 quirk, §12.11).
  Mover gate: cell must read 650..657 or the mover removes itself.
- Quark explosion (`CQuark.Explode`): sound 132; own cell := 358; every non-hardware cell
  of the 3×3 gets `Explode` (chain) + anim **[358,357,356,355,354,353,352,351,850]**
  (9 frames — `shrinkinfo` played backwards, ending as an infotron 850) + a settle chore
  9 frames later. The anim chore aborts if a normal explosion (600..610) overwrites the cell.

### 5.8 Scissors (final `CScissorMover`) — sizes, turns, exact icon arithmetic

State: `currentDir` ∈ {0,2,4,6} (init `levelIcon mod 8`), `currentSize` ∈ {0 small,
10 open, 20 medium}, `afterMed` alternator, `goingValue` ∈ {+1,−1}, `turnAround`,
`forceNextMove`. Mover gate: cell must read **1000..1026** or the mover removes itself
(cell 600..610 → it explodes itself instead).

Every Perform (movement passes alternate with anim-only passes) first steps the size cycle:
small→**medium**→open→**medium**→small→… (if size=20 then size:=afterMed, afterMed:=10−afterMed
else size:=20).

- **Anim-only pass**: draw `1000 + lastAnim(=dir) + currentSize`
  → small 1000+dir, open 1010+dir, medium 1020+dir. Shipped art exists only for cardinal
  dirs (1000/2/4/6, 1010/12/14/16, 1020/22/24/26) — sufficient, since turns are ±90°.
- **Movement pass** (`DoMovement`, L→S→R→turn-in-place preference, §7.1 of rules spec):
  - Straight move (newDir=currentDir): old cell := 129, new cell := `1000+dir+currentSize`.
  - Deciding to turn toward a free left/right cell: stays put this pass showing
    `1000 + currentDir + goingValue` (goingValue −1 for left turn, +1 for right;
    999 clamps to 1007) — i.e. the *small frame of the adjacent 45° direction* as a turning
    pose — resets size to small/afterMed=10, sets `forceNextMove`; the **next** movement
    pass moves straight ahead without re-validating (§18.3 quirk).
  - Blocked all around: same 45° turning pose, sets `turnAround`; next movement pass
    writes `1000 + newDir` (turn completed in place; two of these = 180°).
  - The dedicated turn art 1150/1152/1154/1156 is **never used** by the final engine
    (only by dead OOP code and the Help dialog).

Because scissors only ever change direction by ±2 (90°), directions stay cardinal, and all
displayed IDs ∈ {1000,1002,1004,1006} ∪ {1001,1003,1005,1007 (turn poses)} ∪
{1010,1012,1014,1016} ∪ {1020,1022,1024,1026}.

### 5.9 Dead animation code (do not port)

`CMoveChore.MakeArray` contains murphy ([205,203,201,203,205]/[206,204,202,204,206]),
scissor (uses 1148–1160), and quark move sequences — the final engine only ever
constructs CMoveChores for **boulder** and **infotron** (verified: the only `IMoveChore`
call sites are `CZonk.Roll` and `CInfotron.Roll`). Murphy, scissors and quarks are drawn
by their own code paths described above.

---

## 6. Level data ↔ icons (from decoding all 51 shipped levels)

IDs that actually occur in shipped level cell data:
128, 129, 130, 131, 132, 133, 134, 135, **198** (Murphy start marker, 1 per level),
300 (bugs), 550 (terminals), 650 (quarks), 700 (zonks), 750–760 (hardware **including
758–760**, see §12.4), 850 (infotrons), 900/902/904/906/950/952 (ports),
1000/1002/1004/1006 (scissors), 1050–1057, 1100/1102/1104/1106 (chip edges).
Levels are resources of type `l\x9FVl` (`'lüVl'` in MacRoman; players are `bl\x9FG`),
IDs 301–351, loaded by *name*; level N ↔ resource 301+N−1.

Notably absent from level data: 704–707, 851–853, 561, 658, 1010+, 1105, snap/explosion
frames — all runtime-only.

---

## 7. Status bar / HUD rendering

Background: PICT **129** drawn at (1+gMoveH, 419+gMoveV); everything drawn over black.
Fields (all coordinates +gMoveH/+gMoveV; `drawInteger(n, digits)` pads with leading zeros;
text is the default system font):

| Field | Position (x, baseline y) | Color |
|---|---|---|
| Player name | centered in x 191..334, y 443 | red (#FF0E0E) |
| Level name | centered in x 191..409, y 466 (text shrinks until ≤130 px wide) | green (#0BCF0B) |
| Level number (3 digits) | x 427, y 466 | green |
| Time h/m/s (2 digits each) | x 359 / 397 / 435, y 442 | blue (#2426FF) |
| Infotrons left (3 digits) | x 495, y 442 | **green when 0, else red** |
| Red disks (3 digits) | x 495, y 466 | **blue when 0, else red** |

Redraw triggers: time digits every frame but only when the value changed
(`DrawTBoxes(FALSE)`); infotron/disk counters immediately on change; full bar on level
load/win/update. The clock shows the current session's elapsed time (`GetGameTime`);
while paused (or not playing) it shows the player's saved cumulative time `gPlayer.time`
instead (original quirk — the displayed number can jump on pause).
RGB values: red (65535,3500,3500), green (2845,53247,2845), blue (9191,9716,65535),
yellow (65535,45000,0) — yellow is used only in the level picker and error text.

---

## 8. Sound effects — verified trigger table

Player: unit QIST (`InfoSounds.p`). 4 mono sample channels for FX (a 5th reserved for
music). `PlaySound` picks the lowest free channel at **full volume (amp 255)**; if all 4
are busy the sound is **dropped**. FX are gated by the Sound menu toggle (`SoundState`);
music is independent.

| snd ID | Name (shipped) | Exact trigger (final code) |
|---|---|---|
| 128 | GotInfotron | `gotInfotron` when `numInfos ≠ 1` before decrement (walk **and** snap collection; also plays for surplus infotrons at 0) |
| 135 | GotLastInfo | `gotInfotron` when `numInfos = 1` (the quota-completing one) |
| 129 | Thud | `CFallChore.HitBottom` / `CFallRollChore.HitBottom` — every zonk/infotron landing (green-disk landings explode via `CFallExplChore` and do **not** thud). Not distance-gated |
| 131 | Explode | `CExplChore` frame 3 of every 3×3 explosion (incl. each chained one, each yellow-disk detonation, red-disk bombs) |
| 132 | QuarkExplode | `CQuark.Explode` (once per quark, before its 3×3) |
| 133 | GotRedDisk | `gotRedDisk` (pickup by walk or snap) **and** `UseRedDisk` (successful drop) |
| 134 | RedDiskReady | space held ≥ 90 ticks with `numBombs > 0`, once per hold (`diskDropSndPlayed`) |
| 137 | MurphyDied | `KillMurphyMain` — any death (enemy, crush, explosion, Esc, >99 disks) |
| 138 | Pause | caps-lock pause (after `HaltSounds`) |
| 139 | Unpause | caps-lock release |
| 140 | BugZap | bug activation, **only sound with distance gating**: `PlaySoundDistRange(…, kBugRange=4)` — audible iff Manhattan dist(Murphy,bug) < 4, volume = (4−dist)·255/4 |
| 141 | Push | `MurphyMove` after a *successful* push of zonk/green/yellow — fires every frame while pushing (≈12/s). (Literal 141 in code; no named constant) |
| 144/145/146 | Click / Tick / Ding! | Help-dialog tutorial steps only (step records in `'I†i†'` resources carry a soundID) |
| 150 | QLogoSnd | in shipped resources; **no call site in these sources** (sources play missing ID 143 for the intro logo — shipped binary differs, see §12.2) |
| 151 | endLevel | in shipped resources; **no call site in these sources** (probably the shipped binary's win sound; unverifiable from code on hand) |

Not sounds: winning (silent in final sources), portal transit, snapping base, terminal
touch itself (only the resulting explosions), scissors/quark movement, scrolling, menus.
Unused constants: `sOHit=130` (old OOP win-jingle instrument via `PlayMusicNote` — the
final engine has no music-note API at all) and `sAboutBox=136`; snd 130/136/143 are **not
in the shipped Sounds file**. Constants `kExplRange=22`, `kThudRange=20`, `kHearingRange=20`
and `PlaySoundDist` are dead code — only the bug zap is distance-gated.

`HaltSounds` (pause, level end) silences all 4 FX channels and stops music.

---

## 9. Music

- Resources: `'snd '` in "Infotron Music". **Shipped v1.1 has only two: 1000 "music Intro"
  and 1001 "m.loop"** despite the code choosing among three.
- Selection (`SmoothMusic`, runs once per frame during play): if music enabled and not
  paused, and (`musicRefNum = −1` or 120 s elapsed since `lastMusicChange`):
  pick `ARandom(3)+999` → **1000..1002 uniformly**. If starting fresh, play it; if a
  rotation picks a *different* ID than current, stop current and play new; picking the
  *same* ID leaves the track running (no restart).
- A picked track **loops seamlessly forever** (channel-5 callback re-`SndPlay`s the same
  handle) until the next rotation/stop.
- **Quirk: picking 1002 yields silence** (GetResource nil → nothing plays) until the next
  rotation up to 120 s later — ⅓ of rotations are a "music break". Faithful ports may
  reproduce or drop this.
- Stops: pause (`DoPause`), music-menu toggle off, and end of level — the `CKillGameChore`
  (4 s after death/win) sets `shouldKillChores`, which stops music + all sounds. Music does
  **not** stop at the instant of death — it plays through the 4-second freeze.
- Restarts: unpause / next `StartNewLevel` → next `SmoothMusic` sees `musicRefNum=−1` and
  immediately starts a random track. No music outside of play (menus/dialogs are silent
  except dialog FX).

---

## 10. Icons and art drawn outside the board

- **Level picker** (`LevelChoice` + `CInfoScrollDialog`, dialog WIND/PICT 130): a plain
  **text list** of level names (from level-resource names, ID order 301..) — **no icons or
  thumbnails**. Name colors: green = completed (`n < highLev`, not skipped), blue = skipped,
  yellow = current frontier (`n = highLev`), red = unreached. Choose/Skip/Cancel buttons;
  arrow keys + double-click.
- **Help dialog** (PICT 143 background, then PICT 144 for part 2): plots icl8s *directly*
  via the true `PlotIconID` trap: piece showcase animations (yawn 500–510+499+214, quark
  650–657, infotron shrink 350–358 both directions, terminal 550–561, bug ping-pong
  300–305 pattern, scissors demo using 1002/1152/1006/1156/1007/1000 …, zonk 700–707 full
  8-frame rotation, red-disk 400–407, base 450–457), and an interactive tutorial: 14 slides
  (`'I†i†'` resources 501–514), each a 10×7 icon board (32-px cells at offset 78,54) with
  timed icon-change steps + sounds (144/145/146). VCR-style buttons.
- **About dialog** (PICT 149): scrolling-credits PICTs 150/151 blitted through a GWorld;
  Murphy yawn animation (500–510, 500, 198) as an easter egg button.
- **Keys dialog** (PICT 132; key-cap PICTs 303–308 in sources / v1.0 — v1.1 ships PICTs
  500–507 instead, the shipped binary differs). New/rename (133/134), open/delete
  (131/145), register (155), sure (146), error window (PICT 137).
- **Startup**: warning dialog → logo sequence (sources: grayscale palette, PICT 1012
  "Q" animation + PICT 2000–2016 "Incorporated" flip-book + snd 143; **those resources
  don't ship in v1.1** — shipped shows its own Q-logo with snd 150, then PICT 3000 logo),
  then "Loading Graphics…" progress bar (blue-framed, red-filled, `statHeight=4`,
  90 px half-width, red 9-pt "Loading Graphics…" caption) while the icon sheet builds,
  gamma **FadeToBlack/FadeFromBlack(2)** transitions throughout, then a title PICT
  (sources: 140; not in shipped file — likely 128/138 in the shipped binary) waiting for a
  click, then menus + black game screen.
- **Suspend** (background switch): game window is torn down and a 55×55 mini-window with
  PICT 139 is shown bottom-left; resume rebuilds the window.
- Status bar PICT 129 (§7).

---

## 11. Port rendering decisions (summary)

1. Opaque 32×32 tiles; ignore ICN# masks; empty = solid black tile art (or just black).
2. Standard Mac 8-bit system palette to decode icl8. Board area 640×419 + 61-px status bar
   in a 640×480 virtual screen.
3. Viewport: 21×15 tiles drawn at (−16,−16), clipped — edge cells are half-visible.
   Tile-quantized scrolling (no pixel smoothing), thresholds per rules spec §6.2.
4. Redraw every frame is fine; treat snap/win/corpse "flash" sequences as instantaneous
   for logic, cosmetic at most 1 game frame.
5. Animation clock = game frame (~12 fps); every sequence in §5 advances once per frame.
6. Sounds per §8 table: 4-voice polyphony cap with drop-when-full, full volume, bug zap
   distance-gated (Manhattan < 4, linear falloff). FX toggle ≠ music toggle.
7. Music per §9: random-of-{intro, loop, silence} every 120 s, seamless loop, stop on
   pause/level-end, restart on unpause/level-start. (A sane port may drop the silence slot.)
8. Do not reproduce: icon-sheet overflow garbage (win frames 1209–1211), stale-pixel
   behavior for missing icons.

---

## 12. Corrections & additions to `infotron-reference.md`

1. **§11/§13 — there is no win jingle in the final code.** `WonGame` = `EndLevel` +
   `loadNextLevel`; nothing plays. The "victory jingle" (`PlayMusicNote` arpeggio with
   instrument snd 130 `sOHit`) exists only in the old OOP `CMurphy.p`, and the final sound
   unit has no note API. Shipped v1.1 does contain an unreferenced snd 151 "endLevel", so
   the shipped binary (newer than these sources) may play it on win — flag for playtesting
   against emulation; port recommendation: play snd 151 on win.
2. **§13 — sound-ID table corrections**: 130 and 136 are unused constants and are absent
   from the shipped Sounds file; 143 "logo" is referenced by these sources but not shipped
   (shipped has 150 "QLogoSnd" instead); add 144/145/146 (help tutorial), 150, 151.
   "141 push" and distance-gating facts confirmed.
3. **§13/§16 — music**: only **2** tracks shipped (1000 intro, 1001 loop); the 3-way random
   rotation includes a missing ID 1002 → ⅓ chance of silence per 120 s slot. Rotation only
   switches tracks when a different ID is drawn; loops are seamless; stops on pause and at
   the deferred level-kill (4 s after death/win), not at death itself.
4. **§1.2/§12 — new classifier gap: hardware icons 758–760.** They have art, and shipped
   levels use them (139 cells across 16 of the 51 levels, incl. level 1 "Warm Up",
   "Obstacle" ×48, "Evade" ×25), but `GetGridObj` only maps 750–757 to hardware → 758–760
   are the generic inert piece, whose `WalkOnto` returns TRUE: per these sources Murphy
   **walks through and erases these "walls"** (no sound, no score); they are
   explosion-proof and not rounded. Caveat: the shipped binary is newer than these sources
   and may have widened the range to 750–760 — verify in emulation before deciding whether
   the port treats them as true hardware or as walk-through (if walk-through, they are
   de-facto secret passages).
5. **§1.2 — Murphy start cell**: level data stores icon **198** at Murphy's start (all 51
   levels); the engine overwrites it on his first move. Bugs are stored only as 300;
   scissors only as 1000/1002/1004/1006; quarks only as 650; zonks/infotrons only as
   700/850 (no rotated variants in data).
6. **§6.4 — snap frame counts**: infotron snap shows 350..**357** (the loop is
   `kInfoStart..kInfoEnd−1`), red disk 400..406, base 450..456; each step is
   `SetData`+full blit with **zero delay** (sub-frame flash), then cell := 129.
7. **§6.8 — sleep details**: first yawn displays 501–511 (500 skipped), later yawns
   500–511; 499 is never displayed; mad = 139/142 (by last horizontal facing), idea =
   140/143, asleep = 141/144. Kill-range values 146, 148–193, 207–213 have no art and
   never occur; 214 exists but is dialog-only.
8. **§2.2 step 4 — exact redraw rule**: `if iuLen > 0 then RedrawBoard` after all Performs
   (one full blit); Murphy movement and `DrawOne` calls add further immediate blits
   (§3 above). The board buffer is only *modified* per-cell (`Draw1`); the *blit* is always
   full-board 640×419.
9. **§8 — bug initial visibility**: `DrawOffscreen` (level load) remaps cell value 300→128
   when drawing, so bugs are invisible from the first frame; `StartNewLevel` then sets
   their data to 128.
10. **§5.1 — explosion residue**: confirmed; icon 610 exists in art but is never drawn
    (`explAnim` is 600..609 + 129); checks use ≤610 anyway.
11. **New emergent quirk (rules-relevant)**: the final movers clear-and-rewrite their own
    cell in one Perform — `CQuarkMover`'s anim-only pass and `CScissorMover`'s in-place
    turn/`DoMove` do `SetData(cell,129)` then immediately `SetData(cell,icon)`. The
    momentary 129 fires `SetData`'s reactive checks: a faller resting directly on top of a
    stationary quark (or turning scissor) is granted a fall into the enemy's cell and will
    crush-detonate it a frame later. Ports replicating rules exactly should reproduce this
    write order (or its observable effect).
12. **§7.1 — scissor mover gate**: each Perform requires the cell to read 1000..1026
    (explodes itself if 600..610, otherwise removes the mover). Level values 998/999 or
    1148–1160 would produce a permanently inert scissor; shipped levels avoid them. The
    "turning frames 1148–1160" in the legend are dead art in the final engine (§5.8).
13. **§10 — terminal animation**: `termAnim` = 550..560 (11 icons), `cycleDelay=0`,
    continuous; 561 is never part of the cycle.
14. **§14 — status bar**: exact field positions/colors in §7 here; time digits repaint
    only on change; disk counter is blue at 0 / red otherwise; infotron counter red until
    quota met, then green.
15. **§18.1 — icon-range tails resolved by resource inventory**: 706/707 exist (art only,
    unused by engine and levels); 561 exists (unused); 658 exists (unused); 610 exists
    (never drawn); 900-range ports shipped are exactly 900/902/904/906/950/952; medium/open
    scissors have art only for cardinal directions; 1105 is runtime-only.

## 13. Known unknowns / flags

- The shipped v1.1 binary is newer than these sources: intro (Q logo + snd 150), possibly a
  win sound (snd 151), keys-dialog PICTs 500–507, and the title PICT differ from the source
  code's resource references (1012/2000–2016/143/140/303–308 — absent from shipped files;
  PICTs 303–308 do exist in v1.0). None of this affects in-game board rendering or rules.
- No `OpenResFile` call for the "Info Datafiles" appears in these sources (dev builds kept
  resources in the project file); the shipped binary opens them at launch. The port loads
  assets directly, so this is moot.
- Level decode: 51/51 levels parsed. (An earlier ad-hoc RLE reader used while writing this
  doc produced a few trailing-cell artifacts; the actual asset pipeline's decoder
  (`scripts/extract-resources.mjs`, faithful to `DecompressBoard` + `FixSize`) has since been
  verified clean: all 51 boards decode with exactly one Murphy marker 198 at the header
  position, and the cell-value histogram is exactly the set listed in §6 — no stray or zero
  cells. Use `public/assets/levels/*.json` as ground truth.)
- `drawInteger` zero-pads (e.g. "05"); the HUD font is the default system font (Chicago) —
  confirm exact glyph rendering from screenshots rather than code.
