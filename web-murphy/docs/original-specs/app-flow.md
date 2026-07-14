# Murphy's Revenge — Application Flow, Loop Timing, Camera, Input, Interface, HUD, Persistence

Spec extracted from the 1997 Mac OS 7 source in
`/Users/dave/dev/games/Murphy/legacy/murphy/os7code/mr/` (files: `main.c`, `main.h`,
`MRLoop.c/h`, `MRInterface.c/h`, `MRInit.c/h`, `MRFade.c/h`, `Desktop.c`, `MRPlayers.c/h`,
`MRPrefs.c/h`, `MRRegistration.c`, `RequestVideo.c`, `MRMap.c/h`, `MRPlay.c`,
`MRPlayCommon.c/h`, `MRPlayConstants.h`, `MRPlaySprite.c`, `MRGfxUtils.c/h`, `MRResources.c`,
`MRSound.h`, `MacIncludes.h`), plus the later ports
`/Users/dave/dev/games/Murphy/iPhone/TMEngine.m` (+`MurphyView.m`, `MurphyAppDelegate.m`) and
`/Users/dave/dev/games/Murphy/osx/Murphy/MurphyLevelScene.swift`.

NOTE on source files: the legacy `.c/.h` files use classic-Mac CR (`\r`) line endings and
MacRoman encoding. Rendering/sprite mechanics are covered by other specs; this spec covers
application flow, loop timing, camera, input, interface screens, HUD, and persistence.

Units: 1 Mac **tick** = 1/60 second (`TickCount()`). All tick constants below are 60ths of a
second. Rects are QuickDraw `(left, top, right, bottom)`; `SetRectDelta(l,t,w,h)` means
`(l, t, l+w, t+h)`.

---

## 1. Screen geometry

* The game window is a borderless color window (`WIND` id `kMainWindID`=1000) covering the
  **entire chosen monitor**, filled black. All other monitors get black cover windows
  (`FillRemainingMonitors`, MRInit.c).
* Minimum requirements checked at startup (`MRInit.h`): `kMinimumWidth 640`,
  `kMinimumHeight 480`, `kMinimumDepth 8` (256 colors, 8-bit indexed). The game optionally
  switches the monitor to 640×480×8 via Display Manager (`RequestVideo.c`, Apple sample code —
  not relevant to a web port beyond "the game is designed for 640×480 @ 8-bit").
* All game content lives in a **640×480 "fitRect" centered in the window**
  (`SetRect(&fitRect,0,0,640,480); CenterRect(&fitRect,&wind->portRect)` — used identically in
  MRInterface.c, MRMap.c (`FitRectFromWind`), MRPlay.c (`MakeLevelCBData`)). Everything outside
  is black, except the interface screen on taller-than-480 windows (see banners, §7.1).
  **For the web port: just use a fixed 640×480 canvas.**

### 1.1 Gameplay screen layout (MRPlay.c `MakeLevelCBData`)

```
statRect  = RectFromPictureID(kGameStatPict)            // stat bar PICT, id 159
levelRect = fitRect; levelRect.right -= statRect.right  // viewport = 640 - statBarWidth
statRect offset to (fitRect.right - statRect.width, fitRect.top)   // flush right
InsetRect(&levelRect, mp.screenSize, mp.screenSize)     // 0 / 75 px per side (pref)
```

* The scrolling level viewport occupies the **left part** of the 640×480 area, full 480 tall
  (at normal screen size); the **status bar** is a vertical strip on the **right edge**.
* The stat bar width comes from the PICT resource and is not in the code; the 2014 Swift port
  (`MurphyLevelScene.swift`) uses `STATUS_WIDTH: CGFloat = 40` with `SCENE_WIDTH 640`,
  `SCENE_HEIGHT 480`, status bar positioned at `x = 640-40` — so **stat bar = 40×480 px,
  level viewport = 600×480 px**. (Confirm against the actual PICT if available.)
* Preference `screenSize` ("Smaller screen" checkbox) insets the viewport by
  `kPrefsSmallScreenSize 75` px on every side (toggles 0 ↔ 75 via
  `mp.screenSize = kPrefsSmallScreenSize - mp.screenSize`). A `kPrefsVerySmallScreenSize 125`
  constant exists but is unreachable. This was a 1997 perf option; a web port can drop it.
* Level tiles are **32×32 px** (`kLevelItemSize 32`). Map-room tiles are **24×24 px**
  (`kWorldItemSize 24`).

### 1.2 In-game HUD contents (right-edge stat bar)

All coordinates relative to `statRect.topLeft`. Drawn directly to the window (not part of
the scrolling world). Source: `MakeLevelCBData`, `DrawLevelRoom` (MRPlay.c),
`DrawLevelStatistics`, `DrawNumberInRect(Size)`, `DrawTimeRemaining` (MRPlayCommon.c).

* Background: PICT `kGameStatPict` (159) from the current terrain file; for **timed** levels
  the variant `kGameStatMResPict` (133, from the main app resource fork) is used instead —
  it has the timer readout area.
* **Bubbling Oliver** idle animation, 32×32 at offset (4,4) (`MakeNewBubble`; see §7.6).
* **Level name**, drawn *vertically* with bitmap lettering (`MRLettering`, green variant
  `kGreenLettering`, letter cells 17×17 px from PICT 151 `kGreenLettersPict` / 152 white) in
  rect `(11, 77, 29, 267)` (i.e., `SetRect(11, 73+4, 11+18, 73-4+198)`).
* **Infotron counter**: 16×16 mini icon at (4,324) — icon taken from `kInfoMiniPict` (160),
  column = `extraInfo.levelWorld` (each terrain has its own mini icon in a 4×1 strip);
  count in rect (20,324,36,340) (= icon rect offset +16 h). Shows infotrons **remaining**
  (starts at `reqInfo`, decrements, floors at 0).
