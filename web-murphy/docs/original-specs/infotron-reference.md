# Infotron (1994 Mac, shipped) — Complete Game-Rules Reference Spec

Purpose: reference for the TypeScript web port of **Murphy's Revenge** (the sequel). Murphy's
Revenge C code (`legacy/murphy/os7code/mr/*.c`) is the authoritative target; this document
captures the complete, shipped rules of the predecessor **Infotron** so gaps/ambiguities in the
unfinished MR code can be filled. Places where Infotron likely differs from MR are flagged
inline with **[MR-DIFF?]** and collected in §18.

## 0. Sources and which code is authoritative

Sources are Think Pascal files in `/Users/dave/dev/games/Murphy/legacy/infotron/src/`
(classic-Mac CR line endings). Infotron was rewritten mid-project from a THINK Class Library
(TCL) OOP design into a monolithic "NO OOP" design. **The shipped game is the "NO OOP" set**:

- `main.p` — program, main loop, input, Murphy movement (`Process1Event`/`HandlePlaying`/`MurphyMove`)
- `everything.p` — board data, drawing, chore-list management, global helpers
- `TheChores.p` — all chore (task) implementations (final versions)
- `TheGridPieces.p` — all piece behavior (final versions; supersedes the individual `C*.p` files)
- `dataIntf.p` — all types, constants, globals (final versions)

Older OOP files (`CZonk.p`, `CQuark.p`, `CBug.p`, `CGamePane.p`, `CBoard.p`, `CMoves.p`,
`CContMoveChore.p`, `CMoveReplyChore.p`, `InfoIntf.p`, `Infotron.p`, `CInfoApp.p`, `NEWprocess1`,
`OLDProc1_`, …) are earlier revisions; where they differ from the final files this is noted in
§17. When implementing, use the final ("NO OOP") behavior.

Everything below states behavior of the final version unless marked otherwise.

---

## 1. Board model, coordinates, and the icon-ID cell encoding

- The board is a 2-D array of **integers**, `theBoard[x, y]`, max **120×120**, **1-based**
  indexing. `x` = column (h), `y` = row (v). Row 0 / col 0 are never used by gameplay
  (bounds are trusted, no clamping — levels are expected to have solid borders).
- Each cell holds a single integer which is simultaneously the **icl8 icon (graphic) resource
  ID** and the **logical piece ID**. All game logic is done by numeric range checks on this
  value. There is no separate "piece object per cell"; instead 17 singleton behavior objects
  exist and a classifier function maps cell value → behavior singleton.
- Tiles are 32×32 px; the visible playfield is **21×15 tiles** (640×480 screen; bottom 61 px is
  a status bar). `upperLeft` is the top-left visible board coordinate.
- Point↔int packing used by the level format: `PtToInt(x,y) = x*256 + y`;
  `IntToPt(i) = (h: i div 256, v: i mod 256)`.

### 1.1 Directions

```
kUp = 0, kUpRight = 1, kRight = 2, kDownRight = 3,
kDown = 4, kDownLeft = 5, kLeft = 6, kUpLeft = 7, kNoMove = 8
```
- `LocFromDir(x, y, d)` offsets one cell in direction `d` (kNoMove = same cell).
- Turn left (counterclockwise 90°): `(dir + 6) mod 8`. Turn right: `(dir + 2) mod 8`.
- Opposite direction is computed as `8 - dir` in roll code (valid for kRight/kLeft: 8−2=6,
  8−6=2). (For dir 0 this yields 8 = kNoMove — only ever used with horizontal dirs.)

### 1.2 Complete cell/icon ID legend

| ID(s) | Meaning | Behavior class |
|---|---|---|
| 128 | Base ("circuit" — the edible dirt; `kBaseNum`, board default fill) | CCircut |
| 129 | Empty space | CBlank |
| 130 | Exit | CExit |
| 131, 1100–1106 | Chip (RAM chip wall; **1105 doubles as the "dropped red disk" graphic**) | CChip |
| 132 | 4-way portal | CPortal |
| 133 | Green disk | CGreenDisk |
| 134 | Yellow disk | CYellowDisk |
| 135 | Red disk (pickup) | CRedDisk |
| 139–214 | Murphy graphics (see §6.8) | CMurphyObj |
| 300–305 | Bug, electrified (active animation frames; level data stores 300–304) | CBug |
| 350–358 | Infotron being snapped/eaten (animation) | — (none) |
| 400–406 | Red disk being snapped (animation) | — |
| 450–456 | Base being snapped (animation) | — |
| 499–511 | Murphy yawning/sleep-transition frames | CMurphyObj (via 499–511 range) |
| 550–561 | Terminal (anim cycles 550–560) | CTerminal |
| 600–609 | Explosion frames (610 also included in some checks) | — |
| 650–657 | Quark (electron) frames (classifier accepts 650–666) | CQuark |
| 700–707 | Zonk (boulder). 700 canonical; 701–703 rotation frames | CZonk |
| 750–757, 1050–1057 | Hardware (indestructible decorative wall variants) | CHardware |
| 850–853 | Infotron (the collectible). 850 canonical; 851–853 roll frames | CInfotron |
| 900–952 | Portals: directional if `icon mod 8` = a direction; 950 = vertical 2-way; 952 = horizontal 2-way | CPortal |
| 998–1009, 1148–1160 | Scissors (snik-snak). `1000+dir` small frames; `+10` open; `+20` medium; 1148–1160 turning frames | CScissor |
| 1200–1211 | Murphy-entering-exit / win animation | CMurphyObj (1200–1210) |

**The "Murphy kill range"** used by every enemy/faller check is the union:
`(499..511) ∪ (139..214) ∪ (1200..1210)`.

### 1.3 Classifier (`GetGridObj`) exact mapping (final version, `everything.p`)

