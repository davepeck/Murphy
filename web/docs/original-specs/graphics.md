# Murphy's Revenge — Graphics Assets Specification

Scope: tile atlases, tile-ID ↔ image mapping, animation frame sequences and timing,
Oliver (player) sprite sheets, masks/transparency, world-map tileset, bitmap fonts, and
interface screens — everything needed to render the game in a TypeScript web port
without consulting the original code.

Sources read (all under `/Users/dave/dev/games/Murphy`):

- `osx/Murphy/MurphyTiles.swift`, `osx/Murphy/EnumRanges.swift`, `osx/Murphy/MurphyLevelScene.swift`
- `osx/Murphy/{infotron,aquatron,savanatron}.atlas/` (305 PNGs each)
- `iPhone/TMTileAtlas.[hm]`, `iPhone/TMTileMap.m`, `iPhone/TMSprite.[hm]` (empty stub),
  `iPhone/MurphyView.m`, `iPhone/MurphyConstants.h`, `iPhone/{infotron,aquatron,savanatron,world}.png`
- `legacy/murphy/artwork/assemble-opengl-texture.py`, `legacy/murphy/tile-info.txt`
- `legacy/murphy/artwork/{Infotron,Aquatron,Savanatron}/*.bmp`, `artwork/World/*.bmp`,
  `artwork/Interface/*.bmp`, `artwork/Misc/*.bmp`, `artwork/level-designer/img/`
- `legacy/murphy/os7code/mr/`: `MRPlayConstants.h`, `MRPlaySprite.c/.h`, `MRPlay.c`,
  `MRPlayCommon.c`, `MRChore.c`, `MRMap.c/.h`, `MRGfxUtils.c/.h`, `MRInterface.c/.h`,
  `MRResources.c/.h`  (NOTE: os7code files use classic-Mac CR line endings)
