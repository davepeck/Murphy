# Murphy's Revenge — Level File Formats (Byte-Exact Specification)

Scope: every level container format found in the repo, in order of provenance:

1. Classic Mac resource formats (`'MRlv'`, `'MRwd'`) — the original 1997 game's authoritative on-disk format (documented from `legacy/murphy/os7code/mr/MRPlay.c`, `MRMap.c/.h`, `MRPlayCommon.h`, `MRPlayConstants.h`, `MRResources.c`, and the level designer `legacy/murphy/os7code/ld/MRMap.p`).
2. `.lvl` — plain-text dump of the resource data, one number/string per line (`legacy/murphy/levels/*.lvl`). Read by `assemble-binary-level.py`.
3. `.mbl` — "Murphy Binary Level," produced from `.lvl` by `legacy/murphy/levels/assemble-binary-level.py`; shipped in the 2009 iPhone port (`iPhone/*.mbl`), parsed by `iPhone/MurphyLevel.m`.
4. `.mlv` — `.mbl` with corrected width/height and truncated grid, produced by `tools/fix_mbl.py`; shipped in the 2014 OS X port (`osx/Murphy/Levels/*.mlv`), parsed by `osx/Murphy/MurphyLevel.swift`.

All multi-byte integers in ALL of these formats are **big-endian**. All strings are ASCII (originally Mac Str255, so ≤255 chars). Verified: rebuilding `Bolder.mbl`, `World.mbl`, `Achtung!.mbl`, and `Wizard.mbl` from their `.lvl` sources via the algorithm below reproduces the shipped `.mbl` files byte-for-byte.

---

## 1. `.mbl` binary format (iPhone port)

There is **no magic number and no version field**. Files are identified by extension only.

```
offset  size            field
------  --------------  -----------------------------------------------
0       N1+1            level name: ASCII bytes, NUL-terminated
        N2+1            graphics-set name: ASCII bytes, NUL-terminated
                        (always lowercase: "infotron" | "aquatron" | "savanatron")
        2               infotrons required (uint16, big-endian)  — win quota
        2               width  (uint16, big-endian)              — always 120 in shipped files
        2               height (uint16, big-endian)              — always 120 in shipped files
        width*height*2  tile grid, row-major, origin top-left, y (row) outer,
                        x (column) inner. Each cell is TWO unsigned bytes:
                          byte 0 = iconX  (a.k.a. "s"; tile-sheet column)
                          byte 1 = iconY  (a.k.a. "t"; tile-sheet row)
------  --------------  -----------------------------------------------
EOF     (no trailing data — file ends exactly after the grid)
```

- Logical **tile ID = iconY \* 10 + iconX** for game levels (game tile sheets are 10 tiles wide; `TILESET_WIDTH = 10` in `MurphyLevel.swift`, `kLevelTilesWide = 10` in `MRPlayConstants.h`). Valid game tile IDs are 0–213 (`kMaxLevelTiles = 214`), though shipped level boards only use iconY ≤ 6 (IDs ≤ 69).
- Tile IDs are conceptually 8-bit ×2 (a byte pair), **not** a 16-bit integer; there is no endianness question inside a cell — X byte always precedes Y byte.
- Example (`iPhone/Bolder.mbl`, 28,822 bytes):
  `42 6f 6c 64 65 72 00` ("Bolder\0") `69 6e 66 6f 74 72 6f 6e 00` ("infotron\0") `00 14` (infotrons=20) `00 78` (width=120) `00 78` (height=120), then 120×120×2 = 28,800 grid bytes. 7+9+6+28800 = 28,822 ✓.
- **The declared 120×120 size is a lie**: every shipped level's real playing field is a smaller rectangle in the top-left corner, delimited by border tiles (see §5 "size detection"). Cells outside the real field are junk/zero fill (mostly `00 00` = CircuitTile) and must be ignored.
- Parser behavior:
  - `iPhone/MurphyLevel.m` (`initWithData:`): reads name, graphicsSet (NUL-terminated, `NSASCIIStringEncoding`), infotrons/width/height as big-endian uint16 (`BinaryDataScanner` with `littleEndian:NO`), then `height`×`width` byte pairs into parallel `iconS[]`/`iconT[]` arrays indexed `(y*width)+x`. No length validation beyond throwing if the buffer runs out.
  - Fields **not present** in `.mbl` (dropped relative to the original format): hint text, time limit, numeric world ID (only the graphics-set *name* survives), per-cell trigger/state/kind/levelID, player start (derived from the grid instead — the cell whose tile ID is 9, `kOliverTile`).

### What the iPhone renderer does with the grid (context, `iPhone/MurphyView.m`)
- Finds the real bounds by scanning row-major for the **first** cell with tile ID 37 (`kBottomRightBorderTile`); sets `actual_width = x+1`, `actual_height = y+1` (loop conditions `actual_width == 0` / `actual_height == 0` make it first-occurrence).
- Tile substitutions when filling the display grid: ID 19 (`kInfotronTile`) → 94 (`kFirstInfotronTile`); 20 (`kQuarkTile`) → 189 (`kFirstQuarkTile`); 7 (`kTerminalTile`) → 110 (`kFirstTerminalTile`); 9 (`kOliverTile`) → 220 (`kBubFrame`, records Murphy start x/y); IDs 21–24 (scissors) → 198 (`kFirstUpScissorTile`).
- Tile atlas: `[TMTileAtlas tileAtlasWithResourcePNG:levelFile.graphicsSet tilePixelWidth:32 tilePixelHeight:32 tilesAcross:10 tilesDown:31]` — i.e. the PNG named exactly by the graphics-set string ("infotron"/"aquatron"/"savanatron"), 32×32-px tiles, 10 per row, 31 rows (tile IDs 0–309; PNGs are physically 512×1024 with content in the top-left 320×992).

---

## 2. `.mlv` binary format (OS X port) and `tools/fix_mbl.py`

`.mlv` layout is **identical to `.mbl`** (same header, same byte-pair grid) except `width`/`height` are the *actual* level dimensions and the grid contains only `width*height` cells.

`tools/fix_mbl.py` conversion algorithm (`.mbl` → `.mlv`):
1. Parse the `.mbl` (big-endian, as above).
2. Pass 1: read all `width*height` (x,y) pairs; for each, compute `tilemap = tilemap_y*10 + tilemap_x`; every time `tilemap == 37` (bottom-right border), set `actual_width = x+1; actual_height = y+1`. Net effect: the **last** occurrence in row-major order wins. If no 37 exists, dimensions stay 120×120.
3. Pass 2: keep only cells with `x < actual_width` **and** `y < actual_height` (i.e. crop the top-left rectangle).
4. Write name, graphics (NUL-terminated), infotrons, actual_width, actual_height (uint16 BE), then the cropped grid pairs.

Parser (`osx/Murphy/MurphyLevel.swift`, `fromData`):
- Reads the same header (big-endian: `BinaryDataScanner(littleEndian: false, encoding: NSASCIIStringEncoding)`).
- **Strict length check**: fails (returns nil) unless remaining bytes == `width*height*2` exactly. (Consequently raw `.mbl` files also parse *if* their junk area maps to valid enum values, but the file is expected to be an `.mlv`.)
- Each pair becomes `tileMapRaw = Int(tileMapY)*10 + Int(tileMapX)`; mapped through `enum LevelTile : Int` (214 cases, raw values 0–213, `CircuitTile = 0` … `UpLeftScissorTile = 213`). Any raw value outside the enum → whole level load fails (returns nil).
- Loads levels from bundle resources with extension `"mlv"`; texture atlas chosen by `SKTextureAtlas(named: level.graphicsSetName)` (`infotron.atlas`, `aquatron.atlas`, `savanatron.atlas` exist in `osx/Murphy/`).

### Known defects of fix_mbl's heuristic (verified against shipped files)
- **Crystalline**: the `.lvl` contains TWO cells with tile 37 — (24,18) (the true bottom-right corner; original-game algorithm gives 25×19) and a stray one at (24,23) below the playing field. fix_mbl takes the last → shipped `Crystalline.mlv` header says **25×24** (`00 19 00 18`), five junk rows taller than the true level. The iPhone first-occurrence scan and the original game both get 25×19.
- **World**: the world map has NO true (7,3) pair, but tile-pair (x=17, y=2) — a world-tileset icon — satisfies `2*10+17 == 37`. Five such cells exist; the last is at grid (60,65), so shipped `World.mlv` is a meaningless **61×66** (`00 3d 00 42`) crop of the 120×120 map. `World.mlv` should be considered corrupt; use `World.lvl`/`World.mbl` for map-room data.
- All other 25 game levels have exactly one tile-37 cell; fix_mbl, the iPhone scan, and the original algorithm agree for them.

---

## 3. `.lvl` text format and `assemble-binary-level.py`

One value per line (lines are `strip()`ped; files use LF). Layout (from the header comment and code of `legacy/murphy/levels/assemble-binary-level.py`):