Checked in this order; first match wins; **anything unmatched behaves as an inert,
indestructible, non-round piece** (generic `CGridPiece`, `gos[17]`):
128→Circut(base); 129→Blank; 700–707→Zonk; 850–853→Infotron; 131 or 1100–1106→Chip;
750–757 or 1050–1057→Hardware; 998–1009 or 1148–1160→Scissor; 650–666→Quark; 300–305→Bug;
132 or 900–952→Portal; 133→Green; 135→Red; 134→Yellow; 550–561→Terminal; 130→Exit;
murphy-kill-range→MurphyObj.

Note this means animation frames (350–358, 400–406, 450–456, 600–610) and the *medium/open*
scissor frames (1010–1027) classify as the inert generic piece — see quirk §18.2.

### 1.4 Per-class static flags (set in `InitPiece`)

Base flags default to all-FALSE: `destructable, rounded, falls, explodes, moves`.
`explAnim[1..10] = 600..609`, `explAnim[11] = 129` for every piece.

| Class | destructable | rounded | falls | explodes | moves | notes |
|---|---|---|---|---|---|---|
| CGridPiece (generic) | – | – | – | – | – | inert; survives explosions |
| CBlank (129) | ✓ | | | | | explosion anim plays on empty cells too |
| CCircut (base 128) | ✓ | | | | | |
| CChip | ✓ | ✓ | | | | things roll off chips |
| CHardware | | | | | | indestructible |
| CExit | ✓ | | | | | |
| CPortal | ✓ | | | | | |
| CTerminal | ✓ | | | | | |
| CBug | ✓ | | | | | |
| CZonk | ✓ | ✓ | ✓ | | | |
| CInfotron | ✓ | ✓ | ✓ | | | |
| CExplodes (abstract) | ✓ | | | ✓ | | 3×3 exploders |
| CRedDisk | ✓ | | | **FALSE** (overridden) | | destroyed w/o chain |
| CYellowDisk | ✓ | | | **FALSE** (overridden) | | destroyed w/o chain; detonated by terminals |
| CGreenDisk | ✓ | | ✓ | ✓ | | chain-explodes; explodes on landing |
| CMoves (abstract) | ✓ | | | ✓ | ✓ | |
| CScissor | ✓ | | | ✓ | ✓ | blank residue |
| CQuark | ✓ | | | ✓ | ✓ | **explodes into 3×3 infotrons** |
| CMurphyObj | ✓ | | | ✓ | | Explode/Destroy → kill Murphy |

---

## 2. Timing, main loop, and the chore scheduler

### 2.1 Frame clock

- Time unit: Mac tick = **1/60 s**.
- Main loop (`main.p`): while playing and not paused, each iteration runs `Process1Event` →
  `HandlePlaying`. At the end of `HandlePlaying` it **busy-waits so a frame is at least
  `kTicksBetween = 5` ticks** (comment: "was 4"). So the game logic runs at ≈ **12 fps**
  (83.3 ms per game frame) on fast machines; slower on slow Macs (frame = max(5 ticks, actual)).
- Every chore's `Perform` is called **once per game frame**. All gameplay chores use
  `delayBetween = 0`, i.e. one animation frame per game frame.

### 2.2 The chore engine

Global arrays (capacity `maxList = 1000` each): `infoUrgents` (active chores), `toBeAdded`,
`toBeRidded` (deferred add/remove queues), with lengths `iuLen/tbaLen/tbrLen`.

Each frame, at the very start of `HandlePlaying`, `DoAllChores` runs:
1. **Removals** first: for each queued removal, replace its slot with the *last* list element
   (swap-with-last; order is not preserved), dispose it.
2. **Additions**: appended to end of `infoUrgents` in queue order.
3. **Perform**: iterate `infoUrgents[1..iuLen]` in order, calling `Perform` on each.
   (Adds/removes triggered during this loop are deferred to next frame.)
4. If any chores exist, redraw the board (one blit per frame).

**Chore identity constants** (`dataIntf.p`): kInfoChore=1, kAnimChore=2, kContMoveChore=3,
kFallChore=4, kSuicideChore=5, kWaitInsChore=6, kContAnimChore=7, kExplChore=8,
kFallExplChore=9, kMoveReplyChore=10, kMoveChore=11, kKillGameChore=12, kWaitTimeChore=13,
kRollAfterChore=14.

**Per-cell chore dedupe** (`FudADD`): if the chore being added is a `kFallChore`,
`kFallExplChore` or `kMoveChore` and `fallChores[x,y]` is already TRUE for its cell, the new
chore is **discarded**; otherwise it is queued and `fallChores[x,y] := TRUE`. `FudRID`
(removal) clears the flag. `Remove` on a chore is idempotent (`myIdent += 1000` marks removed).

**Self-destruct rule**: every `CInfoChore` has `ShouldRemove` = TRUE when its cell's data is in
**600..610** (an explosion is consuming the cell); fall/move choreс check this each Perform and
remove themselves (clearing `fallChores`).

### 2.3 The two board-state bit arrays

- `fallingInfo[x,y]` (accessors `MoveInfo(x,y)` read / `MoveThere(x,y,b)` write):
  **TRUE = the cell is free to move into**. Initialized at level start to `(data = 129)`.
  Maintained by `SetData` (below), and manually toggled to *reserve* cells for in-flight moves.
- `fallChores[x,y]`: TRUE = a fall/move chore currently owns the piece in this cell.

### 2.4 `SetData(x, y, what)` — the reactive update rule

Setting a cell does bookkeeping *and triggers gravity checks*:

```
theBoard[x,y] := what
if what = 129 then
    fallingInfo[x,y] := TRUE
    GetGridObj(x, y-1).CheckFall(x, y-1)          // piece above may fall
    GetGridObj(x-1, y-1).CheckRoll(x-1, y-1, kRight)  // up-left may roll right
    GetGridObj(x+1, y-1).CheckRoll(x+1, y-1, kLeft)   // up-right may roll left
    GetGridObj(x-1, y).CheckRoll(x-1, y, kRight)      // left may roll right
    GetGridObj(x+1, y).CheckRoll(x+1, y, kLeft)       // right may roll left
else
    fallingInfo[x,y] := FALSE
```