- `tools/unbundle_images.py`
- `web/public/assets/**` and `web/src/assets.ts` (the in-progress web port's already-converted assets)

Timing convention: the original Mac code measures time in **ticks = 1/60 second**
(`TickCount()`). Millisecond values come from SpriteWorld frame times.

---

## 1. Tile ID space and canonical names

### 1.1 ID space

There is a single per-world tile ID space of **310 grid slots (IDs 0–309)**, laid out
**10 tiles across × 31 rows** of **32×32 px** tiles in one texture
(iPhone atlases are 512×1024 PNGs; used area 320×992, remainder filled opaque red
`(255,0,0)`; missing tiles filled opaque blue `(0,0,255)`).

- IDs **0–213**: `LevelTile` — level pieces + tile-based animation frames (214 names).
- IDs **214–219**: unused padding (named `UnusedTile1..6` in the osx atlases).
- IDs **220–304**: `OliverTile` — the 85 Oliver ("Bub") player frames.
- IDs **305–309**: unused padding (unnamed; filler in the packed texture).

Mapping ID → atlas cell (iPhone `TMTileMap`): `s = id % 10`, `t = id / 10`
(`t` counted downward from top). Level files (`.mbl`) store each cell as an `(s,t)`
byte pair; `tileId = t*10 + s`. Confirmed identical in `iPhone/MurphyView.m`
(`tileAtlasWithResourcePNG:... tilePixelWidth:32 tilePixelHeight:32 tilesAcross:10 tilesDown:31`).

The same enum with the same raw values exists in three places and they agree:
`os7code/mr/MRPlayConstants.h` (`TileTypes`, 0-based; `OliverFrameIDs` 0-based within the
Oliver sprite), `iPhone/MurphyConstants.h` (`LevelTileIds` 0-based, `OliverTileIds`
starting at 220), `osx/Murphy/MurphyTiles.swift` (`LevelTile` 0-based, `OliverTile`
starting at 220).

### 1.2 Full canonical list, ID → name

Atlas texture names are `<name>.png` in the osx `.atlas` bundles; the enum case name is
the name without `.png`. IDs 0–213 (LevelTile):

```
0:CircuitTile        1:BlankTile          2:ExitTile            3:GreenDiskTile
4:YellowDiskTile     5:RedDiskTile        6:GoldDiskTile        7:TerminalTile
8:BugTile            9:OliverTile
10:LeftPortTile      11:DownPortTile      12:RightPortTile      13:UpPortTile
14:LeftRightPortTile 15:UpDownPortTile    16:FourWayPortTile
17:ZonkTile          18:UpZonkTile        19:InfotronTile
20:QuarkTile         21:UpScissorTile     22:RightScissorTile   23:DownScissorTile
24:LeftScissorTile   25:ChipTile          26:TopChipTile        27:BottomChipTile
28:LeftChipTile      29:RightChipTile     30:TopLeftBorderTile  31:TopBorderTile
32:TopRightBorderTile 33:RightBorderTile  34:LeftBorderTile     35:BottomLeftBorderTile
36:BottomBorderTile  37:BottomRightBorderTile 38:BlankChipTile
39:FirstHardwareTile 40..64:HardwareTile2..HardwareTile26 65:LastHardwareTile
66:PhantomTile       67:LaserTile         68:LaserPowerTile     69:ForeMirrorTile
70:BackMirrorTile    71:EmptyPhaseTile    72:FullPhaseTile      73:PhaseTermTile
74:EmptyTile4 75:EmptyTile5 76:EmptyTile6 77:EmptyTile7 78:EmptyTile8 79:EmptyTile9
80:BlankLaserPiece   81:LaserEmptyPiece   82:LaserPowerPiece    83:LaserPiece
84:ForeMirrorPiece   85:BackMirrorPiece   86:TLBRLaserForeMirrorLightPiece
87:RLLaserLightPiece 88:UDLaserLightPiece 89:TLLaserForeMirrorLightPiece
90:BRLaserForeMirrorLightPiece 91:BLLaserBackMirrorLightPiece
92:TRLaserBackMirrorLightPiece 93:BLTRLaserBackMirrorLightPiece
94:FirstInfotronTile 95..108:InfoTile2..InfoTile15 109:LastInfotronTile
110:FirstTerminalTile 111..120:TermTile2..TermTile11 121:LastTerminalTile
122:FirstBugTile 123..126:BugTile2..BugTile5 127:LastBugTile
128:FirstExplTile 129:ExplTile2 130:ExplTile3 131:IgniteExplTile 132:ExplTile5
133:ExplTile6 134:ExplTile7 135:ExplTile8 136:ExplTile9 137:LastExplTile
138:FirstExitTile 139..144:ExitTile2..ExitTile7 145:LastExitTile
146:FirstZapRedDiskTile 147..152:ZapRedDiskTile1..6 153:LastZapRedDiskTile
154:EmptyZapTile8 155:EmptyZapTile9
156:FirstZapCircuitTile 157..161:ZapCircuitTile11..15 162:LastZapCircuitTile
163:EmptyZapTile17 164:EmptyZapTile18 165:EmptyZapTile19
166:FirstZapInfotronTile 167..177:ZapInfotronTile21..31 178:LastZapInfotronTile
179:FirstRDBlinkTile 180:LastRDBlinkTile
181:FirstZonkTile 182:ZonkTile1 183:ZonkTile2 184:LastZonkTile
185:FirstUpZonkTile 186:UpZonkTile1 187:UpZonkTile2 188:LastUpZonkTile
189:FirstQuarkTile 190..196:QuarkTile1..7 197:LastQuarkTile
198:FirstUpScissorTile 199:UpScissorTile1 200:LastUpScissorTile 201:UpRightScissorTile
202:FirstRightScissorTile 203:RightScissorTile1 204:LastRightScissorTile 205:DownRightScissorTile
206:FirstDownScissorTile 207:DownScissorTile1 208:LastDownScissorTile 209:DownLeftScissorTile
210:FirstLeftScissorTile 211:LeftScissorTile1 212:LastLeftScissorTile 213:UpLeftScissorTile
```

IDs 214–219: `UnusedTile1..UnusedTile6` (padding; exist only as named PNGs in the osx atlases).

IDs 220–304 (OliverTile; Oliver-sprite-local frame index = ID − 220):

```
220:BubFrame 221:ABubFrame1 222:ABubFrame2 223:BBubFrame1 224:BBubFrame2
225:CBubFrame1 226:CBubFrame2 227:DBubFrame1 228:DBubFrame2
229:LeftBubFirstFrame 230:LeftBubFrame2 231:LeftBubLastFrame
232:RightBubFirstFrame 233:RightBubFrame2 234:RightBubLastFrame
235:EmptyBubFrame1
236:PushLeftBubFirstFrame 237:PushLeftBubLastFrame
238:PushRightBubFirstFrame 239:PushRightBubLastFrame
240:PushDownBubFirstFrame 241:PushDownBubLastFrame
242:PushUpBubFirstFrame 243:PushUpBubLastFrame
244:ZapLeftBubFrame 245:ZapRightBubFrame 246:ZapDownBubFrame 247:ZapUpBubFrame
248:EmptyBubFrame2 249:EmptyBubFrame3
250:FadeDeathBubFirstFrame 251..258:FadeDeathBubFrame2..9 259:FadeDeathBubLastFrame
260:BubbleDeathBubFirstFrame 261..268:BubbleDeathBubFrame2..9 269:BubbleDeathBubLastFrame
270:StaticDeathBubFirstFrame 271..278:StaticDeathBubFrame2..9 279:StaticDeathBubLastFrame
280:YawnBubFirstFrame 281..290:YawnBubFrame2..11 291:YawnBubLastFrame
292:ShockDeathBubFirstFrame 293..297:ShockDeathBubFrame2..6 298:ShockDeathBubLastFrame
299:EmptyBubFrame4 300:EurekaBubFrame 301:SadBubFrame 302:FuckYouBubFrame
303:ExLaxBubFirstFrame 304:ExLaxBubLastFrame
```

Total named tiles = 214 + 6 + 85 = **305** — exactly the file count in each of
`osx/Murphy/{infotron,aquatron,savanatron}.atlas/` (each PNG is 32×32, RGB8 with an
alpha channel that is fully opaque — sources were 24-bit BMPs).

`kMaxLevelTiles` = 214. `kMaxBubFrames` = 85.

### 1.3 Level-file tile substitutions at load time

From `iPhone/MurphyView.m` (`loadCurrentLevel`) — when populating a live grid from a
level file, static "base" IDs are replaced with their first animation frame:

- `kInfotronTile (19)` → `kFirstInfotronTile (94)`
- `kQuarkTile (20)` → `kFirstQuarkTile (189)`
- `kTerminalTile (7)` → `kFirstTerminalTile (110)`
- `kOliverTile (9)` → `kBubFrame (220)` (and records Murphy's start position)
- `kUpScissorTile..kLeftScissorTile (21..24)` → `kFirstUpScissorTile (198)`
  (iPhone demo loses initial facing; the original keeps facing — see §4.6)

Level bounds detection (iPhone): scan for the first `kBottomRightBorderTile (37)`;
its coordinates + 1 give actual width/height.

---

## 2. How the atlases were assembled (ground truth ordering)

`legacy/murphy/artwork/assemble-opengl-texture.py` packs the per-piece BMP sheets into
a 512×1024 RGBA texture, 10 tiles across, 32-px tiles, in this exact order
(`(file name, vertical separation px between rows inside the BMP, tile count consumed)`):

```python
TILE_SIZE = 32
TEXTURE_TILES_ACROSS = 10
FILE_INFO = [
    ("basic-pieces.bmp", 0, 10),    # -> IDs 0-9
    ("ports.bmp", 0, 7),            # -> IDs 10-16
    ("zonks-infotron.bmp", 0, 3),   # -> IDs 17-19
    ("basic-pieces-2.bmp", 0, 60),  # -> IDs 20-79
    ("laser.bmp", 1, 14),           # -> IDs 80-93
    ("infotron.bmp", 1, 16),        # -> IDs 94-109
    ("terminal.bmp", 1, 12),        # -> IDs 110-121
    ("bug.bmp", 0, 6),              # -> IDs 122-127
    ("explosion.bmp", 0, 10),       # -> IDs 128-137
    ("exit.bmp", 0, 8),             # -> IDs 138-145
    ("zaps.bmp", 1, 33),            # -> IDs 146-178
    ("red-disk.bmp", 0, 2),         # -> IDs 179-180
    ("red-zonk.bmp", 0, 4),         # -> IDs 181-184
    ("blue-zonk.bmp", 0, 4),        # -> IDs 185-188
    ("plasmoid.bmp", 0, 9),         # -> IDs 189-197
    ("robot.bmp", 1, 22),           # -> IDs 198-219 (16 real + 6 filler)
    ("oliver.bmp", 1, 90) ]         # -> IDs 220-309 (85 real + 5 filler)
```

Comment in the script for robot.bmp: "really 16, but extend to include oliver.bmp in
nice location". Missing files or short files are filled with opaque blue `(0,0,255)`;
the texture is initialized to opaque red `(255,0,0,255)`. Source BMPs are read
left-to-right, top-to-bottom; when a `vertical separation` of 1 is given, each source
row of tiles is 33 px apart (32 tile + 1 px gutter).

The BMPs correspond 1:1 to the original game's PICT resources (IDs from
`MRPlayConstants.h`): `kTilesAPict=128` (basic-pieces), `kTilesBPict=129` (ports),
`kTilesCPict=130` (zonks-infotron), `kTilesDPict=131` (basic-pieces-2),
`kTilesBMaskPict=132` (port-masks), `kOliverSpritePict=133` (oliver),
`kInfotronSpritePict=134`, `kQuarkSpritePict=135` (plasmoid), `kTerminalSpritePict=136`,
`kBugSpritePict=137`, `kZapsPict=138`, `kExplosionPict=139`, `kExitPict=140`,
`kZonkPict=141` (red-zonk), `kUpZonkPict=142` (blue-zonk), `kScissorSpritePict=143`
(robot), `kRedDiskBlinkPict=144` (red-disk), `kLaserPiecesPict=145` (laser).

`legacy/murphy/tile-info.txt` records the per-file vertical-separation flags (0/1) —
the same numbers as FILE_INFO's second column, i.e. **the 0/1 flag is
"1-px gutter rows between tile rows inside the sheet", not a mask flag** (it lists
`robot.bmp 0`, but the actual BMP is 128×131 which requires separation 1; trust the
pixels/FILE_INFO). It also notes `basic-pieces-2.bmp ... [+13 extra non-infotron]`:
tiles 67–79 (laser/mirror/phase pieces + empties) exist only in the Infotron world's
sheet.

### 2.1 Per-piece BMP sheet inventory (all are 24-bit BMP, no alpha)

Measured with `sips`; identical across worlds unless noted. All tiles 32×32.

| file | px size | grid (cols×rows) | sep | tiles used | contents / IDs |
|---|---|---|---|---|---|
| basic-pieces.bmp | 320×32 | 10×1 | 0 | 10 | IDs 0–9 |
| ports.bmp | 224×32 | 7×1 | 0 | 7 | IDs 10–16 |
| port-masks.bmp | 224×32 | 7×1 | 0 | 7 | mask sheet for ports (entirely black = fully opaque; see §5) |
| zonks-infotron.bmp | 96×32 | 3×1 | 0 | 3 | IDs 17–19 |
| basic-pieces-2.bmp | Infotron 320×192; Aqua/Sava 320×160 | 10×6 / 10×5 | 0 | 60 / 50 | IDs 20–79 (20–69 only in Aqua/Sava; 70–79 blue filler there) |
| laser.bmp | 224×65 (Infotron only) | 7×2 | 1 | 14 | IDs 80–93; absent in Aquatron/Savanatron → blue filler |
| infotron.bmp | 320×65 | 10×2 | 1 | 16 | IDs 94–109 (sheet holds 20 cells; last 4 unused) |
| terminal.bmp | 320×65 | 10×2 | 1 | 12 | IDs 110–121 |
| bug.bmp | 320×32 | 10×1 | 0 | 6 | IDs 122–127 |
| explosion.bmp | 320×32 | 10×1 | 0 | 10 | IDs 128–137 |
| exit.bmp | 320×32 | 10×1 | 0 | 8 | IDs 138–145 |
| zaps.bmp | 320×131 | 10×4 | 1 | 33 | IDs 146–178 |
| red-disk.bmp | 64×32 | 2×1 | 0 | 2 | IDs 179–180 (red-disk blink) |
| red-zonk.bmp | 128×32 | 4×1 | 0 | 4 | IDs 181–184 (zonk roll frames) |
| blue-zonk.bmp | 128×32 | 4×1 | 0 | 4 | IDs 185–188 (up-zonk roll frames) |
| plasmoid.bmp | 320×32 | 10×1 | 0 | 9 | IDs 189–197 (quark) |
| robot.bmp | 128×131 | 4×4 | 1 | 16 | IDs 198–213 (scissors, 4 facings × 4 frames) |
| oliver.bmp | 320×296 | 10×9 | 1 | 85 | IDs 220–304 (Oliver frames, row-major; last 5 cells of row 9 unused) |

World directories: `legacy/murphy/artwork/Infotron/` (18 files incl. laser.bmp),
`Aquatron/` and `Savanatron/` (17 files each, no laser.bmp). There is **no Deathtron
artwork directory** even though the code reserves a 4th world
(`kDeathtronTerrain=3`, res file `:MR Data:Deathtron`, and
`level-designer/img/tiles-deathtron.bmp` exists for the editor).

### 2.2 Original in-game tile loading (MRPlay.c `MakeLevelSWorld`)

SpriteWorld `SWLoadTilesFromPictResource(sw, firstID, lastID, pictID, maskResID,
maskType, borderWidth, verticalSep)` calls, in order:

- IDs 0–9 from PICT 128, `kNoMask`, sep 0
- IDs 10–16 (ports) from PICT 129, **`kSolidMask`**, sep 0  ← only tiles loaded with a mask type
- IDs 17–19 from PICT 130, `kNoMask`, sep 0
- non-Infotron worlds: IDs 20–66 (`kQuarkTile..kPhantomTile`) from PICT 131, sep 0
- Infotron world: IDs 20–79 from PICT 131 sep 0, **plus** IDs 80–93 from PICT 145 sep 1
- IDs 94–109 from PICT 134, sep 1; IDs 110–121 from PICT 136, sep 1
- IDs 122–127 from PICT 137, sep 1; IDs 128–137 from PICT 139, sep 1
- IDs 138–145 from PICT 140, sep 1; IDs 146–178 from PICT 138, sep 1
- IDs 179–180 from PICT 144, sep 1; IDs 181–184 from PICT 141, sep 1
- IDs 185–188 from PICT 142, sep 1; IDs 189–197 from PICT 135, sep 1
- IDs 198–213 from PICT 143, sep 1

(The sep argument is harmless for single-row sheets.) `SWInitTiling` uses 32×32 tiles,
`kMaxLevelTiles`=214 slots. Level area: `kLevelItemSize`=32, max level 120×120 tiles.

---

## 3. Modern atlas variants

1. **iPhone prebuilt atlases** `iPhone/{infotron,aquatron,savanatron}.png`: 512×1024,
   10 across × 31 down, IDs 0–309 as in §1.1. Output of `assemble-opengl-texture.py`.
   `world.png`: 512×512; the 432×504 world-map tileset (see §6) pasted at top-left,
   white padding elsewhere.
2. **osx SpriteKit atlases** `osx/Murphy/*.atlas/`: 305 individually named 32×32 PNGs
   per world (filenames exactly the §1.2 names + `.png`, plus `UnusedTile1..6.png`).
   Produced by `tools/unbundle_images.py`:
   ```
   python unbundle_images.py tile_file tile_names_file target_directory
   # TILES_ACROSS = 10, TILE_WIDTH = TILE_HEIGHT = 32
   # reads a JSON array of tile names; crops tiles left→right, top→bottom
   ```
   The `tile_names_file` JSON it consumed is **not present in the repository**
   (searched; only `web/public/assets/atlas/tiles.json` exists, which is a different,
   alphabetical format). The name list is trivially reconstructable from §1.2 order.
3. **Web port atlases (current work-in-progress)** `web/public/assets/atlas/`:
   `{infotron,aquatron,savanatron}.png` are repacked 640×512 sheets, **20 columns**,
   tiles ordered by **alphabetically sorted name**; `tiles.json` =
   `{ "tileSize": 32, "columns": 20, "names": [ ...305 sorted names... ] }`;
   index of a name in `names` = cell index (row-major, 20 per row). Loader:
   `web/src/assets.ts` (`TileAtlas { image, tileSize, columns, index: Map<name, i> }`).

---

## 4. Animation sequences and timing (level play)

Global speed constants (`MRPlayConstants.h`): `kSpriteFrameMilli=33` ms,
`kQuarkFrameMilli=66` ms, `kOliverBubbleMilli=100` ms, `kSpriteFrameTicks=2` ticks,
`kMaxLevelFPS=30`. Moving-sprite pixel speeds: `kSpriteMoveDelta=8` px/frame
(fall/roll; push at 8/2=4 px/frame; a pushed gold disk keeps 8),
`kMoverSpriteMoveDelta=4` px/frame (quark/scissor). One tile = 32 px, so a fall takes
4 frames/tile, enemies 8 frames/tile, pushes 8 frames/tile.

The osx SpriteKit prototype uses: infotron 0.075 s/frame; quark, terminal, scissor and
Oliver-bub cycles 0.175 s/frame. The iPhone demo uses the same values
(`animateTileId` intervals 0.075/0.175). These are close to the original
(2 ticks ≈ 0.033 s stepped at ~1/2 the 30fps loop; and 66 ms respectively) but not exact;
treat the original tick math below as authoritative.

### 4.1 Ambient tile animations (whole-map, all instances share one image)

From `MRPlay.c` `Tile_ChangeProc`: every > `kSpriteFrameTicks` (2 ticks ≈ 33 ms):

- **Infotron (static piece, ID 19)**: displayed image cycles 94→95→…→109→94 (16 frames).
- **Terminal (ID 7)**: cycles 110→…→121→110 (12 frames).

(All infotrons/terminals on the map animate in lockstep because the *tile image* for
IDs 19 and 7 is swapped, not per-cell frames.)

### 4.2 Explosion (chore, per cell)

`_process_Explosion`: step every `kExplDelayTicks=3` ticks (~50 ms). Draw tile IDs
128,129,130,131,132,…,137 in order; when the frame drawn is `IgniteExplTile (131)` and
the explosion was started with `shouldIgnite`, neighbors are ignited (chain reaction
trigger frame). After 137, the cell becomes `kBlankTile (1)`.

### 4.3 Quark explosion (chore)

`_process_QuarkExplosion`: step every 3 ticks; walks the **ZapInfotron frames backwards**
from a start frame (started at `kLastZapInfotronTile (178)`) down to
`kFirstZapInfotronTile (166)`; ignition happens when the configured `igniteTileID`
frame is drawn; when finished the cell becomes an **`kInfotronTile (19)`** (a killed
quark leaves an infotron) and falls/rolls are re-checked.

### 4.4 Exit opening (chore)

When the last infotron is collected, `OpenAllExits` starts a chore per exit cell:
step every `kExitDelayTicks=3` ticks through IDs 138→145 and stop (exit stays at
`kLastExitTile (145)`; that ID is what Murphy must step on to win).

### 4.5 Zap (Murphy "eating" a piece at distance) (chore)

`_process_Zap`: step every `kZapDelayTicks=3` ticks. Frame ranges by zapped piece:

- Infotron: 166→178 (13 frames)
- Red disk: 146→153 (8 frames)
- Circuit ("base"): 156→162 (7 frames)

then the cell becomes `kBlankTile` and the piece is credited (`EatPiece`).
While zapping, Oliver shows a single directional frame (see §4.9) frozen for
`kZapFreezeTicks=15` ticks.

### 4.6 Scissors (enemy, sprite with 16 frames, IDs 198–213)

Frame time `kQuarkFrameMilli=66` ms. Facings: 1=Up, 2=Right, 3=Down, 4=Left
(`kMoverFacingUp..kMoverFacingLeft`). Sprite frame indices are 0–15 relative to 198.

- Base frame per facing: `base = (facing−1)*4` → Up 0–3 (IDs 198–201),
  Right 4–7 (202–205), Down 8–11 (206–209), Left 12–15 (210–213).
- **Moving**: ping-pong `base+0 → base+1 → base+2 → base+1 → base+0 → …`
  (internalFrame 0..2 with direction flip at ends).
- **Turning**: the 4th frame of each group is the diagonal:
  201=Up-Right, 205=Down-Right, 209=Down-Left, 213=Up-Left.
  Clockwise turn shows `frame = facing*4 − 1`; counter-clockwise shows
  `frame = (facing−1)*4 − 1`, wrapping −1→15. While mid-turn it shows the closed
  frame of the target facing (`(turningTo−1)*4`). Turn cadence `kScissorTurnTime=2` ticks.
- Movement AI checks directions in order Left, Up, Right, Down relative to facing
  (wall-hugger), 4 px/frame.

### 4.7 Quark (enemy, 9 frames, IDs 189–197)

Simple loop 189→197→189… at 66 ms/frame (`_frame_Default` over 9 frames).
Initial facing Up; direction preference left/up/right/down relative to facing.

### 4.8 Zonk / Up-zonk rolling (4 frames each)

`_make_PFInfo_Zonk` (used for both push- and roll-sprites of tiles 17/18):

- Rolling/pushed **right**: frames in order `[181, 182, 183, 184]` (First,1,2,Last);
  up-zonk `[185, 186, 187, 188]`.
- Rolling/pushed **left**: `[181, 184, 183, 182]` (i.e. First, Last, Last−1, Last−2) —
  reverse rotation; up-zonk `[185, 188, 187, 186]`.
- `frameSpeedMult=2` → frame time = 66 ms. Falling zonks don't animate
  (single frame of their own tile).
- Every other pushable piece animates with a single frame (its own tile image).

### 4.9 Oliver (player sprite; frame indices below are Oliver-local 0–84 = atlas ID − 220)

Sprite built from PICT 133 (`oliver.bmp`) as 32×32 cells, 85 frames, row-major
(10 per row), `kNoMask` (fully opaque blit!). Frame-change interval
`kOliverBubbleMilli=100` ms. Drawn **under** the tile layer
(`SWSetSpriteUnderTiles(oliver, true)`) so masked port tiles cover him.

State machine (`_frame_Oliver` in MRPlaySprite.c):

- **Idle ("bubbling")**: frame 0 (`BubFrame`) → pick random axis a∈0..3 →
  `1+2a` → `2+2a` → `1+2a` → 0 → repeat. Axes A/B/C/D = frames (1,2),(3,4),(5,6),(7,8).
- **Walking**: left base frame 9 (`LeftBubFirstFrame`), right base 12
  (`RightBubFirstFrame`); cycles base+0, base+1, base+2 repeatedly
  (9,10,11,9,… or 12,13,14,12,…). Vertical movement keeps the last horizontal facing.
- **Pushing**: 2 frames per direction, played first→last then held:
  left 16,17; right 18,19; down 20,21; up 22,23. Push state also halves move speed.
- **Zapping (special key + direction)**: single frame per direction, frozen
  `kZapFreezeTicks=15` ticks: left 24, right 25, down 26, up 27.
- **Red-disk drop ("ExLax")**: after holding special on a blank/phantom cell
  `kRedDiskDropDelay=30` ticks, loop frames 83↔84 (`ExLaxBubFirst/Last`);
  after `kRedDiskDropFinish=90` ticks the disk is armed (RD-blink chore starts,
  §4.11) and the count decrements.
- **Deaths** (advance 1 frame/frame-time until final, then sprite removed):
  - Shock (electric killers): frames 72→78 (7 frames)
  - Fade: 30→39 (10 frames) — chosen with probability 3/9
  - Bubble-pop: 40→49 (10) — else if `GetRandom(0,9)<5`
  - Static: 50→59 (10) — otherwise
  An explosion is started at his cell simultaneously.
- **Boredom** (ticks since last keypress; 60 ticks = 1 s):
  - 5400–5430: frame 81 (`SadBubFrame`)
  - 5430–5490: frame 82 (`FuckYouBubFrame`)
  - 5490–5520: frame 80 (`EurekaBubFrame`)
  - >5520 (`kYawnDelay = 92*60`): play 60→71 (Yawn, 12 frames) once, hold last.
- Frames 15, 28, 29, 79 (`EmptyBubFrame1..4`) are unused placeholders.

### 4.10 Bug (hidden zapper in a circuit tile) (chore)

A bug cell normally *displays* `kCircuitTile (0)`. Cycle: wait
`kBugZapDelayTime=120 + 15*GetRandom(0,16)` ticks (2–6 s), then play IDs
122→127 stepping every `kBugAnimDelayTicks=5` ticks (with zap sound), then revert to
displaying 0 and wait again. (The static `BugTile (8)` graphic is only used in the
editor/level data; in play it's disguised.)

### 4.11 Red-disk blink (armed disk) (chore)

Alternate IDs 179↔180 every `kRDBlinkDelayTicks=20` ticks; after `kRDBlinkCount=12`
frame-changes (~4 s) start a full ignition explosion at that cell.

### 4.12 Laser (Infotron world only) (chore)

Modes `kLaserInitMode→kLaserDrawnMode→kLaserSpreadMode`, waits
`kLaserAfterDrawTicks=1` / `kLaserAfterSpreadTicks=1` tick between steps; beam pieces:

- vertical beam `UDLaserLightPiece (88)`; horizontal `RLLaserLightPiece (87)`
- fore-mirror "/" hit going right/down → `TLLaserForeMirrorLightPiece (89)`,
  going left/up → `BRLaserForeMirrorLightPiece (90)`; reflection swaps and negates deltas
- back-mirror "\" hit going left/down → `TRLaserBackMirrorLightPiece (92)`,
  else → `BLLaserBackMirrorLightPiece (91)`; reflection swaps deltas
- after spreading, the cell reverts to its pre-beam piece (mirror or blank);
  a consumed `LaserPowerTile (68)` push leaves `LaserEmptyPiece (81)`.
(86 `TLBRLaserForeMirrorLightPiece` and 93 `BLTRLaserBackMirrorLightPiece` are
crossed-beam variants present in art; not referenced in the surviving chore code.)

### 4.13 Ports

Port tiles (IDs 10–16) are ordinary static tiles but drawn with a mask **over** the
Oliver sprite, so Murphy visually slides "through" them. Port-use is instantaneous
motion restriction logic, no frame animation. Sound `kPortUseSnd=130`.

---

## 5. Masks and transparency

- **In-level tiles and sprites are fully opaque 32×32 blits.** Oliver, zonks, quarks,
  scissors, falling/rolling pieces: created with `kNoMask`. There is **no color-key
  or white-is-transparent convention in level rendering**; art already contains its
  black background.
- **Ports** are the exception: loaded with `kSolidMask` so they form the "over" layer
  above the Oliver sprite. `port-masks.bmp` (224×32) is **entirely black**, i.e. the
  mask marks every pixel opaque (classic Mac mask convention: black=opaque,
  white=transparent). Net effect for a port: draw the full 32×32 port tile over
  Murphy. A web port can simply draw port tiles after/above the player sprite.
- **World-map mini-Oliver** (`oliver-mini.bmp` 24×49: two 24×24 frames stacked with a
  1-px gutter) uses a real shape mask `oliver-mini-masks.bmp` (same layout: black
  circle on white per frame). Loaded via
  `SWCreateSpriteFromSinglePict(map, …, kMapSpritesPict=149, kMapMasksPict=150,
  kWorldItemSize=24, kFatMask)` — i.e. pixel-mask transparency; only the bubble
  circle is opaque. For the web: convert white mask pixels to alpha 0.
- Interface art (PICTs/BMPs) is drawn opaque with CopyBits; the "interface Oliver
  bubble" (§7.3) is an opaque 32×32 copy.
- The assembled OpenGL/PNG atlases carry an alpha channel but it is fully opaque;
  filler cells are solid red/blue and must never be sampled.

---

## 6. World map (overworld) graphics

- Tileset `legacy/murphy/artwork/World/world-tiles.bmp`: **432×504 = 18 cols × 21 rows
  of 24×24 tiles**, no gutters (`kWorldItemSize=24`). Original PICT `kMapTilesPict=148`,
  loaded as map tile IDs **0–377** (`kStartTileLoad=0`, `kEndTileLoad=377`,
  `kNumMapTiles=378`), `kNoMask`, row-major 18-wide: `tileID = row*18 + col`
  (`SetUpTileData`: `tileID = p.v*kTilesWide + p.h`, `kTilesWide=18`; note 378 = 18×21,
  and `kTilesDeep` is declared 20 — off by one vs the real 21-row sheet; the load uses
  0–377 so all 21 rows are used).
- World data: 120×120 `WorldPoint`s (`kMaxWorldSize=120`), each holding icon point
  `p (h,v)` into the 18-wide grid, a trigger id, alternate icon `trigP`, state, kind
  (`kPassablePiece=1, kSwitchPiece=2, kLevelPiece=3, kPassedLevelPiece=4, kNoPassPiece=5`),
  and `levelID`. Resource `'MRwd' 128` in the World data file.
- `kOliverStartTile = 18` (row 1, col 0 — the little bubble visible in the sheet):
  marks Murphy's map start; that cell's displayed tile is replaced by its `trigP` tile.
- Passed levels: a level icon whose `p.v < 3` gets `p.v += 3` (i.e. the "passed"
  variant lives exactly 3 rows below the normal variant in the sheet).
- Map FPS `kMaxMapFPS=30`.

Ambient map tile animations (`Tile_MapChangeProc`, delays in ticks):

| feature | tile-image cycle | step delay |
|---|---|---|
| Ocean (4 phase-shifted instances) | tiles 238,239,240,241 each cycle over 238→241 | 12 |
| Level markers, world row 1 | 1→8 (8 frames) | 10 |
| Level markers row 2 | 19→26 | 10 |
| row 3 | 37→44 | 10 |
| row 4 (passed variants) | 55→62 | 10 |
| row 5 | 73→80 | 10 |
| row 6 | 91→98 | 10 |
| Whirlpool | 256→259 | 17 |
| "Thing 1" (grab plant) | 168→170, forward once then reset; triggers randomly (`GetRandom(0,1000) < 75` per 15-tick poll) | 15 |
| "Thing 2" | 186→188 | 15 |
| Machine | 288→293 | 15 |

(A commented-out "wave" animation shifted rows 9–17 by +126 with wraparound at 377.)

- **Mini-Oliver map sprite**: 2 frames (24×24, masked, see §5), frame time
  `kSpriteFrameMilli=500` ms (map-local constant), moves 4 px/frame
  (map `kSpriteMoveDelta=4`), i.e. 6 frames per 24-px map cell.
- Map switch flips: all cells sharing a trigger id swap `p ↔ trigP` and redraw with a
  cross-fade (`SWCrossFadeTransition, 4, 4`).
- Map level-info popup: PICT `kInfoPict=163` (≙ `level-stats.bmp`, 200×147), "Play"
  button rect (125,116)–(189,136) inside it; positioned 10 px (`kInfoBufferSpace`)
  from a corner opposite Murphy. Level title/name/time text drawn in Helvetica 9
  colored RGB16(39168, 52224, 52224) ≈ #98CCCC.
- iPhone `world.png` (512×512) = this sheet at top-left, white padding right/bottom.
- Web copy already at `web/public/assets/world/{world-tiles,oliver-mini,oliver-mini-masks}.png`.

---

## 7. Interface graphics

### 7.1 Resource files / PICT IDs

Original resource files (`MRResources.c`): main app + `:MR Data:` `Aquatron`,
`Players`, `Interface` (a.k.a. "general"/"pictures"), `Infotron`, `Deathtron`,
`Savanatron`, `World`. `MRSetTerrainAlias(world)`: 0=Infotron, 1=Aquatron,
2=Savanatron, 3=Deathtron.

Known Interface-file PICT IDs (`MRGfxUtils.h`, `MRPlayConstants.h`, `MRMap.c`,
`MRInterface.h`) and their BMP exports (name match is definitive where noted,
otherwise inferred from dimensions/usage):

| PICT | constant | BMP (in `artwork/Interface/` unless noted) | px | purpose |
|---|---|---|---|---|
| 128 | kQSoftLogoPict | q-software-logo.bmp | 107×100 | Q Software logo |
| 129 | kInterfacePict | primary-interface.bmp | 480×480 | main menu, normal |
| 130 | kSelInterfacePict | primary-interface-select.bmp | 480×480 | main menu, clicked |
| 142 | kMouseInterfacePict | primary-interface-rollover.bmp | 480×480 | main menu, hover |
| 131–134 | kSavanaMRPict/kInfoMRPict/kDeathMRPict/kAquaMRPict | (no matching BMP found) | — | per-world pictures (unmapped) |
| 135 | kMapRoomPict | bar-map-room.bmp (inferred) | 40×480 | map-room side banner |
| 136 | kIntroPict | bar-intro.bmp (inferred) | 40×480 | intro screen banner |
| 137 | kAboutPict | bar-about.bmp (inferred) | 40×480 | credits screen banner |
| 138 | kHelpPict | bar-help.bmp (inferred) | 40×480 | help screen banner |
| 143 | kIntroTextPict (= 136+`kAddForTextPict(7)`) | story.bmp (inferred) | 480×480 | intro/story text page |
| 144 | kAboutTextPict | credits.bmp (inferred) | 480×480 | credits text page |
| 145 | kHelpTextPict | help-text.bmp (inferred) | 480×480 | help text page |
| 139 | kQSoftBannerPict | bar-q-software-presents.bmp | 480×40 | "Q Software presents" banner |
| 140 | kMRBannerPict | bar-murphys-revenge.bmp | 480×40 | "Murphy's Revenge" banner |
| 141 | kIntfBubblesPictID | oliver-basics.bmp | 608×32 | menu Oliver bubble strip (§7.3) |
| 146 | kMapStatPict | status-bar-map.bmp (inferred) | 40×480 | map screen status bar |
| 147 | kMapStatHilitePict | status-bar-map-select.bmp (inferred) | 40×480 | map status bar, hover |
| 148 | kMapTilesPict | World/world-tiles.bmp | 432×504 | overworld tileset |
| 149 | kMapSpritesPict | World/oliver-mini.bmp | 24×49 | map Oliver, 2 frames |
| 150 | kMapMasksPict | World/oliver-mini-masks.bmp | 24×49 | map Oliver masks |
| 151 | kGreenLettersPict | font-small.bmp (green glyphs) | 55×469 | bitmap font, green |
| 152 | kWhiteLettersPict | font-large.bmp (white glyphs) | 55×469 | bitmap font, white |
| 153 | kBackArrowPict | back.bmp | 40×40 | back arrow |
| 154 | kBackArrowHilitePict | back-select.bmp | 40×40 | back arrow, hover |
| 159 | kGameStatPict | status-bar-game.bmp | 40×480 | in-level status bar (untimed) |
| 133* | kGameStatMResPict (main res file) | level-stats-time-left.bmp (inferred) | 40×480 | in-level status bar for timed levels |
| 160 | kInfoMiniPict | infotron-minis.bmp | 64×16 | four 16×16 infotron icons, index = world 0–3 |
| 161 | kRDMiniPict | red-disk-minis.bmp | 64×16 | four 16×16 red-disk icons, index = world |
| 162 | kGamePausedPict | game-paused.bmp | 480×40 | pause banner |
| 163 | kInfoPict | level-stats.bmp (inferred) | 200×147 | map level-info card (Play at 125,116,64×20) |
| — | — | level-stats-select.bmp | 200×147 | hover variant of the card |
| 165 | kDeathPict | replay-level.bmp (inferred) | 200×147 | "replay level?" card; No=(26,111,66×20) Yes=(108,111,66×20); 2 sprite frames (select variant = replay-level-select.bmp) |
| — | — | player-delete.bmp / player-delete-select.bmp | 200×147 | delete-player confirm card |
| — | — | player-select.bmp / player-select-select.bmp | 200×480 | player chooser panel |
| 1000 | kPrefLoPict | preferences.bmp | 200×480 | prefs panel |
| 1001 | kPrefHiPict | preferences-select.bmp | 200×480 | prefs panel, hover |
| 1002–1005 | kPrefSilent/Audible/Pleasant/ExcruciatingPict | audio-{silent,audible,pleasant,excruciating}.bmp | 124×19 | volume labels |
| 3000 | kMRTitlePict | murphys-revenge-logo.bmp (inferred) | 421×267 | title logo (old variant 349×313) |
| — | — | bar-red-line-software.bmp | 225×67 | later-era publisher banner (not referenced in os7 code) |

`Misc/` extras (registration-era): enter-code.bmp 316×48, print-forms.bmp 316×48,
shareware-notice.bmp 582×753, shareware-notice-dog.bmp 579×754,
shareware-notice-rw.bmp 316×48, murphy-oliver.bmp 118×332 (mascot art).

All of these are already converted 1:1 to PNG in `web/public/assets/interface/` and
`web/public/assets/misc/` (same base names).

The 40×480 "bars" are vertical strips: the original 640×480 screen places a 40-px-wide
status/banner column beside the 480×480 content square; banner corners are extended
with gradient colors `kTopLeftRGBColor {39168,39168,52224}`,
`kTopRightRGBColor {0,26112,39168}`, `kBotLeftRGBColor {65535,52224,39168}`,
`kBotRightRGBColor {65535,39168,0}` (16-bit components).

### 7.2 Main menu hot rects (relative to the centered 480×480 `primary-interface`)

From `MakeCallbackData` (`SetHiliteRect(x, y, w, h)`):

- Intro (32, 96, 120×40); Credits (32, 156, 120×40); Help (32, 216, 120×40);
  Quit (32, 276, 120×40)
- Start (258, 343, 222×97) — 222×129 on taller windows
- Prefs (9, 400, 182×32)
- Oliver bubble cell (304, 198, 32×32)

Rendering: draw normal art; on hover swap the button region from the rollover sheet;
on click from the select sheet (all three sheets are pixel-aligned 480×480).

### 7.3 Interface Oliver bubble (`oliver-basics.bmp`, 608×32 = 19 cells of 32×32)

`BubbleOliver` (MRGfxUtils.c) at most every `kOliverTickDelay=8` ticks, states
Normal→Bub1→Bub2→Bub3→Normal; picks random axis a∈0..3 at cycle start. Source X
offsets in the strip: Normal = 0; Bub1 & Bub3 = `32 + 64a`; Bub2 = `64 + 64a`
(so cells 0–8 are used: 0 normal, then (1,2),(3,4),(5,6),(7,8) per axis).
Cells 9–18 exist in the BMP but are not referenced by surviving code.

### 7.4 In-level status bar composition

`DrawLevelRoom` (MRPlay.c): draw `status-bar-game` (or timed variant) at `statRect`;
level title lettered vertically in **green** bitmap font within rect
(11, 77)–(29, 271) relative to the bar; world-specific 16×16 mini icons copied from
`infotron-minis`/`red-disk-minis` (source rect `x = levelWorld*16`); numeric counters
(infotrons left, red disks, time) in Helvetica 9 pt, RGB16(39168,52224,52224) on
black, centered per `DrawNumberInRect` (baseline `bottom-4`, or `bottom-2` for the
2-digit clock variant).

---

## 8. Bitmap fonts (`font-large.bmp` = white, `font-small.bmp` = green; both 55×469)

Grid (from `CalculateLetterRect`, `kLetterBoxSize=17`, `kLetterBoxPadding=1`):
3 columns × 26 rows of **17×17** glyph cells separated by a 1-px grid;
cell (col,row) at pixel `(col*18 + 1, row*18 + 1)`. 3·18+1 = 55, 26·18+1 = 469. ✓

Character map → (col, row):

- col 2: `'A'..'Z'` → rows 0–25
- col 1: `'a'..'z'` → rows 0–25
- col 0: `'1'..'9'` → rows 0–8; `'0'` → 9; then rows 10–24:
  `! @ # $ % ^ & * ( ) ' " . , -`; row 25 = solid black cell
  (default for unknown characters; also drawn for space).

**Orientation**: glyphs are stored **rotated 90°** (verified visually: e.g. "N" reads
as "Z", "L" as "Γ"). Text is drawn as a vertical run: `MRLettering` places 17×17
cells top-to-bottom (`OffsetRect(&toRect, 0, offsetAmount)`), which produces sideways-
reading text along the 40-px-wide vertical bars (level title on the status bar, player
name on the map). Glyph colors: green sheet used for `kGreenLettering=0`
(PICT 151), white sheet for `kWhiteLettering=1` (PICT 152). Background is black;
copy mode is plain opaque CopyBits (text boxes sit on black bar art).

**Advance/kerning** (`MRCalcOffset`): default advance = `kLetterBoxSize − 5` = 12 px.
For non-space characters, scan the glyph cell row by row (pixels `left+1..right`,
starting at cell top); the first row that is entirely black marks the glyph extent;
advance = `(that row − top) + 2`, clamped to 17. Space: the code *intends* +5 but has a
pointer-arithmetic bug (`offsetAmount += 5` on the pointer), so a space effectively
advances the default 12 px and stamps a black cell. Reproduce advance-by-measurement
(it is what gives the tight vertical spacing); decide deliberately whether to keep the
space bug (12 px + black square) or the intended 17 px.

Web copies exist at `web/public/assets/interface/font-{large,small}.png`; osx keeps
copies in `Images.xcassets` (`font-small` is loaded by `MurphyLevelScene.swift`);
`status-bar-default.png` (40×480) is in the osx xcassets too.

---

## 9. Recommended extraction strategy for the web port

1. **Level tiles / Oliver frames**: the repo already contains three equivalent
   encodings; pick one and generate the rest:
   - *Most convenient + already wired*: `web/public/assets/atlas/*.png` +
     `tiles.json` (20-col, alphabetized names). Pros: single texture per world,
     name-indexed. Cons: order no longer equals game tile IDs, so keep a
     name-keyed indirection (as `web/src/assets.ts` already does).
   - *Most faithful to IDs*: `iPhone/{world}.png` — cell index == original tile ID
     (`id = t*10 + s`), which matches `.mbl` level bytes directly. Crop to 320×992 and
     never sample cells 214–219 / 305–309 (filler).
   - *Canonical name→pixels source*: `osx/Murphy/*.atlas/` per-name 32×32 PNGs.
   These were all derived from the legacy BMPs via `assemble-opengl-texture.py`;
   the BMPs remain the ground truth if re-extraction is ever needed (mind the 1-px
   row gutters listed in §2.1).
2. **Transparency**: nothing in the level layer needs alpha. Implement ports by
   drawing the port tile after the player sprite (or on an upper layer). Only
   `oliver-mini` needs real alpha — bake it by applying `oliver-mini-masks`
   (white→transparent) into an RGBA PNG.
3. **Animation data**: hard-code the frame ranges/sequences of §4 (they are ranges in
   the canonical enum, so they survive any atlas repacking as name lists). Use the
   original tick timings (1 tick = 1/60 s) rather than the SpriteKit approximations if
   fidelity to the Mac game is the goal; `web/src/main.ts` already fixes
   `TICK_HZ = 60`.
4. **Fonts**: use `font-large.png`/`font-small.png` with the §8 grid; implement
   measured advance by scanning glyph columns once at load time (equivalent to
   `MRCalcOffset`, but precomputed). If horizontal text is wanted in the web UI,
   rotate glyph cells 90° at load.
5. **Interface**: `web/public/assets/interface/*.png` are complete 1:1 conversions;
   use the §7.2 hot rects and the three-sheet (normal/rollover/select) swap technique.
6. **World map**: `world-tiles.png` 18-wide indexing with §6 animation table; the
   world graph data lives in the `'MRwd'` resource (World data file), not in the repo
   as a plain file — see open questions.

---

## 10. Ambiguities, bugs, and unfinished things (do not guess silently)

- `iPhone/TMSprite.[hm]` is an **empty stub** — no sprite implementation exists in the
  iPhone demo; only tile-image cycling (`TMTileMap animateTileId:...`) was implemented.
  Its `allInRange:YES` mode rotates every tile image in the range; the game only uses
  `NO` (single base tile cycles through the range).
- The tile-names JSON consumed by `tools/unbundle_images.py` is not in the repo.
  Reconstruct from §1.2 (order = ID order) if needed.
- `MurphyTiles.swift` carries the comment "XXX all hacked up from ancient MR C code —
  who knows what the right thing might be": the Swift/iPhone `220` base for Oliver
  frames is an atlas-packing artifact; the original engine treated Oliver frames as
  sprite-local indices 0–84 (`OliverFrameIDs` in MRPlayConstants.h).
- PICT IDs 131–134 (`kSavanaMRPict` etc.) have no matching BMP export; unknown art.
- The mapping of `bar-*`/`story`/`credits`/`help-text` BMPs to PICTs 135–138/143–145,
  of `status-bar-map*` to 146/147, `level-stats*` to 163, `replay-level*` to 165, and
  `level-stats-time-left` to main-res PICT 133 is inferred from sizes and usage, not
  from an explicit table.
- `oliver-basics.bmp` cells 9–18 and `infotron.bmp` sheet cells 17–20 are unused by
  surviving code.
- `tile-info.txt` says `robot.bmp 0` (no gutter) but the file is 128×131, which
  implies 4 rows with 1-px gutters; `assemble-opengl-texture.py` uses 1. Trust 1.
- `MRMap.h` declares `kTilesDeep=20` but the sheet/loader use 21 rows (tiles 0–377);
  `PointFromTile` would reject tile 377 (`p.v >= kTilesDeep`) — latent off-by-one in
  the original.
- Space-advance pointer bug in `CalculateLetterRect` (§8).
- Laser crossed-beam tiles 86/93 unreferenced; laser feature generally looks
  half-finished ("Load in only if you're HAPPY" comment; Deathtron world reserved but
  has no artwork/atlas).
- Death-animation randomization uses `GetRandom(1,9)<=3` then `GetRandom(0,9)<5`;
  exact RNG doesn't matter for graphics but the three non-electric variants are
  roughly 1/3, 1/3, 1/3.
- The web atlases (`web/public/assets/atlas`) contain 305 names sorted
  alphabetically; the six `UnusedTileN` entries are included. Filler pixels from the
  red/blue padding may be present in unused cells — never render by raw index without
  the name table.

## Follow-up: effective step rate of the ambient infotron/terminal tile animation

**Resolved: 15 Hz (one image step every 4 ticks ≈ 66.7 ms) when the game holds its
30 fps cap — i.e., the tile image advances on every *other* rendered frame.**
This supersedes the conflicting figures elsewhere: game-rules.md's "~20 Hz" is wrong
(that is only the degenerate slow-machine ceiling, see below), this file's earlier
"every >2 ticks ~33 ms" is wrong as a rate (the strict `>` aliases against the 2-tick
frame poll to 4 ticks), and app-flow.md's "~30 fps" is the *polling* rate, not the
step rate.

### Verified chain of evidence

1. **The gate** — `legacy/murphy/os7code/mr/MRPlay.c`, `Tile_ChangeProc`
   (lines 737–759 after CR→LF conversion; the file uses classic-Mac CR line endings):

   ```c
   if ( TickCount() - cb->lastTileAnimTicks > kSpriteFrameTicks ) {
       ... SWChangeTileImage( theWorld, kInfotronTile, curImage );  // 94..109 cycle
       ... SWChangeTileImage( theWorld, kTerminalTile, curImage );  // 110..121 cycle
       cb->lastTileAnimTicks = TickCount();
   }
   ```

   `kSpriteFrameTicks = 2` and `kMaxLevelFPS = 30`
   (`legacy/murphy/os7code/mr/MRPlayConstants.h`, lines 51–52:
   `#define kSpriteFrameTicks 2`, `#define kMaxLevelFPS 30`). The comparison is
   **strictly greater**, so a step requires ≥ 3 ticks elapsed since the last step.

2. **How often the proc is polled** — once per *rendered frame*, not once per loop
   pass. In both SpriteWorld copies in the repo
   (`legacy/murphy/spriteworld/swscrolling-iphone/swx/SpriteWorld.c` and
   `swx-imurphy/swx/SpriteWorld.c`, both at lines 1581–1584),
   `SWProcessSpriteWorld` first applies the fps cap
   (`runningTimeCount - timeOfLastFrame >= fpsTimeInterval`, else
   `frameHasOccurred = false; return;`) and only on the frame-occurred path runs:

   ```c
   // Call the tileChangeProc, if there is one
   if (spriteWorldP->tileChangeProc != NULL)
       (*spriteWorldP->tileChangeProc)(spriteWorldP);
   ```

   `SWProcessScrollingSpriteWorld` (Scrolling.c:317) just calls
   `SWProcessSpriteWorld` and early-returns if `!frameHasOccurred`.
   `SWSetSpriteWorldMaxFPS(sw, 30)` sets `fpsTimeInterval = 1000/30` ms in the
   modern port (SpriteWorld.c:1041; classic SW 2.x used `60/30 = 2` ticks — same
   33.3 ms interval).

3. **Hookup and drive rate** — `MRPlay.c` line 1095
   `SWSetSpriteWorldMaxFPS( cbData->sw, kMaxLevelFPS )`, line 1106
   `SWSetTileChangeProc( cbData->sw, Tile_ChangeProc )`; during gameplay
   `MRSetSleepTime(0)` (line 466) makes the event loop spin continuously
   (`MRLoop.c` `MREventLoop`, lines 516–534, `OSEventAvail` + 1-tick `WaitNextEvent`),
   and `Level_IdleBack` calls `SWProcessScrollingSpriteWorld` /
   `SWAnimateScrollingSpriteWorld` on every idle pass (MRPlay.c lines 417–418).
   So the SpriteWorld fps gate — not the loop — sets the poll cadence: frames land
   every 2 ticks.

4. **The aliasing math** — frames (and therefore polls) at ticks 0, 2, 4, …
   A step at tick T sets `lastTileAnimTicks = T`. Poll at T+2: elapsed 2,
   `2 > 2` false → skip. Poll at T+4: `4 > 2` → step. **Steps land every 4 ticks
   = 66.7 ms ≈ 15 Hz** (15.04 Hz with the true 60.15 Hz Mac tick).

### Consequences

- Infotron shimmer: 16 images (`kFirstInfotronTile 94` … `kLastInfotronTile 109`),
  initial image set to 94 at MRPlay.c:1107 → full cycle 16 × 4 ticks = 64 ticks
  ≈ **1.07 s per loop**.
- Terminal: 12 images (110…121, initial set at MRPlay.c:1108) → 48 ticks =
  **0.8 s per loop**.
- Slow-machine quirk (faithful detail, optional): if a frame arrives ≥ 3 ticks after
  the previous step, that frame's poll passes and the tiles step *every* frame — so
  at a degraded ~20 fps the ambient animation actually runs slightly faster
  (up to 20 Hz, the strict-`>` ceiling) than the canonical 15 Hz at 30 fps.
- **Port rule:** poll the tile step on each 30 fps render frame and step when
  `(now - lastStep) > 2 ticks` (tick = 1/60 s), resetting `lastStep = now` after
  stepping. On a machine holding 30 fps this yields exactly one step per 4 ticks
  (15 Hz), including the original's slow-frame behavior for free. If you want a
  fixed-rate simplification, hard-code one step every 66.7 ms.