```
line 0  TITLE            (level name, e.g. "Bolder"; may contain spaces, '!', '&', ''', '.')
line 1  HINT             (one line of hint text; may be empty — e.g. Wizard.lvl)
line 2  infotrons        (int; number of Infotrons required)
line 3  graphics set     ("Infotron" | "Aquatron" | "Savanatron" — capitalized)
line 4  hours            (int, time limit)
line 5  minutes          (int)
line 6  seconds          (int; 0/0/0 = no time limit)
line 7  width            (int; always 120 in shipped files)
line 8  height           (int; always 120)
line 9+ width*height cells, row-major (y outer, x inner), 8 lines per cell:
          +0  IconX            } the tile: tile ID = IconY*10 + IconX (game levels)
          +1  IconY            }           or IconY*18 + IconX (World map)
          +2  Trigger ID       (map room only; 0 in all game levels)
          +3  Trigger IconX    } icon this cell toggles to when its trigger fires
          +4  Trigger IconY    }
          +5  State            (0 in all shipped files)
          +6  Kind             (game level: 0=normal, 1=keycoded, 2=gravitySwitch,
                                3=hardwarePhase [per ld/MRMap.p comment — never used:
                                all 26 shipped game levels are all-zero];
                                world map: 0=normal, 1=trigger, 2=level)
          +7  Level ID         (world map only: which level this cell launches;
                                0 in all game levels)
```
Total lines = 9 + 120·120·8 = 115,209 (+1 trailing blank). The 8 per-cell fields are exactly the fields of the original `WorldPoint`/`LevelPoint` record (§5), with `p`/`trigP` Points emitted X-before-Y (note the resource stores v/Y first — the text dump swapped to X-first).

**`.lvl` → `.mbl` mapping** (`assemble-binary-level.py`, Python 2):
- `lvl_graphics_set = lvl[3].strip().lower()` — this is where "Infotron" becomes "infotron".
- Writes header `struct.pack(">%ds%dsHHH" % (len(name)+1, len(gfx)+1), name, gfx, infotrons, width, height)` — the `+1` on the string lengths produces exactly one NUL terminator each (Python `struct` pads `s` with NULs).
- Then for each cell in row-major order: `struct.pack(">BB", cell[ICON_X], cell[ICON_Y])`.
- **Everything else is discarded**: hint, hours/minutes/seconds, trigger ID, trigger icon, state, kind, level ID. There is no tile-code translation table — icon X/Y pass through unchanged.
- `assemble-them.py` just runs `./assemble-binary-level.py "<f>"` for every `*.lvl` in the cwd.
- The file's comment sketches a future JSON format ("version: 0 …") that was never implemented.
- The tool that *exported* `.lvl` from the Mac resource forks is **not in the repo** (an older, different text dump of the same data exists at `legacy/murphy/sdl-test/World.txt` in a `field: value`/`%`-separated style).

---

## 4. Authoritative tile-code table (game levels)

Single source of truth, identical in `legacy/murphy/os7code/mr/MRPlayConstants.h` (`enum TileTypes`), `iPhone/MurphyConstants.h` (`enum LevelTileIds`), and `osx/Murphy/MurphyTiles.swift` (`enum LevelTile`, plus texture names = enum name + ".png"). ID = iconY·10 + iconX; (iconX, iconY) given for the level-file byte pair.

| ID | (X,Y) | Name | Meaning |
|----|-------|------|---------|
| 0 | (0,0) | kCircuitTile | "circuit board" base terrain (edible, like Supaplex base) |
| 1 | (1,0) | kBlankTile | empty space |
| 2 | (2,0) | kExitTile | exit (closed) |
| 3 | (3,0) | kGreenDiskTile | green disk |
| 4 | (4,0) | kYellowDiskTile | yellow disk |
| 5 | (5,0) | kRedDiskTile | red disk |
| 6 | (6,0) | kGoldDiskTile | gold disk |
| 7 | (7,0) | kTerminalTile | terminal (static form; animates via 110–121) |
| 8 | (8,0) | kBugTile | "bug" electrified circuit |
| 9 | (9,0) | kOliverTile | **player (Murphy/Oliver) start position** |
| 10 | (0,1) | kLeftPortTile | port, enter leftward |
| 11 | (1,1) | kDownPortTile | port, enter downward |
| 12 | (2,1) | kRightPortTile | port, enter rightward |
| 13 | (3,1) | kUpPortTile | port, enter upward |
| 14 | (4,1) | kLeftRightPortTile | horizontal 2-way port |
| 15 | (5,1) | kUpDownPortTile | vertical 2-way port |
| 16 | (6,1) | kFourWayPortTile | 4-way port |
| 17 | (7,1) | kZonkTile | zonk (falling boulder) |
| 18 | (8,1) | kUpZonkTile | "up zonk" (floats upward variant) |
| 19 | (9,1) | kInfotronTile | infotron (collectible; static form) |
| 20 | (0,2) | kQuarkTile | quark (roaming enemy; static form) |
| 21 | (1,2) | kUpScissorTile | scissor enemy facing up |
| 22 | (2,2) | kRightScissorTile | scissor facing right |
| 23 | (3,2) | kDownScissorTile | scissor facing down |
| 24 | (4,2) | kLeftScissorTile | scissor facing left |
| 25 | (5,2) | kChipTile | RAM chip (full) |
| 26 | (6,2) | kTopChipTile | chip, top half |
| 27 | (7,2) | kBottomChipTile | chip, bottom half |
| 28 | (8,2) | kLeftChipTile | chip, left half |
| 29 | (9,2) | kRightChipTile | chip, right half |
| 30 | (0,3) | kTopLeftBorderTile | **level border: top-left corner — always at grid (0,0)** |
| 31 | (1,3) | kTopBorderTile | border, top edge |
| 32 | (2,3) | kTopRightBorderTile | **border, top-right corner — marks level width** |
| 33 | (3,3) | kRightBorderTile | border, right edge |
| 34 | (4,3) | kLeftBorderTile | border, left edge |
| 35 | (5,3) | kBottomLeftBorderTile | border, bottom-left corner |
| 36 | (6,3) | kBottomBorderTile | border, bottom edge |
| 37 | (7,3) | kBottomRightBorderTile | **border, bottom-right corner — marks level width+height (size sentinel)** |
| 38 | (8,3) | kBlankChipTile | blank chip |
| 39–65 | (9,3)–(5,6) | kFirstHardwareTile, kHardwareTile2 … kHardwareTile26, kLastHardwareTile | 27 decorative hardware/wall tiles (indestructible) |
| 66 | (6,6) | kPhantomTile | phantom tile |
| 67 | (7,6) | kLaserTile | laser emitter |
| 68 | (8,6) | kLaserPowerTile | laser power source |
| 69 | (9,6) | kForeMirrorTile | mirror "/" |
| 70 | (0,7) | kBackMirrorTile | mirror "\\" |
| 71 | (1,7) | kEmptyPhaseTile | phase hardware, phased-out |
| 72 | (2,7) | kFullPhaseTile | phase hardware, phased-in |
| 73 | (3,7) | kPhaseTermTile | phase terminal |
| 74–79 | (4,7)–(9,7) | kEmptyTile4 … kEmptyTile9 | unused/empty slots |
| 80–93 | (0,8)–(3,9) | kBlankLaserPiece, kLaserEmptyPiece, kLaserPowerPiece, kLaserPiece, kForeMirrorPiece, kBackMirrorPiece, kTLBRLaserForeMirrorLightPiece, kRLLaserLightPiece, kUDLaserLightPiece, kTLLaserForeMirrorLightPiece, kBRLaserForeMirrorLightPiece, kBLLaserBackMirrorLightPiece, kTRLaserBackMirrorLightPiece, kBLTRLaserBackMirrorLightPiece | laser-beam render pieces |
| 94–109 | (4,9)–(9,10) | kFirstInfotronTile, kInfoTile2 … kInfoTile15, kLastInfotronTile | infotron animation, 16 frames |
| 110–121 | (0,11)–(1,12) | kFirstTerminalTile … kLastTerminalTile | terminal animation, 12 frames |
| 122–127 | (2,12)–(7,12) | kFirstBugTile … kLastBugTile | bug animation, 6 frames |
| 128–137 | (8,12)–(7,13) | kFirstExplTile, kExplTile2, kExplTile3, kIgniteExplTile ("the one where chain reactions start"), kExplTile5 … kLastExplTile | explosion animation, 10 frames |
| 138–145 | (8,13)–(5,14) | kFirstExitTile … kLastExitTile | exit-opening animation, 8 frames |
| 146–155 | (6,14)–(5,15) | kFirstZapRedDiskTile, kZapRedDiskTile1–6, kLastZapRedDiskTile, kEmptyZapTile8, kEmptyZapTile9 | red-disk zap animation |
| 156–165 | (6,15)–(5,16) | kFirstZapCircuitTile, kZapCircuitTile11–15, kLastZapCircuitTile, kEmptyZapTile17–19 | circuit zap (eat) animation |
| 166–178 | (6,16)–(8,17) | kFirstZapInfotronTile, kZapInfotronTile21–31, kLastZapInfotronTile | infotron zap animation, 13 frames (First + 21…31 + Last) |
| 179–180 | (9,17)–(0,18) | kFirstRDBlinkTile, kLastRDBlinkTile | red-disk blink, 2 frames |
| 181–184 | (1,18)–(4,18) | kFirstZonkTile, kZonkTile1, kZonkTile2, kLastZonkTile | zonk roll animation, 4 frames |
| 185–188 | (5,18)–(8,18) | kFirstUpZonkTile, kUpZonkTile1, kUpZonkTile2, kLastUpZonkTile | up-zonk animation, 4 frames |
| 189–197 | (9,18)–(7,19) | kFirstQuarkTile, kQuarkTile1–7, kLastQuarkTile | quark animation, 9 frames |
| 198–213 | (8,19)–(3,21) | kFirstUpScissorTile, kUpScissorTile1, kLastUpScissorTile, kUpRightScissorTile, kFirstRightScissorTile, kRightScissorTile1, kLastRightScissorTile, kDownRightScissorTile, kFirstDownScissorTile, kDownScissorTile1, kLastDownScissorTile, kDownLeftScissorTile, kFirstLeftScissorTile, kLeftScissorTile1, kLastLeftScissorTile, kUpLeftScissorTile | scissor animations, 16 tiles = 4 facings × 3 frames + 4 diagonal turning frames (198–200 up, 201 up-right, 202–204 right, 205 down-right, 206–208 down, 209 down-left, 210–212 left, 213 up-left) |
| 214 | — | kMaxLevelTiles | count sentinel (214 tile IDs, 0–213) |