This *event-driven* propagation (rather than a per-frame full-board gravity scan) is the heart
of the engine. **[MR-DIFF?]** MR's C code (MRChore/MRQueue) may scan differently.

### 2.5 Effective speeds (in tiles and seconds, at the nominal 12 fps)

| Entity | Rate | ≈ Speed |
|---|---|---|
| Murphy walking (key held) | 1 tile per frame | 12 tiles/s |
| Zonk/Infotron/Green disk falling | 1 tile per frame | 12 tiles/s |
| Zonk/Infotron roll (sideways step) | 5-frame move chore; cell occupancy switches at frame 3 | 2.4 rolls/s |
| Scissors | 1 action per 2 frames (move pass alternates with anim pass) | 6 tiles/s max |
| Quark | 1 action per 2 frames | 6 tiles/s max |
| Explosion | 11 frames total; neighbors damaged at frame 3 | ~0.9 s; chain step ~0.25 s |
| Bug active zap | 7-frame animation | ~0.6 s active |
| Bug dormant gap | random 121–360 ticks | 2–6 s |

---

## 3. Level format and level start

### 3.1 Data structures

```
LevelRecord (decompressed):
  name: Str255
  sizeX, sizeY: integer         // extent right/down (≤120)
  murphy, term: integer         // packed points (x*256+y); term = terminal loc (unused by engine)
  numScissors, numQuarks, numInfotrons, numBugs: integer
  scissors, quarks, bugs: array[1..110] of integer   // packed points (kMaxThings = 110)
  theBoard: array[1..120, 1..120] of integer
```