* **Red-disk counter**: mini icon `kRDMiniPict` (161) at (4,364); count at (20,364,36,380)
  (= infotron rects offset +40 v). Starts 0, increments on pickup. (Collecting a 10th red
  disk kills Oliver — `EatPiece`, MRPlayCommon.c.)
* **Timer** (timed levels only): three digit boxes, hours `(5,428,10,440)` (1 digit),
  minutes `(12,428,22,440)` (2 digits), seconds `(24,428,34,440)` (2 digits).
* Number/text style: font **Helvetica 9 pt**, color RGB16 `(39168, 52224, 52224)` ≈
  `#99CCCC` (an "aqua green"), on black. Numbers horizontally centered in their rect;
  baseline at `rect.bottom - 4` (counters) or `rect.bottom - 2` (+1 px right; timer digits).
  Counters redraw only when the value changed (`last*` caches; caches reset to −1 on unpause).

### 1.3 Map-room screen layout (MRMap.c `MakeMapCBData`)

* Left panel: PICT `kMapRoomPict` (135) at fitRect.topLeft, with the **back arrow** PICT
  `kBackArrowPict` (153; hilite 154) at its bottom-left.
* Right panel: PICT `kMapStatPict` (146; hilite 147) flush right.
* Scrolling **map viewport** = the horizontal span between the two panels
  (`mapRect = (mapRoomRect.right, fitRect.top, mapStatRect.left, fitRect.bottom)`, inset by
  `screenSize`). Panel widths come from the PICTs (not in code).
