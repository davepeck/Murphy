# Murphy's Revenge — Complete Gameplay Rules Specification

Source: `/Users/dave/dev/games/Murphy/legacy/murphy/os7code/mr/` (classic Mac OS 7 C, SpriteWorld library, 1997–98, by Dave Peck / Q Software). Primary files analyzed: `MRPlay.c`, `MRPlayCommon.c/.h`, `MRPlayConstants.h`, `MRPlaySprite.c/.h`, `MRChore.c/.h`, `MRQueue.c/.h`, `MRMap.c/.h`, `MRPlayers.c/.h`, `MRLoop.c`, `MRPrefs.c/.h`, `MRSound.h`, `main.c`.

The player character is called **Oliver** in the code (a bubble creature); the game is a Supaplex-style tile puzzle ("Murphy" appears in comments as a synonym for the player).

---

## 1. Timing model

All game logic timing uses the Mac Toolbox `TickCount()`, which advances at **60 ticks per second**. "Ticks" below always means 1/60 s.

- The level animation runs through SpriteWorld with `SWSetSpriteWorldMaxFPS(sw, kMaxLevelFPS)` where **`kMaxLevelFPS = 30`** — i.e., at most 30 rendered/processed frames per second (2 ticks per frame), not synced to VBL.
- Per idle iteration: `SWProcessScrollingSpriteWorld` (runs every sprite's move proc + time-based frame procs), `SWAnimateScrollingSpriteWorld` (render), then **only if a frame actually occurred** (`sw->frameHasOccured`) → `HandleAllChores` (each queued chore processed at most once per rendered frame).
- Sprite animation-frame intervals are in **milliseconds**: `kSpriteFrameMilli = 33` (falling/rolling/pushed pieces), `kQuarkFrameMilli = 66` (quark & scissor anim), `kOliverBubbleMilli = 100` (Oliver's frame proc).
- Tile-image animation (`Tile_ChangeProc`): whenever `TickCount() - lastTileAnimTicks > kSpriteFrameTicks` (**`kSpriteFrameTicks = 2`**), the global tile images for `kInfotronTile` (cycling 94→109) and `kTerminalTile` (cycling 110→121) advance one frame (≈20 Hz).

### Movement speeds (pixels per processed frame; tile = 32 px)
- `kLevelItemSize = 32` (tile size in pixels).
- `kSpriteMoveDelta = 8` → Oliver walking, falling pieces, rolling pieces, and the self-propelled gold disk move 8 px/frame = **4 frames per tile** (≈7.5 tiles/s at 30 fps).
- `kMoverSpriteMoveDelta = 4` → quarks and scissors move 4 px/frame = **8 frames per tile** (≈3.75 tiles/s).
- Pushing: both Oliver and the pushed piece move at `kSpriteMoveDelta/2 = 4` px/frame = **8 frames per tile** (half walking speed). Exception: a pushed **gold disk** departs at full 8 px/frame under its own power while Oliver follows at 4.
- `kSpriteMoveDistance = 100` is used only for camera scrolling (see §20).

### Grid alignment rule (core update primitive)
Every MR sprite has a shared SpriteWorld move proc (`_sprite_SWMoveProc`) that runs each processed frame. It computes `row = destFrameRect.top / 32`, `col = destFrameRect.left / 32`; **only when the sprite is exactly grid-aligned** (`row*32 == top && col*32 == left`) does it call the sprite's MR-specific move proc (decision logic). Afterwards it offsets the sprite by its current `(horizMoveDelta, vertMoveDelta)`. So all decisions (Oliver's input handling, mover AI, fall continuation) happen exactly once per tile entered.

**There is no per-tick full-grid scan.** The world is event-driven:
1. A **tile map** (`cb->tiles[row][col]` of `short` tile IDs) is the authoritative grid.
2. **Sprites** animate pieces in motion (Oliver, falls, rolls, pushes, quarks, scissors); a moving piece's grid cells are reserved with the special `kPhantomTile` (ID 66).
3. **Chores** (a FIFO queue) run cell-local timed processes (explosions, exits opening, bugs, zaps, red-disk countdowns, lasers).
4. `DrawLevelTile(sw,row,col,tileID)` is the single write-hook to the tile map: it calls `SWDrawTile` and, **if the written tile is `kBlankTile`**, immediately checks whether neighbors should start falling/rolling into it (see §9). Writing via raw `SWDrawTile` bypasses the cascade check (used deliberately in several places).

---

## 2. Tile / piece enumeration (exact IDs)

From `enum TileTypes` in `MRPlayConstants.h` (values are sequential from 0; comments in source confirm checkpoints at 30, 40, 50, 60, 70, 80, 90, 100, 120, 170):

| ID | Name | Notes |
|----|------|-------|
| 0 | kCircuitTile | "base"/dirt. Edible, passable |
| 1 | kBlankTile | empty space |
| 2 | kExitTile | closed exit |
| 3 | kGreenDiskTile | orange-disk analog: falls, ignitable, pushable (horiz only) |
| 4 | kYellowDiskTile | pushable 4-dir bomb; detonated by terminals |
| 5 | kRedDiskTile | collectible, droppable bomb |
| 6 | kGoldDiskTile | pushable 4-dir "torpedo" |
| 7 | kTerminalTile | detonates all yellow disks |
| 8 | kBugTile | editor marker: converted at load to circuit + bug chore |
| 9 | kOliverTile | editor marker: player start; converted to blank |
| 10 | kLeftPortTile | port, pass only moving left |
| 11 | kDownPortTile | port, pass only moving down |
| 12 | kRightPortTile | port, pass only moving right |
| 13 | kUpPortTile | port, pass only moving up |
| 14 | kLeftRightPortTile | port, pass only horizontal |
| 15 | kUpDownPortTile | port, pass only vertical |
| 16 | kFourWayPortTile | port, any direction |
| 17 | kZonkTile | boulder: falls down, rolls, rounded, pushable horiz |
| 18 | kUpZonkTile | anti-gravity boulder: falls **up** (`fallRowDelta = -1`), rolls, rounded, pushable horiz |
| 19 | kInfotronTile | collectible quota item; falls, rolls, rounded, edible, passable |
| 20 | kQuarkTile | enemy (electron/snik-snak analog), converted to sprite at load |
| 21 | kUpScissorTile | scissor enemy start facing up |
| 22 | kRightScissorTile | scissor start facing right |
| 23 | kDownScissorTile | scissor start facing down |
| 24 | kLeftScissorTile | scissor start facing left |
| 25 | kChipTile | RAM-chip analog; rounded (things roll off), not passable |
| 26 | kTopChipTile | rounded |
| 27 | kBottomChipTile | rounded |
| 28 | kLeftChipTile | rounded |
| 29 | kRightChipTile | rounded |
| 30 | kTopLeftBorderTile | border (non-explodable) |
| 31 | kTopBorderTile | |
| 32 | kTopRightBorderTile | used to detect level width |
| 33 | kRightBorderTile | |
| 34 | kLeftBorderTile | |
| 35 | kBottomLeftBorderTile | |
| 36 | kBottomBorderTile | |
| 37 | kBottomRightBorderTile | used to detect level height |
| 38 | kBlankChipTile | pushable 4-dir AND rounded |
| 39 | kFirstHardwareTile | decorative wall, non-explodable |
| 40–64 | kHardwareTile2 … kHardwareTile26 | decorative walls, non-explodable |
| 65 | kLastHardwareTile | non-explodable |
| 66 | kPhantomTile | runtime-only: cell reserved by a moving sprite or Oliver |
| 67 | kLaserTile | laser emitter (Infotron terrain) |
| 68 | kLaserPowerTile | pushable battery that activates adjacent lasers |
| 69 | kForeMirrorTile | "/" mirror, pushable |
| 70 | kBackMirrorTile | "\\" mirror, pushable |
| 71 | kEmptyPhaseTile | phase block starting phased-out (converted to blank at load) |
| 72 | kFullPhaseTile | phase block phased-in (non-explodable) |
| 73 | kPhaseTermTile | phase terminal (bump to toggle phase blocks) |
| 74–79 | kEmptyTile4 … kEmptyTile9 | unused padding |
| 80 | kBlankLaserPiece | laser art padding |
| 81 | kLaserEmptyPiece | spent laser battery (pushable) |
| 82 | kLaserPowerPiece | art |
| 83 | kLaserPiece | art |
| 84 | kForeMirrorPiece | art |
| 85 | kBackMirrorPiece | art |
| 86 | kTLBRLaserForeMirrorLightPiece | beam-on-mirror art |
| 87 | kRLLaserLightPiece | horizontal beam |
| 88 | kUDLaserLightPiece | vertical beam |
| 89 | kTLLaserForeMirrorLightPiece | |
| 90 | kBRLaserForeMirrorLightPiece | |
| 91 | kBLLaserBackMirrorLightPiece | |
| 92 | kTRLaserBackMirrorLightPiece | |
| 93 | kBLTRLaserBackMirrorLightPiece | |
| 94–109 | kFirstInfotronTile … kLastInfotronTile | 16-frame infotron idle animation |
| 110–121 | kFirstTerminalTile … kLastTerminalTile | 12-frame terminal animation |
| 122–127 | kFirstBugTile … kLastBugTile | 6-frame bug zap animation (deadly+electric while shown) |
| 128–137 | kFirstExplTile … kLastExplTile | 10-frame explosion; **131 = kIgniteExplTile** (chain-reaction frame, 4th) |
| 138–145 | kFirstExitTile … kLastExitTile | 8-frame exit-opening; **145 = open exit (passable)** |
| 146–153 | kFirstZapRedDiskTile … kLastZapRedDiskTile | 8-frame red-disk zap |
| 154–155 | kEmptyZapTile8/9 | padding |
| 156–162 | kFirstZapCircuitTile … kLastZapCircuitTile | 7-frame circuit zap |
| 163–165 | kEmptyZapTile17/18/19 | padding |
| 166–178 | kFirstZapInfotronTile … kLastZapInfotronTile | 13-frame infotron zap; non-explodable; also used (reversed) as the quark explosion |
| 179–180 | kFirstRDBlinkTile, kLastRDBlinkTile | dropped red disk blink (2 frames) |
| 181–184 | kFirstZonkTile, kZonkTile1, kZonkTile2, kLastZonkTile | zonk rolling animation |
| 185–188 | kFirstUpZonkTile … kLastUpZonkTile | up-zonk rolling animation |
| 189–197 | kFirstQuarkTile … kLastQuarkTile | 9-frame quark animation |
| 198–200 | kFirstUpScissorTile, kUpScissorTile1, kLastUpScissorTile | scissor facing-up (3 frames) |
| 201 | kUpRightScissorTile | diagonal turn frame |
| 202–204 | kFirstRightScissorTile … kLastRightScissorTile | facing-right |
| 205 | kDownRightScissorTile | diagonal |
| 206–208 | kFirstDownScissorTile … kLastDownScissorTile | facing-down |
| 209 | kDownLeftScissorTile | diagonal |
| 210–212 | kFirstLeftScissorTile … kLastLeftScissorTile | facing-left |
| 213 | kUpLeftScissorTile | diagonal |
| 214 | kMaxLevelTiles | count |

Directions: `kNoMove=0, kMoveUp=1, kMoveDown=2, kMoveLeft=3, kMoveRight=4, kMoveSpecial=5`. Facings: `kMoverStatic=0, kMoverFacingUp=1, kMoverFacingRight=2, kMoverFacingDown=3, kMoverFacingLeft=4`.

Terrains (`extraInfo.levelWorld`): `kInfotronTerrain=0, kAquatronTerrain=1, kSavanatronTerrain=2, kDeathtronTerrain=3`. Terrain selects an aliased resource set (different tile art & sounds per world). **Laser pieces and phase hardware are only loaded/active when `levelWorld == kInfotronTerrain` (0).**

---

## 3. Piece property table (`GetPieceKind`, MRPlayCommon.c)

Defaults for every tile: `isPassable=false, isRounded=false, isExplodable=true, isIgnitable=false, isPort=false, isDeadly=false, isElectric=false, isEdible=false, isPushable=false, canFall=false, canRoll=false, fallRowDelta=+1, isQuark=false, isOliver=false, moverSprite=NULL`.

Specific overrides (note the deliberate switch fall-throughs):

| Tile | Overrides |
|------|-----------|
| kCircuitTile (0) | edible; **falls through to** passable |
| kBlankTile (1) | passable |
| kBlankChipTile (38) | pushable, **rounded** |
| kChipTile & 4 sided chips (25–29) | rounded |
| kInfotronTile (19) | passable, edible, canFall, canRoll, rounded |
| kGreenDiskTile (3) | **ignitable**, pushable, canFall (NOT rounded, NOT canRoll) |
| kRedDiskTile (5) | edible, passable |
| kGoldDiskTile (6) | pushable |
| kPhaseTermTile (73) | explodable (redundant) |
| kFullPhaseTile (72) | **not** explodable |
| kTerminalTile (7) | **not** explodable |
| kUpZonkTile (18) | `fallRowDelta = -1`; falls through ↓ |
| kZonkTile (17) | canFall, canRoll, rounded; falls through ↓ |
| kYellowDiskTile (4) | pushable (yellow disk gets ONLY pushable — it does **not** fall) |
| kLastExitTile (145) | passable (the open exit) |
| kLaserPowerTile (68), kForeMirrorTile (69), kBackMirrorTile (70), kLaserEmptyPiece (81) | pushable |
| 166–178 (zap-infotron range) | not explodable |
| 128–137 (explosion range) | not explodable, **deadly** |
| 30–37 (borders) | not explodable |
| 39–65 (hardware) | not explodable |
| 10–16 (ports) | isPort, explodable |
| 122–127 (bug range) | passable, **deadly**, **electric** |

`GetPieceKindFromLoc` additionally handles `kPhantomTile` (66): if a **mover sprite** (quark or scissor, matched by `loc` or `newLoc`) is at that cell → `moverSprite` set, `isQuark = (spriteType==kQuarkSpriteType)`, and `isIgnitable = true` (movers chain-explode). Else if Oliver's `loc` is that cell → `isOliver = true`.

Sprite types: `kOliverSpriteType=1, kPushSpriteType=2, kRollSpriteType=3, kFallSpriteType=4, kScissorSpriteType=5, kQuarkSpriteType=6`. Chore types: `kBugChoreType=1, kExplChoreType=2, kExitChoreType=3, kZapChoreType=4, kRDBlinkChoreType=5, kQuarkExplChoreType=6, kLaserChoreType=7`.

`FindMoverAtLocation` walks the sprite list from the head and **returns NULL as soon as the first sprite at that location is not a quark/scissor** — an original-engine quirk (a fall sprite sharing the cell can mask a mover).

---

## 4. Level format & map model

- Level resources: type **`'MRlv'`**, ID **`300 + levelIndex`** (`kLevelBaseID = 300`; levelIndex ≥ 1).
- Grid capacity: `kMaxLevelSize = 120` × 120 (`kMaxLevelSizeSquared = 14400`).
- Resource layout: header struct, then three RLE-compressed `short` arrays.
  - Header `OldExtraLevelInfo` (untimed): `short reqInfo; short levelWorld; char levelTitle[256]; char levelInfo[256]; short phCount; short pvCount; short kindCount;` (title/info are Pascal strings converted with `P2CStr`).
  - If `reqInfo < 0`, the level uses the **extended** header `ExtraLevelInfo` which appends: `short actualInfo; short hours, mins, secs; short dataOne, dataTwo, dataThree;`. Then `reqInfo = actualInfo`, the **timer is enabled**, and h/m/s initialize the countdown.
  - The three arrays (lengths `phCount`, `pvCount`, `kindCount` in shorts) decompress to three 14400-entry arrays: icon-`p.h`, icon-`p.v`, and `kind` for every cell.
  - **RLE scheme** (`RLEDecompress`): stream of shorts; value `128` (`kRLEFlag`) means "next short = run length, following short = value repeated runLength times"; value `129` (`kRLEEnd`) terminates; any other value is copied literally. (This means literal cell values 128/129 cannot appear; editor tile coordinates keep values < 128.)
- Unflattening: index runs x-fastest: `level[x][y]` with `x=0..119` inner. `level` is indexed **[col][row]**; the runtime `tiles` map is **[row][col]**.
- Tile ID from icon point: `tileID = p.v * kLevelTilesWide + p.h` with **`kLevelTilesWide = 10`** (`kLevelTilesDeep = 20`): the editor icon sheet is 10 tiles wide.
- `LevelPoint` also carries `trig, trigP, state, kind, levelID` fields — loaded but **unused during level play** (they matter only for the world map format, which shares the structure).
- **Level size detection** (`GetLevelSize`): if `level[0][0]` is the top-left border icon (`p.v==3 && p.h==0`, i.e. tile 30), scan across row 0 until the top-right border icon (`p.v==3 && p.h==2`, tile 32) → width; then scan down that column until bottom-right border icon (`p.v==3 && p.h==7`, tile 37) → height (both inclusive, +1). If cell (0,0) is not a top-left border, the size defaults to 120×120.
- **Border handling:** there is no bounds checking anywhere in play logic (`tiles[row±1][col±1]` freely). Levels are expected to be fully enclosed by border/hardware (non-explodable) tiles. A port must re-implement this assumption or add clamping.
- Load-time conversions (`InitialTileAction`): `kOliverTile` (9) → record `oliverStartLoc`, cell becomes `kBlankTile`; `kBugTile` (8) → allocate a Bug chore at that cell, cell becomes `kCircuitTile`.
- After the sprite world is built: quark/scissor tiles are converted into mover sprites (`MakeMovers`, scan row-major rows 0..v-1, cols 0..h-1), matching tile 20→quark, 21→scissor(up), 22→scissor(right), 23→scissor(down), 24→scissor(left). The underlying tile cell is blanked once the mover departs (or left as-is if boxed in, with the sprite drawn on top).
- `infotronCount` initialized to `extraInfo.reqInfo`; `redDiskCount = 0`.

---

## 5. The update loop (Level_IdleBack, MRPlay.c)

Order per idle call:
1. `KeepMusicGoing()`.
2. **One-time initialization** (first idle): `InitialFalls` — scan the whole grid row-major and call `DrawLevelTile(row,col,kBlankTile)` for every already-blank cell, triggering all initial falls/rolls; then `MakeMovers` creates quark/scissor sprites. (`doneInitialFalls` flag.)
3. If paused → return (nothing runs; pause is toggled by the pause key on key-up; pause dims the screen, shows `kGamePausedPict` (162), fades music, and freezes the timer via `timerTickDiff`; unpausing clears all held-key state).
4. If `oliverSprite == NULL`: if `wonLevel` → `doneWithLevel = true`; else if no replay dialog yet → `MakeReplaySprite` (death/“play again?” dialog, picture 165, with yes/no buttons; `y`/`Y`/Return/Enter = replay, `n`/`N` = back to map; mouse clicks tracked too; cursor shown).
5. If the death dialog exists and the mouse is inside it → return (freezes the world under the dialog).
6. `SWProcessScrollingSpriteWorld` → all sprite move procs (grid-aligned decisions) + offsets + time-based frame procs; `SWAnimateScrollingSpriteWorld` → render.
7. If a frame occurred → `HandleAllChores`: snapshot `qSize`, dequeue that many chores, run each chore's process proc; if it returns true re-enqueue at tail, else deallocate. FIFO order; queue capacity `MAX_QUEUE_SIZE = 2048` (ring buffer, no overflow check).
8. **Timer** (only if `cb->timer`, i.e. extended levels): every ≥60 ticks decrement one second (with carry from mins/hours). When secs would go below 0 and mins==0 and hours==0 → `KillOliver(false)` and disable the timer. During the last 10 seconds (secs ≤ 10, mins==0, hours==0) play `kNoMoreTimeSnd` (ID 128, from the main resource file) each second with proximity `secs * 1.5` (gets louder as time runs out). Redraw the time display.

Key handling (`Level_KeyBack`, uses key-up/-down events): a 4-deep stack `theKeys[kMaxLevelPresses=4]`; a fresh press is inserted at index 0 (most recent key wins), releases remove the entry. `theKeys[0]` is the current movement direction. Also: pause key toggles pause on key-up; suicide key (`prefs.suicideKey`, default ESC `'\33'`) → `KillOliver(false)` instantly; special key (`prefs.specialKey`, default space `' '`) sets `specialKeyDown` while held. Default movement keys: `'8'`/`'2'`/`'4'`/`'6'` (up/down/left/right). Every keystroke refreshes `lastPressTime` (idle-animation timer).

---

## 6. Player (Oliver) mechanics (`_move_Oliver`, MRPlaySprite.c)

Runs each time Oliver's sprite is grid-aligned (every 4 frames while walking):

1. If dead → return.
2. Read `curTile = tiles[row][col]` (his own cell). If it `isDeadly` (explosion tiles 128–137, active bug tiles 122–127) → `KillOliver(isElectric)` (electric = special shock animation) and return.
3. **Determine deltas** (`_move_Oliver_DeltasFromKeys`):
   - If `lastWasPort` (he just entered a port) → forced to continue with the stored port deltas for exactly one more move; no input read this move.
   - Else if the special key is held (and not `denySpecial`) → zero out his move deltas and run the special-key state machine (§6.1). If it returns true, no movement occurs this move.
   - Else deltas from `theKeys[0]`: left `colDelta=-1`, right `+1`, up `rowDelta=-1`, down `+1`; no keys → `(0,0)`.
4. Clear `specialAnim` and `oliverDiskDrop`; record motion for animation.
5. Draw `kPhantomTile` at his current cell (marks occupancy) — **skipped** if his current tile is a port (10–16) or an rd-blink tile (179–180), so those tiles aren't destroyed.
6. Read `newTile = tiles[row+rowDelta][col+colDelta]`, get its kind; if it's a port, recompute `isPassable` via `GetPortPassable` (§12).
7. Resolve in priority order:
   - `newKind.isDeadly` → `KillOliver(newKind.isElectric)`. (Walking into an explosion or active bug is fatal.)
   - `newKind.isPushable && SafeToPush(...)` → push (§10).
   - `newKind.isPassable` → move (§6.2).
   - else → blocked (§6.3).
8. `oliverCurLoc = theTag->loc` (used by sound proximity/panning).

### 6.1 Special key ("snap without moving", red disk drop)
While the special key is held (`_move_Oliver_SpecialKeyDown`), Oliver stands still. State:
- On entry: `specialAnim = true`, `startAnim = TickCount()`, `didZap=false`; `allowDiskDrop = (curTile == kBlankTile || kPhantomTile)` (can only drop a disk while standing on empty ground).
- **Zap (snap):** if a direction key is held and no zap is in progress: target cell = adjacent cell in that direction. If the target tile `isEdible` (circuit 0, infotron 19, red disk 5): set the directional zap pose frame (`kZapUp/Left/Right/DownBubFrame`), allocate a **Zap chore** at the target, set `didZap`, freeze. If NOT edible: `zappedBlankPiece = true` — Oliver does nothing and **will not move** until either that direction key or the special key is released (`specialKeyRelease` mechanism; per comment: "dave powell suggested that you shouldn't move after a zap if your special key is still held down").
  - **Zap chore** (`_process_Zap`, `kZapDelayTicks = 3` between steps): animates the target cell through its zap tile sequence — infotron 166→178, red disk 146→153, circuit 156→162 — then draws `kBlankTile` (cascade check fires) and calls `EatPiece(origTile)` (so the infotron/red-disk credit lands ~0.5–0.7 s after the zap starts).
  - After a zap, Oliver's pose is frozen for `kZapFreezeTicks = 15` ticks; holding the same direction key re-zaps repeatedly (state machine resets `didZap` after the freeze expires).
- **Red disk drop:** with NO direction key held, if special has been held > `kRedDiskDropDelay = 30` ticks, `redDiskCount > 0`, and `allowDiskDrop` → enter the RD anim state (`kRDAnimState`, "ExLax" animation frames 83–84). Once held > `kRedDiskDropFinish = 90` ticks → drop: `redDiskCount -= 1`, play `kRedDiskReadySnd` (134), allocate an **RD-blink chore** at Oliver's cell, set `oliverDiskDrop` (one drop per press), clear `specialKeyDown`.
  - **RD-blink chore** (`_process_RedDiskBlink`): every `kRDBlinkDelayTicks = 20` ticks draws alternating tiles 179/180; after `kRDBlinkCount = 12` steps (240 ticks = 4 s) → `StartExplosion(loc, ignite=true)` (3×3 blast). Oliver may stand on / walk over the blinking disk (his phantom drawing skips rd-blink tiles; when moving off it the cell is not blanked).
  - If `redDiskCount == 0` after 30 ticks → special anim cancels.

### 6.2 Moving into a passable cell (`_move_Oliver_PassPiece`)
- Move deltas = `±kSpriteMoveDelta` (8 px). `loc` updates to the destination immediately (Oliver "occupies" the destination from the start of the move).
- If destination tile is `kBlankTile` **or `kInfotronTile`** → immediately overwrite destination with `kPhantomTile` (an infotron being eaten is removed from the grid at move start, so it can't also fall). Circuit/red-disk tiles are NOT phantomized until Oliver arrives (his next aligned move-proc draws the phantom).
- If destination `isPort` → set `lastWasPort` and store the deltas; play `kPortUseSnd` (130) at fixed proximity 8. (Port traversal = two forced consecutive moves; see §12.)
- Else if destination `isEdible` → `EatPiece(newTile)`:
  - `kInfotronTile`: `infotronCount -= 1` (clamped ≥ 0). If it reached exactly 0 → `kGotLastInfotronSnd` (135) + `OpenAllExits`; else `kGotInfotronSnd` (128). HUD updated.
  - `kRedDiskTile`: `redDiskCount += 1`, `kGotRedDiskSnd` (133), HUD updated; **if `redDiskCount > 9` → `KillOliver(false)`** (carrying more than 9 red disks is fatal).
  - `kCircuitTile`: nothing (no sound, no counter) — eating base is silent in this code.
- Else if destination is `kLastExitTile` (145, open exit) → `kEndLevelSnd` (151), `wonLevel = true`, stop play clock, timer off, Oliver's sprite removed (`oliverSprite = NULL`) → next idle ends the level.
- Finally blank his origin cell via `DrawLevelTile(row,col,kBlankTile)` (fires cascade) — skipped if origin was a port or an rd-blink tile.

### 6.3 Blocked (`_move_Oliver_NoPassPiece`)
Move deltas zeroed. Bump side effects:
- Bumping a `kTerminalTile` (7) → `ExplodeYellowDisks` (§13).
- Bumping a `kPhaseTermTile` (73) **in Infotron terrain** → `PhaseHardware` (§15).
- Bumping a `kPhantomTile` that belongs to a mover (quark/scissor) → **Oliver dies** (`KillOliver(false)`) and an igniting explosion starts at the mover's cell (both die). Bumping a phantom reserved by a falling/pushed piece = simply blocked.

### 6.4 Death (`KillOliver(oliverSprite, electric)`)
- Ignored if already dead. Sets death anim state; timer disabled; `stopPlayTime` recorded.
- Animation choice: electric → shock frames (72–78). Otherwise random: `GetRandom(1,9) <= 3` → fade death (frames 30–39); else `GetRandom(0,9) < 5` → bubble death (40–49); else static death (50–59). (Frame proc advances one frame per 100 ms tick of the Oliver sprite; after final frame the sprite is removed and `oliverSprite=NULL` → replay dialog.)
- Explosion: if Oliver's own cell already holds an explosion tile (128–137) → `IgniteExplosion` (3×3 around him, skipping center); else `StartExplosion(his loc, ignite=true)`.
- `NextSoundPriority(); PlaySoundID(kOliverDiedSnd)` (137).

Ways Oliver dies (complete list): standing on / walking into deadly tiles (explosions, zapping bugs — bug = electric death); a falling piece coming to rest in his cell (§9); a quark or scissor stepping to an adjacent cell that he occupies (§11); walking into a mover's phantom; a laser beam entering his cell (§14); carrying a 10th red disk; timer reaching 0; suicide key.

---

## 7. Idle/boredom animations (frame proc, exact tick thresholds)

From `lastPressTime` (any key event resets):
- `kSadStartTicks = 5400` (90 s) → `kSadEndTicks = 5430`: sad face (frame 81).
- `kFUStartTicks = 5430` → `kFUEndTicks = 5490`: "f*** you" frame (82).
- `kEurekaStartTicks = 5490` → `kEurekaEndTicks = 5520`: eureka frame (80).
- After `kYawnDelay = 5520` (92 s): yawn sequence frames 60–71, then holds the last frame.
(As coded, entering the yawn state at 90 s sets frame to `kYawnBubFirstFrame` but the sub-proc shows the sad/FU/eureka frames until 92 s.)

Normal animation states (`kStaticAnimState=0, kMovingAnimState=1, kDeathAnimState=2, kZapAnimState=3, kRDAnimState=4, kPushAnimState=5, kYawnAnimState=6`):
- Static/bubbling: frame 0 base; picks a random axis 0–3, shows frame `1+2*axis` then `2+2*axis`, then back to base (the classic 4-direction bubble idle: frames 1–8).
- Walking: left cycle 9,10,11; right cycle 12,13,14 (3-frame cycles; up/down keeps last horizontal facing).
- Push poses: left 16–17, right 18–19, down 20–21, up 22–23 (2-frame ping via `_frame_Oliver_Push`; a `pushHack` flag plays one more push frame after the push ends).
- Zap poses: left 24, right 25, down 26, up 27 (held for 15 ticks).
- RD-drop "ExLax" loop: 83–84.

---

## 8. Gravity, falling, rolling (core physics)

### Trigger: blanking a cell
`DrawLevelTile(..., kBlankTile)` runs these checks in order, **starting at most one motion** (first match returns). If Oliver's `loc` equals the blanked cell, no checks run.
1. `Blank_CheckFall(row-1, col, +1)`: if the piece **above** has `canFall && fallRowDelta==+1` → it starts falling down.
2. `Blank_CheckFall(row+1, col, -1)`: if the piece **below** has `canFall && fallRowDelta==-1` (up-zonk) → it starts falling up.
3. `Blank_CheckRoll(row-1, col, +1)`: if the cell above the hole is blank, then a `canRoll`/`fallRowDelta==+1` piece diagonally **up-left** of the hole resting on a **rounded** piece rolls right (into the cell above the hole, then falls); else the piece **up-right** rolls left. (Both sub-checks inside; left neighbor gets priority.)
4. `Blank_CheckRoll(row+1, col, -1)`: mirror case for up-zonks below.
5. `Blank_CheckSideRoll(row, col, -1)`: the piece directly **left** of the hole, if `canRoll`, the cell below the hole (in that piece's fall direction) is blank, and the piece below it is rounded → rolls right into the hole.
6. `Blank_CheckSideRoll(row, col, +1)`: same for the piece to the **right** (rolls left).

Pieces that fall: zonk (down), up-zonk (up), infotron (down), green disk (down; cannot roll). Pieces that roll: zonk, up-zonk, infotron. **Rounded** (things roll off them): chips (25–29), blank chip (38), zonk, up-zonk, infotron.

### Fall sprite (`MakeSprite_Fall`, `_move_Fall`)
- On creation: `kPhantomTile` drawn at the destination cell (via `DrawLevelTile`) and at the source cell (via raw `SWDrawTile`, no cascade). Move delta = `fallRowDelta*8` px. A `firstFall` flag makes the first aligned move-proc call a no-op — the piece visibly hesitates one alignment cycle, "gives oliver some lag-time to beat the bullet".
- Each cell entered: blank the cell it left (`DrawLevelTile` → cascades, so columns of zonks follow), look at the next cell:
  - Next cell blank → phantomize it and keep falling.
  - Otherwise **land**: play `kThudSnd` (129) at *half* Manhattan-distance proximity (louder), write own tile ID at rest cell, then:
    - If the blocking piece `isIgnitable` (green disk, or mover-phantom) → `StartExplosion(blockerCell, ignite=true)`.
    - If the **falling piece itself** `isIgnitable` (green disk) → `StartExplosion(restCell, ignite=true)` (green disks explode on landing).
    - If the blocking cell is Oliver's `loc` → `KillOliver(false)` (a piece coming to rest on Oliver's head kills him).
    - If the blocking tile is `kPhantomTile` → `HitBottomCheck`: if a mover (quark/scissor) is there → igniting explosion at the mover's cell (falling pieces crush enemies; quark leaves an infotron via the quark-explosion path, scissors leave nothing).
    - If the piece `canRoll` and the blocker `isRounded` → try to roll: uses `cb->nextRollDirection` (initialized **−1 = left**) first, then the opposite; on any successful roll `nextRollDirection` flips sign (global alternation across the level). Roll target requires: target side cell blank AND the cell diagonally below-target blank AND (for the primary check) Oliver not standing in the way (`_sprite_CanRoll` with `checkOliver=true` blocks a roll into a column where Oliver's `loc.h == col+dCol && loc.v == row`).
- Fall speed = 8 px/frame = 4 frames/tile.

### Roll sprite (`MakeSprite_Roll`, `_move_Roll`)
- On creation: phantom at destination (via `DrawLevelTile`), blank at source (raw `SWDrawTile`, no cascade), moves horizontally at 8 px/frame, and is offset immediately (no hesitation).
- Rolling zonks animate through tiles 181–184 (up-zonks 185–188); rolling right uses frame order first→last, rolling left uses first, last, last−1, last−2. Infotrons keep a single frame.
- On arrival: write own tile; blank the source cell via `DrawLevelTile` **only if it still reads `kBlankTile`** (fires cascade); then if the cell below (fall direction) is blank → start a fall; if it's a phantom → `HitBottomCheck` (crush mover). No landing thud for a pure roll.

Rolls triggered from pushes (`_move_Push`) roll in the push direction if the piece lands on a rounded piece (no Oliver check, `checkOliver=false`).

---

## 9. (reserved)

*(merged into §8)*

## 10. Pushing (`SafeToPush`, `MakeSprite_Push`, `_move_Push`)

Pushable pieces: zonk (17), up-zonk (18), yellow disk (4), green disk (3), gold disk (6), blank chip (38), laser power (68), fore mirror (69), back mirror (70), spent battery (81).

`SafeToPush(h, v, hDelta, vDelta)` (h,v = Oliver's cell, deltas = his direction):
- Zonk, up-zonk, green disk: only horizontal (`vDelta != 0` → false). All others push in all 4 directions.
- The cell beyond the pushed piece (`tiles[v+2*vDelta][h+2*hDelta]`) must be `kBlankTile` — **except for the gold disk**, which may also be pushed into `kCircuitTile`.

There is **no push delay**: the push starts on the first aligned move-proc with the direction held (unlike Supaplex's hold-to-push). Push motion is half speed (4 px/frame; 8 frames/tile) for both Oliver and the piece, and `kPushSnd` (141) plays.

Mechanics: a push sprite is created at the pushed piece's cell with `newLoc` two cells over; the target cell is phantomized (`DrawLevelTile`) — except a gold disk entering circuit (only phantomized if blank); the pushed piece's own cell is pre-blanked with raw `SWDrawTile` **only** for gold disk/zonk/up-zonk (comment: using the cascading draw here would make "all hell break loose"); Oliver's origin is blanked normally; Oliver's `loc` becomes the pushed piece's old cell; push pose frames set by direction.

On the pushed piece reaching its target (`_move_Push`):
- Write its tile at rest.
- `kLaserPowerTile` → `_move_Push_LaserCheck`: if any 4-neighbor is `kLaserTile` → `ActivateLaser` on each such neighbor and the battery becomes `kLaserEmptyPiece` (81).
- If the piece `canFall` and below is blank → start a fall (green disk pushed off a ledge falls, then explodes on landing).
- If the piece `canRoll` and rests on a rounded piece → roll continues in push direction if clear.
- Oliver: `isPushing=false`, `pushHack=true` (one extra anim frame).

**Gold disk special (`_move_Push_GoldDisk`)**: the disk travels at full 8 px/frame on its own. At each cell boundary: look at the next cell in the push direction; if that piece `isExplodable && !isIgnitable` → start a **non-igniting** explosion at the disk's current cell ("keep a trail of fire going behind you"), play `kExplodeSnd` (131), and keep moving (it plows through explodable material, leaving single-cell explosions behind). Otherwise (non-explodable wall, or an ignitable piece) → start an **igniting** (3×3) explosion at the current cell and the disk is destroyed. I.e., the gold disk is a fire torpedo ending in a bomb blast.

---

## 11. Enemies

Both enemies use the same wall-following scan; both move at 4 px/frame (8 frames per tile); both reserve their next cell with `kPhantomTile` and blank the cell they left; both track `loc` (current) and `newLoc` (next) so collision checks cover both cells.

**Relative-direction scan** (`_move_Quark`, `_move_Scissor_AttemptTurn`, `_make_MoverFacing`): starting from the mover's current `facing`, test relative directions in this exact order: **relative LEFT, then STRAIGHT, then relative RIGHT, then BACK** (implemented as checkDir = Left(4), then RotateClockwise wraps to Up(1)=straight, Right(2), Down(3)=behind; when rotation returns to Left the search fails). First direction whose target tile is exactly `kBlankTile` wins; `facing` becomes that absolute direction. If no direction is free: deltas (0,0); the quark additionally rotates its facing clockwise once per decision (spins in place); the scissor just idles.

**Kill check during the scan:** for each tested direction whose tile is `kPhantomTile`, if that phantom is Oliver (`GetPieceKindFromLoc().isOliver`) → `KillOliver(false)` and the mover explodes at its own cell — quark via `StartQuarkExplosion(ignite=true)`, scissor via `StartExplosion(ignite=true)` — and the move proc aborts. Note this triggers on *any* scanned direction, so a mover can detonate on Oliver in a cell it wasn't even going to enter (it checks left/straight/right/back in order until a blank is found).

### Quark (`kQuarkSpriteType`)
- Starts facing **Up**, so initial preference is left, up, right, down (comment confirms).
- 9 animation frames (tiles 189–197), 66 ms frame time, cycling.
- Death: crushed by falling/rolling piece, caught in an explosion (its phantom is `isIgnitable`), laser, Oliver-contact suicide. A quark always dies with a **quark explosion**, which after its animation leaves an **infotron** in the center cell (§ Explosions).

### Scissor (`kScissorSpriteType`)
- Initial facing from its tile (up/right/down/left). Same left-wall-following preference.
- If the chosen direction is **not straight**, the scissor stops (move delta 0) and enters a **turning state** instead of instantly changing direction: `clockwiseTurn = (checkDir == relative Right)`; `turningTo` = the found direction, except a reversal (relative Back) first targets `RotateCounterClockwise(oldFacing)` (turns 90° CCW, then continues turning). Turn stepping: every `kScissorTurnTime = 2` ticks, `turnedCount` 0→1 shows the diagonal transition frame, then `_move_Scissor_FurtherTurn` sets `facing = turningTo` and re-checks straight-ahead: if blank → resume motion (phantom placed, deltas set, sprite nudged); else rotate `turningTo` one more step (CW or CCW per `clockwiseTurn`) and repeat. So a boxed-in scissor keeps rotating in place, 2 ticks per 90° step.
- Animation (`_frame_Scissor`): 16 frames = 4 facings × (3 open/close frames + 1 diagonal turn frame). While moving with stable facing, the 3-frame cycle ping-pongs (`internalFrame` 0..2, direction flips at ends), base frame = `(facing-1)*4`. During a turn, frame = `(facing*4)-1` (CW) or `((facing-1)*4)-1`, wrapping to 15 (CCW), then the closed frame of the new facing.
- Death: same as quark, but a **normal explosion** — scissors leave nothing (blank).

### Bugs (electrified base) — chore, not sprite
Bug cells look like circuit (0). Chore state machine (`_process_Bug`):
- If the cell's tile is neither circuit nor in 122–127 (e.g., eaten, zapped, exploded, or covered by Oliver's phantom) → the chore silently dies; **the bug is permanently gone** (walking over a dormant bug destroys it).
- Init/waiting: `lastTime = TickCount() + kBugZapDelayTime + 15*GetRandom(0,16)` where `kBugZapDelayTime = 120` — i.e. dormant for 120–360 ticks (2–6 s), uniformly random in 15-tick steps.
- When the wait expires: draws `kFirstBugTile` (122), plays `kBugZapSnd` (140, positional), enters zapping.
- Zapping: every ≥ `kBugAnimDelayTicks = 5` ticks the tile advances 123→127; after 127 it reverts to `kCircuitTile` and re-enters waiting with a new random delay. Active zap ≈ 30+ ticks (~0.5 s).
- While showing 122–127 the cell is `isDeadly + isElectric` (still passable): Oliver walking into it, or standing on it when it fires, dies with the **electric shock** death animation. Bugs are explodable (they die in blasts).

---

## 12. Ports

Tiles 10–16. Port passability (`GetPortPassable(hLoc,vLoc,hDelta,vDelta)` where (hLoc,vLoc) is the port's cell):
- The cell **beyond** the port (`tiles[vLoc+vDelta][hLoc+hDelta]`) must be exactly `kBlankTile` (not circuit, not infotron, not phantom).
- Direction rules: LeftPort ⇢ only `hDelta == -1`; DownPort ⇢ `vDelta == +1`; RightPort ⇢ `hDelta == +1`; UpPort ⇢ `vDelta == -1`; LeftRightPort ⇢ `vDelta == 0`; UpDownPort ⇢ `hDelta == 0`; FourWayPort ⇢ always.
- (`hDelta==0 && vDelta==0` returns passable — not reachable in practice.)

Traversal is a forced two-cell move: entering the port sets `lastWasPort` and stores the deltas; on the next aligned decision the same deltas are applied unconditionally (input ignored for that move). `kPortUseSnd` (130) plays at proximity 8. Oliver renders **under** tiles (`SWSetSpriteUnderTiles(oliver, true)`) and port tiles are loaded with a solid mask, so he visually passes beneath the port. Oliver never phantomizes or blanks a port cell (both writes are skipped when `curKind.isPort`), so ports are never destroyed by walking. Ports ARE explodable (a blast removes them). Only Oliver can use ports (movers require `kBlankTile`; falling pieces stop on them).

There are **no gravity-change or "special" ports** in this code. The gravity variety comes from up-zonks (`fallRowDelta = -1`) instead.

---

## 13. Terminals & yellow disks

- Yellow disk (4): pushable in all 4 directions, does not fall, not rounded, explodable, not ignitable by itself (it is detonated only via terminal or by being caught in a blast — as a plain explodable piece it just vanishes in someone else's blast without its own 3×3... **however** the terminal detonation calls `StartExplosion(loc, ignite=true)` so each disk produces a full 3×3 igniting explosion).
- Terminal (7): not passable, **not explodable** (indestructible). When Oliver bumps it: `ExplodeYellowDisks` scans the whole grid (cols outer, rows inner) and starts an igniting explosion at **every** `kYellowDiskTile` simultaneously. Sound: if ≥1 disk exploded → `kTermUseSnd` (142), else `kTermBadSnd` (143); either sound is rate-limited to once per `kTermSoundDelay = 60` ticks (`lastTermSound`).
- Terminal tiles animate continuously (tiles 110–121 via the global tile-image cycler).

---

## 14. Lasers & mirrors (Infotron terrain only)

Pieces: emitter `kLaserTile` (67, blocks movement, explodable), battery `kLaserPowerTile` (68, pushable), spent battery `kLaserEmptyPiece` (81, pushable), mirrors `kForeMirrorTile` (69, "/") and `kBackMirrorTile` (70, "\\"), both pushable.

Activation: when a pushed battery comes to rest with any 4-neighbor being `kLaserTile`, every adjacent emitter fires (`ActivateLaser`) and the battery becomes `kLaserEmptyPiece`. `ActivateLaser(v,h)` fires 4 beams: for each direction `(dv,dh) ∈ {(1,0),(-1,0),(0,1),(0,-1)}`, if the adjacent piece `isExplodable` and is not another battery → enqueue a Laser chore at that adjacent cell.

Laser chore (`_process_Laser`) — 3 modes (`kLaserInitMode=0, kLaserDrawnMode=1, kLaserSpreadMode=2`; `kLaserAfterDrawTicks = 1`, `kLaserAfterSpreadTicks = 1`):
1. **Init** (immediately): at its cell — if `isIgnitable` → igniting explosion; else if Oliver → `KillOliver(false)`; else if explodable and not blank and not an existing beam/light piece (IDs (67,81) exclusive and (81,93]) → non-igniting explosion. Then draw the beam graphic and handle mirrors:
   - Fore mirror "/" (69 or its lit variants 89/90): beam going right or down draws tile 89, else 90; deltas transform `dh' = -dv, dv' = -dh` (reflection).
   - Back mirror "\\" (70 or lit variants 91/92): beam going left or down draws 92, else 91; deltas swap `dh' = dv, dv' = dh`.
   - Otherwise: remembers old tile as blank, draws 88 (vertical) or 87 (horizontal).
2. **Drawn → spread** after >1 tick: `ActivateSingleLaser` continues one more cell in the (possibly reflected) direction — the beam propagates cell-by-cell, one cell per ~2 chore steps, stopped by non-explodable tiles (borders, hardware, terminals) and batteries.
3. **Spread → cleanup** after >1 more tick: restore the remembered tile (blank or the mirror), chore ends. Beams are transient.

---

## 15. Phase hardware (Infotron terrain only)

At load (`InfoWorld_PhaseTerm`): scan for `kEmptyPhaseTile` (71) and `kFullPhaseTile` (72); record each location and state (`kPhasedOut = 0`, `kPhasedIn = 1`); every `kEmptyPhaseTile` cell is rewritten to `kBlankTile`.

Bumping a phase terminal (`kPhaseTermTile`, 73; not passable) calls `PhaseHardware`: if ≥ `kPhaseTermDelay = 45` ticks since last toggle, **every** phase block flips: phased-in blocks become `kBlankTile` (via `DrawLevelTile` → cascades, things can fall into the gap), phased-out become `kFullPhaseTile` (solid, non-explodable). No occupancy check is made — a block can be drawn over whatever is in the cell (including, in principle, Oliver's phantom; the original does not handle this).

---

## 16. Explosions

**Normal explosion** (`StartExplosion(loc, shouldIgnite)`):
- Target must be `isExplodable` (excluded: borders 30–37, hardware 39–65, terminals, full-phase blocks, zap-infotron tiles 166–178, and already-exploding cells 128–137).
- If the target is a quark (mover phantom with `isQuark`) and `shouldIgnite` → reroutes to a quark explosion.
- Effects: allocate an Explosion chore (`explTileID = kFirstExplTile = 128`, `igniteTileID = kIgniteExplTile = 131`), immediately draw tile 128 at the cell (reserving it — deadly, non-explodable), play `kExplodeSnd` (131) positionally if igniting; if the cell held a mover sprite → dispose the sprite; if the cell was Oliver → `KillOliver(false)`.
- Chore (`_process_Explosion`): every ≥ `kExplDelayTicks = 3` ticks draw the current explosion tile and increment; when the drawn tile **is 131 (`kIgniteExplTile`)** and `shouldIgnite` → `IgniteExplosion`: for each of the **8 neighbors** (3×3 minus center) call `StartExplosion(neighbor, neighborKind.isIgnitable)` — i.e., every explodable neighbor is destroyed with a plain explosion, and neighbors that are *ignitable* (green disks, movers) chain into full igniting explosions of their own. After tile 137 the cell becomes `kBlankTile` (cascade check fires — debris can fall into the crater). Full animation: 10 frames × ≥3 ticks ≈ 0.5–0.7 s; chain waves propagate at the ignite frame (4th).
- So `shouldIgnite=true` = a 3×3 blast; `shouldIgnite=false` = single-cell destruction.

**Quark explosion** (`StartQuarkExplosion` / `_process_QuarkExplosion`): same structure but runs the zap-infotron tiles **backwards** from `kLastZapInfotronTile` (178) **down** to 166, igniting neighbors (via `IgniteQuarkExplosion` → recursively quark-style for quarks, normal for others) when the drawn tile is `kZapInfotronTile25` (171). When the countdown passes below 166 the cell becomes **`kInfotronTile`** and `CheckForMotions` runs (3×3 scan re-triggering fall checks on any blank neighbors) — **an exploded quark turns into a fresh infotron** (which can then fall). Sound: `kQuarkExplodeSnd` (132).

Ignitable pieces (`isIgnitable`): green disks, and (dynamically) mover phantoms. These chain 3×3 explosions.

Trigger sources for igniting explosions: Oliver's death, red-disk timer, yellow disks via terminal, green disk landing / being landed on, movers being crushed or touching Oliver, gold disk's final blast, lasers on ignitable pieces.

---

## 17. Exit & level completion

- `EatPiece` on the **last** required infotron (`infotronCount` hits exactly 0) → `OpenAllExits`: for every `kExitTile` (2) on the grid, an Exit chore animates the cell 138→145 at ≥ `kExitDelayTicks = 3` ticks per frame; the final tile `kLastExitTile` (145) is passable. Extra infotrons beyond the quota can still be eaten (count clamps at 0, normal got-infotron sound).
- Walking into tile 145 wins: `kEndLevelSnd` (151), `wonLevel = true`, play clock stopped, Oliver's sprite removed; next idle sets `doneWithLevel`. `RunGameLevel` then marks `player->passedLevel[levelIndex-1] = true` and credits elapsed play time (ticks minus pause time) to the player profile via `AddTicksToPlayer` (accumulates days/hours/mins/secs; 216000 ticks/hour).
- There are **no lives, no score, and no time bonus** anywhere in this code. Progress = per-level passed flags + total time played + switch states on the world map. Death → optional instant replay of the same level (return codes: `kPlayErrorReturn=1, kPlayQuitReturn=2, kBackToMapReturn=3, kPlayAgainReturn=4`).

---

## 18. Time limit

Only levels with the extended header have a timer (`cb->timer = true`) initialized from `hours/mins/secs`. Countdown: 1 s per 60 ticks (drift-corrected: `timerTicks = TickCount() - leftoverDiff`); pausing preserves the remainder via `timerTickDiff`. secs→mins→hours borrow. At 0:00:00 → Oliver dies (normal, non-electric death). Final 10 seconds: per-second warning sound `kNoMoreTimeSnd` (128, main res file) with proximity `secs * 1.5`. The timer is turned off when Oliver dies or wins. Timed levels use an alternate stat-bar picture (`kGameStatMResPict` 133) with the h/m/s boxes; untimed use `kGameStatPict` (159).

---

## 19. HUD / level screen

- Screen: 640×480 centered in the window. Right-side stat bar picture; play area = remaining width, optionally inset by `prefs.screenSize` (0 / 75 / 125 px — smaller windows for slow Macs).
- Stat bar contents: bubbling Oliver head (32×32 at offset (4,4); the in-level idle bubbling call is commented out), vertical level title (green lettering, rect (11, 77, 29, 267) relative to bar), infotron mini-icon + remaining count, red-disk mini-icon + carried count (16×16 icons selected by `levelWorld * 16` from strips `kInfoMiniPict` 160 / `kRDMiniPict` 161), and (timed levels) h:mm:ss boxes at rects (5,428,10,440), (12,428,22,440), (24,428,34,440). Numbers drawn in 9-pt Helvetica, RGB (39168, 52224, 52224) on black; counts redraw only when changed.
- Camera: proportional catch-up scrolling. Each frame `scrollDelta = 8 * (spritePos - screenMidpoint) / 100` px on each axis (`kSpriteMoveDelta`/`kSpriteMoveDistance`), giving smooth "snap-back" follow of Oliver; scrolling stops when Oliver is dead. Interlaced mode forces even vertical deltas.
- Sounds are positional: volume scales with Manhattan distance from Oliver (`proximity`, max audible `kMaxProximity = 22`), stereo pan = `(pieceCol - oliverCol) / 2` clamped to ±10.
- Music: `kGamePlayMusic` (2000) loops during play.

---

## 20. World map (meta-game, MRMap.c) — summary

- World resource `'MRwd'` ID 128: raw 120×120 array of `WorldPoint {Point p; short trig; Point trigP; short state; short kind; short levelID}` (indexed [col][row]); tile sheet 18 wide × 20 deep (`kTilesWide=18`), tile size 24 px, 378 tile images, map FPS 30, map sprite speed 4 px/frame (24-px grid → 6 frames/cell).
- Piece kinds are **hard-coded** by icon sheet position (`MakePieceKinds`): sheet rows 0–2 col 1 = level nodes, rows 3–5 col 1 = passed-level nodes, sheet row 6 (cols 0–8) = switches, rows 7–8 = passable path; everything else impassable. (A large alternative table is commented out — this is prototype-stage.)
- Levels: stepping onto a level node records `levelNumber = world[col][row].levelID`; after `kInfoDelayTicks = 15` ticks an info panel pops up (title, description, required infotron count from the level resource header); Return/Enter or the Play button launches the level (`RunMapRoom` returns `kMapPlayReturn(10) + levelNumber`). A player may not walk off an unpassed level node except back the way they came; passed levels display an alternate icon (`p.v += 3`).
- Switches: stepping on a switch tile plays `kMapSwitchSnd` (130) and swaps `p`/`trigP` for every world cell sharing the same `trig` ID (opening/closing routes), with a cross-fade; toggles are persisted per player as a list of thrown trigger IDs (re-thrown = removed — parity toggling on load).

---

## 21. Player input defaults (MRPrefs.c)

up `'8'`, down `'2'`, left `'4'`, right `'6'`, special = space `' '`, suicide = ESC `'\33'`, pause = TAB `'\t'`; music/sound volume 255; options: screenSize {0,75,125}, interlacedMode, progressScreen, compatibleDraws. Movement uses a most-recent-key-wins stack of up to 4 held keys.

---

## 22. Sound ID reference

From `MRSound.h` (terrain-aliased resource files): kGotInfotronSnd 128, kThudSnd 129, kExplodeSnd 131, kQuarkExplodeSnd 132, kGotRedDiskSnd 133, kRedDiskReadySnd 134, kGotLastInfotronSnd 135, kOliverDiedSnd 137, kPauseSnd 138, kUnpauseSnd 139, kBugZapSnd 140, kPushSnd 141, kClickSnd 144, kTickSnd 145, kDingSnd 146, kLogoSnd 150, kEndLevelSnd 151. From `MRPlayConstants.h`: kPortUseSnd 130, kTermUseSnd 142, kTermBadSnd 143, kNoMoreTimeSnd 128 (main file). Music: interface 1000, gameplay 2000, map room 5000.

---

## 23. Visibly unfinished / TODO / quirks in the original (port decisions needed)

1. `LevelCBInfo.stepOn` ("step key") is declared but never referenced — an unimplemented step-one-frame feature.
2. `LevelCBInfo.pieceKinds[20][10]` is declared but never used in play code (vestigial copy of the map-room concept).
3. `kMaxRemoveFrames = 6` and `kMaxTileBoundaries = 32` are defined but unused; `AddTileBound` calls are all commented out.
4. `NoSuchLevel` guard with comment "Won't be a problem in the actual completed version of the game, but for now..." — level set incomplete at the time.
5. On losing a level, restoring the map position is commented out (`pd->mapLocation = pd->previousLocation`).
6. In-level bubbling Oliver HUD animation deliberately disabled ("two bubbling olivers at once is (1) annoying, and (2) slow").
7. World-map `MakePieceKinds` is a hard-coded prototype (most of a richer table is commented out); map/world interactivity is clearly unfinished.
8. `AbsoluteFacingDeltas` contains a typo `defaut:` (harmless — default case empty anyway).
9. `_frame_Oliver_Yawn` state logic is slightly self-contradictory (yawn state entered at 90 s but yawn frames only shown after 92 s; sad/FU/eureka frames shown meanwhile). Port may faithfully reproduce or simplify.
10. `FindMoverAtLocation` stops at the first sprite at a location even if it isn't a mover — can mask a mover sharing a cell with a fall sprite (edge case).
11. Explosions do not cancel in-flight fall/roll/push sprites whose phantoms they overwrite; a landing piece can overdraw an explosion tile. Rare timing edge cases; the original just lives with them.
12. A boxed-in (never-moved) quark/scissor still has its original tile (20–24) in the map; exploding that *tile* doesn't remove the mover *sprite* (mover disposal keys off `kPhantomTile` lookup) — potential ghost-sprite edge case in the original.
13. Zap-tile ranges: only the zap-infotron range (166–178) is marked non-explodable; red-disk (146–153) and circuit (156–162) zap tiles remain explodable — inconsistency in the original.
14. `PhaseHardware` can phase a block into an occupied cell (no crush/death handling).
15. The chore ring buffer (2048) has no overflow protection.
16. `Enq`/`Deq` store raw pointers as `long` — 32-bit assumption (irrelevant to a TS port, but noted).
17. Debug break-in key `'` guarded by `DEBUG_INT_KEY`.
18. `kSixteenConstant 24` (map header) — a joke/leftover constant, unused meaningfully.

---

## 24. Assumptions a port must supply (not in these sources)

- ~~The SpriteWorld library (`SWIncludes.h`, `SWGameUtils.h`) is not in the repo.~~ **SUPERSEDED — see "Follow-up: SpriteWorld semantics verified" below.** The library *is* in the repo (`legacy/murphy/spriteworld/swx-imurphy/swx/`, the SWX/SDL port used by the iPhone build), and the exact classic version the 1997 game targeted (SpriteWorld **2.0**) plus 2.2 and 2.3.1 were retrieved from public archives and checked. All §24 assumptions are now VERIFIED (with corrections noted below); `GetRandom(min,max)` inclusive-uniform is VERIFIED.
- Actual level/world/tile/sound resource data live in resource forks not examined here; graphics sheet layouts are inferred from the load calls in `MakeLevelSWorld` (PICT IDs 128–145 & per-terrain aliases).

## Follow-up: SpriteWorld semantics verified against library source (canon for the port)

**Status: VERIFIED, not assumption.** Sources examined (2026-07-13):

- **In-repo:** `/Users/dave/dev/games/Murphy/legacy/murphy/spriteworld/swx-imurphy/swx/` (SpriteWorld X — the SDL port the iPhone build used; identical byte-for-byte to `swscrolling-iphone/swx/`). Key files: `SpriteWorld.c`, `Scrolling.c`, `SpriteLayer.c`, `SpriteCreation.c`.
- **Classic SpriteWorld 2.3.1** (SourceForge `spriteworld` project, `SpriteWorld-2.3.1.sit.bin`): `Sources/SpriteWorld.c`, `Sources/SpriteLayer.c`, `Utils/SWGameUtils.c`.
- **Classic SpriteWorld 2.2** (vintageapplemac.com `spriteworld-22.hqx`): same files.
- **SpriteWorld 2.0 Pascal interfaces** (files.stairways.com `SpriteWorld-20-Pascal-Dist.zip`): `Interfaces/SpriteWorld.p`, `Interfaces/SpriteLayer.p` — these mirror the 2.0 C headers.

**Which version the original game used: SpriteWorld 2.0 (1997).** Three fingerprints in the game code match 2.0 and *only* 2.0: (a) `MRPlay.c` reads `cb->sw->frameHasOccured` — one "r"; the field is spelled `frameHasOccured` in the 2.0 interface (`SpriteWorld.p:107`) but `frameHasOccurred` in 2.2, 2.3.1, and SWX. (b) `MRPlaySprite.c:201` calls `SWInsertSpriteAfterSprite( cbData->sLayer, newSprite, cbData->oliverSprite )` — the 3-arg 2.0 signature (`SpriteLayer.p:58`); 2.2+ take 2 args (layer dropped). (c) `MRPlayConstants.h` names its frame-time constants in milliseconds (`kSpriteFrameMilli 33`, `kOliverBubbleMilli 100`), matching 2.0's ms-based API. The iPhone-era port compiled against SWX, whose loop is behaviorally identical (see below).

### 1. Frame gate / `frameHasOccured` under `SWSetSpriteWorldMaxFPS(sw, 30)` — AT-LEAST-33ms, not exactly-2-ticks

Verified in all of 2.2, 2.3.1, SWX (identical logic; quoted from classic 2.3.1 `SpriteWorld.c:1534-1544`, same in SWX `SpriteWorld.c:1528-1555`):

```c
if ( (spriteWorldP->runningTimeCount - spriteWorldP->timeOfLastFrame >=
            spriteWorldP->fpsTimeInterval) )
{
    spriteWorldP->frameHasOccurred = true;
    spriteWorldP->timeOfLastFrame = spriteWorldP->runningTimeCount;
}
else
{
    spriteWorldP->frameHasOccurred = false;
    return;
}
```

- `SWSetSpriteWorldMaxFPS` is **millisecond-based in every version, including 2.0** (2.0 header comment: `fpsTimeInterval: Integer; { milliseconds per frame of animation (1000/fps) }`). Implementation (identical in 2.2 `SpriteWorld.c:2195`, 2.3.1 `:771`, SWX `:1050`): `spriteWorldP->fpsTimeInterval = 1000/framesPerSec;` — integer division, so **`kMaxLevelFPS = 30` → `fpsTimeInterval = 33 ms`** (not 33.33; max ≈30.3 logic fps). The spec §1 phrase "2 ticks per frame" is a close approximation but the mechanism is ms, not ticks. Classic versions accumulate `runningTimeCount` in ms from `Microseconds()` (2.2/2.3.1 `SWProcessSpriteWorld`); SWX uses `SWGetMilliseconds()`.
- **The step is at-least-33ms with NO catch-up**: `timeOfLastFrame = runningTimeCount` (snap-to-now, not `+= interval`), so a slow frame permanently loses time — the world never runs multiple logic steps to compensate. If `SWProcessSpriteWorld` decides no frame is due, it sets `frameHasOccured = false` and **returns before touching any sprite** — no move procs, no frame procs.
- `SWProcessScrollingSpriteWorld` = `SWProcessSpriteWorld` then, only if `frameHasOccurred`, the world move proc (camera) + `SWOffsetVisScrollRect` + non-scrolling layers (2.2 `Scrolling.c:270-292`, SWX `Scrolling.c:317-348`). `SWAnimateScrollingSpriteWorld` renders and **returns immediately if `!frameHasOccurred`** (2.2/2.3.1/SWX alike). So the game loop in `Level_IdleBack` (`MRPlay.c:417-420`) — Process, Animate, then `if (cb->sw->frameHasOccured) HandleAllChores(...)` — yields exactly one logic step + one render + one chore-queue drain per 33ms-gated frame. Chores therefore always run **after** all move/frame procs and after render, once per processed frame.
- Sprite-creation defaults (SWX `SpriteCreation.c:273,282`; identical in 2.2 `Sprite.c:107,116`): `frameTimeInterval = -1` (never), `moveTimeInterval = 0`. Murphy never calls `SWSetSpriteMoveTime`, so **every sprite's move proc fires on every processed frame**; Murphy sets per-sprite frame times via `SWSetSpriteFrameTime` (33ms for most sprites, 100ms for Oliver's bubble).

### 2. Per-frame ordering — VERIFIED with one correction

Ordering is **per-sprite interleaved, not two global passes**. In `SWProcessSpriteWorld` (2.2 `SpriteWorld.c:1330-1430`, 2.3.1 `:1560-1671`, SWX `SWProcessSpriteLayer` `:1615-1752`), for each layer in add-order, for each sprite in list order:

1. `nextSpriteP = curSpriteP->nextSpriteP;` — **next pointer captured first** (see race notes below);
2. frame-advance check: if `frameTimeInterval >= 0` and its ms timer elapsed → advance frame index and call `frameChangeProc` (Murphy's `_sprite_SWFrameProc`);
3. move check: if ms timer elapsed (always, for interval 0) → call `spriteMoveProc` (Murphy's `_sprite_SWMoveProc`).

So a given sprite's **frame proc runs immediately before its own move proc within the same frame**, and the whole sequence runs sprite-by-sprite in list order — *not* "all move procs, then all frame procs" as §24 assumed. (For Murphy this correction is mostly benign — frame procs drive idle/bubble animation — but it is canon.)

List order (verified): `MakeOliverSprite` (`MRPlaySprite.c:1755-1810`) creates a fresh layer, adds Oliver as its first (head) sprite, then adds that layer to the world first, `againLayer` second. `_sprite_AllocSWSprite` (`MRPlaySprite.c:171-206`) appends every other gameplay sprite to the tail via `SWAddSprite` (verified tail-append, 2.2/2.3.1/SWX `SpriteLayer.c`), except push sprites: `SWInsertSpriteAfterSprite(sLayer, newSprite, oliverSprite)` splices the new sprite **immediately after Oliver** (verified splice, all versions). Murphy never calls any sprite z-sort/reorder API, so list order = insertion order for a sprite's whole lifetime. **Per-frame order: Oliver → (live push sprite, if any) → all other sprites in creation order; layers: sLayer, then againLayer (death sprite).**

### 3. Same-frame race semantics (the load-bearing quirks)

Because `nextSpriteP` is captured **before** the callbacks run:

- **Push sprites never run on their creation frame.** `MakeSprite_Push` is called from `_move_Oliver_PushPiece` (`MRPlaySprite.c:1228`) inside Oliver's move proc; the loop's already-captured `nextSpriteP` points past the splice point, so the freshly inserted push sprite is skipped and first executes on the **next** processed frame.
- **Tail-appended sprites (falls/rolls/enemies) DO run on their creation frame** whenever the creating sprite is not the current tail — the traversal reaches the new tail through the live `nextSpriteP` chain. This is exactly what the fall sprite's `firstFall` flag defends against: `_move_Fall` (`MRPlaySprite.c:530-533`) does `if (theTag->info.fall.firstFall) { firstFall = false; return; }`, turning that same-frame (and grid-aligned, hence logic-triggering) call into a no-op; the dispatcher `_sprite_SWMoveProc` (`MRPlaySprite.c:60-87`) still applies `SWOffsetSprite(±8px)` afterwards. Net effect: the fall commits its tile-map consequences one aligned cycle (4 frames at 8px per 32px cell) later — `MakeSprite_Fall`'s comment: "this gives oliver some lag-time to beat the bullet, so to speak." A port that steps sprites in the order above (with the capture rule) reproduces this exactly.
- Mover-mask quirk (§23 #10): `FindMoverAtLocation`'s first-sprite-wins scan is over this same insertion-ordered list, so which sprite "masks" a mover in a shared cell is deterministic: earliest-inserted wins (Oliver, then push, then creation order).

### 4. `GetRandom` — VERIFIED inclusive

Classic C implementation, identical in 2.2 `SWGameUtils.c:30-41` and 2.3.1 `:43-54`:

```c
short GetRandom(short min, short max)
{
    unsigned short random;  long range, temp;
    random = Random();                 // QuickDraw Random(), treated as 0..65535
    range = (max - min) + 1;
    temp = (random * range) / 65536;
    random = temp + min;
    return random;
}
```

Inclusive of both endpoints, uniform to within 1/65536. Port canon: `min + floor(u16 * (max-min+1) / 65536)` over a uniform `u16`.

### 5. One residual unknown → PORT DECISION (not fact)

The per-sprite timer comparison operator changed between classic releases: 2.2 uses **`>`** (`SpriteWorld.c:1338,1417`: `runningTimeCount - timeOfLast... > interval`) while 2.3.1 and SWX use **`>=`**. The 2.0 C core (what the 1997 build shipped with) could not be retrieved — only 2.0's Pascal interfaces/compiled libs are archived — so 2.0's operator is unverified. This never affects move procs (interval 0 fires either way) and, given real-clock jitter, rarely affects 33ms frame procs; it can only slightly slow frame-proc cadence in the `>` case when logic frames land exactly 33ms apart. **DECISION for the port: use `>=` (the 2.3.1/SWX behavior, also what the iPhone build ran), i.e. a sprite frame proc with interval N fires whenever ≥N ms of world time has elapsed since it last fired, evaluated only on processed frames.** The world-level frame gate is `>=` in every version examined — that part is fact, not decision.