Player-sprite frames occupy IDs **220–304** in the modern tile sheets (`iPhone/MurphyConstants.h` `enum OliverTileIds` and `osx/Murphy/MurphyTiles.swift` `enum OliverTile`, 85 names: `kBubFrame = 220` … `kExLaxBubLastFrame = 304`; the count sentinel `kMaxBubFrames = 305` exists only in the iPhone enum — the classic `MRPlayConstants.h` `enum OliverFrameIDs` is the same list 0-based, so there `kMaxBubFrames = 85`): idle bubble frames 220–228, walk left 229–231, walk right 232–234, empty 235, push left/right/down/up 236–243, zap L/R/D/U 244–247, empty 248–249, fade death 250–259, bubble death 260–269, static death 270–279, yawn 280–291, shock death 292–298, empty 299, Eureka 300, Sad 301, "FuckYou" 302, ExLax 303–304. IDs 214–219 are unused padding between the two ranges (`UnusedTile1..6` in the osx atlases). These never appear in level files; they're renderer tiles.

Shipped level boards only contain IDs 0–69 (max iconY observed = 6, in Dash Dash and Wizard). IDs ≥ 70 in a level file would be a data error.

Original tile pixel size: 32×32 (`kLevelItemSize = 32` in both `MRPlayConstants.h` and the designer).

---

## 5. Level-size detection ("the level is smaller than 120×120")

Original game (`MRPlay.c` `GetLevelSize`) — the authoritative algorithm:
1. If cell (0,0) is **not** icon (h=0, v=3) (tile 30, top-left border), the level size is the full 120×120.
2. Otherwise scan row 0 rightward until icon (h=2, v=3) (tile 32, top-right border) → that column = width−1.
3. Then scan **down that same column** until icon (h=7, v=3) (tile 37, bottom-right border) → that row = height−1.
4. Size = (col+1, row+1). No bounds checks — a malformed level without the sentinels would run off the array.

Every shipped game level passes step 1 and yields the sizes in §8. Prefer this algorithm over fix_mbl's last-37 heuristic (see §2 for the Crystalline discrepancy) and over the iPhone first-37 heuristic (correct on shipped data but relies on tile 37 appearing first at the true corner).

---

## 6. Original Mac resource formats (for completeness / provenance)

Resource files live inside the game folder (`OpenResFile(":MR Data:…")`): `Aquatron`, `Players`, `Interface`, `Infotron`, `Deathtron`, `Savanatron`, `World`. Level and world-map resources live in the **World** file.

### 6.1 `'MRlv'` — one game level (also used for map-room level info)
- Resource type `'MRlv'` (`kLevelResType`), resource ID = **300 + levelNumber** (`kLevelBaseID = 300`); levelNumber is the map's `levelID` (1-based), so IDs start at 301. `MAX_LEVELS = 100`.
- Byte layout (all integers int16 BE; `Str255` = 1 length byte + 255 data bytes = 256 bytes fixed):

```
offset  size  field            notes
0       2     reqInfo          infotrons required; **-1 (<0) = sentinel: extended header present**
2       2     levelWorld       0=Infotron 1=Aquatron 2=Savanatron 3=Deathtron
4       256   levelTitle       Str255 (Pascal string)
260     256   levelInfo        Str255 — the hint text
516     2     phCount          count of int16s (incl. end marker) in compressed iconX stream
518     2     pvCount          same for iconY stream
520     2     kindCount        same for kind stream
--- if reqInfo >= 0, header ends here (522 bytes, "OldExtraLevelInfo") ---
--- if reqInfo < 0, extended header ("ExtraLevelInfo", 536 bytes): ---
522     2     actualInfo       the real infotron quota (used instead of reqInfo)
524     2     hours            time limit
526     2     mins
528     2     secs
530     6     dataOne/dataTwo/dataThree   reserved, unused
--- then three RLE streams back-to-back: ---
        phCount*2     RLE-compressed iconX values (int16 each)
        pvCount*2     RLE-compressed iconY values
        kindCount*2   RLE-compressed kind values
```

- RLE scheme (`kRLEFlag = 128`, `kRLEEnd = 129`, both as int16 values in the stream):
  - Literal int16s are copied through.
  - `128, runLength, value` = emit `value` `runLength` times.
  - `129` terminates the stream.
  - Compressor (`ld/MRMap.p RLECompress`) only encodes runs of length ≥ 4; shorter runs are emitted literally. Literal values 128/129 are unrepresentable, but real values never exceed ~21 (iconY) so it's safe. The returned count includes the end marker.
- Each decompressed stream has exactly `kMaxLevelSizeSquared = 14400` (=120·120) entries, unflattened row-major (x fastest): `level[x][y].p.h = ph[i]; level[x][y].p.v = pv[i]; level[x][y].kind = kind[i]`.
- Note: only iconX/iconY/kind are stored per-cell for game levels; trig/trigP/state/levelID exist in the in-memory `LevelPoint` but are not in `'MRlv'`.
- Timer: only extended levels have one (`cb->timer = true` iff reqInfo<0). Levels with a time limit in `.lvl` (nonzero H/M/S) were extended-format resources.
- The map room reads the same resources for its info box via `MRMap.c GetLevelInfo` (struct `MapLevelInfo` = ExtraLevelInfo fields + an in-memory-only `Boolean isExtended`; "there will be a few garbage bytes to read instead" for old-format resources).

### 6.2 `'MRwd'` — the overworld map
- Resource type `'MRwd'` (`kWorldResType`), ID **128** (`kWorldResID`), in the World file.
- Raw uncompressed dump of `WorldPoint theWorld[120][120]` — 16 bytes per cell × 14400 = **230,400 bytes** ("there are 120x120 of these! = 230k").
- `WorldPoint` layout (Mac `Point` is `{v: int16; h: int16}` — **v/Y first**):

```
offset  size  field    meaning (comments from ld/MRMap.p)
0       2     p.v      icon Y   } "represents the actual icon location on the icons grid"
2       2     p.h      icon X   }
4       2     trig     "for normal piece, is assoc. trigger; for trigger, is ID; for lev, is index"
6       2     trigP.v  } "for norm piece, what to trig to; for trigger, what kind it is"
8       2     trigP.h  }
10      2     state    "for trigger, =0,1 (off, on); for lev, =0 nopass, =1 passed"
12      2     kind     "=0 for normal, =1 for trigger, =2 for level"
14      2     levelID  "level associated with this piece" (1-based; loads 'MRlv' 300+levelID)
```

- Array memory order: C declaration `theWorld[kMaxWorldSize][kMaxWorldSize]` indexed `theWorld[col][row]` (Pascal `array[1..120,1..120]` indexed `[x, y]`), so the blob is **x-major** (all 120 y entries for x=0, then x=1, …) — unlike the row-major `'MRlv'` streams and `.lvl`/`.mbl` grids.

---

## 7. `World.mbl` / the map room

`World.lvl` (internal name **"Work With World Worldview"**, hint empty, infotrons 0, graphics set line says "Infotron", time 0:0:0, 120×120) is a text dump of the full `WorldPoint` array — the only `.lvl` where the trigger/state/kind/levelID fields are populated. `World.mbl` (28,841 bytes) keeps only the icon pairs, so **`.mbl`/`.mlv` cannot drive the map room** (triggers and level links are lost); a port needs `World.lvl` (or the `'MRwd'` resource).