* Map stat panel contents: bubbling Oliver at (4,4) 32×32; **player name** vertical lettering
  in `(11,77,29,267)` (same rect as level name); **total play time** string (Helvetica 9,
  same #99CCCC) drawn at pen position `(mapStat.left+4, mapStat.top+331)`; **switch-player
  button** hotspot `(10, 352, 30, 467)` (clicking opens the player-select dialog; only enabled
  while the map sprite is standing still — `safeToSwitch`).
* Play-time string format (`MakePlayerTimeString`, MRPlayers.c): `H:MM.SS` if days==0 and
  hours≤9; `HH:MM` if days==0; `Dd HH:MM` if days≤9; `DDd HHh` otherwise.

---

## 2. Event loop & frame timing

### 2.1 The shared event loop (MRLoop.c)

One generic WaitNextEvent loop (`MREventLoop`) drives *every* state (interface, map room,
gameplay, dialogs) via swappable callbacks:

* `MRSetClickCallback` — mouse-down in window content (point already in local coords).
* `MRSetKeyCallback` / `MRSetKeyUpDownCallback` — key events; `MRAllowKeyUp()` widens the OS
  event mask to include key-ups (needed for game input and menu "press/release" behavior).
* `MRSetIdleCallback` — called for every null event; **this is the game tick**.
* `MRSetUpdateCallback` — window update events.
* Callbacks return `true` to end the loop. `loop_userForcedQuit` (set by **Cmd-Q** anywhere)
  also ends the loop.
* `MRSetSleepTime(n)`: WNE sleep in ticks. `kDefaultSleepTime 0x10` (16 ticks) is the idle
  default; the interface, map room, prefs dialog, and gameplay all set sleep **0** (busy poll:
  `OSEventAvail` then WNE with sleep 1) so idle callbacks run as fast as possible.
* App-switching: normally suppressed; while **paused** in-game, `MRAllowSwitches(true)` lets a
  click shrink the game to a 32×32 icon window at the bottom-left of the monitor (32×32 PICT
  21314) and shows the menu bar (Apple menu id 300, File menu id 301 with Play=item 1,
  Quit=item 3). Irrelevant to a web port beyond "pause hides the game".

### 2.2 Frame rate

Rendering/animation is done by SpriteWorld scrolling worlds pumped from the idle callback:

```
SWProcessScrollingSpriteWorld(sw);   // logic: move procs, frame procs, scroll proc
SWAnimateScrollingSpriteWorld(sw);   // blit changed tiles/sprites, then screen
```

* **FPS cap = 30** for both gameplay (`kMaxLevelFPS 30`, MRPlayConstants.h) and map room
  (`kMaxMapFPS 30`, MRMap.c) via `SWSetSpriteWorldMaxFPS(sw, 30)`; VBL sync off
  (`SWSyncSpriteWorldToVBL(sw, false)`).
* So the **game logic step = one SpriteWorld frame at ≤30 fps** (nominally every 2 ticks).
  `Level_IdleBack` runs "chores" (queued tile animations: explosions, exit opening, zaps…)
  once per *rendered* frame: `if (cb->sw->frameHasOccured) HandleAllChores(...)`.
* The countdown timer runs on real time, not frames: each elapsed 60 ticks decrements `secs`
  (see §9.4).
* Later ports: iPhone port ticks a timer at **1/60 s** (`setAnimationRate:1.0/60.0`,
  MurphyAppDelegate.m/MurphyView.m); SpriteKit port uses the default 60 Hz `update:`.
  **Recommendation for web port: 60 fps requestAnimationFrame with game-logic movement values
  expressed per 1/30 s step (or halved per 60 Hz frame).**

### 2.3 Movement speeds (pixels per frame @ ≤30 fps)

* Gameplay (`MRPlayConstants.h`): `kSpriteMoveDelta 8` px/frame for Oliver → a 32-px tile
  takes **4 frames ≈ 133 ms**. Pushing a zonk: `kSpriteMoveDelta/2` = 4 px/frame (8 frames ≈
  267 ms/tile). Movers (scissors/quarks): `kMoverSpriteMoveDelta 4` px/frame (8 frames/tile).
* Map room (`MRMap.c`): `kSpriteMoveDelta 4` px/frame with 24-px tiles → **6 frames ≈ 200 ms
  per map tile**.
* Movement is grid-locked: key input is only sampled when the sprite is exactly on a grid
  boundary (`destFrameRect.top/left` divisible by tile size); between cells the sprite glides
  at the deltas above.

### 2.4 Tile/sprite animation cadences

* Gameplay tile animations (`Tile_ChangeProc`, MRPlay.c): idle infotron and terminal tile
  images advance every `kSpriteFrameTicks 2` ticks (≈30 fps cycling through their frame runs).
* Oliver's own sprite frame time: `kOliverBubbleMilli 100` ms; quark `kQuarkFrameMilli 66` ms;
  generic sprite `kSpriteFrameMilli 33` ms (MRPlayConstants.h).
* Map-room ambient tile animation periods in ticks (MRMap.c `Tile_MapChangeProc`): ocean
  waves 12 (4 variants, tiles 238–241 cycling), level beacons 10 (six 8-frame runs: tiles
  1–8, 19–26, 37–44, 55–62, 73–80, 91–98), whirlpool 17 (tiles 256–259), "thing2" 15 (tiles
  186–188), machine 15 (tiles 288–293), "grab thing" (tiles 168–170): random start with
  probability 75/1000 per check, animates up then resets.
* Map sprite (Oliver's head): 2 frames (`SWSetSpriteFrameRange(0,1)`) at **500 ms**
  (`kSpriteFrameMilli 500` — MRMap.c redefines this locally; do not confuse with the
  gameplay 33 ms constant).
* iPhone/Swift ports use: infotron 0.075 s/frame; quark, terminal, scissors, Oliver-bub
  0.175 s/frame.

---

## 3. Camera / scrolling

Identical algorithm in map room (`Scroll_WorldMove`) and gameplay (`Scroll_LevWorldMove`),
run once per frame by SpriteWorld's scrolling-world move proc:

```
midRect = sprite-frame-sized rect centered in the offscreen work area (backRect)
midLeft = midRect.left + visScrollRect.left      // world-x of "sprite centered" position
midTop  = midRect.top  + visScrollRect.top

horizScrollDelta = (kSpriteMoveDelta * (sprite.left - midLeft)) / kSpriteMoveDistance
vertScrollDelta  = (kSpriteMoveDelta * (sprite.top  - midTop )) / kSpriteMoveDistance
```

with `kSpriteMoveDistance 100` and `kSpriteMoveDelta` = 8 (gameplay) / 4 (map). I.e. a
**proportional follow camera**: per-frame scroll speed = spriteSpeed × (pixel distance of
sprite from viewport center) / 100, integer division. Properties:

* No dead zone in the formula, but because of integer division the camera stops moving once
  the sprite is within `100/kSpriteMoveDelta` px of center (12 px in gameplay, 25 px on map)
  — an effective small dead zone / ease-out ("smooth scrolling with snap-back" per the
  comment in MRPlayCommon.h).
* When the sprite moves at full speed away from center, the camera accelerates the farther
  the sprite gets from center; equilibrium distance is 100 px (where camera speed equals
  sprite speed).
* Scroll bounds: `SWSetScrollingWorldMoveBounds` = the whole tile map
  (level: `levSize * 32`; map: `120*24 = 2880²` px). SpriteWorld clamps `visScrollRect` to
  these bounds (edge clamping — camera never shows outside the map).
* Initial position: `SWMoveVisScrollRect(sw, sprite.left − backRect.right/2, sprite.top −
  backRect.bottom/2)` — sprite roughly centered, clamped to bounds.
* If Oliver is dead (gameplay), scroll deltas are forced to 0 (camera freezes).
* Interlaced-mode pref forces `vertScrollDelta` even (`>>1<<1`). (Drop for web.)
* Offscreen (`offRect`) is the viewport rounded **up** to a multiple of the tile size in each
  dimension.

---

## 4. Input

### 4.1 Configurable keys (MRPrefs)

`MRPrefs` char fields with defaults (`MRLoadPreferences`, MRPrefs.c):

| Action | Field | Default | Notes |
|---|---|---|---|
| Up | `upKey` | `'8'` | numeric-pad style defaults |
| Down | `downKey` | `'2'` | |
| Left | `leftKey` | `'4'` | |
| Right | `rightKey` | `'6'` | |
| Special (snap/zap & red-disk drop) | `specialKey` | Space `' '` | held modifier |
| Pause | `pauseKey` | Tab `'\t'` | acts on key-up |
| Suicide (kill Oliver / restart) | `suicideKey` | Esc `'\33'` (0x1B) | immediate |

All are single Mac char codes; the prefs UI displays arrows/special keys as strings
("Up","Lt","Rt","Dn","Ret","Esc","Spc","Ent","Tab","Home","Help","PgUp","PgDn","End") —
so **arrow keys are assignable** (Mac char codes 0x1C–0x1F).

### 4.2 Key-stack model (gameplay `Level_KeyBack` and map `Map_KeyBack`)

Both keep a most-recent-first pressed-direction stack `theKeys[4]` (`kMaxLevelPresses` /
`kMaxMapPresses` = 4) + `numKeys`. Key-down pushes the direction on the front; key-up removes
it. The movement code reads only `theKeys[0]` — i.e., **the most recently pressed,
still-held direction wins**; releasing it falls back to the previously held one. The stack is
force-cleared on unpause and when opening the player-select dialog (stuck-key workaround).

### 4.3 Other gameplay keys

* `specialKey` down/up sets `cb->specialKeyDown`; with a direction key it "zaps" (eats an
  adjacent piece without moving — snap); held alone ≥ `kRedDiskDropDelay 30` ticks it starts
  the red-disk-drop animation, completing at `kRedDiskDropFinish 90` ticks.
* `pauseKey` (on key-up) toggles pause (§9.3).
* `suicideKey` → `KillOliver(oliverSprite, false)` immediately.
* **Cmd-Q** quits from anywhere (MRLoop.c). Every keypress refreshes `lastPressTime` (drives
  Oliver's boredom animations: sad face at 90 s, rude gesture 90.5–91.5 s, "eureka"
  91.5–92 s, yawn ≥92 s since last keypress — `kSadStartTicks (90*60)` etc.).
* On the death/replay dialog: `y`/`Y`/Return/Enter = play again, `n`/`N` = back to map.
* Map room: Return/Enter (char 13 or 3) while standing on a level tile = **play that level**
  (immediately if the info popup is up; on key-up otherwise).

### 4.4 Mouse

Mouse is used in all non-gameplay UI: main-menu hotspots with hover + pressed states, prefs
dialog, player-select dialog, map-room back arrow / switch-player / info-popup **Play**
button, death dialog Yes/No buttons. Buttons universally use press-track-release semantics
(`TrackGraphicalButton`: hilite while inside, act only if released inside; plays `kTickSnd`
145 on press, `kClickSnd` 144 on successful release). During gameplay the cursor is hidden;
it is shown when the death dialog appears. In-game clicks do nothing while Oliver is alive.

---

## 5. Top-level application flow (main.c)

```
main():
  StartUpMR()                 // toolbox init, datafile check (res files), machine check
  IntroSequence()             // splash screens (below)
  if (!MRIsRegistered()) DoRegistrationDialog()   // both are no-op stubs — see §11
  InterfaceLoop()             // interface <-> map room <-> gameplay
  ExitSequence(); CleanUpForExit()
```

* `InterfaceLoop`: `RunInterface(wind)` returns `kErrorReturn 1` / `kMapRoomReturn 2` /
  `kQuitReturn 3`. On `kMapRoomReturn` → `MapLoop()`; on quit/error → exit app.
* `MapLoop`: `RunMapRoom(wind)` returns `kMapErrorReturn 1` / `kMapQuitReturn 2` /
  `kMapBackReturn 3` (→ back to interface) or `kMapPlayReturn 10 + levelNumber`
  (→ `PlayLoop(levelNumber)`).
* `PlayLoop(levelIndex)`: repeats `RunGameLevel(wind, levelIndex)` while it returns
  `kPlayAgainReturn`; `kBackToMapReturn` → back to map room (map is fully rebuilt);
  `kPlayErrorReturn`/`kPlayQuitReturn` → quit app. (Return constants in MRPlay.h:
  error 1, quit 2, backToMap 3, playAgain 4.)

### 5.1 Boot / splash (`IntroSequence`)

1. Optional monitor depth/resolution dialogs (drop for web), screen faded out.
2. Black window; **RedLine logo** PICT 132 (`kRedLineLogoPict`) centered; gamma **FadeIn**;
   wait 2 s or mouse click (`DelayMouseTime(2)`).
3. `FadeOutToColor` to RGB16 (24320, 36864, 45312) ≈ `#5F90B1`; start **interface music**
   (`PlayMusic(kInterfaceMusic 1000, loop)`); set custom palette; fill with that color; draw
   **title splash** PICT 131 (`kSplashScreenPict`) centered; FadeIn; wait 5 s or click
   (`DelayMouseTimeMusic(5)`); FadeOut; restore palette; `FadeOutImmediate` (stay black for
   the interface to fade in).

Interface music starts here and keeps playing through the main menu (RunInterface itself has
its `PlayMusic` commented out — after returning from the map room to the menu, **no music
plays**, a quirk/bug to fix or replicate).

---

## 6. Fades (MRFade.c) — what to mimic

Wraps Matt Slot's gamma fader; fades operate on the **monitor gamma**, i.e. the whole screen
fades, content untouched underneath.

* `kNumStepsInFade 50` steps.
* `FadeOut()` → black, **quadratic** easing; `FadeIn()` → restore, **inverse-quadratic**;
  `FadeOutToColor(c)` / `FadeOutToWhite()` — fade to arbitrary color (used: splash-blue,
  and white after video-mode switch with `kLogoSnd` 150).
* `FadeOutImmediate()` / `FadeInImmediate()` — 2 steps, linear (effectively a snap).
* State guarded by `fade_screenIsFaded`; redundant fades are no-ops. If gamma fading is
  unavailable, all fades are disabled (instant cuts).
* Web equivalent: full-screen overlay animating opacity over ~50 steps (the original has no
  fixed per-step delay in this code; total duration is hardware-speed-bound. Suggest
  ~400–600 ms, quadratic ease).
* Usage pattern: fade to black **before** rebuilding a screen, draw, then fade in.
  `RunInterface` fades out → draws menu → fades in. Map room and level fade in after drawing,
  and use `FadeOutImmediate` after their exit wipe.

### 6.1 Wipe transitions (MRInterface.c / MRGfxUtils.c)

* `WipeWindow` / `WipeWindowFX` ("eye candy"): blacken the 640×480 area by horizontally
  sliding 32-px-tall stripes (`kWipeStep 32`, `kWipeHeight 32`), alternating direction per
  stripe row, 20 steps, ~2 ticks per step, filling revealed area black. Used when leaving the
  menu for a sub-screen, leaving map room, and leaving a level.
* `WipePictIntoWindow(pict)`: reveals a picture bottom-up in 32-px slices, `kWipeDelay 3`
  ticks per step (help/intro/credits screens).
* `RevealPrefs`/`PSReveal`: dialogs reveal from vertical center outward in `16`-px slices
  (`kPrefRevealStep`/`kPSRevealStep 16`) every 3 ticks (`k*RevealDelay 3`), with whoosh sound
  (id 200 in Interface file).
* `DimRectangle(r)`: draws black lines every 2 px horizontally *and* vertically over a region
  — a "dim/disable" screen-door effect shown behind modal dialogs (prefs, player select,
  delete-confirm) and under the pause overlay.
* Map-room switch flips animate with a SpriteWorld **cross-fade transition**
  (`SWPerformVisualTransition(SWCrossFadeTransition, 4, 4)`) limited to affected tiles.

---

## 7. Main interface (menu) — MRInterface.c

### 7.1 Layout & art

* Menu background PICT `kInterfacePict` 129 centered in the window; two more full-screen
  variants provide button states: `kMouseInterfacePict` 142 (hover) and `kSelInterfacePict`
  130 (pressed). Item drawing copies the item's rect from the appropriate variant.
* On windows taller than 480 px (`BiggerWindow`), decorative banners: `kQSoftBannerPict` 139
  centered at top and `kMRBannerPict` 140 at bottom, with edge-extension fills
  (top-left RGB16 (39168,39168,52224), top-right (0,26112,39168), bottom-left
  (65535,52224,39168), bottom-right (65535,39168,0)). Irrelevant at fixed 640×480.

### 7.2 Hotspots (offsets relative to the centered 640×480 menu picture; w×h form)

| Item | Rect (x, y, w, h) | Hotkey |
|---|---|---|
| Intro (story) | (32, 96, 120, 40) | `i` |
| Credits/About | (32, 156, 120, 40) | `c` |
| Help | (32, 216, 120, 40) | `h` |
| Quit | (32, 276, 120, 40) | `q` |
| **Start** | (258, 343, 222, 97) (222×129 on tall windows) | `s`, Return, Enter |
| Preferences | (9, 400, 182, 32) | `p` |
| (Oliver bubble, not a button) | (304, 198, 32, 32) | — |

* Hover: idle callback polls the mouse; entering a hotspot draws the hover state
  (no sound); leaving restores. Clicking tracks press state with `kTickSnd`/`kClickSnd`.
* Hotkeys hilite on key-down and act on key-up of the same key (press/release like a button).

### 7.3 Actions

* **Quit** → fade music out, return `kQuitReturn`.
* **Start** → fade+stop music, hide cursor, full `WipeWindow` (including banners), return
  `kMapRoomReturn`.
* **Intro / Credits / Help** → `WipeWindow`, then `WipePictIntoWindow` of PICT `kIntroPict`
  136 / `kAboutPict` 137 / `kHelpPict` 138, then draw a text overlay PICT `pictID + 7`
  (143/144/145) centered then offset +40 px horizontally. State changes to
  kIntroState/kCreditsState/kHelpState; **any click or Return key-up** wipes back to the
  menu.
* **Preferences** → in-place modal (below).

### 7.4 Preferences dialog (`BeginPreferences`)

* Screen dimmed (`DimRectangle` over whole 640×480), dialog art `kPrefLoPict` 1000 (normal) /
  `kPrefHiPict` 1001 (hilite states), centered; revealed center-out (§6.1); bubble Oliver at
  (83,120,32,32) within the dialog.
* Tracked boxes (`SetRectDelta` = x,y,w,h within dialog): Up key (77,68,44,42), Right
  (125,114,42,44), Left (31,114,44,44), Down (77,162,44,42), Special (129,213,34,17), Pause
  (129,238,34,17), Suicide (129,263,34,17); Music vol arrows L(27,317,10,11) R(161,317,10,11),
  vol icon at (37,313); Sound vol arrows L(27,363) R(161,363), icon at (37,359); checkboxes
  Interlace (17,385,12,11), Smaller (17,404,12,11), Compatible (17,423,12,11); Cancel
  (11,440,92,468 — l,t,r,b) and Okay (108,440,189,468 — l,t,r,b).
* **Key remapping**: click a key box → box hilites (direction boxes swap in the hilite art
  and redraw the key label; other boxes get a white frame); next **key-up** assigns the key.
  Assigning a key already used by another action is rejected with an error sound
  (`kPrefBadKeySnd` 201) and deselects. Key labels drawn via `DrawCharBox` — Helvetica bold,
  white, 12 pt for direction boxes / 10 pt for the small boxes.
* **Volume steps**: 4 levels — `kSilentVolume 0`, `kAudibleVolume 85`, `kPleasantVolume 171`,
  `kExcruciatingVolume 255`, displayed as PICTs 1002–1005 (silent/audible/pleasant/
  excruciating). Right arrow increases (wraps 255→0), left arrow decreases (wraps 0→255).
  Separate music and sound volumes.
* Return/Enter = Okay (with press/release animation). **Cancel restores the prefs snapshot
  taken on entry.** On Okay with changed music volume: music restarts (or stops if 0).
  Sound/music enable state derives from volume>0. After close: un-dim by full redraw of the
  menu.
* NOTE: only volume **on/off** semantics are guaranteed; `SetSoundState(mp.soundVolume==255)`
  style calls elsewhere suggest intermediate volumes were partially implemented.
* `DoTempPrefs` (System-7 dialog DLOG 8500 version of prefs) exists but is **never called** —
  dead development code.

### 7.5 Return semantics

`RunInterface` initializes `interface_returnValue = kErrorReturn` and runs the loop; on exit
(non-error) it does `FadeOutImmediate`, shows cursor, disposes state, returns the value.

### 7.6 "Bubbling Oliver" idle widget (MRGfxUtils.c)

A 32×32 animated Oliver face used on the menu, both stat bars, prefs and player-select
dialogs. Art: PICT `kIntfBubblesPictID` 141 — a strip of 32×32 cells: cell 0 = neutral; for
each of 4 "axes" a 2-frame bubble (cells laid out as offset `32 + 64*axis` and `64 + 64*axis`).
State machine advances at most every `kOliverTickDelay 8` ticks: neutral → pick random axis
→ frame1 → frame2 → frame1 → neutral (i.e., ~7.5 fps pulse loop).

---

## 8. Map room (world / level select) — MRMap.c

The overworld is a scrolling 2D tile map the player walks around; levels are entered from
level tiles. There are no menus/lists of levels.

### 8.1 World data

* World map: resource `'MRwd'` id 128 in the **World** data file; a flat
  `120×120` (`kMaxWorldSize`) array of `WorldPoint`
  `{Point p; short trig; Point trigP; short state; short kind; short levelID}` where `p` is
  the tile's (column,row) position **within the tile-sheet** (sheet is `kTilesWide 18` ×
  `kTilesDeep 20` = up to 360 cells; 378 tile images loaded, ids 0–377 from PICT
  `kMapTilesPict` 148); `tileID = p.v*18 + p.h`.
* `trig`/`trigP`: switch trigger group and the alternate tile to swap to. `levelID`: which
  level (1-based) this tile belongs to.
* Player sprite start: the cell whose tileID == `kOliverStartTile 18`; that cell's displayed
  tile is replaced by its `trigP` tile. Sprite art `kMapSpritesPict` 149 + mask 150, 24×24,
  2 frames.
* **Tile classification is hard-coded by sheet position** (`MakePieceKinds`): the class of a
  map cell = `pieceKinds[p.v][p.h]` of its sheet coords. Current (dev-state) table: sheet
  col 1, rows 0–2 = `kLevelPiece` (unpassed level), rows 3–5 = `kPassedLevelPiece`; sheet
  row 6 cols 0–8 = `kSwitchPiece`; rows 7–8 cols 0–8 = `kPassablePiece`; everything else
  `kNoPassPiece`. (A big commented-out older table exists — this mapping is clearly still in
  flux; kUsableTilesWide 9.)

### 8.2 Movement & interactions

* Same key-stack input as gameplay; grid-locked movement, 4 px/frame, 24-px tiles.
* Move onto `kSwitchPiece` → sound 130 (`kMapSwitchSnd`), `TriggerSwitch`: every cell with
  the same `trig` id swaps `p` ↔ `trigP` (opening bridges etc.), animated with a tile
  cross-fade; the throw is recorded in the player profile as a **toggle**
  (`PlayerThrowUpdate`: removes the trig id if present, else appends).
* Move onto a level tile (`kLevelPiece`/`kPassedLevelPiece`) → `onLevel=true`,
  `canProceed = passed?`, `levelNumber = cell.levelID`, remember `previousLocation`.
* **Gating**: from an *unpassed* level tile you may only move back to the tile you came from
  (`previousLocation`); passed levels are freely walkable through.
* Passed levels are shown with a different tile: when the player has passed `levelID`, the
  level tile's sheet position gets `p.v += 3` (`AdjustLevelID` / on-the-fly in
  `TriggerSwitch`).
