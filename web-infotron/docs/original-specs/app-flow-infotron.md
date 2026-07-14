# Infotron (1994 Mac, shipped) — Application Flow & Shell Reference Spec

Purpose: companion to `infotron-reference.md` (the game-*rules* spec). That document covers the
board engine; **this one covers everything around it**: boot sequence, screens, palettes, window
and status-bar geometry, menus, dialogs, level progression, player persistence, sound/music
plumbing, non-gameplay input, and shipped v1.0 ↔ v1.1 differences. Read the rules spec first;
cross-references below use its section numbers (e.g. "RULES §2.1").

## 0. Sources, what is authoritative, and what actually shipped

### 0.1 Source files (Think Pascal, `/Users/dave/dev/games/Murphy/legacy/infotron/src/`)

The final "NO OOP" build (`program NEWInfotron` in `main.p`) consists of exactly these units
(from `main.p`'s `uses` clause plus the class-implementation units):

| File | Role |
|---|---|
| `main.p` | program: boot (`InitInfotron`), event loop (`peckMain`), game frame (`Process1Event`/`HandlePlaying`), menus dispatch, suspend/resume, prefs, quit |
| `everything.p` | board data/drawing, scrolling, status bar, player save/load, level load, chore-list plumbing, `StartNewLevel` |
| `dataIntf.p` | all types/constants/globals |
| `TheChores.p`, `TheGridPieces.p` | chore + piece behavior (RULES spec territory) |
| `InfoWindows.p` | all custom dialogs (name entry, level picker, open/delete, set-keys, help/tutorial, about, registration, error) + `PlayerType`/resource-type constants |
| `CInfoDialog.p` | the PICT-background dialog/scroll-list mini-framework (`CInfoDialog`, `CInfoScrollDialog`, `skipSet`) |
| `InfoSounds.p` | **contains `unit QIST`** — the final sound/music engine ("hybrid of QIST for the music and PSST for the speed. THIS IS NOT QIST.") |
| `MenuBars.p` | menu-bar hide/show via the `MBarHeight` low-memory global hack |
| `PictPal.p` | `SetPictPalette` — hard-coded 256-entry palette used only for the PICT-3000 splash |
| `myRGBColors.p` | the 8 named UI colors, `changeKey` (raw keymap → char), `drawInteger`, `centerString` |
| `QuickDrawProcs` | `DrawPic`/`DrawPicCenterGraf` etc. (PICT plotting helpers) |
| `Picker.p` | Apple's stock Color Picker interface (used only for the `SmallFract` type) |
| `gammaPasLibIntf.p` + `Gamma Pas Utils.lib` | Matt Slot gamma-fade library: `FadeToBlack(speed)` / `FadeFromBlack(speed)` (full-screen gamma ramps), `SetupGammaTools` |
| `ColorOffScreen.p` / `RevisedColorOff.p` | GWorld helpers: `MakeGWorld`, `CopyWorldBits`, `CopyWorldBits2` |

**Not part of the final build**: `InfotronMain.p` + `MainRoutines.p` (TCL/OOP-era stubs),
`InfoQIST.p` (older music-score engine, `µsik` resources — abandoned), `PSST5.p` (older sound
engine), `Infotron.p`, `InfoIntf.p`, all `C*.p` piece/chore files, `NEWprocess1`, `OLDProc1_`,
`sci_quaMovers.p`, `NewChores.p`, `HandlePlaying_old`.

Note: THINK Pascal allows passing a `rect` parameter as four integers in **top, left, bottom,
right** order (the code comments confirm: `{TOP LEFT BOTTOM RIGHT ---> V H V H}`). All
`fillRect(a,b,c,d,black)` calls below use that order. `SetRect(r, l, t, r, b)` is the normal
toolbox order (left, top, right, bottom). Rects in this doc are written `(left,top)-(right,bottom)`
unless labeled otherwise.

### 0.2 The repo sources are a dev snapshot, not exactly either shipped version

Cross-checking the extracted shipped archives shows the repo code sits *between* 1.0 and 1.1:

- The repo's menu constants and keys-dialog PICT IDs (303–308) match the **1.0** app resources.
- The repo's boot sequence references PICTs **1012, 2000–2016, 140**, and logo sound **143** —
  none of which exist in either shipped Pictures/Sounds file. The shipped boot art is different
  (see §3.4).
- The repo build had all resources merged into the project file: **there is no `OpenResFile`
  call anywhere in the sources.** The shipped apps open the six "Info Datafiles" (Read Me!:
  "The Info Datafiles folder MUST be in the same folder that Infotron is in"). A port must load
  the equivalent assets itself.
- v1.1 features (autoload player, diagonal movement, "Care To Try Again?", auto bit-depth
  switching) are **absent** from the repo sources; behavior below is the repo/1.0 behavior with
  1.1 differences flagged `[1.1]`.

### 0.3 Shipped datafile inventory (v1.1; v1.0 differences in §13)

| File | Resources |
|---|---|
| Infotron Levels | 51 × `lüVl` (bytes 6C 9F 56 6C), IDs **301–351**, named; RLE-compressed levels (RULES §3.1) |
| Infotron Pieces | 183 × `icl8` + 234 × `ICN#` (+ small icon families for Finder); the game draws only `icl8` |
| Infotron Pictures | 28 × `PICT` (see §4.1) |
| Infotron Sounds | 17 × `snd ` (see §11.1) |
| Infotron Music | 2 × `snd `: **1000 "music Intro"** (474 KB), **1001 "m.loop"** (633 KB) |
| Infotron Players | `MuPf` 128 "User Preferences" (2 bytes), one `blüG` per player (552 bytes each), `plål` 128 "AutoloadPlayer" (256 bytes) `[1.1 only]` |

App resource fork (both versions): `MBAR` 1, `MENU` 1/128/129/130, `WIND` ×15, `DLOG`/`DITL`
(incl. the startup-notice DLOG 2000), `ALRT`s, 14 × `I†i†` tutorial resources (IDs 501–514,
named Movement, Eating, Falling, Zapping, Pushing, Rolling, Portals, Green disks, Red disks,
Yellow disks, Bugs, Snik-Snaks, Quarks, Exit), and an easter-egg resource `Mrfy` 0 containing
the Pascal string `"Infotron™, written by a large aardvark with an attitude. ©1994 "`.

`icl8` inventory (v1.1) — useful to know which IDs actually exist within the code's ranges:
128–137, 139–145, 147, 194–206, 214, 250, 300–305, 350–358, 400–407, 450–457, 499–511, 550–561,
600–610, 650–658, 700–707, 750–760, 850–853, 900, 902, 904, 906, 950, 952, 1000–1007,
1010/1012/1014/1016, 1020/1022/1024/1026, 1050–1057, 1100/1102/1104/1105/1106,
1150/1152/1154/1156, 1200–1211. (v1.0 = same minus 1150/1152/1154/1156.)

---

## 1. Machine requirements, window, and screen geometry

### 1.1 Requirements enforced at boot (`MachineAbilities`)

- Color QuickDraw required, else fatal dialog: *"Infotron™ requires a color machine to run! Sorry!"*
- A monitor strictly larger than 639×479 (i.e. ≥ 640×480) required; on multi-monitor systems it
  searches for a monitor that is ≥640×480 **and already in 8-bit depth** (`FindGoodMonitor`).
- Single-monitor systems must be in exactly **256 colors** (8-bit), else: *"Infotron™ needs
  exactly 256 colors. Please select 256 colors from the Monitors control panel…"*.
- Fatal messages go through `QuitError` → `DLOG 501` (OK + ^0 text) → `ExitToShell`.
- `[1.1]` the shipped 1.1 app switches the monitor to 8-bit itself and restores depth on quit
  (Version History).

### 1.2 Window model

- One fullscreen color window: `gMainWind := NewCWindow(nil, screenBits.bounds, '', TRUE, 2,
  WindowPtr(-1), false, 0)` — procID 2 (plainDBox, no chrome), covering the entire screen.
- The 640×480 play area is centered on larger monitors via `gMoveH := (monWidth−640) div 2`,
  `gMoveV := (monHeight−480) div 2` (or centered within the chosen monitor's rect on
  multi-monitor rigs). **Every game-area coordinate in the code is offset by (gMoveH, gMoveV).**
- On monitors >640×480, `BigMonUpdate` fills the surround with black and frames the game area
  with three nested 2-px `FrameRect`s: light gray (inset −3 from the area), then gray, then dark
  gray (colors from §10.3).
- Menu bar: hidden during play by writing 0 to the `MBarHeight` low-mem global and unioning the
  bar area into the window's visRgn (`MenuBars.p`); shown at boot/menus/pause.
- **Suspend/resume** (MultiFinder): on suspend the fullscreen window is destroyed and replaced
  by a tiny 55×55 window near the bottom-left corner (screen bounds offset by (+10,−10); rect
  `top := bottom−55, right := left+55`) showing **PICT 139** (55×55 badge). On resume the
  fullscreen window is recreated and everything redrawn. Port equivalent: none needed.

### 1.3 Board viewport geometry — the exact answer to "672 vs 640"

- The board viewport occupies game-area pixels **(0,0)–(640,419)** — i.e. 640 wide ×
  **419 = 480 − 61** tall. The bottom **61 px** band (419–480) is the status bar.
- Tiles are 32×32. The renderer maintains an offscreen GWorld `myOff` with rect
  **(−32, −32, 672, 451)** (i.e. 704×483) and plots a **21 column × 15 row** tile window into it
  at `(col·32 − 16, row·32 − 16)` for col 0..20, row 0..14 relative to `upperLeft`
  (`DrawOffscreen`, `Draw1`).
- The visible blit is source rect (0,0,640,419) → screen (offset by gMoveH/gMoveV)
  (`RedrawBoard`, one `CopyWorldBits2` per frame).
- Because of the **(−16,−16) half-tile offset**, what the player actually sees is:
  - Horizontally: col 0 and col 20 are 16-px half-columns; cols 1–19 full → 21 columns spanning
    exactly 640 px (20 tile-widths).
  - Vertically: row 0 shows its bottom 16 px, rows 1–12 full, row 13 shows its top 19 px, and
    **row 14 is drawn but entirely clipped** (it exists as a scroll-in buffer). Visible height =
    16 + 12·32 + 19 = 419 px (≈13.1 rows).
- So: neither "672×480" nor "640×480 of whole tiles" — the playfield is a **640×419 window onto
  a 21×15 rendered tile grid offset half a tile up-left**. With the usual solid level border and
  `upperLeft=(1,1)`, the border wall appears as 16-px slivers at the screen edges. Reproduce
  this exactly; level design relies on it (Murphy's scroll margins in RULES §6.2 match it).
- Viewport init (`GotNewBoard`): `upperLeft := murphy − (10, 7)`, then clamp:
  if `sizeX − murphyX < 10` → `upperLeft.h := sizeX − 20`; if `sizeY − murphyY < 7` →
  `upperLeft.v := sizeY − 13`; finally floor both at 1.
- Scrolling is **whole-tile stepped**, one tile per Murphy move that passes `CanScroll`
  (thresholds in RULES §6.2): `MoveUp/Down/Left/Right` shift the offscreen by ±32 px with
  `CopyBits` onto itself, plot the single incoming row/column (21 or 15 cells) at the exposed
  edge, adjust `upperLeft`, and reblit. There is no smooth scroll ever.
- `DrawOffscreen` quirk: a cell whose value is exactly **300** (dormant bug in raw level data)
  is drawn as **128** (base) during full redraws, so bugs are invisible in the pre-game preview.

### 1.4 Board preview outside play

`LoadBoard` (called by New/Open player, Choose Level, level-won bookkeeping) ends with
`GotNewBoard` + `DrawStatus`, and `doUpdate` redraws board + status on every update event. So on
the "menu screen" the player always sees the **current level, frozen, with the status bar**, and
the menu bar on top. There is no separate menu backdrop; PICT 3000 / the title PICT are only
shown during boot.

---

## 2. Status bar (bottom 61 px) — exact layout

Background: **PICT 129 (638×60)** drawn at game-area (1, 419) (`drawStatus`), i.e. covering
y 419–479, x 1–639, with 1-px black margins left/right/bottom. All text is drawn over it with
`TextMode(srcOr)`-style fills: each field first fills its box with black, then draws colored
text. Default port font (Chicago 12/system font); `drawInteger` prints fixed-width digit counts
with leading zeros; `centerString` (myRGBColors) centers a string between two x-coords,
**shrinking `textSize` from 12 until the string is ≤130 px wide**, and draws decorative
horizontal rules at `y−4` from the sides to the text.

All coordinates below are game-area coords, boxes as (top,left,bottom,right):

| Field | Black box | Text position | Color | Notes |
|---|---|---|---|---|
| Player name | (429,187,447,338) | `centerString(name, 442, 191, 334)` | red | only if `hasPlayer` |
| Time — hours | (429,357,447,379) | `moveTo(359,442)`, 2 digits | blue | `drawTBoxes` |
| Time — minutes | (429,395,447,417) | `moveTo(397,442)`, 2 digits | blue | |
| Time — seconds | (429,433,447,455) | `moveTo(435,442)`, 2 digits | blue | |
| Infotrons still needed | (429,492,447,524) | `moveTo(495,442)`, 3 digits | **green when 0, else red** | `drawInfoStat` |
| Level name | (453,187,471,413) | `centerString(level, 466, 191, 409)` | green | |
| Level number | (453,425,471,455) | `moveTo(427,466)`, 3 digits | green | `gPlayer.current` |
| Red disks held | (453,492,471,524) | `moveTo(495,466)`, 3 digits | **blue when 0, red when >0** | `drawDiskStat` |

- Time display: `convertTime` from ticks — `hrs = t div 216000; mins = (t mod 216000) div 3600;
  secs = ((t mod 216000) mod 3600) div 60`.
- What the clock shows (`drawTBoxes`): while **playing and not paused** → the current round's
  elapsed time (`GetGameTime`); while paused or not playing → the player's **cumulative total**
  (`gPlayer.time`). Each of h/m/s only redraws when its value changed (anti-flicker); a
  `drawTBoxes(FALSE)` runs at the end of every game frame.
- Infotron/red-disk counters are redrawn immediately by `GotInfotron`/`GotRedDisk`/`UseRedDisk`;
  `drawGameStats` blanks both boxes when not playing.
- Time accounting: `startGameTime` at level start and on unpause; `stopTime` accumulates into
  `gMurphy.totTime` on pause (paused time is excluded); on level end (win *or* death) the
  `CKillGameChore` runs `AddToTime(getGameTime)` which adds the round to `gPlayer.time` and
  saves the player (§9.3).

Loading progress bar (`CreateStatus`/`UpdateStatus`, used only while building the icon atlas at
boot): centered on screen, a blue `FrameRect` box ~184×12 (inset −(90+2), −(4+2) around center;
`statHeight = 4`), red fill bar growing 1 px per 6 icons loaded, label **"Loading Graphics…"**
in red, TextSize 9, centered above the bar.

---

## 3. Boot sequence and screens

### 3.1 `InitInfotron` — exact order (repo sources)

1. `PeckInit(15)` (15 × MoreMasters, arrow cursor); `gMyResFile := CurResFile`.
2. `MachineAbilities` (§1.1); `FigureCenterMonitor` (gMoveH/gMoveV).
3. `Init_QIST` (allocate 4 fx channels; music channel lazy), `SoundState(TRUE)`.
4. Allocate `fallingInfo` / `fallChores` (each `array[1..120,1..120] of boolean`, NewPtrClear).
5. **Startup notice**: `GetNewDialog(2000)` — DLOG 2000, rect (38,24)–(443,461) i.e. 405×437,
   whose single DITL item is **PICT 128** (shareware/title notice, 239×309 art stretched to the
   dialog). Two `SysBeep`s; modal loop until item 1 is hit (clicking the picture dismisses).
   This is the shareware nag — shown once per launch, before anything else.
6. `InitMenuBars`, `SetupGammaTools`, `nextRollDir := kRight`, `FadeToBlack(2)`, `HideCursor`.
7. Create fullscreen `gMainWind`; **GrayPal**: build + activate a grayscale palette (entries
   1–254 ramp from near-white down to near-black; `pmTolerant+pmExplicit`); `HideMenuBar`.
8. `ShowQAnim`: fill black, draw **PICT 1012** centered ("Q" logo), `FadeFromBlack(2)`,
   2-tick delay. Returns FALSE (not skippable itself).
9. `makeGObjs` (the 17 piece singletons, RULES §1.3); `shouldKillChores := FALSE`.
10. `ShowIncorporated` (skipped if a click already happened): with the Q logo still up, draws
    **PICT 2000** centered horizontally, positioned just below PICT 1012's frame; 20-tick delay;
    `PlaySound(143)` (logo sound); then **PICTs 2000→2016** at 2-tick intervals (the
    "Incorporated" typing animation); then wait up to **180 ticks (3 s)** or until mouse click;
    `FadeToBlack(2)`. Returns TRUE if clicked.
11. `SetPictPalette` (the hard-coded purple 256-entry palette, §10.2); `ShowLogo(3000)` (skipped
    if any prior click): black fill, **PICT 3000** centered, fade in, wait ≤180 ticks or click,
    fade out. In shipped 1.1 PICT 3000 is a full 640×480 title screen; in 1.0 it is a small
    297×112 wordmark.
12. `NormalPal`: `RestoreDeviceClut` + restore the original palette — **the game proper runs in
    the standard Mac 8-bit system CLUT** (§10.1).
13. `InitalizeGraphics`:
    - `SetUpIcons`: create the **icon atlas GWorld (482×386)**; for every ID 128..1211 try
      `GetResource('icl8', id)`; present icons are plotted (classic `PlotIconID` trap) into
      consecutive 32×32 slots, **15 per row starting at (1,1)**, recording each slot rect in
      `gIconLocs[128..1211]` (missing IDs get an invalid rect). Progress bar per §2. Runs
      between a `FadeFromBlack`/`FadeToBlack` pair. All later tile drawing is
      `CopyWorldBits2(gIconOff → target, gIconLocs[id], destRect)`.
      ⚠ capacity quirk: 482×386 holds exactly 12 rows × 15 = **180 icons**; v1.1 ships 183
      icl8s, so the last three by ascending ID (1209–1211, tail of the win animation) overflow
      the atlas in this code. The shipped 1.1 binary presumably enlarged it. Port: size the
      atlas from the actual icon count.
    - `SetUpOffScreen`: the 704×483 board offscreen (§1.3); fill window black.
14. **Title/menu screen**: `DrawPicCenterGraf(140)` (PICT 140 — missing from shipped datafiles,
    see §3.4), `FadeFromBlack(2)`, wait for a complete mouse click (down + up), `FadeToBlack(2)`.
15. `ShowMenuBar`; build menus: Apple (`peckApple('About Infotron...')` + DA list), then
    `getMenu(128/130/129)` inserted in MBAR order **Apple, File, Player, Game**.
16. `ResetRandom` (`randSeed := tickCount`), `InitChoreLists`, `ldHrs/ldMins/ldSecs := 0`,
    `musicRefNum := −1`, `musicID := ARandom(3)+999`, `repeatCnt := 1`, `SetRegInfo`
    (**forces `registered := TRUE`** and fills the `firstTen` level-name array — only
    `firstTen[1] = 'Warm Up'` is ever used), `GetPrefs` (§9.2).
17. `setUpDials` — construct all 9 dialogs (hidden) with their PICT backgrounds & hit boxes (§6).
18. `ShowCursor`, `drawStatus`, `loadNextLevel := FALSE`, `FadeFromBlack(2)`. → enter `peckMain`.

`[1.1]` also auto-loads the `plål 128 "AutoloadPlayer"` name and opens that player at startup
(1.0 makes you Open/New a player manually every launch).

### 3.2 Main loop (`peckMain`)

```
while not wantsToQuit:
    CleanSounds                      (no-op)
    if (isPlaying in gameState) and not (isPaused in gameState):
        Process1Event                (one 5-tick game frame; NO event processing at all)
    else:
        WaitNextEvent(10-tick sleep) → doEvent (windows/menus/keys)
        FixMenuHilites
        if isPlaying and isPaused: poll caps lock; unpause when restored (§7.3)
        if not isPaused and loadNextLevel: do level-won bookkeeping (§8.3)
```

`Process1Event` = `HandlePlaying` (chores → input → Murphy, RULES §2) then `SmoothMusic`
(§11.3), then the deferred `shouldKillChores` cleanup (kill chores, show menu bar, stop music,
halt sounds), then `DrawTBoxes(FALSE)`. Key consequence: **during play no OS events are
serviced** — no menus, no cmd-keys, no window updates; input is raw `GetKeys` polling. Menus are
reachable only when not playing or when paused.

### 3.3 State model

`gameState: set of (hasPlayer, hasLevel, isPlaying, isPaused, hasMusic, hasSound, fadeFromB,
freezeMurphy)` — `fadeFromB` is never used. Shell-relevant transitions:

- New/Open player → `hasPlayer` (+ board loaded → `hasLevel`).
- Game ▸ Start → `StartNewLevel` → `+isPlaying −freezeMurphy` (RULES §3.2 for the board side;
  shell side: captures caps-lock reference state, hides menu bar + cursor, reloads the board
  from `gPlayer.level`).
- caps lock toggled → `+isPaused` … toggled back → `−isPaused`.
- Death/win → `+freezeMurphy` (input ignored, world keeps simulating 4 s) → `CKillGameChore` →
  `−isPlaying`, cursor shown, time saved, chores killed; menu bar returns on the next frame's
  `shouldKillChores` sweep.
- Win additionally sets `loadNextLevel` (handled next time the event loop runs, §8.3).

### 3.4 What the shipped builds actually show at boot

Resource evidence (neither shipped Pictures file has 1012/2000–2016/140; 1.1 Sounds has no 143
but adds 150 "QLogoSnd"; 1.1 PICT 3000 is 640×480):

- v1.0: small 297×112 PICT 3000 logo; keys-dialog PICTs 303–308; no autoload.
- v1.1: startup notice PICT 128 (DLOG 2000), a Q-logo moment with `snd` 150 "QLogoSnd", and the
  full-screen 640×480 PICT 3000 title; sounds 144 "Click"/145 "Tick"/146 "Ding!" support the
  help tutorial (and possibly the 1.1 retry UI); sound 151 "endLevel" presumably plays at
  level end (ships unreferenced by the repo sources — unverified, see §7.4).
- The exact shipped boot choreography can't be read from the repo sources (binary-only). For
  the port, recommended boot: notice/title → (skippable) logo w/ sound → 640×480 title PICT →
  menu screen. Don't replicate the missing-PICT dev sequence literally.

---

## 4. Picture and window inventory

### 4.1 PICTs (shipped "Infotron Pictures", v1.1)

| ID | Size (w×h) | Used by / meaning |
|---|---|---|
| 128 | 239×309 | startup shareware notice art (stretched into DLOG 2000, 405×437) |
| 129 | 638×60 | status bar background |
| 130 | 313×157 | Choose Level dialog background (WIND 130) |
| 131 | 313×126 | Open Player dialog background (WIND 131) |
| 132 | 193×204 | Set Keys dialog background (WIND 132) |
| 133 | 298×72 | New Player dialog background (WIND 133) |
| 134 | 298×72 | Rename Player dialog background (same WIND 133) |
| 135 | 12×12 | scroll-bar thumb ("piece") |
| 136 | 12×12 | scroll-bar thumb eraser ("blank") |
| 137 | 409×64 | error banner window background (WIND 137) |
| 138 | 12×12 | scroll-bar thumb while dragging |
| 139 | 55×55 | suspended-app mini window badge (also DLOG 750 leftover) |
| 143 | 474×418 | Help part 1: piece gallery background (WIND 143) |
| 144 | 474×418 | Help part 2: interactive tutorial background |
| 145 | 313×126 | Delete Player dialog background (WIND 131 shape) |
| 146 | 225×67 | "Are you sure?" yes/no dialog background (WIND 146) |
| 149 | 283×212 | About box background (WIND 149) |
| 150 | 140×1219 | About: scrolling credits column #1 |
| 151 | 140×871 | About: scrolling credits column #2 |
| 500–507 | 27×18 each | `[1.1]` Set Keys key-slot highlight overlays (replace 1.0's 303–308) |
| 3000 | 640×480 (`[1.0]`: 297×112) | boot splash/title |

v1.0 instead of 500–507 has **303–308 (32×32)**: pressed-arrow overlays drawn at (80,54) in the
Set Keys dialog (303=up,304=left,305=right,306=down,307=special,308=none), matching the repo
source `drawPic(300 + event)`.

Note PICT 140 (repo title screen), 147 (repo `ShowLogo(147)` — commented out), 155 (registration
dialog background, `regDial.init(155,155)`) exist in **no** shipped file; the registration
dialog could never draw and is unreachable anyway (§6.9).

### 4.2 WIND resources actually used

| WIND | rect (t,l,b,r) | size | dialog |
|---|---|---|---|
| 130 | (68,159,225,472) | 313×157 | Choose Level |
| 131 | (111,162,237,475) | 313×126 | Open Player / Delete Player |
| 132 | (101,220,305,413) | 193×204 | Set Keys |
| 133 | (94,93,166,391) | 298×72 | New Player / Rename Player |
| 137 | (66,108,130,517) | 409×64 | error banner |
| 143 | (27,84,445,558) | 474×418 | Help / tutorial |
| 146 | (126,168,193,393) | 225×67 | Are-you-sure |
| 149 | (40,40,252,323) | 283×212 | About |
| 155 | (122,85,247,523) | 438×125 | Registration (unreachable) |

(WIND 128, 200, 500, 501, 1000, 9999 are dev leftovers.) DLOG 500 = fatal `GenericError`
("Fatal Error! Please Report to Q Inc.!", button "AARGH!", then `ExitToShell`); DLOG 501 =
`QuitError` (OK + message, exits if fatal); DLOG 2000 = startup notice.

---

## 5. Menus

### 5.1 Menu bar (MBAR 1 order: Apple, File, Player, Game)

**v1.0 / repo source** (matches `dataIntf.p` item constants):

|  | Apple (1) | File (128) | Player (130) | Game (129) |
|---|---|---|---|---|
| 1 | About Infotron… | Quit ⌘Q | New… ⌘N | Start Playing ⌘S *(toggles to "Abort")* |
| 2 | ─ | | Open… ⌘O | Pause ⌘P *(permanently disabled — dead item)* |
| 3 | DAs… | | Rename… ⌘R | ─ |
| 4 | | | Delete… ⌘D | Choose Level… ⌘L |
| 5 | | | | Set Keys… ⌘K |
| 6 | | | | ─ |
| 7 | | | | Sound Effects ✓ |
| 8 | | | | Music ✓ |
| 9 | | | | ─ |
| 10 | | | | Help! |

**v1.1 shipped** Game menu is restructured: **Play ⌘P**, ─, **Choose Level... ⌘L**,
**Set Keys... ⌘K**, ─, **Sound Effects**, **Music**, ─, **Help! ⌘H** (Pause item deleted;
File/Player unchanged). Port should model item IDs on 1.1.

### 5.2 Behaviors

- **About Infotron…** → About box (§6.7). (`DorkyButts(1)`; other Apple items open DAs.)
- **File ▸ Quit**: `DoFileMenu` ignores the item number — *any* File selection runs
  `KillBoard; KillPlayer; KillLevel; PeckQuit` → main loop exits → `PrepareQuit` (`SetPrefs`,
  free bit arrays, `Close_QIST`) → windows/menus disposed. Prefs are saved **only at quit**.
- **Player ▸ New…** (§6.3): name entry; on OK constructs a fresh `PlayerType` (level='Warm Up',
  highLev=current=1, empty skip set, numSkipped=0, time=0, default keys {91,86,88,84,49}),
  cheat check (§8.5), `savePlayer`, `loadBoard(level)`, `+hasPlayer`, `drawStatus`.
- **Player ▸ Open…** (§6.4): scroll list of players sorted by name; loads record, installs its
  key bindings, cheat check, `LoadBoard(gplayer.level)`, `drawStatus`.
- **Player ▸ Rename…**: name entry pre-shown over PICT 134; sets `gPlayer.name`, `savePlayer`
  (note: saves under the new name → effectively duplicates the record; the old-name resource
  remains — original quirk).
- **Player ▸ Delete…**: scroll list + "Are you sure?"; removes the named `blüG` resource unless
  it's the loaded player, in which case: *"This game does not condone suicide. Please call
  Dr. Jack Kevorkian."*
- **Game ▸ Start Playing**: if not playing → `+isPlaying`, `StartNewLevel`. If playing (item
  reads "Abort") → **all handler code is commented out; it does nothing** (and it's unreachable
  during play anyway since no events are processed; while paused it's disabled). Abort
  effectively doesn't exist in 1.0.
- **Game ▸ Choose Level…** (§6.5): snapshots the player's progress fields, runs the picker; on
  choose → `LoadBoard(newLevel)`, `SavePlayer`, `DrawStatus`.
- **Game ▸ Set Keys…** (§6.6): edits `gPlayer.keySet`, then `savePlayer`.
- **Game ▸ Sound Effects / Music**: toggle `hasSound`/`hasMusic` + checkmark. Sound-off is
  enforced inside the sound engine (`SoundState`). Music-off stops the loop channel
  immediately; music-on re-rolls `musicID` and lets `SmoothMusic` start it (only during play).
  Toggles are **not** persisted until quit.
- **Game ▸ Help!** → §6.8.

### 5.3 Enable rules (`FixMenuHilites`, run each idle pass)

Baseline: New/Open/Rename/Delete/Start/Pause/Level/Keys disabled; Help/Quit/SoundFX/Music/
Delete enabled (Delete is enabled even with no player); checkmarks synced.
Then: not paused → enable New, Open. `hasPlayer` → enable Rename, Keys; and (not paused) →
enable Level; and (`hasLevel` and not `loadNextLevel` and not paused) → enable Start.
Start item text = "Abort" while `isPlaying` else "Start". Cmd-key shortcuts work wherever
events are processed (menu screen and pause).

---

## 6. Dialog system

### 6.1 Framework (`CInfoDialog` / `CInfoScrollDialog`)

- A dialog = a `WIND` color window + a full-window background **PICT** + up to 15 hit rects
  (`boxes`), each either a "button" (gets hand-drawn 3D press/release bevels + 1px content
  shift, `pressButton`/`unPressButton`) or a plain hot zone. No Dialog Manager, no TextEdit.
- Modal loops poll `button`/`GetKeys` directly (busy-wait). Common keys: **Return (code 36)** =
  simulate button 1 (OK, with a 4-tick pressed flash), **Esc (53)** = cancel, arrows
  **125/126** = down/up in scroll lists. Double-click (within `GetDblTime`) in a list = OK.
- `CInfoScrollDialog` adds a black list rect (`blackRect`), 14-px rows (`charHeight`), a custom
  scrollbar (up/down arrow rects from the ctor, thumb = PICT 135, erased with 136, drag ghost
  138, page-jump by clicking the bar), and a `list: array[1..100] of str255` filled from
  resources: either **by ID sequence 301..300+n** with numbers shown (levels) or by index with
  a selection-sort by name (players). Selected row is drawn bold.
- `doError(str)`: opens WIND 137 + PICT 137, message in bold 14-pt drawn twice (dark-gray at
  +1,+1 then yellow) starting at (67,26); wraps to a second line at ~320 px; `SysBeep`; waits
  for a click. Used for all non-fatal complaints.
- `CheckSure`: WIND/PICT 146, Yes (141,12)-(214,31), No (141,40)-(214,59); Return = yes.

### 6.2 setUpDials inventory (hit boxes as (l,t)-(r,b))

| Dialog | WIND/PICT | Boxes |
|---|---|---|
| regDial | 155/155 | OK (348,67)-(421,86), Cancel (348,94)-(421,113), name field (55,67)-(345,86), code field (55,94)-(345,113) |
| aboutDial | 149/149 | three buttons (20,109)-(93,128), (20,138)-(93,157), (20,167)-(93,186) |
| newDial | 133/133 | OK (200,13)-(273,32), Cancel (200,41)-(273,60), text box (17,42)-(178,59) |
| renDial | 133/134 | same as newDial |
| keysDial | 132/132 | OK (14,170)-(87,189), Cancel (105,170)-(178,189); key slots: up (84,14)-(111,32), left (30,61)-(58,80), right (135,62)-(163,80), down (84,110)-(111,129), special (137,137)-(164,155); center art zone (77,51)-(114,88) |
| helpDial | 143/143 | 13 piece hot-zones + OK (403,14)-(456,37) — see §6.8 |
| lvlDial | 130/130 | scroll ctor: up (283,12)-(296,25), down (283,129)-(296,142), list width 162, 9 rows → blackRect (117,14)-(278,141); buttons Choose (19,32)-(92,51), Skip (19,68)-(92,87), Cancel (19,101)-(92,120) |
| openDial | 131/131 | up (179,11)-(192,24), down (179,101)-(192,114), width 161, 7 rows → blackRect (14,13)-(174,113); OK (215,33)-(288,52), Cancel (215,76)-(288,95) |
| delDial | 131/145 | same shape as openDial |

### 6.3 Name entry (`getWinName` — New Player / Rename)

Hand-rolled text field at baseline y=55 starting x=22, red text, blinking red caret
(`GetCaretTime`), black box fill behind. `changeKey` maps the raw keymap to a char (§12.2);
uppercase unless shift... actually inverted: **unshifted letters are lower-cased**; keypad keys
insert spaces; `~` (backspace, code 51) deletes; width-limited to <140–148 px; Return/click
OK → accept (rejects empty: *"Sorry, you have to have at least one or two letters in your
name…"*), Esc/Cancel → abort.

### 6.4 Open / Delete Player

Scroll list (names sorted); Return/double-click/OK chooses; Esc cancels; empty selection →
*"Please select an item first..."*. Delete additionally runs `CheckSure`.

### 6.5 Choose Level picker (`levelChoice`)

- List = level resource names for IDs 301..300+n, shown as `NNN Name` rows (numbers always
  drawn, 3 digits). Colors (`setTextColorOf`): **green** = passed (`< highLev`, not skipped);
  **red** = `> highLev` (locked); **blue** = in skip set; **yellow** = `= highLev` (the
  frontier). Selection bold. Keyboard: ↑/↓ move selection, Return = Choose, Esc = Cancel,
  double-click = Choose.
- **Choose**: rejected with *"You may only play the levels marked in green, blue, or yellow."*
  if `sel > highLev`. On success: `current := sel`, `level := list[sel]` (the **name** is what's
  stored), skip set/highLev/numSkipped written back, board loaded, player saved.
- **Skip** (max 3 outstanding): allowed only when `sel = highLev` and `numSkipped < 3`; adds
  `sel` to the skip set, `numSkipped += 1`, `highLev += 1`, selection moves to the new frontier.
  Error texts otherwise: `sel > highLev` → *"You cannot skip levels that you have not
  reached."*; already skipped → *"Do you really want to skip that level twice? It seems
  redundant."*; `sel < highLev` → *"Why do you want to skip a level that you have already
  passed?"*; else → *"You may only have three skipped levels at one time."*
- Cancel restores nothing (the snapshot in `ChooseLevel` is taken but never rolled back —
  harmless because `levelChoice` only mutates its VAR params on success).

### 6.6 Set Keys (`doSetKey`)

Shows the five current keys as characters (keypad keys are displayed as their **digit**: raw
codes 82..92 → '0'..'9'; space shows as '_'; keypad-clear (71) shows as 'Ÿ'). Click a key slot →
it highlights (PICT 30x/50x overlay at (80,54)) and the next pressed key (via `changeKey`) is
assigned — duplicates rejected (*"You have already used that key! Please choose again."*).
Return/OK commits to `player.keySet` (then caller saves player); Esc cancels. Key order in
`keySet`: [1]=up, [2]=left, [3]=right, [4]=down, [5]=special.

### 6.7 About box (`doAbout`)

WIND/PICT 149. Top-left region (10,63)-(89,93) shows registration status: reads resource
**'QarD' 128** (record whose 5th field `theS` is the registered name) — if present "Registered
to: <name>" else "Unregistered" (9-pt text). No shipped file contains a QarD, so shipped = always
"Unregistered". Buttons 1/2 select scrolling credits **PICT 150 / PICT 151**, rendered through a
116×138 GWorld at (130,60)-(246,198), scrolling up 2 px per loop iteration, wrapping at −975
(150) / −800 (151); button 3 exits. (A fourth Murphy-yawn easter-egg branch exists but its hit
box is disabled.) The `sAboutBox` sound (136) is defined but never played (and isn't shipped).

### 6.8 Help (`doHelp`) — two phases

**Phase 1 — piece gallery** (PICT 143): 13 hot zones (murphy (32,53)-(70,91), quark
(32,120)-(70,158), infotron (175,53)-(213,91), terminal (336,53)-(374,91), bug
(336,120)-(374,158), snik-snak (175,185)-(213,223), zonk (336,185)-(374,223), disks
(32,250)-(141,288), circuit (32,185)-(70,223), exit (175,120)-(213,158), ports
(293,250)-(400,288), chips (32,332)-(141,370), hardware (177,332)-(439,370)). Clicking one
writes a two-line 9-pt description into (10,390)-(470,415) and plays a busy-wait icon animation
in place (e.g. quark spins 650→657 ×3, zonk 700→707, bug pulse, scissors sequence, red-disk
400→407→back, circuit 450→457→back, terminal 550→561). Idle easter egg: after 20 s Murphy's
gallery icon yawns (500→510, then 499, then 214), 3.5 s later gets mad (142), ~1.7 s later
falls asleep (143→144). OK → phase 2. (The description strings are hard-coded in
`InfoWindows.p` and worth copying verbatim for the port's help screen.)

**Phase 2 — interactive tutorial** (PICT 144): VCR controls skip-back (77,284)-(154,308), play
(158,284)-(235,308), pause (239,284)-(317,308), skip-next (321,284)-(398,308); OK exits. 14
slides = resources `I†i†` 501–514 (names in §0.3). Slide format (`TutorialData`): header text
(1 title + up to 5 body Str255s, drawn in the (25,324)-(451,401) area — title 12-pt bold, body
9-pt); `initialBoard: array[1..10,1..7] of integer` icon IDs (a 10×7 tile mini-board drawn at
78,54 with 32-px tiles); `numSteps`; steps[0..75] each = up to 10 `(newIconNum,x,y)` cell
changes + `soundID` (0 = none) + `timeDelay` (ticks). Playback: paused by default (pause button
pressed-in); play applies one step per `timeDelay`, drawing into a GWorld over (78,54)-(398,278)
and playing the step sound; when a slide's steps run out it auto-advances (slide 14 ping-pongs
back to 13). This is effectively a scripted demo player — port note: these 14 resources fully
define the tutorial and should be extracted as data.

### 6.9 Registration (`doReg`) — present but dead code

Two-field dialog (name + code). Code check: `FindCode(name)` lower-cases the name, maps each
char to a "bank" (`(ord−'a') div 5 + 1`, space→7, z→6) and offset (`mod 5`), rotates a 5-number
bank window, emits a 2-char token per letter from a fixed 35-token table, prepends
`chr(len mod 26 + 'A')`; ≥5 letters required, letters/spaces only. Nothing in the final build
opens this dialog (`SetRegInfo` hard-sets `registered := TRUE`; File menu has no Register item;
`DorkyButts` only handles item 1), no code writes 'QarD', and its background PICT 155 never
shipped. **Port: drop it.**

---

## 7. Session flow: starting, pausing, dying, winning, quitting

### 7.1 Starting a level

Game ▸ Start (`iCmdStart`, only enabled with a player + level loaded, not already playing) →
`+isPlaying`, `StartNewLevel` (RULES §3.2). Shell effects: `DoKillChores`; caps-lock reference
captured (`pauseKeyState := not keyDown(57)`); menu bar hidden; **board re-loaded by name from
`gPlayer.level`** (so Start after death = retry, always a fresh board); cursor hidden; status
redrawn. Murphy init: `Init` also calls `startGameTime` (round clock starts now).

### 7.2 During play

12-fps frame loop per RULES §2.1. Shell notes: `DrawTBoxes(FALSE)` updates the clock once per
frame; music auto-start/rotate (§11.3); no event processing (⌘Q etc. dead); screen updates are
the board blit + dirty status fields only.

### 7.3 Pause = caps lock

- Trigger: caps-lock key state ≠ its level-start state (pure toggle — works whichever position
  caps lock was in when the level began). Checked at the top of every frame.
- `DoPause`: `+isPaused`, `ShowCursor`, stop music channel (`musicRefNum := −1`), `HaltSounds`,
  play **sPause (138)**, `stopTime`, `ShowMenuBar`.
- While paused, the normal event loop runs: menus available per §5.3 (New/Open/Level/Start
  disabled; Rename, Delete, Keys, Help, Sound, Music, Quit, About available — you *can* quit,
  re-bind keys, or run Help mid-level).
- Unpause: event loop polls caps lock each pass; when restored to the level-start state:
  `−isPaused`, `HideCursor`, **sUnpause (139)**, `startGameTime`, `lastMoved := tickCount`
  (resets idle/sleep timers), `HideMenuBar`. Music restarts on the next play frame (fresh
  random track pick).
- ⚠ real-time leak: `CWaitTimeChore` (red-disk 2-s fuse, the 4-s end-of-level timer) compares
  against wall-clock `tickCount`, so **pausing runs those fuses down** — pause during a lit
  red-disk fuse and the explosion fires on the first frame after unpause. Faithful-port
  decision; recommend fixing to game-time (flag as deliberate deviation).

### 7.4 Death

Esc (raw code 53) or any lethal event → `KillMurphy`/`ExplodeTWO` (RULES §6.7) → `EndLevel`:
`+freezeMurphy`, schedule `CKillGameChore` behind a 4-s `CWaitTimeChore`, play **sMurphyDied
(137)** — the board keeps animating for 4 s with input dead. Then `CKillGameChore`:
`−isPlaying`, `ShowCursor`, `AddToTime(getGameTime)` (persists total time), `KillChores` →
deferred sweep also re-shows the menu bar and silences music/sounds. Back on the menu screen the
(reset-on-Start) level remains displayed; **1.0 has no retry prompt — the player re-issues
Start**. `[1.1]`: an "endLevel" sound (snd 151) presumably plays (it ships unreferenced by
the repo sources — unverified); a **"Care To Try Again?"** prompt appears
(press **y** to restart instantly), and during the 4-s freeze the **fast-restart keys r / p / s**
skip the summary and restart the level immediately.

### 7.5 Winning

Exit walk-in with quota met (RULES §11) → win-anim icons 1200–1211 flashed into Murphy's cell →
`WonGame` = `EndLevel` (same 4-s freeze) + `loadNextLevel := TRUE`. No jingle in the repo code
(`sOHit` 130 defined but never played, not shipped). After the freeze the event loop's
`loadNextLevel` block runs (§8.3), which loads and displays the next board **but does not start
it** — the player presses Start for the next level.

### 7.6 Quit

File ▸ Quit (or any File item) — only reachable outside play/paused. `PrepareQuit` saves prefs
(§9.2), frees the bit arrays, closes sound channels. `[1.1]` also writes the `plål` autoload
name and restores the saved monitor bit depth.

---

## 8. Levels: identity, ordering, progression, skipping, cheats

### 8.1 Identity & ordering

- A level = one `lüVl` resource. **Order and number come from the resource ID: level N ↔ ID
  300+N** (first = 301). **Loading is by NAME** (`GetNamedResource`) — `gPlayer.level` stores
  the level *name*; the picker and win-bookkeeping translate number→name via the resource map.
  Names must therefore be unique; IDs must be contiguous from 301. **v1.0 violated this**: it
  shipped two levels both named "Trouble" (IDs 339 and 349), so reaching level 49 loaded the
  wrong board by name — the "winning level 49 would cause the game to crash" bug the 1.1
  Version History mentions; 1.1 renamed 349 to "Thought".
- v1.1 ships **51** resources: IDs 301–350 = the 50 real levels ("Warm Up" … "Brainiac"), plus
  ID **351 "You Win!"** — a playable end-screen level, new in 1.1 (v1.0 ends at 350). Winning
  level 50 loads "You Win!"; it is presumably uncompletable (winning *it* would look up ID 352,
  fail, and hit the fatal `GenericError('Error: ldcpbd1')`).
- Level count for cheats/pickers = `Count1Resources('lüVl')` (51). The picker list caps at 100
  names; `skipSet` caps at 150.

### 8.2 Player progress fields

`current` = level number now loaded/being played; `highLev` = frontier (highest reachable);
`skippedLev` = set of skipped level numbers; `numSkipped` = its cardinality (≤3). Color/choose
rules in §6.5.

### 8.3 On winning level N (`loadNextLevel` block in `peckMain`)

```
myNewLevName := name of resource (N + 301)        // i.e. level N+1
gPlayer.current := N + 1
gPlayer.level := myNewLevName; LoadBoard(it)
if N in skippedLev: remove it, numSkipped -= 1     // beat a previously skipped level
if highLev = N: highLev += 1                       // advanced the frontier
SavePlayer; GotNewBoard; DrawStatus
```

Note: beating a *skipped* level (blue) still advances you to level N+1 — `current` always
becomes N+1 regardless of where the frontier is. Replaying a green (passed) level is allowed
any time via the picker.

### 8.4 No score, no per-level time, no demo

The only per-player stats are cumulative play time and progress. Nothing else is recorded.

### 8.5 Cheats / easter eggs

- Player named **`NoRtOfNi42`**: on create/open, `highLev := Count1Resources('lüVl')` — every
  level unlocked. While playing under that name, **Majik Murphy™**: keys I/J/L/M (raw 34/38/37/46)
  teleport-walk Murphy one cell up/left/right/down through anything (swaps `whatToBlank`,
  draws icon 198, scrolls normally, no rules applied).
- \>99 red disks explodes Murphy (RULES §6.3). `Mrfy` 0 signature resource (§0.3). The shipped
  1.1 `plål` autoload record still contains remnants of the string "NoRtOfNi42".

---

## 9. Player records & persistence

### 9.1 `blüG` resource (one per player, name = player name, ID = `UniqueID`)

`PlayerType` — 552 bytes, big-endian, verified against the shipped "Zippy" record:

| Offset | Size | Field | Notes |
|---|---|---|---|
| 0 | 20 | `skippedLev: set of 1..150` | Pascal bitset (bit k = level k skipped) |
| 20 | 2 | `highLev: integer` | frontier level number |
| 22 | 2 | `current: integer` | current level number |
| 24 | 2 | `numSkipped: integer` | |
| 26 | 256 | `name: Str255` | length byte + chars (MacRoman) |
| 282 | 256 | `level: Str255` | current level **name** |
| 538 | 4 | `time: longint` | cumulative play time in ticks |
| 542 | 10 | `keySet: array[1..5] of integer` | up, left, right, down, special (raw key codes) |

Defaults for a new player: empty set, 1, 1, 0, name, 'Warm Up', 0, {91,86,88,84,49} (keypad
8/4/6/2 + space).

### 9.2 `MuPf` 128 "User Preferences" — 2 bytes: `wantsMusic: boolean; wantsSound: boolean`
(1 byte each). Read at boot (`GetPrefs`; created as TRUE/TRUE if missing), written **only at
quit** (`SetPrefs`). Lives in the Players datafile in shipped builds (it was created via
`AddResource` into whatever `curResFile` was — port: just a settings blob).

### 9.3 When players are saved (`savePlayer` — writes the whole record + `UpdateResFile`)

- On create, rename, choose-level OK, Set Keys OK, every level win (`SavePlayer` in the
  bookkeeping block), and every level end via `AddToTime` (win *and* death — so cumulative time
  is never lost). `savePlayer` also refreshes the live key-binding globals from `keySet`.
- `[1.1]` `plål` 128 "AutoloadPlayer": a Str255 holding the last-used player name; read at boot
  to auto-open that player.

---

## 10. Palettes / color

### 10.1 The important fact

**All in-game art (icl8 tiles, the status bar PICT, dialog PICTs) is authored against the
standard Macintosh 8-bit system CLUT.** The game runs the main window with the default palette
(`NormalPal` restores the device CLUT after the splash screens). For the port: decode `icl8`
pixels through the standard Mac 256-color table, and decode PICTs with their own embedded color
tables (PICT pixmaps carry a CLUT). No custom game palette exists at runtime.

### 10.2 `SetPictPalette` (PictPal.p) — splash-only

A hard-coded 256-entry `pmTolerant+pmExplicit` palette used **only** while PICT 3000 is on
screen: entry 0 = white, entry 255 = black, entries 1–254 a designer ramp of purples/magentas/
dark reds (16-bit components; full table in `PictPal.p`, lines 24–1049 — port only needs it if
it reproduces the original splash rendering path; rendering PICT 3000 via its embedded CLUT is
equivalent). `GrayPal` (main.p) similarly builds a 254-step white→black ramp for the Q-logo
sequence. Fades between screens are gamma fades (`FadeToBlack(2)`/`FadeFromBlack(2)` ≈ 50 steps,
near-instant), not palette animations.

### 10.3 Named UI colors (`myRGBColors.p`, 16-bit components)

| Name | R,G,B | Used for |
|---|---|---|
| myBlackColor | 0,0,0 | fills |
| myWhiteColor | 65535×3 | |
| myLightGrayColor | 47476×3 (`smallfract(637300)` truncates mod 65536) | bevel highlight |
| myGrayColor | 32856×3 (`smallfract(295000)`) | help text bg |
| myDarkGrayColor | 18000×3 | bevel shadow, text shadow |
| myRedColor | 65535,3500,3500 | player name, disk count, locked levels, carets |
| myYellowColor | 65535,45000,0 | frontier level, error text |
| myBlueColor | 9191,9716,65535 | clock, skipped levels, disk count when 0 |
| myGreenColor | 2845,53247,2845 | level name/number, passed levels, infotron count at 0 |

(`smallfract()` = truncation to 16 bits; the two gray constants overflow intentionally.)

---

## 11. Sound & music

### 11.1 Effect resources (shipped v1.1 "Infotron Sounds") vs code constants

| ID | Constant | Shipped name | Trigger |
|---|---|---|---|
| 128 | sGotInfotron | GotInfotron | collect infotron (not the last) |
| 129 | sThud | Thud | faller lands (RULES §4.4) |
| 130 | sOHit | — *(not shipped)* | never played |
| 131 | sExplode | Explode | explosion frame 3 |
| 132 | sQuarkExp | QuarkExplode | quark explosion |
| 133 | sGotRedDisk | GotRedDisk | collect **and** use red disk |
| 134 | sRedDiskReady | RedDiskReady | special key held 90 ticks with a disk |
| 135 | sGotLastInfo | GotLastInfo | collect final needed infotron |
| 136 | sAboutBox | — *(not shipped)* | never played |
| 137 | sMurphyDied | MurphyDied | death |
| 138 | sPause | Pause | pause |
| 139 | sUnpause | Unpause | unpause |
| 140 | sBugZap | BugZap | bug activation, distance-gated ≤4 tiles |
| 141 | — | Push | pushing zonk/green/yellow |
| 143 | — | — *(1.0-era logo sound; not shipped)* | boot logo (repo code) |
| 144–146 | — | Click, Tick, Ding! | `[1.1]` UI sounds (binary-only usage) |
| 150 | — | QLogoSnd | `[1.1]` boot logo |
| 151 | — | endLevel | `[1.1]` end-of-level |

This confirms the RULES §13 table. Only the bug zap uses distance attenuation
(`PlaySoundDistRange`, linear falloff, Manhattan distance, range 4; `kExplRange=22`/
`kThudRange=20`/`kHearingRange=20` exist but are unused in the final code).

### 11.2 Engine (`unit QIST` in InfoSounds.p)

4 sampled-sound channels for effects (first-free allocation, amp 255, callback frees the slot;
if all 4 busy the sound is simply dropped) + channel 5 dedicated to music. `SoundState(false)`
(Sound Effects off) gates `PlaySound`/`PlaySoundDist*` — **it does not gate music**. Music =
`ContSound(id)`: loads the whole `snd ` into memory and replays it endlessly via completion
callback; `ContSoundStop` disposes the channel. `HaltSounds` (pause) quiets fx and stops music.

### 11.3 Music selection (`SmoothMusic`, kSecondsForMusic = 120)

Repo logic: only while playing & unpaused; if no music playing, start `musicID`; every 120 s
re-roll `ARandom(3)+999` ∈ {1000,1001,1002} and switch if different. **Shipped music files
(both versions) contain only 1000 "music Intro" and 1001 "m.loop"** — a roll of 1002 silences
music until the next 120-s tick (or the shipped binary's logic differs, e.g. intro-then-loop;
its names strongly suggest intro→loop was the design). Music never plays on menu screens.
Preference `hasMusic` on → next play frame starts a track; off → immediate stop. Read Me: music
is optional; the file may be deleted (code tolerates missing resources = silence), and low
memory (<~3 MB) can make music silently fail — port should treat music as fully optional.

---

## 12. Input reference (shell level)

### 12.1 During play (all raw `GetKeys` keymap polling, once per 5-tick frame)

- Movement: the player's 4 direction codes; priority when several held: **Up, Right, Down,
  Left** (single move per frame). `[1.1]` "improved keyboard control: you can now move
  diagonally by holding two keys down" — exact shipped algorithm unknown (binary only);
  most plausibly alternating axes on successive frames.
- Special (default Space, code 49): with a direction = snap; alone = red-disk arm/drop
  (RULES §6.4/6.6).
- **Esc (53)** = kill Murphy. **Caps lock (57)** = pause toggle (§7.3).
- `[1.1]` during the 4-s death freeze: **r (15), p (35), s (1)** = instant restart; at the 1.1
  "Care To Try Again?" prompt: **y (16)** = restart.
- Cheat keys I/J/L/M for MajikMurphy (§8.5).

### 12.2 Key naming (`changeKey`, myRGBColors.p)

Scans codes 0–51 then 60–122 (last hit wins), shift = code 56. Full US-ADB mapping to
characters; notable: 49→'_' (space), 51→'~' (backspace), 50→'`', keypad 82–92 → pseudo-chars
'a'–'j' (displayed as digits 0–9 in the keys dialog), keypad-clear 71 → 'Ÿ'. Anything unmapped
→ '+' (ignored). Key bindings are stored as **raw key codes** per player; the web port should
map to `KeyboardEvent.code` equivalents and default to Numpad8/4/6/2 + Space with arrows as a
sensible modern default.

### 12.3 Outside play

Dialog keys per §6.1 (Return/Esc/arrows/double-click). Menu cmd-keys per §5.1. Esc during play
is suicide but in dialogs is cancel; caps lock does nothing outside play. The shipped
"Using a Numeric Keypad" doc warns of ADB hardware quirk (keypad between computer and keyboard
makes keyboard keys read unpressed) — irrelevant to the port beyond trivia.

---

## 13. v1.0 vs v1.1 (shipped) — differences worth honoring

From "Version History"/"Read Me!"/"Upgrade Readme!" + resource diffs:

1. **Diagonal movement** added in 1.1 (two direction keys at once). 1.0/repo = strict
   U>R>D>L priority.
2. **Player autoload** (`plål` resource) added in 1.1.
3. **Retry UX** added in 1.1: "Care To Try Again?" prompt + y; fast-restart r/p/s during the
   4-s death freeze.
4. **Auto 8-bit depth switching** in 1.1 (1.0 refuses to run unless already 256 colors).
5. Bug fixes in 1.1 (all present as bugs in the repo sources — implement the 1.1 behavior):
   - Quarks can no longer "dodge" boulders (falling zonk vs. moving quark race).
   - **Quark explosions no longer detonate yellow disks** (repo `CQuark.Explode` calls
     `Explode` on neighbors unconditionally; yellow disk's full explosion is reachable that way
     even though `explodes=FALSE` protects it from normal chains).
   - Snapping a falling infotron no longer duplicates it.
   - Error-message drawing bug + menu-redraw-after-error bug fixed.
   - Level resource ordering fixed (win on level 49 crashed 1.0).
6. Menu re-arrangement (§5.1): 1.1 = Play ⌘P / no Pause item / ⌘H Help.
7. Art/resources: PICT 3000 grew to full 640×480; keys-dialog overlays 303–308 (32×32) →
   500–507 (27×18); scissor-turn icl8s 1150/1152/1154/1156 added; sounds 144/145/146/150/151
   added, 143 dropped. Level lists differ: 1.0 = 50 levels (301–350) with a duplicate name
   "Trouble" at 339 & 349 (the level-49 crash) and "Quick!" at 304; 1.1 renames 349→"Thought",
   304→"Quick", swaps 303/304 to Buggy/Quick order, and appends 351 "You Win!". (The repo
   `firstTen` array — "…Quick!, Buggy, Down they fall!, …, Outsie" — reflects an even older dev
   ordering; only `firstTen[1]='Warm Up'` is ever used.)
8. An alternate piece-art set by Daniel Sandler ships in the v1.1 "Pieces" folder (swap the
   Pieces datafile) — nice optional skin for the port.

---

## 14. Port decisions (recommendations)

1. **Target the v1.1 shipped behavior** everywhere it's knowable: 1.1 menu layout, autoload
   player, retry prompt + fast-restart, the five 1.1 bug fixes, 51-level file with "You Win!".
   Where 1.1 behavior is binary-only (diagonal input, exact boot art sequence, "Care To Try
   Again?" presentation, PICT 500–507 usage), implement a sensible reconstruction and mark it.
2. **Geometry**: render a fixed 640×480 virtual screen (board viewport 640×419 with the
   (−16,−16) half-tile grid offset per §1.3, whole-tile scroll steps; status bar per §2) and
   scale it to the browser window (letterbox = the original big-monitor black surround; the
   triple gray frame is optional flavor). Do not "fix" the viewport to whole tiles — sliver
   columns/rows are authentic and level-visible.
3. **Assets**: decode resources at build time into web-native assets: icl8 → PNG atlas via the
   standard Mac CLUT (don't reproduce the 180-slot atlas overflow bug — size dynamically);
   PICTs via their embedded CLUTs; levels → JSON (decompress the RLE once, keep both number and
   name); `I†i†` tutorial slides → JSON script for the help screen; sounds/music → wav/ogg
   (music = intro-then-loop; drop the random-of-3 rotation, or keep a 120-s re-roll as a no-op).
4. **Persistence**: replace resource-file records with localStorage: a players map keyed by
   name storing the §9.1 fields (skipped set as an array), a prefs blob (sound/music), and the
   autoload name. Key levels by **number** internally (names for display only) — the original's
   lookup-by-name is fragile. Save at the same moments (§9.3).
5. **Level end**: keep the 4-s freeze-and-simulate before returning to the shell; auto-load the
   next level on win but require an explicit "Play" (or add auto-advance as an option).
   Implement a real Abort (menu/Esc-from-pause) — the original's Abort is a dead menu item, but
   the *intent* is clear and modern UX needs it. After "You Win!" (level 51), show a finished
   state instead of the original's fatal-error-on-win.
6. **Timing hygiene**: convert every busy-wait/blocking construct to frame-based async — boot
   logos (make them skippable with any key/click, as original clicks did), the 9/12-tick death
   `Delay`s, snap animations drawn 8-frames-within-one-frame (render as a fast flash), dialog
   modal loops, About/Help animations. Base the red-disk fuse and end-of-level timers on
   **game time**, not wall clock, so pause doesn't eat fuses (deviation from original, §7.3).
7. **Pause**: caps-lock-as-pause doesn't translate to the web (no reliable toggle-state);
   use P/Esc-menu pause instead, keep the pause/unpause sounds and the show-cursor/menu
   affordance. Keep Esc = suicide *during play only* (guard against it while any UI is up).
8. **Menus/dialogs**: reimplement as HTML/canvas UI with the same structure and copy (§5, §6 —
   the error strings and help texts are worth preserving verbatim for flavor). The PICT-based
   dialog framework itself should not be ported. Keep the level picker's color code
   (green/yellow/blue/red) and 3-skip rule exactly.
9. **Registration/shareware**: drop `doReg`/'QarD'/the startup nag dialog (or show PICT 128
   once as a historical splash). Keep the About box with the scrolling credits PICTs.
10. **Cheats**: keep `NoRtOfNi42` + Majik Murphy and the >99-red-disk explosion — they're
    harmless heritage. Keep the `Mrfy` aardvark string somewhere.
11. **Sound engine**: 4 concurrent effect voices with drop-when-full, plus a music channel,
    reproduces the original mix; only bug zaps are distance-attenuated (linear, Manhattan,
    range 4). Sound-off must not mute music and vice versa.
12. **Suspend/resume**: browser tab blur ≈ suspend — auto-pause (original just shrank to a
    badge; auto-pause is the faithful-in-spirit choice since the original froze play by
    starving the loop).

### Open questions (need emulator run or binary disassembly of shipped 1.1)

- Exact 1.1 boot visuals/order and what plays QLogoSnd; exact "Care To Try Again?" screen.
- Exact 1.1 diagonal-movement algorithm.
- Usage sites of 1.1 sounds 144 "Click"/145 "Tick"/146 "Ding!" and PICTs 500–507.
- Whether the shipped binaries changed `SmoothMusic` to intro→loop (names imply yes).
- Whether 1.1 enlarged the icon atlas (it must have, or win-anim frames 1209–1211 were corrupt).