Key differences from game levels (from `mr/MRMap.c`):
- **Tile sheet is 18 tiles wide** (`kTilesWide = 18`), tiles are **24×24 px** (`kWorldItemSize = 24`), tile ID = iconY·18 + iconX, 378 tiles total (`kNumMapTiles = 378`, loaded from PICT 148 `kMapTilesPict`; caution: `kTilesDeep = 20` but 378 tiles need 21 rows — `PointFromTile` fatals on v ≥ 20). Modern assets: `iPhone/world.png` (512×512) exists but no modern code loads it; both modern ports would wrongly use the infotron sheet for World because its graphics-set string is "infotron".
- **Passability is a function of the icon's position in the tile sheet** (`MakePieceKinds`, `pieceKinds[tileY][tileX]`, `kUsableTilesWide = 9`):
  - (X=1, Y=0..2) → `kLevelPiece` (3) — unpassed level marker
  - (X=1, Y=3..5) → `kPassedLevelPiece` (4)
  - (X=0..8, Y=6) → `kSwitchPiece` (2)
  - (X=0..8, Y=7..8) → `kPassablePiece` (1) — paths
  - everything else → `kNoPassPiece` (5)
- **Player start marker**: tile ID 18, i.e. icon (X=0, Y=1) (`kOliverStartTile`). In `World.lvl` it's at grid (58,54). When building the display map the marker cell is drawn as its `trigP` icon instead (for (58,54): trigP=(4,7) → tile 7·18+4 = 130).
- **Switches**: stepping on a `kSwitchPiece` runs `TriggerSwitch`: every cell whose `trig` equals the switch's `trig` swaps `p` ↔ `trigP` (toggling scenery/bridges), and the player's persistent `theThrows[]` list is updated (`MAX_SWITCHES = 100`). Trigger IDs used in `World.lvl`: 1,2,3,4,5,6,7,9,11,13,16,17,18,20,21,55. 24 cells have kind==1.
- **Passed levels**: when the current player has `passedLevel[levelID-1]`, the level piece's icon is shifted `p.v += 3` (unpassed row 0–2 → passed row 3–5) (`AdjustLevelID`/`SwapPlayerLevels`).
- **Entering a level**: standing on a level piece sets `levelNumber = theWorld[x][y].levelID`; after `kInfoDelayTicks = 15` ticks an info box appears using `GetLevelInfo(levelNumber)` = `'MRlv'` resource **300 + levelNumber**; Return/Enter (or the Play button) launches it. A player may not move off an unpassed level except back the way they came.
- Level pieces present in `World.lvl` (kind==2 with levelID): 25 distinct level IDs {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,18,22,23,24,25,26,27,28,30}; additional kind==0 cells carry levelIDs {4,5,15,16,20,21,22,23,24,26,30} (scenery associated with a level, e.g. its "passed" path). IDs 17, 19 and 29 never appear; 448 cells total have kind==2 (most are decorative uses of the kind field).
- Animated world tiles (tile-sheet IDs, from `MRMap.c` constants): ocean 238–241 (cycling, 4 phases at slightly different tick offsets: `kOceanStartTile=238` … `kOceanEndTile=241`), level markers animate 1–8, 19–26, 37–44, 55–62, 73–80, 91–98 (`kLev1InitTile`–`kLev6EndTile` — the six level-marker slots at columns 1–8 of rows 0–5), whirlpool 256–259, "things" 168–170 and 186–188, machine 288–293. World map scroll speed `kSpriteMoveDelta = 4` px/frame, max 30 fps.

---

## 8. The shipped levels (26 playable + World)

All data extracted from the `.lvl` headers plus a border-scan of the grids (original `GetLevelSize` algorithm). "Size" = actual playable rectangle including the border ring; "Murphy @" = grid coordinates of tile 9 (0-based, x,y from top-left); "on board" = count of infotron tiles (ID 19) actually present (can be less than the quota when infotrons must be produced by exploding enemies/yellow disks, or more than needed).

| Internal name (.mbl name field) | File base name | World / graphics set | Quota | Time limit | Size (w×h) | Murphy @ | Infotrons on board | Hint |
|---|---|---|---|---|---|---|---|---|
| Achtung! | Achtung! | Aquatron | 12 | 0:04:00 | 38×30 | (2,2) | 12 | Robots verboten. |
| And So It Begins... | And So It Begins | Savanatron | 10 | 0:01:30 | 55×43 | (2,21) | 10 | Take heed |
| Bolder | Bolder | Infotron | 20 | 0:08:00 | 31×29 | (15,14) | 20 | Assert yourself, but don't get crushed! |
| Catacombs | Catacombs-Aquatron | Aquatron | 85 | — | 43×50 | (3,17) | 78 | Each small puzzle is part of a larger enigma. |
| Combinations | Combinations | Infotron | 1 | — | 23×31 | (11,4) | 6 | Pay attention |
| Crystalline | Crystalline | Aquatron | 22 | — | 25×19 (25×24 in shipped .mlv — bug, §2) | (22,1) | 22 | T3 |
| Dash Dash | Dash Dash | Infotron | 40 | — | 33×31 | (16,15) | 40 | These ain't potato chips, no sir. |
| Daylight | Daylight | Infotron | 20 | 0:15:00 | 37×39 | (18,7) | 2 | Don't ignite the yellow disks until 2-3, 5-6! |
| Dot Dot | Dot Dot | Infotron | 90 | 0:20:00 | 49×29 | (2,14) | 90 | Every little bit counts. |
| Excavation | Excavation | Infotron | 1 | — | 41×30 | (27,2) | 1 | One wrong move, and that Infotron is buried forever. |
| Falling | Falling | Infotron | 20 | — | 19×110 | (1,2) | 20 | You can't defy gravity, punk. |
| Going Up | Going Up | Infotron | 45 | — | 55×31 | (27,15) | 29 | Careful, it's not a straight shot out! Not everything goes down, you know... |
| Gold Rush | Gold Rush | Savanatron | 44 | — | 32×23 | (1,1) | 44 | Push 'em at the right time, and the Infotrons will come. |
| Golden Rule | Golden Rule | Infotron | 45 | 0:15:00 | 37×36 | (1,1) | 45 | Golden shells pack a punch - but which ones do you push first? |
| Inflamatory | Inflamatory | Infotron | 15 | 0:10:00 | 35×31 | (17,15) | 15 | Have a blast! - but don't get too close... |
| It's Alive!! | It's Alive!! | Infotron | 30 | — | 23×36 | (9,10) | 21 | All evil contains some good |
| Labyrinth | Labyrinth | Savanatron | 72 | — | 39×35 | (20,19) | 72 | I'm dreaming of France |
| Love Boat | Love Boat | Infotron | 45 | — | 37×31 | (18,15) | 45 | We've been waiting for you... |
| No More Secrets | No More Secrets | Infotron | 39 | — | 99×99 | (49,49) | 3 | The secret is speed. |
| Out In Out | Out In Out | Infotron | 24 | — | 35×31 | (1,6) | 24 | Death is (still) right around the corner. |
| Robots & Plasmoids | Robots & Plasmoids | Infotron | 20 | 0:08:00 | 33×29 | (16,14) | 2 | These little critters can be a pain in the patookus, so blow 'em up! |
| Short Circuit | Short Circuit | Infotron | 1 | 0:00:35 | 24×17 | (1,14) | 1 | Nonstop explosions for the violence within you... |
| Stress | Stress | Aquatron | 20 | 0:02:30 | 26×21 | (1,3) | 11 | Hurry the !@#^ up! |
| Thriller | Thriller | Savanatron | 53 | — | 39×31 | (20,22) | 36 | An exit is far more important than an infotron. |
| Trapped Inside | Trapped Inside | Aquatron | 10 | — | 43×22 | (21,16) | 10 | They're stuck, but you can get 'em out if you think about it. |
| Wizard | Wizard | Infotron | 17 | — | 37×31 | (18,15) | 17 | (empty hint) |
| Work With World Worldview | World | (map room; graphics field says "Infotron") | 0 | — | 120×120 | n/a (start marker at (58,54)) | n/a | (empty) |

- World IDs (numeric, original): 0=Infotron, 1=Aquatron, 2=Savanatron, 3=**Deathtron** (`kInfotronWorld…kDeathtronWorld` in `ld/MRMap.p`; `kInfotronTerrain…kDeathtronTerrain` in `MRPlayConstants.h`; `MRSetTerrainAlias(levelWorld)` maps 1→Aquatron res file, 2→Savanatron, 3→Deathtron, default→Infotron). **No shipped level uses Deathtron** and no deathtron tile sheet exists in the modern ports (only `infotron.png/atlas`, `aquatron.png/atlas`, `savanatron.png/atlas`, `world.png`).
- Distribution: 18 Infotron, 5 Aquatron, 3 Savanatron.
- **Play order**: the iPhone port hardcodes alphabetical order (`iPhone/MurphyView.m` `levelNames`): Achtung!, And So It Begins, Bolder, Catacombs-Aquatron, Combinations, Crystalline, Dash Dash, Daylight, Dot Dot, Excavation, Falling, Going Up, Gold Rush, Golden Rule, Inflamatory, It's Alive!!, Labyrinth, Love Boat, No More Secrets, Out In Out, Robots & Plasmoids, Short Circuit, Stress, Thriller, Trapped Inside, Wizard (26 entries; World excluded). The *original* order was nonlinear via the map room's `levelID`s; the mapping levelID→level name **has been fully recovered** from `MR-Dev2.cpt` — see "## Follow-up" at the end of this file.
- All levels have kind=0, trig=0, state=0, levelID=0 on every cell — the keycoded/gravity-switch/hardware-phase cell kinds were designed but never used in shipped content.
- Cells in each `.lvl`/`.mbl` outside the actual w×h rectangle are junk (mostly zeros = tile 0) and one stray tile-37 in Crystalline; always crop before use.