* Player's map position (`mapLocation`) persists in the profile; (0,0) means "use the world's
  start tile".

### 8.3 Level-info popup

* After standing on a level tile for `kInfoDelayTicks 15` (0.25 s), an info card
  (PICT `kInfoPict` 163, 2 frames: normal + button-hilite) slides in as a screen-anchored
  sprite with sound 128 (`kLevInfoUpSnd`); removed (sound 129) when leaving the tile.
* Placement: bottom-right of the map viewport if the player sprite is in the top half of the
  view, else top-left; inset `kInfoBufferSpace 10` px.
* Contents (Helvetica 9, #99CCCC), text at x=74: **level title** (y 47); **goal line**
  (y 60): `"%03d"` required infotrons, or for timed levels `"%03d in H hr. MM min."` /
  `"%03d in MM min., SS sec."` / `"%03d in MM sec."` etc.; **status** (y 72): `Passed` or
  `Undefeated`; **description** (y 85+): level info text word-wrapped
  (`FlowWordsInRect`) to the card width −4, down to y 110.
* **Play button** at (125,116,64,20) within the card → enter the level. Level data for the
  popup is read from resource `'MRlv'` id `300 + levelNum` header (`MapLevelInfo`:
  reqInfo, levelWorld, title[256], info[256], phCount, pvCount, kindCount, actualInfo,
  hours, mins, secs; `reqInfo<0` marks the extended/timed format, in which case
  reqInfo=actualInfo). Missing level → title "Nonexistant Level", info "This level doesn't
  exist yet.".
* Quirk: while the mouse hovers over the popup, world processing pauses (`frozeLast`) —
  animation freezes but key handling still pumps one frame.

### 8.4 Map room flow

`RunMapRoom`: build everything → apply player's recorded switch throws and passed-level tile
swaps → draw panels → run one SW frame → **FadeIn** → show cursor (obscured until mouse
moves) → play map music (5000) → event loop. Exits:

* **Back arrow** click (bottom-left) → wipe + `FadeOutImmediate` → `kMapBackReturn` → main
  menu.
* **Play** (popup button or Return on level tile) → same outro → `kMapPlayReturn + levelNumber`.
* **Switch player** button → player-select dialog appears over the map (map redrawn &
  rebuilt for the new player on return — `ReturnFromPS` reloads the world, re-applies
  throws/passes, repositions the sprite at the new player's `mapLocation`).

### 8.5 Player-select dialog (MRPlayers.c)

* Art `kPSPict` 157 / hilite 158; centered; revealed center-out; screen behind dimmed.
* Track rects (x,y,w,h in dialog space): Prev (23,134,52,55), Next (125,134,52,55), name list
  (21,208,158,69), Rename (26,310,148,20), New (26,336,66,20), Delete (108,336,66,20),
  **Select** (26,421,148,47); bubble Oliver at (84,145,32,32).
* Shows `kPSPlayersSeen 4` names, 16-px rows (`kPSPlayerHeight`), Helvetica 11 white,
  current player **bold**; list scrolls with Prev/Next using a 2-px-step slide animation
  (`kPSSlideStep 2`, delay 2 ticks). Initial top = currentPlayer − 1 (clamped ≥0).
* Click a name → select it; **double-click** (within `GetDblTime()`) = Select. Return key
  = Select (press/release animation). Select closes the dialog.
* **Rename** → inline edit mode: white frame around the name, blinking 7×2 px underscore
  cursor (blink at caret time), type to append (max `MAX_PLAYER_NAME_LENGTH 20` chars,
  key sound 201, delete sound 202, too-long sound 203), Backspace deletes, Return/Enter ends.
* **New** → appends player #N with empty name and enters edit mode (up to `MAX_PLAYERS 50`).
* **Delete** → modal confirm card (`kPSConfirmPict` 155/156) centered offset (−40,−120),
  buttons No (26,111,66,20) / Yes (108,111,66,20). Yes removes the player, compacts the
  array, zeroes freed slot; **if the last player is deleted a default player
  "Oliver Layton III" is recreated** (that's also the initial default profile).

---

## 9. In-game flow — MRPlay.c

### 9.1 Level start

`RunGameLevel(wind, levelIndex)`:
1. If resource `'MRlv'` `300+levelIndex` missing → beep, `FadeOutImmediate`, back to map.
2. Hide cursor; build level (load, RLE-decompress, build tile grid; terrain resource file
   selected by `extraInfo.levelWorld`: 0=Infotron, 1=Aquatron, 2=Savanatron, 3=Deathtron).
3. Draw stat bar and HUD, **FadeIn**, start gameplay music (2000).
4. `startPlayTime = timerTicks = TickCount()`. **No countdown/"ready" sequence — play begins
   immediately after the fade-in.** Initial falls/movers are set up on the first idle pass
   (`doneInitialFalls`).

### 9.2 Per-frame idle (`Level_IdleBack`)

Keep music pumped → first-pass setup → if paused: return → if Oliver gone: spawn death
dialog or (if `wonLevel`) finish → if death dialog up and mouse over it: freeze frame →
process+animate sprite world → if a frame occurred, run chore queue → timer tick (below).

### 9.3 Pause

* Trigger: pause key **key-up** while Oliver exists. Sound `kPauseSnd` 138. Screen-door dim
  over the whole 640×480 + `kGamePausedPict` 162 centered. Music fades out. Timer offset
  preserved via `timerTickDiff`. App-switching allowed while paused.
* Unpause: key-up again → `kUnpauseSnd` 139, accumulate `totalPauseTime`, redraw stat bar &
  HUD (caches invalidated), music fades back in, black-fill the pause graphic's rect,
  clear all held keys, restore `timerTicks`.
* While paused all click/key/idle game handling is skipped.

### 9.4 Countdown timer (timed levels only)

Every ≥60 elapsed ticks: `secs--` with borrow through mins/hours; at ≤10 s remaining (and
0 m 0 h) plays `kNoMoreTimeSnd` 128 each second with proximity `secs*1.5` (fades as time runs
out); at −1 s with nothing to borrow → clamp 0 and `KillOliver` (time-out death). Timer is
also stopped when Oliver dies or wins.

### 9.5 Death → retry flow (no lives system)

* `KillOliver` (MRPlayCommon.c): picks a death animation (shock if electric; else fade 30% /
  bubble 35% / static 35% split via `GetRandom`), stops the timer, records `stopPlayTime`,
  triggers an explosion at Oliver's tile, plays `kOliverDiedSnd` 137. When the death
  animation finishes, the sprite removes itself (`oliverSprite = NULL`).
* Next idle pass spawns the **replay dialog** (`MakeReplaySprite`): PICT `kDeathPict` 165
  (2 frames), centered in the level viewport (positioned at viewport-center −(100,73), so the
  card is ~200×146). Buttons relative to card: **No** (26,111,66,20), **Yes** (108,111,66,20).
  Cursor is shown. The world keeps animating behind it (explosions finish).
* Yes / `y` / Return / Enter → `playAgain=true`, level exits and `RunGameLevel` returns
  `kPlayAgainReturn` → same level restarts (full reload). No / `n` → `kBackToMapReturn`.
* **There are no lives, no score, and no game-over screen.** Retry is unlimited.

### 9.6 Level complete

* Winning = walking into the fully-open exit tile (`kLastExitTile`, after the exit-opening
  chore triggered by collecting the last infotron — `OpenAllExits`): plays `kEndLevelSnd`
  151, sets `wonLevel`, records `stopPlayTime`, removes Oliver.
* Next idle pass (or a click) sets `doneWithLevel` → loop exits. **No bonus tally / results
  screen** — straight to outro: wipe FX, `FadeOutImmediate`, mark
  `pd->passedLevel[levelIndex-1] = true`, return `kBackToMapReturn`.
* Play time credit: `totalPlayTime = stopPlayTime − startPlayTime − totalPauseTime` is added
  to the profile clock (`AddTicksToPlayer`; note: if the player quits via death-dialog "No",
  stopPlayTime was set at death — pause time during the dialog is excluded).

### 9.7 Level outro (both outcomes)

Restore key mask → fade/stop music → hide cursor → `WipeWindowFX` stripes-wipe →
`FadeOutImmediate` → show cursor → black fill → flush sounds → restore resource aliases.

---

## 10. Persistence

Everything persists as **resources in the "Players" data file** (`:MR Data:Players`),
written only via `MROverwriteResource` (delete+add+UpdateResFile):

1. **Preferences** — type `'Mprf'` id 128, raw `MRPrefs` struct
   `{char up,down,left,right,special; char suicide; short musicVolume, soundVolume;
   short screenSize; Boolean interlacedMode; Boolean progressScreen; char pauseKey;
   Boolean compatibleDraws}`. Loaded at init (`MRLoadPreferences`), **saved only at clean app
   exit** (`MRClosePreferences` from `CleanUpForExit`). `progressScreen` is never used.
2. **Player list** — type `'MRpl'` id 128, raw `MRPlayerListStruct`
   `{short playerCount; short currentPlayer; MRPlayerDataStruct players[50]}` with per-player
   `{short hours,mins,secs,days; Point mapLocation; Point previousLocation; short numThrows;
   char myName[20]; short theThrows[100]; Boolean passedLevel[100]; short levelsWon,
   levelsTried, recentLevel, expansion1..3}`. A v1 struct (without the trailing 6 shorts) is
   auto-migrated on load by size sniffing. Also **saved only at clean exit**
   (`ClosePlayers`). The trailing `levelsWon/levelsTried/recentLevel` fields are written by
   nobody — reserved/unused.
   * Per-player progress = `passedLevel[]` (levels completed → unlocks free movement across
     that level's tile and swaps its map art), `theThrows[]` (map switches currently toggled),
     `mapLocation`/`previousLocation` (position on the world map), cumulative play time.
3. **Monitor prefs** — type `'mPrf'` id 1250 in the **app's own resource fork**
   (`MonitorPrefsStruct {short hasShownDepth, hasShownResolution, changeResolution,
   alwaysShowResolution, alwaysShowDepth}`) — remembers the resolution/depth dialog answers.
   Web port: drop.
4. Desktop-icon snapshot ("Desktop Snap" file in Preferences folder, Desktop.c) — saves/
   restores Finder icon positions around resolution switches. Web port: drop.

Because saving happens only in `CleanUpForExit`, a crash loses all session progress —
a web port should instead persist (e.g. localStorage) after each level completion / prefs
change / player edit.

Data files opened at startup (`MRResourcesInit`; all required):
`:MR Data:Aquatron`, `:MR Data:Players`, `:MR Data:Interface` (a.k.a. general/pictures),
`:MR Data:Infotron`, `:MR Data:Deathtron`, `:MR Data:Savanatron`, `:MR Data:World`,
plus the app's own fork ("main"). A gfx/snd "alias" mechanism redirects generic art/sound
lookups to the current terrain file during gameplay.

---

## 11. Registration / shareware — drop it

`MRRegistration.c` is a stub: `MRIsRegistered()` returns `true`; `DoRegistrationDialog()`,
`GetRegistrationInfo()`, `SetRegistrationInfo()` are empty. main.c calls the dialog only if
unregistered — i.e. never. There is also a disabled dev-build expiration check
(`ExpirationExit`, commented out) and a `DevelopmentNotice` modal (DLOG 10101, "this is a dev
copy", exits unless item 1 hit) gated on `DEV_NOTICE 1`. **Nothing to port.**

---

## 12. Known-unfinished / quirks inventory

* **Registration**: stub (see §11). **DevelopmentNotice** dialog active in this build.
* **Map piece-kind table** (`MakePieceKinds`) is placeholder/dev: only sheet column 1 rows
  0–5 are level pieces, row 6 = switches, rows 7–8 = passable; a large alternative table is
  commented out. The final shipped mapping is unknown from this code.
* Level-select popup `MakeInfoSprite` is created from the idle callback after 15 ticks;
  a commented-out call in `ReturnFromPS` plus "play a sound" TODO comments.
* Map wave animation (`waveLineUpdated` block) commented out.
* `DoTempPrefs` (dialog-based prefs) is dead code. `progressScreen` pref unused.
  `kPrefsVerySmallScreenSize` unreachable.
* Interface music does **not** restart when returning to the main menu from the map room
  (PlayMusic call commented out in `RunInterface`; music only starts in `IntroSequence` and
  after a prefs volume change).
* `Scroll_LevWorldMove` casts the *cbData* to `SpritePtr followSprite` param slot
  (intentional but ugly); death dialog freezes world only while mouse hovers it.
* Volume is effectively binary in several places (`SetSoundState(vol==255)`) despite the
  4-step volume UI.
* `CheckValidity`/`TFA/tf0/tf1` fields are workarounds for a SpriteWorld tile bug — ignore.
* Player stats fields `levelsWon/levelsTried/recentLevel` and the "Wins/Rank" sketch in
  MRPlayers.h comments were never implemented.
* Interlaced mode & compatibleDraws prefs are 1997 blitter options — drop.
* `kSixteenConstant 24` (MRMap.h) — misleading name, equals tile size 24.
* Menu Start hotspot height differs on >480-px windows (97 vs 129 px) — fixed 480 port uses 97.

---

## 13. Timing cheat-sheet for the web port

| Thing | Original value | Suggested web equivalent |
|---|---|---|
| Logic/render cap | 30 fps (SpriteWorld maxFPS) | fixed 30 Hz logic (or 60 Hz halved) |
| Oliver walk | 8 px/frame, 32-px tile, 4 frames/tile | 133 ms/tile, per-pixel interpolated |
| Push | 4 px/frame (8 frames/tile) | 267 ms/tile |
| Enemies/movers | 4 px/frame (8 frames/tile) | 267 ms/tile |
| Map walk | 4 px/frame, 24-px tile | 200 ms/tile |
| Camera | Δ = v·(dist from center)/100 px/frame, clamped to map | same proportional follow |
| Infotron/terminal tile anim | every 2 ticks (~33 ms) | 30 fps cycle (iPhone port used 75 ms) |
| Oliver bub idle (HUD & sprite) | ≥8 ticks/step (~133 ms) / 100 ms frame time | ~100–175 ms |
| Level timer | 1 s per 60 ticks, real-time | 1 s wall clock, freeze on pause |
| Red-disk drop | hold special 0.5 s to start, 1.5 s to drop | same |
| Boredom anims | sad 90 s, rude 90.5 s, eureka 91.5 s, yawn 92 s after last input | same |
| Info popup delay | 15 ticks (250 ms) on level tile | same |
| Fades | 50-step gamma quadratic; immediate = snap | ~500 ms ease overlay |
| Wipes | 32-px stripes, ~2–3 ticks/step (≈0.7–1 s total) | ~0.8 s stripe wipe |
| Splash | logo 2 s (skippable), title 5 s (skippable) | same |