Compressed form (`CompressedBoard`, resource of type `levelResType`, one per level, looked up
**by name**; level N's resource ID is `N + 301` — first level = resource 301): same header plus
`numComp` and a flat int stream: first `numScissors`+`numQuarks`+`numBugs` entries are the
entity locations, then **RLE**: positive value = literal cell (also becomes `lastVal`);
negative value `-n` = repeat `lastVal` (n−1) more times. `lastVal` starts at 128. Cells fill
row-major (x fastest), board pre-filled with 128.

`FixSize` then trims: decrement `sizeX` while `theBoard[sizeX, 2] = 128`; decrement `sizeY`
while `theBoard[2, sizeY] = 128` (scans row 2 / column 2!).

`MakeSciStuff` re-scans the whole board (x outer, y inner — column-major) rebuilding
`scissorData[]` (`dir := icon mod 8`) from icons 998–1009/1148–1160 and `quarkData[]`
(`dir := kRight`) from icons 650–666. (These arrays are only bookkeeping in the final engine;
the movers keep their own state.)

### 3.2 `StartNewLevel` (exact sequence)

1. `DoKillChores` (hard-clears all three chore lists), clear death flag
   (`gAARDVARK_PLATYPUSS_WOMBAT := FALSE` — TRUE means "Murphy is dying").
2. Capture caps-lock state: `pauseKeyState := not keyDown(57)`.
3. `LoadBoard(levelName)`; state += isPlaying, −= freezeMurphy; `whatToBlank := 129`;
   `gMurphy.numInfos := numInfotrons` (from header, **not** a recount).
4. Murphy init: `lastMove:=0`, `numBombs:=0`, `lastDir:=kRight(2)`, position from header;
   walk-phase `gPhase := kSml(4)`, `gFromMed := kBig(0)`. Misc: `lastFacingLR := kRight`,
   `isSleeping := FALSE`, `spaceBarWasDown := FALSE`, `diskDropSndPlayed := FALSE`,
   `lastWasPortal := FALSE`.
5. For every cell (x = 1..sizeX outer, y = 1..sizeY inner):
   - `fallingInfo[x,y] := (data = 129)`; `fallChores[x,y] := FALSE`.
   - If the piece **can fall** (zonk/infotron/green disk):
     - if cell below = 129 → add a **CSuicideChore** (a "settle" watcher, §4.6) at (x,y);
     - else if piece below is **rounded** and (both left and left-down = 129) or (both right
       and right-down = 129) → add a CSuicideChore (it may roll).
   - Else if data in 300–304 (**bug**): create its `CContAnimChore` (§8) and set cell to
     **128** (dormant bug looks/acts like base).
   - Else if data in 550–561 (**terminal**): create its looping `CContAnimChore` (§10).
   - Else if piece **moves**: quark → add `CQuarkMover`; scissors → add `CScissorMover`.
6. Draw status.

Viewport initial position: `UpperLeft = murphy − (10, 7)`, clamped so the 21×15 window fits
within the board (`≥1`, and pulled back if murphy is near the right/bottom edges).

---

## 4. Falling and rolling (zonks, infotrons, green disks)

### 4.1 `CheckFall(x,y)`

If the piece has `falls` and `MoveInfo(x, y+1)` (cell below free) → call `Fall(x,y)`:
zonk/infotron create a **CFallChore**; green disk creates a **CFallExplChore**
(same, but explodes on landing). (Dedupe via `fallChores`, §2.2.)

### 4.2 `CheckRoll(x, y, dir)` (dir = kLeft or kRight) — final version

Returns TRUE and starts a roll iff **all** of:
1. `not fallChores[x, y]` (I'm not already falling/moving),
2. `not fallChores[x, y+1]` (the thing below me isn't falling away),
3. `MoveInfo(x±1, y)` — side cell free,
4. `MoveInfo(x±1, y+1)` — diagonal-down cell free,
5. `GetGridObj(x, y+1).IsRounded` — piece below is **rounded** (zonk, infotron, or chip —
   note: chips are rounded, base is not, Murphy is not).

There is **no** "piece above the side cell is falling" check in the final version (the old OOP
`CGridPiece.CheckRoll` had one; see §17).

### 4.3 `Roll(x, y, dir)` — final version rolls **horizontally, then falls**

Reserve destination cells: `MoveThere(x±1, y, FALSE)` and `MoveThere(x±1, y+1, FALSE)`;
then start a **CMoveChore(dir, dir, boulder|infotron, x, y)**.
(The old OOP versions moved diagonally in one step — final does sideways-then-fall, like
Supaplex. See §17.)

**CMoveChore** is a 5-frame animation:
- Zonk (`boulder`) sideways: frames `[700, 701, 702, 703, 700]` for `dir < kDown` (i.e. kRight)
  or `[700, 703, 702, 701, 700]` for `dir > kDown` (kLeft); `moveThereAtFram = 3`.
  (A straight-down variant `[700,700]`, move at frame 1, exists but is unused because rolls are
  always horizontal and plain falls use CFallChore.)
- Infotron: frames `[850, 851, 852, 853, 850]`, move at frame 3, regardless of direction.

**Per frame** (`Perform`): remove self if cell data ∈ 600..610 (exploding) or ∈ 350..358
(being snapped); if it's an *infotron* chore and its own cell now shows Murphy → **KillMurphy**
(Murphy grabbed/stepped into the rolling infotron). Otherwise advance one animation frame.

At `moveThereAtFram` (frame 3) the occupancy transfers: if destination still 129 → old cell
:= 129 (triggering §2.4 reactions), `fallChores` flags move to the new cell, chore's cell
becomes the destination; if destination got blocked meanwhile → un-reserve
(`fallingInfo[mtx,mty] := TRUE; fallingInfo[mtx,mty+1] := TRUE`) and the piece stays.

At sequence end (`SequenceDone`), for boulder/infotron:
- below = 129 → `Fall` (start CFallChore) and reserve below;
- below in Murphy kill range → **KillMurphy**;
- else try `CheckRoll(nextRollDir)` then `CheckRoll(8 − nextRollDir)`;
- finally `ToggleRollDir` (§4.5).

### 4.4 `CFallChore` — plain falling, 1 cell per frame

Per `Perform`: if `ShouldRemove` (cell exploding) → remove; if own cell data now in Murphy kill
range → `Explode` Murphy (Murphy walked into the falling piece's cell → dies). Else `DoOneFall`:

```
if own data in 350..358 (being snapped): remove          // snap steals the falling infotron
else if GetGridObj(x, y+1).CanExplode:                   // murphy, scissors, quark, green disk
    that piece.Explode(x, y+1); remove                   // crush → detonate victim
else if not MoveInfo(x, y+1):                            // blocked → landed
    HitBottom
else: move down one cell (copy data down, old cell := 129 via SetData, move fallChores flags)
```

`HitBottom`: remove chore; then `if self.explodes → self.Explode` (green disk); else try
`CheckRoll(nextRollDir)` / `CheckRoll(8−nextRollDir)`; play thud sound (129).

So: **a falling zonk/infotron kills Murphy, detonates scissors/quarks/green disks by landing on
them; landing on anything else just stops (with a thud), then it may roll.** There is no
minimum fall distance — falling 1 cell onto a victim detonates it.

### 4.5 Roll-direction tie-breaking is RANDOM

Global `nextRollDir` starts as `kRight` at app launch. `ToggleRollDir` sets it to kRight if
`random > 0` else kLeft — i.e. ~50/50 random, re-rolled after every boulder/infotron move
sequence and after every suicide/roll-after tick. Rolls always try `nextRollDir` first, then
the opposite. **[MR-DIFF?]** Supaplex-family games usually prefer a fixed direction; check MR.

### 4.6 `CSuicideChore` (settle-watcher) and `CRollAfterChore`

`CSuicideChore(x,y)` (created at level start for every unstable faller, and on cells left by a
quark explosion): each frame:
- if the piece at (x,y) can't fall (anymore) → remove;
- else if below free → `fallChores[x,y] := FALSE`; `CheckFall` (starts the real fall chore);
- else try both rolls (remove if one succeeds); else `uselessCount++` (give up when > 30);
- `ToggleRollDir` each tick.

`CRollAfterChore(x, y, dir)` (installed 3 frames after a zonk push, §6.5): first, if the cell
shows a rotated zonk graphic (701–703), rotate it one step toward 700 per frame (direction of
rotation follows the push dir); once canonical, behave like a suicide chore (fall/roll checks,
give up after 30 useless ticks).

### 4.7 `CFallRollChore` (pushed-zonk fall) 

Like CFallChore but keeps the zonk's rotation animation while falling (rotates the 700–703
graphic each cell moved, direction per the push), normalizes the graphic to 700 on landing, and
its `HitBottom` does rolls only (no self-explode branch).

---

## 5. Explosion engine

### 5.1 `CExplChore` — the canonical 3×3 explosion

Created at a center cell with anim `[600, 601, ..., 609, 129]` (11 frames, one per game frame).
Each frame the anim value is **written into the cell** (so the cell reads as 600–609 during the
explosion, which is lethal to walk into and suppresses other chores there). Hooks:

- **Frame 1**: if Murphy's location = this cell and he isn't already dying → **KillMurphy**.
- **Frame 3**: play explosion sound (131); then for each of the **8 neighbors**:
  - if `CanExplode` (murphy, scissors, quark, green disk) → call its `Explode` → **chains**
    (murphy: dies; scissors/green: a new full CExplChore at that cell; quark: quark explosion §7.3);
  - else → `Destroy`: if `destructable`, a plain `CAnimChore` runs the same 11-frame
    600–609→129 animation at that cell (no further chaining). Non-destructible pieces
    (hardware, and any unclassified value) are unaffected.
- **Frame 11** writes 129, which via §2.4 triggers falls/rolls into the crater.

Residue: every exploded/destroyed cell ends as **129 (empty)** — except quark explosions (§7.3)
which end as **850 (infotron)**. Murphy's corpse: see §6.7.

Chains propagate every ~3 frames (~0.25 s per ring).

### 5.2 Explosion interaction summary (what happens to each piece caught in a blast)

| Piece in blast | Result |
|---|---|
| Hardware (750–757, 1050–1057) | untouched |
| Base, chip, portal, terminal, exit, zonk, infotron, red disk, yellow disk, blank | destroy animation → 129 |
| Green disk | chain explosion (new 3×3) |
| Scissors (icon 998–1009/1148–1160 at that moment) | chain explosion → 129 residue |
| Scissors (icon 1010–1027 at that moment) | **nothing** (classifier gap; original bug, §18.2) |
| Quark | quark explosion → 3×3 infotrons (§7.3) |
| Murphy | KillMurphy (level lost) |
| Cells showing animation frames (350–358 etc.) | unaffected (classify as inert) |

---

## 6. Murphy

### 6.1 Input and cadence

- Keys are sampled every frame with `GetKeys` (raw keymap; no event queue). Defaults:
  keySet = {up 91, left 86, right 88, down 84, special/space 49} (Mac virtual codes; the
  special key defaults to Space). Suicide key = **53 (Esc)** → instant KillMurphy. Pause =
  **caps lock (57)** *toggling from its level-start position* pauses; toggling back unpauses.
- Direction priority when multiple keys held: **Up, then Right, then Down, then Left**
  (`PrepareMove(u, r, d, l, space)`).
- **Murphy acts once per frame** — there is no separate movement delay: holding a key moves
  (or snaps/pushes) 1 tile per game frame (~12 Hz). **[MR-DIFF?]**

### 6.2 `MurphyMove(oldX, oldY, newX, newY, myDir, spaceDown)` — no space (walk) path

Let `num` = data at destination.
1. If `num` ∈ 600–609 (explosion) or 350–358 (infotron snap anim) → **KillMurphy**.
2. Else ask destination piece `WalkOnto(newX, newY)`; if TRUE and Murphy didn't just die:
   - `SetLoc(newX,newY)`; set destination cell to a Murphy walk icon (§6.8) based on
     direction/facing; walking up/down keeps last horizontal facing.
   - Advance mouth phase: from small/big → medium; from medium → alternate big/small
     (cycle: big, med, small, med, big, …).
   - If `num` was zonk (700–707), green (133) or yellow (134): show push pose (196 left /
     197 right) instead and play push sound (141).
   - Old cell := `whatToBlank` (normally 129; portal icon after portal transit; 1105 after a
     red-disk drop), then portal handling (§9) or `whatToBlank := 129`.
   - Scroll the viewport by one tile if Murphy crosses the scroll thresholds
     (`CanScroll`: up if `my > 7 and my < by−6`; right if `mx < bx−9 and mx > 11`;
     down if `my > 8 and my < by−5`; left if `mx > 10 and mx < bx−10`).

`WalkOnto` results by piece: blank/base → TRUE (base is eaten by being overwritten);
infotron → TRUE + `GotInfotron`; red disk → TRUE + `GotRedDisk`; zonk/green → push
(only if `lastDir` is kLeft/kRight); yellow → push (any of the 4 directions);
bug (active) → TRUE + **ExplodeTWO** (death at bug's cell); scissors/quark → TRUE + **KillMurphy**;
chip/hardware/terminal → FALSE; exit → FALSE (+win check §11); portal → §9.

Collecting an infotron by walking is instant (no animation); the destination simply becomes
Murphy.

### 6.3 Collecting

- `GotInfotron`: play sound 128 (135 when it was the last needed); decrement `numInfos`
  (floored at 0 — extras beyond quota are collected but don't go negative).
- `GotRedDisk`: `numBombs++`; **if numBombs > 99 Murphy explodes** (easter-egg overflow).
- Quota = level header `numInfotrons`; exit opens when `numInfos < 1`.

### 6.4 Snap (space + direction; "eat without moving")

Target = adjacent cell in the pressed direction. Only these cases do anything:
- Active bug (300–305): **ExplodeTWO(oldX, oldY)** — Murphy dies at his own cell.
- Infotron (850): Murphy shows snap pose toward the target (195 left / 199 up / 194 right /
  200 down); target cell rapidly plays 350→357 (each frame drawn immediately, all within the
  same game frame — effectively a flash), `GotInfotron`, then target := 129.
- Red disk (135): same, with anim 400→406, `GotRedDisk`, target := 129.
- Base (128): same, with anim 450→456, target := 129.
Everything else (zonks, disks, enemies, blank, …): no effect.
Murphy stays put. Snapping resets the space-hold state (a snap cancels a pending disk drop).

### 6.5 Pushing

- **Zonk** (`CZonk.Push`, only horizontal): if the cell beyond the zonk is 129:
  - if the zonk is currently falling (`fallChores[x,y]`) → **KillMurphy** (walked into a
    falling zonk) and no push;
  - else move the zonk one cell (its graphic rotates one step in the 700–703 cycle, + for
    right, − for left), **do not** clear the zonk's old cell (Murphy immediately overwrites it),
    and: if the zonk's new below-cell is free → start a `CFallRollChore` (falls with rotation);
    always install `CWaitInsChore(3 frames)` → `CRollAfterChore` (settles/rolls after the push).
  - Push succeeds every frame — pushing is as fast as walking; **no push delay or probability**.
    **[MR-DIFF?]** (Supaplex-likes usually add a delay.)
- **Green disk**: horizontal only; same move-then-`CheckFall` (falling green disk explodes on
  landing). Old cell intentionally not cleared (comment: prevents things falling between Murphy
  and the disk).
- **Yellow disk**: pushable in **all four directions** (WalkOnto simply calls Push regardless of
  direction); moves if the cell beyond is 129; never falls.

### 6.6 Red disk drop (bomb)

- Press special/space with no direction: records `whenWentDown`.
- Held ≥ **90 ticks (1.5 s)** with `numBombs > 0`: plays "disk ready" sound 134 once.
- On release after ≥ 90 ticks: `UseRedDisk` (if `numBombs > 0`: decrement, sound 133) then:
  - `whatToBlank := 1105` — when Murphy steps off his cell, it becomes the **dropped disk**
    graphic 1105 (which classifies as a *chip*: blocks movement, rounded);
  - a `CExplChore` at Murphy's current cell is scheduled via `CWaitTimeChore` with countdown
    **2 seconds** (real time: fires when `tickCount > installTime + 2*60`).
  - If Murphy is still on that cell at explosion frame 1 → he dies. (There is no way to cancel.)
- Any directional move while space is held is a snap and clears the pending hold state.

### 6.7 Death and `ExplodeTWO`

- `KillMurphy` (walked into enemy/explosion, crushed, Esc, etc.): if not already dying —
  start a 3×3 explosion at Murphy's cell, set his cell to corpse icon **145**, blocking
  `Delay(9 ticks)`, set the dying flag, `EndLevel` + death sound 137.
- `ExplodeTWO(x, y)` (bug contact / bug snap): clear Murphy's own cell to 129, relocate to
  (x,y), start 3×3 explosion there, corpse icon **147**, `Delay(12 ticks)`, dying flag,
  `EndLevel` + sound 137.
- `EndLevel`: adds `freezeMurphy` to game state (input ignored; **chores keep running**, so the
  board animates on) and schedules `CKillGameChore` after a `CWaitTimeChore` of **4 seconds**,
  which then stops play (`isPlaying` cleared, cursor shown, elapsed time added to the player
  record, chores killed).
- The dying flag `gAARDVARK_PLATYPUSS_WOMBAT` gates all further Murphy processing.

### 6.8 Murphy graphics / idle / sleep

- Walk cycle icons: `kBase=201`; left = 201/203/205 (big/med/small mouth), right = 202/204/206
  (`kBase + (0 left|1 right) + phase` where phase ∈ {0 big, 2 med, 4 small}).
- Idle: if no move for **15 ticks**, icon 198 (standing), phase reset (med, next big).
- Push poses 196 (L) / 197 (R); snap poses 194 (R), 195 (L), 199 (U), 200 (D).
- Sleep: after **45 s** without moving: yawn sequence icons 500→511 (one per frame), repeat
  after 30 s pauses ×3 total; then 2 s → "mad" icon (139 facing left / 142 right); 3 s →
  "idea" (140/143); 45 ticks → "asleep" (141/144), stays. Any move wakes him. (Sleeping
  Murphy is still killable — 499–511 and 139–144 are in the kill range.)
- Corpses: 145, 147. Win animation: 1200–1211.

---

## 7. Enemies

Both enemies are driven by dedicated per-entity chores created at level start (they are the
only pieces with persistent identity, tracked by the chore's own coordinates). Both **act every
other frame** (`shouldMove` toggles each Perform): one movement pass, one animation-only pass →
max speed 1 tile / 2 frames (~6 tiles/s = half Murphy's speed).

Both movers self-destruct if their cell's data is no longer one of their own icons (e.g. after
being exploded, or — bug — overwritten).

### 7.1 Scissors (snik-snak), `CScissorMover`

State: `currentDir` (init from level icon: `icon mod 8`), `currentSize` ∈ {0 small, 10 open,
20 medium}, `afterMed` (0/10 alternator), `turnAround`, `forceNextMove`, `goingValue` (±1,
picks the turn graphic).

Every pass first advances the size cycle: small(0) → medium(20) → open(10) → medium(20) →
small… (i.e. if size = 20, size := afterMed and afterMed := 10 − afterMed, else size := 20).
Displayed icon = `1000 + currentDir + currentSize` on anim passes.

**Movement pass** (`DoMovement`), with `L = (dir+6) mod 8` (counterclockwise),
`R = (dir+2) mod 8`:
1. If `turnAround` pending → turn left in place again (see below).
2. Else if `forceNextMove` pending → move straight ahead **without re-checking the target**
   (quirk §18.3).
3. Else check in order **Left, Straight, Right**:
   - if that cell's data is in the Murphy kill range → **KillMurphy** (contact kill; scissors
     survive);
   - else if it is 129 → move there (turning as needed);
   - else fall through to the next option.
4. If all blocked → turn left in place.

So scissors are **left-wall followers**: prefer turning left, then straight, then right, else
rotate 90° CCW in place. **[MR-DIFF?]** (check MR's turn preference; Supaplex snik-snaks are
also CCW but electrons are CW — Infotron's quark is CCW too, see §7.2.)

Turning costs an extra beat: moving into the left/right cell first spends one movement pass
showing a turning graphic in place (`currentDir + 1000 + goingValue`, clamped 999→1007), sets
`forceNextMove`, and the *next* movement pass performs the (unvalidated) straight move.
In-place turns when blocked use `turnAround` similarly (icon `newDir + 1000`; two lefts = 180°
over two movement passes).

If the scissor's cell shows 600–610 (an explosion consumed it), the mover effectively idles
until the cell resolves to 129, then removes itself. Residue of an exploded scissor: blank.

### 7.2 Quark (electron), `CQuarkMover`

State: `currentDir` (always initialized **kRight** regardless of level data), `lastAnim`
(current icon 650–657, advanced by 1 with wraparound every pass — continuous spin).

Movement pass: identical **Left → Straight → Right → turn-left-in-place** preference and the
same Murphy-contact kill (KillMurphy), with no turn animation delays: moving/turning happens
immediately (data moved, old cell := 129 via SetData). Anim-only pass redraws in place with the
next spin frame.

### 7.3 Quark explosion (`CQuark.Explode`, final version in TheGridPieces.p)

Triggered when a quark is hit by an explosion or crushed by a falling zonk:
1. Play sound 132; set own cell to 358.
2. For each cell of the **3×3** around it, unless that cell is hardware (750–757/1050–1057):
   - if it's not the center → call that piece's `Explode` (full chaining, like §5.1 frame 3);
   - run anim `[358, 357, 356, 355, 354, 353, 352, 351, 850]` (9 frames) at the cell — the cell
     **ends as an infotron (850)**;
   - schedule via `CWaitInsChore(9 frames)` a `CSuicideChore` at the cell so the new infotron
     falls/rolls if unsupported.
So **quarks explode into a 3×3 block of infotrons** (matching Supaplex electrons). The quark
anim chore aborts if a normal explosion overwrites the cell (data 600–610).

### 7.4 Enemies vs. falling objects

A zonk/infotron falling onto a scissor or quark calls `Explode` on it (CanExplode = TRUE) —
full explosion (blank residue for scissors, infotrons for quarks). Caveat: a scissor currently
displaying a medium/open icon (1010–1027) is *not* recognized (classifier gap §18.2), so the
faller simply lands on it.

---

## 8. Bug (electrified base)

- Level data 300–304 marks bug cells. At level start each becomes a `CContAnimChore` with:
  - random dormant delay: `ARandom(240) + 120` ticks → **121–360 ticks (2–6 s)** between zaps
    (`ARandom(n) = abs(random) mod n + 1`, i.e. 1..n);
  - active animation `[300, 301, 302, 303, 304, 305, 128]` — 7 frames, one per game frame
    (~0.6 s), ending with the cell set back to **128 (plain base)**.
- While dormant the cell is literally 128: it is safe, edible base; eating/snapping it removes
  the bug permanently (the chore removes itself when its cell's data is not one of
  {128, 300–305, 550–561}).
- While active (cell shows 300–305): walking into it or snapping it → **ExplodeTWO** (Murphy
  dies; explosion happens at the bug's cell on walk-in, at Murphy's own cell on snap).
- When a zap cycle starts, the zap sound (140) plays only if Murphy is within
  `kBugRange = 4` cells.
- Bugs are destructible (blast → 129, no chain), and being base-like they cannot fall/roll.

---

## 9. Ports (portals)

Portal icons: **132** = 4-way cross; **950** = vertical-only; **952** = horizontal-only;
any other icon in 900–952 = one-way, enterable only when Murphy's movement direction equals
`icon mod 8` (e.g. an icon ≡ 2 (mod 8) is traversed moving right). There are **no gravity/
special ports and no zonk/enemy traversal — ports are Murphy-only.** [MR-DIFF?]

Traversal (`WalkOnto` + `CoolPortals`): allowed only if the cell **beyond** the portal (one
more step in the same direction) is 129. When allowed:
1. Murphy logically moves onto the portal cell for one frame; the cell's data is immediately
   restored to the portal icon (Murphy is drawn half-in/half-out at ±16 px offsets on both
   sides as a pass-through effect).
2. `whatToBlank := portalIcon` and `lastWasPortal := TRUE`; the currently held direction keys
   are captured, and on the **next frame** they are re-applied (`DoLastWasPortal`), forcing
   Murphy to continue out the far side (where the normal `MurphyMove` logic applies again).
   Leaving restores the portal icon to the cell.

Portals are destructible by explosions (→ 129).

---

## 10. Terminal + yellow disks

- Terminal cells (550–561) run a continuous `CContAnimChore` cycling icons 550–560 with no
  pause (`cycleDelay = 0`).
- Murphy cannot enter a terminal. Touching it (walking into it) triggers: scan the whole board;
  **every cell whose data is exactly 134 (yellow disk) gets `Explode`** — i.e. a full 3×3
  chain-capable explosion centered on each yellow disk. The terminal itself stays.
  (There is no per-terminal pairing; **all** yellow disks detonate. Repeatable — the terminal
  remains functional.)
- Yellow disks: pushable in all 4 directions (§6.5), never fall, `explodes = FALSE` so a nearby
  explosion merely destroys them (no chain) — only terminals detonate them properly.

---

## 11. Exit and quota

- Exit icon 130. `WalkOnto` always returns FALSE (Murphy never occupies the exit cell).
- If `numInfos < 1` when Murphy walks into it: the win animation icons **1200→1211** are
  written to *Murphy's current cell* one after another (drawn immediately, effectively a quick
  flash), a victory jingle plays, then `WonGame` = `EndLevel` (4 s of continued simulation,
  §6.7) + `loadNextLevel := TRUE`.
- Level progression on win: next level = level resource with ID `currentNumber + 301`
  (names looked up from the resource); `current += 1`; `highLev` increments if this was the
  highest; skipped-level bookkeeping updates; player record saved.
- The exit is destructible by explosions (→ 129, softlock possible).
- There is **no level timer / time limit** in Infotron (elapsed time is tracked only as a
  cumulative statistic). **[MR-DIFF?]**

---

## 12. Chip / Circuit(base) / Hardware — static pieces

- **Base = 128** (class name "Circut"): edible ground; destructible; not rounded.
- **Chip** (131, variants 1100–1106): impassable wall, **rounded** (zonks/infotrons roll off),
  destructible. The dropped-red-disk graphic 1105 shares this class.
- **Hardware** (750–757, 1050–1057): impassable, **indestructible** (explosions and quark
  blasts skip it). Pure decoration variants of a wall.
- None of these interact otherwise; there are no "special chip halves" or conductive circuits.

---

## 13. Sounds (resource IDs) and hearing ranges

128 got-infotron; 129 thud (landing); 130 win-jingle note instrument; 131 explosion;
132 quark explosion; 133 got/used red disk; 134 red-disk-ready (held 1.5 s); 135 got LAST
infotron; 136 about-box; 137 Murphy died; 138 pause; 139 unpause; 140 bug zap; 141 push;
143 logo. Distance-gated: bug zap `kBugRange = 4`; (constants `kExplRange = 22`,
`kThudRange = 20` exist; only the bug range is actually used in the final code.)
Background music: 3 tracks (resource 1000–1002), rotated every 120 s.

## 14. Status/HUD facts

- Time displayed h:m:s from ticks (`216000` ticks/hr). Infotron counter turns green at 0;
  red-disk counter red when > 0. Board window 21×15 tiles at 32 px.

## 15. Pause/abort mechanics

- Pause = caps-lock toggle (state captured at level start); pausing shows the cursor and menu
  bar, stops music/sounds and the game timer. Unpause resumes.
- Abort (menu): kills the level; Esc = suicide (KillMurphy).

## 16. Global timing constants (calibration table)

| Constant | Value | Meaning |
|---|---|---|
| tick | 1/60 s | Mac time base |
| `kTicksBetween` | **5 ticks** (~83 ms; "was 4") | min game-frame duration → ~12 fps |
| Murphy move/push/snap | every frame | ~12 actions/s |
| Fall speed | 1 cell/frame | ~12 cells/s |
| Roll (side-step) | 5 frames, occupancy at frame 3 | |
| Scissor/quark action | every 2nd frame | ~6 tiles/s |
| Explosion | 11 frames; damage at frame 3 | |
| Bug dormant | 121–360 ticks random | 2–6 s |
| Bug active | 7 frames | ~0.6 s |
| Red-disk arm hold | 90 ticks | 1.5 s |
| Red-disk fuse | 2 s (wall clock) | via CWaitTimeChore |
| Zonk post-push settle delay | 3 chore frames | CWaitInsChore→CRollAfterChore |
| Quark-residue fall check delay | 9 chore frames | per cell |
| Settle-chore give-up | uselessCount > 30 | |
| Idle → standing icon | 15 ticks | |
| Idle → sleep sequence | 45 s (2700 ticks) | yawns every 30 s ×3; +2 s mad; +3 s idea; +45 ticks asleep |
| Death/win freeze before level end | 4 s | board keeps simulating |
| Corpse blocking delay | 9 ticks (145) / 12 ticks (147) | UI freeze on death |
| Music rotation | 120 s | |

## 17. Final ("NO OOP") vs older OOP sources — differences to be aware of

1. **Zonk/infotron roll**: OOP `CZonk.p`/`CInfotron.p` rolled *diagonally* (kDownLeft/kDownRight
   MoveChore); final rolls **horizontally then falls** (§4.3). Use final.
2. **CheckRoll**: OOP version required `whatBelow <> 129` and "piece above the side cell can't
   fall" (`nextUp` check) and had **no** fallChores guards; final uses fallChores guards and
   drops the nextUp condition (§4.2). Use final.
3. **Bug animation**: OOP `CBug.p` used a 19-frame anim (301,302,303,304,303,…,300,128);
   final uses the 7-frame 300–305,128 (a comment says "was 19"). OOP bug WalkOnto used
   `Explode` at the bug's cell; final uses `ExplodeTWO`.
4. **Zonk push**: OOP push didn't kill Murphy on falling zonks, didn't rotate the graphic, and
   cleared the old cell itself; final adds falling-zonk kill, rotation, RollAfter chore.
5. **Quark explosion**: OOP skipped scheduling a settle chore at the center and didn't set the
   center to 358 first; final does both (§7.3).
6. **Enemy movement**: OOP used CContMoveChore/CMoveReplyChore + `CMoves.Move` (same
   L→S→R→turn preference, but killed by `GetMurphyObj.Explode` at *the enemy's target cell* and
   full 5-frame scissor turn animations from `MakeSciMove`). Final replaced this with the
   simpler `CScissorMover`/`CQuarkMover` (§7.1–7.2). Use final.
7. **Murphy stats UI plumbing** (`CMurphy.p` gotInfotron color changes etc.) — cosmetic only.
8. `Infotron.p` (dated 12/18/93) is an unrelated early prototype; ignore.

## 18. Ambiguities, quirks, and likely Infotron↔Murphy's-Revenge differences

### 18.1 Explicit uncertainty
- Only the code was inspected; actual icl8 artwork and level resources were not decoded, so a
  few icon-range tails are unverifiable (e.g. whether 706/707 zonk variants or 561, 611, 666
  ever occur in shipped levels; ranges are quoted exactly as checked in code).
- `random` is the Mac toolbox 16-bit `Random` (−32768..32767) seeded from `tickCount`;
  `ToggleRollDir` treats `random > 0` as kRight — 0 counts as kLeft (negligible bias).
- `term` field of the level header (terminal location) is stored but never read by the engine.

### 18.2 Shipped-engine quirks (faithful-port decisions needed)
- **Scissor classifier gap**: scissor icons with size offsets +10/+20 (1010–1027) are not in
  `GetGridObj`'s scissor ranges (998–1009, 1148–1160), so during ~¾ of its animation cycle a
  scissor is *immune* to explosions and cannot be crush-detonated (fallers just land on it).
- **Forced scissor moves don't re-validate** the target cell (a one-frame race that can
  overwrite whatever moved into it, including Murphy's icon, without a proper kill).
- Explosion `Destroy` animates on *empty* cells too (blank is destructible) — visual only.
- The exit is destructible; blowing it up makes the level unwinnable.
- Collecting > 99 red disks explodes Murphy.
- Pushing has no delay; walking into a *falling* zonk from the side kills Murphy.
- `CInfoChore.ShouldRemove` uses 600..**610** although explosion frames are 600–609.

### 18.3 Where Murphy's Revenge likely differs (verify against MR C code)
- **New piece types**: MR artwork includes lasers, plasmoids, robots, red zonks, blue zonks,
  zaps — none exist in Infotron; their rules cannot be inferred from this codebase.
- **Roll tie-breaking**: Infotron randomizes (50/50) per attempt; MR may be deterministic.
- **Speeds/scheduler**: MR has its own chore/queue engine (MRChore.c, MRQueue.c, MRLoop.c) and
  likely different frame pacing than Infotron's 5-tick frame and 1-tile-per-frame Murphy.
- **Enemy turn preference**: both Infotron enemies are left(CCW)-preferring; Supaplex electrons
  are CW — MR may distinguish scissors vs quarks.
- Push delay, bug timing distribution, red-disk fuse length, and the 4 s end-of-level freeze
  are all candidates for tuning differences.
- Infotron portals are Murphy-only, non-gravity; MR may add port types.
- No demo/replay, no time limit, no score in Infotron; MR may add them.