---

## 9. Quirks, bugs, and gotchas checklist

1. **No magic/version in `.mbl`/`.mlv`.** Distinguish them only by whether width/height are 120×120 vs. actual.
2. **Big-endian everywhere** (Mac heritage): u16 header fields in `.mbl`/`.mlv`, int16 fields/streams in resources.
3. **Grid cell = (iconX, iconY) byte pair, X first**; the classic resource stores Y (v) first within `Point` — the `.lvl` dump already swapped to X-first.
4. **tileID = iconY·10 + iconX for game levels but iconY·18 + iconX for the World map.** Applying the 10-wide formula to World data (as `fix_mbl.py` and `MurphyLevel.swift` do) produces garbage.
5. **fix_mbl.py picks the LAST tile-37**: correct for 25 levels, wrong for Crystalline (25×24 instead of 25×19) and catastrophically wrong for World (61×66). The original `GetLevelSize` (top-row scan for tile 32, then down-column scan for tile 37) is the correct algorithm.
6. **`.mbl`/`.mlv` drop data**: hint, time limit (H:M:S), numeric world ID, per-cell trigger/trigP/state/kind/levelID. The graphics-set *name* (lowercased) is the only world indicator that survives. Time-limited levels (Achtung!, And So It Begins..., Bolder, Daylight, Dot Dot, Golden Rule, Inflamatory, Robots & Plasmoids, Short Circuit, Stress) lose their timers in `.mbl` — a faithful port should take timers from the `.lvl` files.
7. **Quota ≠ board count**: `infotrons` in the header is the win quota; the board may contain fewer (enemies/disks yield extras when exploded — e.g. Daylight: quota 20, only 2 on board) or more (Combinations: quota 1, 6 on board).
8. **Internal name ≠ file name** for two levels: file `And So It Begins.(lvl|mbl)` has internal name `And So It Begins...`; file `Catacombs-Aquatron` has internal name `Catacombs`.
9. **reqInfo < 0 sentinel** switches `'MRlv'` header from 522 to 536 bytes; the real quota is then `actualInfo`.
10. `MurphyLevel.swift` fails the entire load if any cell's `y*10+x` is outside 0–213 (`LevelTile(rawValue:)` returns nil → `fromData` returns nil), and requires byte length to match exactly; `MurphyLevel.m` performs no such validation.
11. Player start is not a header field anywhere; it is tile 9 (game levels) / tile-18 marker with trigP replacement (world map).
12. `RLECompress` in `ld/MRMap.p` reads `cFrom^[counter]` after its loop ends (out-of-range index) when flushing the final run — harmless in practice (the value is discarded), but don't replicate.
13. Shipped `.mbl` files are byte-identical to `assemble-binary-level.py(.lvl)` output (verified for Bolder, World, Achtung!, Wizard; sizes match for all 27).
14. `.lvl` header line 3 is capitalized ("Infotron"); `.mbl`/`.mlv` graphics-set field is lowercase ("infotron") because of `.strip().lower()` in the assembler.

---

## 10. Open questions / not answerable from this repo

- ~~**levelID → level-name mapping**~~ **RESOLVED** — the `:MR Data:World` resource file is inside `legacy/murphy/MR-Dev2.cpt`; all 30 `'MRlv'` resources (301–330) were extracted and the complete mapping is documented in "## Follow-up" at the end of this file. (Level 1 is "And So It Begins...", not Bolder.)
- The exporter that produced the `.lvl` text dumps from resource forks is not in the repo (older experimental dumps exist in `legacy/murphy/sdl-test/World.txt`, `wpw.txt`, `fw.txt` with a different `field: value` format including `USESEXTENDEDDATA`, `OPENLEVEL`, `WINDOWKIND` fields).
- Exact world tile-sheet layout beyond 18 wide × 378 tiles (the original PICT 148 isn't in the repo as an editable image; `iPhone/world.png` is 512×512 = 16×16 tiles of 32 px, which matches neither 18-wide nor 24-px — its intended indexing is undocumented and it is unused by code).
- Whether `state`/`trigP` semantics for map *triggers* ("for trigger, what kind it is") have additional encoded meaning beyond icon swapping — only the swap behavior is implemented in `MRMap.c`.
- Deathtron: constants and a resource file reference exist, but no levels use it. **Update:** the Deathtron resource *file* itself (1,315,064-byte resource fork, presumably tile/graphics art) exists inside `MR-Dev2.cpt` (`Murphy's Revenge/MR Data/Deathtron`) — see "## Follow-up". Still no shipped level references world 3.
- The older Infotron-era formats (`desBoard.p`: `'l¿Vl'`-type `CompressedBoard`/`LevelRecord` with 150×150 boards, murphy/term start fields, scissors/quarks/bugs arrays) belong to the *prequel* game and are not used by any Murphy's Revenge data in this repo.

---

## Follow-up: levelID → level mapping — RECOVERED (definitive, not a guess)

**Question:** Which level does each of the 25 world-map level nodes launch? (§10 claimed the `'MRlv'` 301+ resources binding IDs to named levels were lost.)

**Answer: the resources are NOT lost.** `legacy/murphy/MR-Dev2.cpt` (and `.sea`; both 6,728,129 bytes, Compact Pro archives of the shipped game folder) contains `Murphy's Revenge/MR Data/World` — the original World resource file (resource fork 373,023 bytes, mod date 1998-07-05) with **all 30 `'MRlv'` resources (IDs 301–330)** and the `'MRwd'` 128 world map (230,400 bytes). Every levelID is therefore bound to a named level with byte-exact certainty.

### Extraction procedure (verified)
```
brew install unar
unar -k hidden MR-Dev2.cpt        # Compact Pro; resource forks land in AppleDouble "._World" etc.
# AppleDouble: magic 0x00051607 at offset 0; entry ID 2 = resource fork.
# Then standard Mac resource-fork parsing (data offset / map offset u32 BE at 0/4).
```
Archive catalog (u32 BE at file offset 4 → catalog at 6,727,489): `MR Data/World` entry has data offset 6,462,514, cptflags=2 (rsrc fork LZH), rsrc 373,023 uncompressed / 37,851 compressed. The archive also contains **all other original resource files** — `Aquatron`, `Deathtron` (1,315,064 bytes — Deathtron tile art DOES exist, resolving another §10 open question), `Infotron`, `Interface`, `Players`, `Deep Players`, `Savanatron` — and the game binary (`APPL`, creator `MrVG`).

### The definitive mapping (all 30 `'MRlv'` resources)

Node coordinates are 0-based (x=col, y=row) in `World.lvl`/`'MRwd'`; "node" means a cell whose icon is tile-sheet column 1, rows 0–2 (the only launchable piece kind, `MakePieceKinds`/`PieceKind` in `MRMap.c`). Quota/time/world are from the resources; ✓ = board grid byte-identical to the repo's `.lvl` dump.

| levelID | 'MRlv' | Level (resource title) | World | Quota | Time | Map node (x,y) | Access |
|---|---|---|---|---|---|---|---|
| 1 | 301 | And So It Begins... | Savanatron | 10 | 0:01:30 | (61,50) | visible ✓ |
| 2 | 302 | Bolder | Infotron | 20 | 0:08:00 | (84,52) | visible ✓ |
| 3 | 303 | Achtung! | Aquatron | 12 | 0:04:00 | (95,55) | visible ✓ |
| 4 | 304 | Inflamatory | Infotron | 15 | 0:10:00 | (86,54) | visible ✓ |
| 5 | 305 | Robots & Plasmoids | Infotron | 20 | 0:08:00 | (86,50) | visible ✓ |
| 6 | 306 | Excavation | Infotron | 1 | — | (98,67) | visible ✓ |
| 7 | 307 | Thriller | Savanatron | 53 | — | (101,22) | visible ✓ |
| 8 | 308 | Golden Rule | Infotron | 45 | 0:15:00 | (88,14) | visible ✓ |
| 9 | 309 | Going Up | Infotron | 45 | — | (104,66) | visible ✓ |
| 10 | 310 | Gold Rush | Savanatron | 44 | — | (106,46) | visible ✓ |
| 11 | 311 | Daylight | Infotron | 20 | 0:15:00 | (91,15) | **hidden** — revealed by switch trig 9 at (85,14) ✓ |
| 12 | 312 | Stress | Aquatron | 20 | 0:02:30 | (102,70) | visible ✓ |
| 13 | 313 | Catacombs | Infotron→**Aquatron** | 85 | — | (81,27) | visible (grid differs — later edit; `.lvl` is newer/authoritative) |
| 14 | 314 | Trapped Inside | Aquatron | 10 | — | (100,63) | visible ✓ |
| 15 | 315 | It's Alive!! | Infotron | 30 | — | (84,25) | visible ✓ |
| 16 | 316 | Falling | Infotron | 20 | — | (98,52) | visible ✓ (has trig=2; trigP is the same marker icon — no-op toggle) |
| 17 | 317 | Insane Infotrons™ | Infotron | 121 | — | — | **cut**: was the hidden node at (69,27) in the 1998 archive; replaced by 25 in final data |
| 18 | 318 | Crystalline | Aquatron | 22 | — | (95,49) | visible ✓ |
| 19 | 319 | test level | Infotron | 1 | — | — | never on map (laser test level) |
| 20 | 320 | Short Circuit | Infotron | 1 | 0:00:35 | — | **vestigial**: lid=20 sits on a path tile at (60,56) — path tiles never launch; unreachable as a level in the original ✓ |
| 21 | 321 | Combinations | Infotron | 1 | — | — | **vestigial**: lid=21 on a no-pass tile at (92,25); never launchable ✓ |
| 22 | 322 | Dot Dot | Infotron | 90 | 0:20:00 | (84,23) | visible (grid differs — later edit; `.lvl` authoritative) |
| 23 | 323 | Dash Dash | Infotron | 40 | — | (84,31) | visible ✓ |
| 24 | 324 | Love Boat | Infotron | 45 | — | (84,29) | visible ✓ |
| 25 | 325 | Wizard | Infotron | 17 | — | (69,27) | **hidden** — revealed by switch trig 13 at (82,25) ✓ |
| 26 | 326 | Out In Out | Infotron | 24 | — | (84,33) | visible ✓ |
| 27 | 327 | **In Out In** | Aquatron | 24 | — | (103,18) | visible — **level data missing from the 26 `.lvl` dumps; recovered from the archive** |
| 28 | 328 | Labyrinth | Savanatron | 72 | — | (103,14) | visible ✓ |
| 29 | 329 | (untitled) | Infotron | 0 | — | — | empty placeholder board (all cells 0) |
| 30 | 330 | No More Secrets | Infotron | 39 | — | (84,21) | visible ✓ |

25 launchable nodes total = 23 always-visible + 2 switch-revealed (11 Daylight, 25 Wizard). This exactly accounts for `World.lvl`'s levelID set {1–16, 18, 22–28, 30}.

### Verification performed
- 23 of the 26 dumped game `.lvl` boards are **byte-identical** to the archive's decompressed RLE streams (title, quota, time, world, and full 120×120 icon grid). Catacombs and Dot Dot differ (post-archive edits; treat repo `.lvl` as final). Catacombs' world changed Infotron (1998 resource) → Aquatron (final `.lvl`).
- Final `World.lvl` vs archived `'MRwd'` 128: **exactly one cell differs** — (69,27): archived levelID 17 (Insane Infotrons™) vs final 25 (Wizard). The author swapped the hidden level before release; everything else (all node positions, triggers, icons) is identical, so the archived map is trustworthy for the final layout.
- Grid unflattening for `'MRlv'` streams confirmed **row-major** (i = y·120 + x), same as `.lvl` order.
- `'MRlv'` parsing wrinkle: old-format resources (reqInfo ≥ 0, 522-byte header) carry **14 bytes of trailing junk after the kind stream** (= sizeof extended-header delta). Parsers must not require exact-length consumption.

### Recovered lost content (exported in the exact `.lvl` dialect; writer validated byte-identical against shipped `Bolder.lvl`)
- `…/scratchpad/recovered/In Out In.lvl` — Aquatron, quota 24, size 35×31, Murphy @ (17,15), 24 infotrons, hint "Death is right around the corner." **Required for a faithful overworld** (node 27 launches it).
- `…/scratchpad/recovered/Insane Infotrons (tm).lvl` — Infotron, quota 121, 19×15, Murphy @ (1,13), 121 infotrons, hint "(Just Do It™)". Cut content.
- `…/scratchpad/recovered/test level.lvl` — Infotron, quota 1, 41×29, Murphy @ (2,2), hint "Power Up, aim those optics, and fire. Oh, and stand out of the way..." Dev test level.

### Related fixes this recovery enables
- **Hint-truncation bug in the `.lvl` dumps**: the dump tool cut three hints at the first colon. Originals: And So It Begins... = "Take heed: the puzzles have only just begun..."; Crystalline = "T3: (try trance techno)"; Combinations = "Pay attention: the correct drops will get you out." A faithful port should use the full strings.
- **lid=0 marker nodes** (14 visible + 2 switch-revealed, clustered x≈46–75/y≈15–44, NW of the start): unfinished second area. Original behavior when stood on: info box shows "Nonexistant Level" / "This level doesn't exist yet." (`GetLevelInfo` else-branch, `MRMap.c` ~line 1238) and launching runs `NoSuchLevel()` → `SysBeep(5); FadeOutImmediate(); return kBackToMapReturn;` (`MRPlay.c` line 1332). A port can reproduce exactly this (it is shipped behavior, not a crash).
- **Short Circuit (20) and Combinations (21) are not reachable from the original overworld** (vestigial levelIDs on non-launchable tiles). *Port decision:* keep them off the map for fidelity, or surface them as bonus levels (the iPhone port's alphabetical list included both). The alphabetical substitute ordering is now unnecessary for the overworld itself.

---

## Follow-up: web-port JSON level format, its generator, full verification, and the World-map conversion

### F.1 The generator exists — `web/scripts/build-assets.mjs` (not in tools/, not in git history)

The converter that produced `web/public/assets/levels/*.json` is **`web/scripts/build-assets.mjs`**, invoked as `pnpm assets` (`web/package.json`: `"assets": "node scripts/build-assets.mjs"`; pass `levels` as argv[2] to rebuild only levels). It is **untracked**: the entire `web/` tree is untracked on branch `dave/agentic-web` (`git status` → `?? web/`; `git ls-files web` is empty), so these assets are *not* committed and the generator has no git history. The historical converters that *are* in git history (`scripts/mlv_to_json.py` @ ebc7f7b, `scripts/mr_to_tiled.py` @ 1f5f470, `scripts/bcp_to_json.py` @ f020f68, `levels/supaplex/converters/*` @ 5aa8663/ea0a974) all emit different, abandoned formats and are unrelated to the current JSON.

Generator algorithm (constants from the file, lines 66–70):

```js
const FULL = 120;                 // declared grid size in every .lvl
const TOP_LEFT_BORDER = 30;
const TOP_RIGHT_BORDER = 32;
const BOTTOM_RIGHT_BORDER = 37;
const OLIVER = 9;
```

- `parseLvl` reads the `.lvl` as **latin1**, splits on `\n`, `trim()`s each line; header = lines 0–6 (name, hint, quota, world `.toLowerCase()`, hours, mins, secs); grid cells start at **line 9**, 8 lines per cell, row-major; only fields +0/+1 are used: `grid[i] = iconY * 10 + iconX`.
- `levelSize` reimplements the original `GetLevelSize` (§5): if `grid[0] !== 30` → 120×120; else scan row 0 for tile 32 (`width = x+1`), then down column `width-1` for tile 37 (`height = y+1`). This gives the *correct* Crystalline 25×19 (not fix_mbl's 25×24).
- Crops to `width×height`, records `start = [x, y]` of tile 9 (last occurrence wins; throws `no player start (tile 9)` if absent), and **explicitly skips World**: `if (base === "World") continue; // the map room, not a playable level`.
- `timed = hours > 0 || mins > 0 || secs > 0`; `time` is `{hours, mins, secs}` or `null`.

### F.2 Per-level JSON schema (`web/public/assets/levels/<base>.json`, minified)

```
{
  "name":   string,                       // .lvl line 0 (internal name, e.g. "And So It Begins...")
  "hint":   string,                       // .lvl line 1; "" for Wizard
  "quota":  int,                          // .lvl line 2 (win quota, not board count)
  "world":  "infotron"|"aquatron"|"savanatron",   // .lvl line 3 lowercased
  "time":   null | {"hours":int,"mins":int,"secs":int},  // null iff 0:0:0
  "width":  int, "height": int,           // actual cropped size (GetLevelSize)
  "start":  [x, y],                       // 0-based coords of tile 9 within the cropped grid
  "grid":   int[width*height]             // tile IDs = iconY*10 + iconX, row-major,
}                                         //   top-left origin; still contains the tile-9 cell
```

Index `web/public/assets/levels/levels.json` (pretty-printed, 26 entries, sorted by file base name): array of `{ "file", "name", "world", "quota", "timed" }` where `file` is the basename — fetch `assets/levels/<file>.json` (needs `encodeURIComponent`: "Robots & Plasmoids", "It's Alive!!", "Achtung!"). `file` ≠ `name` for the two known cases (§9.8).

There are deliberately **no trig/trigP/state/kind/levelID fields** — verified lossless for game levels: all 26×14,400 cells have every non-icon field == 0 (re-checked cell-by-cell, see F.3). What the JSON drops vs `.lvl` is only the junk outside the crop and the constant declared 120×120.

### F.3 Verification: all 26 JSON files match `.lvl` ground truth exactly

An independent re-parse of every `.lvl` per §3 + original `GetLevelSize` (§5), diffed field-by-field against each JSON (script: scratchpad `verify_levels.py`), passes with **zero mismatches** across all 26 levels: name, hint, quota, world, time (null vs {h,m,s}), width, height, start, **every grid cell**, index entry, and exact JSON key set. Additional facts established: each level has **exactly one** tile-9 cell; every `.lvl` declares 120×120; grid tile IDs span 0–69 (max 69, in Wizard; 1–57 in Out In Out — the only level whose cropped grid contains no tile 0); all 10 timed levels carry their `.lvl` timers (Achtung! 0:4:0, And So It Begins... 0:1:30, Bolder 0:8:0, Daylight 0:15:0, Dot Dot 0:20:0, Golden Rule 0:15:0, Inflamatory 0:10:0, Robots & Plasmoids 0:8:0, Short Circuit 0:0:35, Stress 0:2:30); dimensions/starts match the §8 table exactly (Crystalline 25×19 start [22,1], Falling 19×110, No More Secrets 99×99 start [49,49]).

**Stale consumer**: `web/src/assets.ts` predates these assets — line 46 types `levels.json` as `string[]` (it is an object array) and `level()` (line 76) fetches `${BASE}/levels/<name>.mbl`, which does not exist in `web/public/assets/levels/` (only `.json`). Must be updated to `loadJSON` of `<file>.json`.

### F.4 World-map conversion — **DECISION (recommended design; nothing exists in the web port today)**

No `World.json` exists and `buildLevels()` skips `World.lvl` by name. The facts below are verified from `World.lvl` + `os7code/mr/MRMap.c`; the schema itself is a **port decision, not original fact**.

`World.lvl` ground truth (all re-verified): header `["Work With World Worldview", "", "0", "Infotron", "0", "0", "0", "120", "120"]`; cell (x,y) field k lives at text line `9 + (y*120 + x)*8 + k`, k = 0..7 (115,209 data lines + trailing blank). Field stats over the 14,400 cells: iconX 0–17, iconY 0–19 → world tile ID (`iconY*18 + iconX`) 0–350; `state` is 0 **everywhere**; `kind` ∈ {0: 13,928, 1: 24, 2: 448}; `trig` nonzero on 303 cells, IDs {1,2,3,4,5,6,7,9,11,13,16,17,18,20,21,55}; `trigP` nonzero on 309 cells; `levelID` nonzero uses {1–16, 18, 20–28, 30}; exactly **25** cells have kind==2 && levelID>0. Corner tile is 219 (not 30) — **`GetLevelSize`/cropping must NOT be applied**; the map is the full 120×120.

Recommended `web/public/assets/world/map.json` (struct-of-arrays; both Points collapsed to single 18-wide tile IDs, lossless because iconX ≤ 17):

```
{
  "width": 120, "height": 120,
  "tilesWide": 18,                  // tile ID = iconY*18 + iconX (NOT 10!)
  "start": [58, 54],                // the unique tile-18 cell (kOliverStartTile)
  "tiles":     int[14400],          // p:      iconY*18 + iconX
  "trig":      int[14400],          // trigger ID, 0 = none
  "trigTiles": int[14400],          // trigP:  trigY*18 + trigX
  "kind":      int[14400],          // 0 normal, 1 trigger, 2 level (editor metadata — see below)
  "levelID":   int[14400]           // original 'MRlv' 300+n reference, 0 = none
}
```

All 8 `.lvl` per-cell fields survive: iconX/iconY → `tiles`; trigX/trigY → `trigTiles`; `trig`/`kind`/`levelID` verbatim; `state` may be omitted (all-zero in shipped data, semantics never implemented — record `"stateAllZero": true` or just cite this section) or included as a sixth array for strict 8-field fidelity. Generator change: add a `buildWorld()` to `build-assets.mjs` that parses all 8 fields (`lines[line+k]`, k=0..7), uses `*18` not `*10`, does not crop, and locates `start` by tile 18 instead of tile 9.

Runtime semantics the consumer must honor (verified in `MRMap.c`):

- **Passability is icon-position-only, `kind` is dead at runtime**: `MakePieceKinds`/`PieceKind` classify by (iconX, iconY) per §7; verified that all **17** functional switch cells (iconY==6, iconX 0–8) have `kind==0`, and no runtime map code reads `kind` — the 24 `kind==1` cells are level-designer metadata. Preserve `kind` anyway (cheap, and it disambiguates decorative from live `levelID`s).
- **`TriggerSwitch(h,v)`** (MRMap.c line ~821): `trigID = theWorld[h][v].trig;` then for every cell with `trig == trigID`, swap `p ↔ trigP`; afterwards, if the new `p.v < 3 && p.h > 0 && levelID > 0` and the player has `passedLevel[levelID-1]`, do `p.v += 3` (re-mark as passed). Fires only when stepping onto a `kSwitchPiece` (`PlaySoundID(130)` then `TriggerSwitch`).
- **Landmine**: the switch cell at **(43,30)** has `trig == 0` (`[3,6, 0, 0,0, 0, 0, 0]`); stepping on it would swap p↔trigP on all **14,097** `trig==0` cells (mostly to tile 0), destroying the map. The port should no-op switches whose `trig == 0` — mark this as a port decision (the original code has no such guard).
- **Hidden levels 11 and 25** — the reason `trigP` must survive: their markers exist only in `trigP`: cell (91,15) = `[6,18, trig=9, trigP=(1,0), 0, kind=2, levelID=11]` and cell (69,27) = `[7,19, trig=13, trigP=(1,1), 0, kind=2, levelID=25]`. Only 23 of the 25 level markers are visible in the initial `tiles` layer; switches 9 and 13 reveal the other two.
- **Start marker**: cell (58,54) = `[0,1, 0, 4,7, 0, 2, 0]` — tile 18; draw its `trigP` tile 130 (7·18+4) in its place, spawn the player sprite there.
- **levelID → playable-level mapping remains unknown** (§10): converting World.lvl is lossless, but wiring markers to the 26 named JSON levels requires inventing a levelID→file table — a port decision with no ground truth in this repo.

---

## Follow-up: off-by-one correction for tile IDs ≥ 166 (verified against source, 2026-07-13)

An earlier revision of this document shifted every tile ID ≥ 166 down by one and inflated the Oliver range by one, contradicting `graphics.md` §1.2 and `game-rules.md` (both of which were already correct). The table in §4, the substitution list in §1, and the `MurphyLevel.swift` bound in §2/§9 have been regenerated from the actual enums. Verified by counting enumerator positions in all three sources, which declare the identical member list:

- `legacy/murphy/os7code/mr/MRPlayConstants.h` — `enum TileTypes` (CR line endings; the in-file `// 170` comment on `kZapInfotronTile24` confirms the count).
- `iPhone/MurphyConstants.h` — `enum LevelTileIds` (same list, same `// 170` comment) and `enum OliverTileIds` (`kBubFrame = 220`).
- `osx/Murphy/MurphyTiles.swift` — `enum LevelTile : Int` (`case CircuitTile = 0` … `case UpLeftScissorTile`, exactly 214 cases) and `enum OliverTile : Int` (`case BubFrame = 220`, 85 cases).

Correct values (ID = iconY·10 + iconX; (X,Y) = (ID mod 10, ID div 10)):

| Range | (X,Y) | Group |
|---|---|---|
| 166–178 | (6,16)–(8,17) | kFirstZapInfotronTile … kLastZapInfotronTile — **13** frames (First + ZapInfotronTile21…31 + Last), not 12 |
| 179–180 | (9,17)–(0,18) | kFirstRDBlinkTile, kLastRDBlinkTile |
| 181–184 | (1,18)–(4,18) | kFirstZonkTile … kLastZonkTile |
| 185–188 | (5,18)–(8,18) | kFirstUpZonkTile … kLastUpZonkTile |
| 189–197 | (9,18)–(7,19) | kFirstQuarkTile … kLastQuarkTile (9 frames) |
| 198–213 | (8,19)–(3,21) | kFirstUpScissorTile … kUpLeftScissorTile (exactly 16 names; the old "197–216" row spanned 20 IDs for 16 names — internally inconsistent) |
| 214 | — | `kMaxLevelTiles` — valid level tile IDs are **0–213** |

- Load-time substitutions in `iPhone/MurphyView.m loadCurrentLevel` (quoted): `kQuarkTile` (20) → `kFirstQuarkTile` = **189**; `kUpScissorTile..kLeftScissorTile` (21–24) → `kFirstUpScissorTile` = **198** (comment: `/* kUpLeftScissorTile is last */`); also 19→94, 7→110, 9→220 (unchanged, they were already right).
- Oliver/player frames: **220–304** (85 frames). `iPhone/MurphyConstants.h`: `kBubFrame = 220`, `kExLaxBubLastFrame` = 304, `kMaxBubFrames` = **305**. `osx/Murphy/MurphyTiles.swift`: `OliverTile.textureName()` indexes `OliverTileTextureNames[self.rawValue - 220]` into an 85-entry array. Classic `MRPlayConstants.h` `enum OliverFrameIDs` is the same 85-name list 0-based (`kMaxBubFrames` = 85 there).
- `osx/Murphy/MurphyLevel.swift` validation: there is no explicit numeric bound in the file; the bound is implicit — `LevelTile(rawValue: tileMapRaw)` (line ~94) returns nil for any raw value outside **0–213**, which makes `fromData` return nil (whole level rejected).
- Sub-range animation ranges used at runtime (`MurphyView.m` `animateTileId:` calls) with correct IDs: infotron 94→109, quark 189→197, terminal 110→121, scissors 198→213, Oliver idle 220→228.

Cross-checks: `graphics.md` §1.2 lists these exact IDs and the extractor comments (`plasmoid.bmp → IDs 189-197`, `robot.bmp → IDs 198-219 (16 real + 6 filler)`, `oliver.bmp → IDs 220-309 (85 real + 5 filler)`); `game-rules.md` uses 166–178 (zap-infotron, non-explodable; quark explosion runs 178 down to 166 igniting at 171 = kZapInfotronTile25), 179/180 (RD blink), 181–184/185–188 (zonk rolls), 189–197 (quark), 214 (count). All consistent with this correction.

---

## Follow-up: World.lvl histogram vs the MakePieceKinds passability table — is the map playable?

**Question:** Is the shipped `World.lvl` actually playable under the dev-prototype `MakePieceKinds` table (sheet col 1 rows 0–5 = level nodes, row 6 cols 0–8 = switches, rows 7–8 cols 0–8 = passable, all else NoPass)? Histogram every icon (p.h, p.v) pair actually used and check nothing is misclassified.

**Verdict: YES — the map is fully consistent with the active table, and the "placeholder" flag is wrong.** The active table in `MakePieceKinds` (`legacy/murphy/os7code/mr/MRMap.c`, ~lines 1139–1206 after CR→LF conversion) is the *shipped* mapping for the shipped 378-tile 18-wide sheet, not a dev stub. This **corrects app-flow.md §12** ("final shipped mapping is unknown from this code"). Three independent lines of evidence, all from the same file:

1. **The animation constants decode the sheet and agree exactly with the table.** `kLev1InitTile 1`/`kLev2InitTile 19`/`kLev3InitTile 37` = icons (1,0),(1,1),(1,2) — the three *unpassed* level-node slots (`kLevelPiece` cells of the table); `kLev4InitTile 55`/`kLev5InitTile 73`/`kLev6InitTile 91` = (1,3),(1,4),(1,5) — the *passed* variants (`kPassedLevelPiece` cells); the six anim runs 1–8, 19–26, 37–44, 55–62, 73–80, 91–98 fill cols 1–8 of rows 0–5 (cols 2–8 are animation frames only, never map data). `AdjustLevelID` does `p.v += 3` — mapping tile 1→55, 19→73, 37→91, i.e. unpassed row → its passed row three rows down. Ocean (0 + 238–241), whirl (256–259), "things" (168–170, 186–188), machine (288–293) are all in NoPass sheet regions.
2. **Histogram of all 14,400 cells of `World.lvl` (120×120, 137 distinct icon pairs):** class totals under the table are **575 kPassablePiece, 17 kSwitchPiece, 37 kLevelPiece, 0 kPassedLevelPiece, 13,771 kNoPassPiece** (sums to 14,400). Every non-NoPass icon is exactly what it should be:
   - Passable: only walkway icons — row 7: (0,7)×140, (4,7)×229, (5,7)×29, (6,7)×40, (7,7)×2, (8,7)×132; row 8: (0,8), (1,8), (2,8) ×1 each. Nothing else lands in rows 7–8 cols 0–8.
   - Switch: (0,6)×9, (3,6)×6, (6,6)×2. Trigger pairs confirm on/off frames live side by side in row 6: (0,6)↔(1,6), (3,6)↔(4,6), (6,6)↔(7,6).
   - Level: (1,0)×14, (1,1)×9, (1,2)×14 — only column 1, rows 0–2, i.e. exactly the three `kLev*InitTile` slots. No cell ever stores a passed-variant icon (rows 3–5) — those appear only at runtime via `AdjustLevelID`/`TriggerSwitch`.
   - No icon is out of sheet range: max used tileID is 350 = (8,19); `PointFromTile` (fatal for v ≥ `kTilesDeep` 20, i.e. tileID ≥ 360) can never fire on this data.
   - **Nothing intended as a wall is accidentally passable**: row 6/7/8 icons with h > 8 — (9..15,6), (14,8)×3, (10,8)×1 — classify NoPass, and the trigger data shows they are "gap/broken" flip-target states (e.g. cell (60,57): p=(15,6) trig 18 trigP=(7,7) — a bridge gap that a switch repairs), so the `kUsableTilesWide = 9` bound is intentional.
   - All trigger *targets* (trigP) also classify sensibly: walkways (0..8,7), switch on/off partners, three hidden level icons, and NoPass scenery — flips only ever move cells between classified states.
3. **The big commented-out table (same function, `/* ... */`, lines ~1164–1205) is for an older sheet layout** (levels at rows 0–3 cols 3–4, switches rows 0–3 cols 6–8, walkway row 4 cols 0–5, plus `[6][2]` and `[11][1]`). Under it, every level node ((1,0),(1,1),(1,2)) and every walkway icon used by `World.lvl` would be **NoPass** — the game would be frozen at the start tile. Do **not** use it; it predates the shipped sheet.

**Playability walk-check** (BFS from the unique start marker, icon (0,1)=tile 18 at grid (58,54), start cell replaced by its trigP (4,7) at load per `SetUpTileData`):
- Initial state (no switches thrown): a 32-cell island; only **level 1** (node at (61,50)) is enterable — matches "first level" design.
- Progressive simulation (BFS; throw every reachable switch; repeat to fixpoint): **15 of 16 trigger groups** fire (1–7, 9, 11, 13, 16, 17, 20, 21, 55) and **all 25 level nodes that carry a levelID become enterable**: IDs {1–16, 18, 22–28, 30}, including the two switch-revealed hidden nodes — levelID 11 at (91,15) (p=(6,18), trig 9, trigP=(1,0)) and levelID 25 at (69,27) (p=(7,19), trig 13, trigP=(1,1)).
- The remaining gaps are **level-data gaps in this working file, not table errors** (`World.lvl`'s internal name is "Work With World Worldview" — a WIP dump):
  - **levelID 20** exists only as a tag on plain walkway (4,7) at (60,56) — inert, because the runtime reads `levelID` only when standing on a Level/PassedLevel-classified tile. The adjacent hidden node at (59,50) (p=(4,7), trig 18, trigP=(1,2)) has **levelID 0** — almost certainly the level-20 node missing its ID.
  - **levelID 21** exists only as a tag on signpost icon (2,18) at (92,25) (NoPass, trig 0) — **no enterable node for level 21 exists in the file**.
  - **Trigger group 18 is unreachable**: its only switch-faced cell, (6,6) at (69,32), sits on a walkway islet not connected to the network (verified unreachable even allowing any single switch to be toggled back). Group 18 would reveal the (59,50) hidden node and repair the (60,57) bridge gap.
  - **14 level-icon cells have levelID 0** (western/northern clusters: (58,15), (58,17), (50,18), (65,21), (63,23), (55,25), (46,29), (47,31), (63,31), (56,32), (67,36), (71,36), (66,41), (75,41)) plus a second ID-less hidden node at (57,44) (p=(0,0), trig 20, trigP=(1,1)). Standing on one sets `levelNumber = 0` → `GetLevelInfo` shows "Nonexistant Level". IDs 17, 19, 29 never appear anywhere.
  - Other inert levelID tags on non-level icons (designer metadata, never read): IDs {4,5,15,16,22,23,24,26,30} on walkway/signpost/scenery cells.
- `state` is 0 in **all** 14,400 cells; `kind` (0×13,928 / 1×24 / 2×448) is never read by runtime map code (re-confirms §8 note). Coordinates above are (inner-index, outer-index) of the text file, matching §7's "(58,54)" start convention; note the histogram, class totals and BFS results are transpose-invariant, so the §6 x-major-vs-row-major ambiguity does not affect any conclusion.

**Port decision (recommendation, not a 1997 fact):** build the map room with the active `MakePieceKinds` table verbatim (col 1 rows 0–2 = level, rows 3–5 = passed level, row 6 cols 0–8 = switch, rows 7–8 cols 0–8 = passable, else NoPass) and ignore the commented-out table. Keep original behavior for the levelID-0 nodes (enterable, "Nonexistant Level" info box) or render them as scenery; do not "fix" levels 20/21/trig-18 in data unless a later `'MRwd' 128` resource is recovered — the repo's `World.lvl` is itself the work-in-progress source and is playable end-to-end for all 25 wired levels.
