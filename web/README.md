# Murphy's Revenge — web port

The unfinished 1996 Macintosh shareware game, ported to TypeScript and running
in a browser at its original 640×480. The gameplay engine is a faithful port of
the original Mac Toolbox C code in `../legacy/murphy/os7code/mr/`.

## Run it

```sh
pnpm install
pnpm assets   # one-time: builds public/assets from the originals elsewhere in the repo
pnpm dev      # http://localhost:5173
```

`pnpm test` runs the engine unit tests; `pnpm build` makes a production build
in `dist/`.

## What's here

- **Engine** (`src/engine/`) — a port of MRPlay.c/MRPlayCommon.c/MRPlaySprite.c/
  MRChore.c: the authoritative 32px tile grid, event-driven gravity cascades
  (no per-tick grid scan), pixel-gliding sprites that decide only at grid
  alignment, and a FIFO "chore" queue for cell-local processes (explosions,
  bugs, zaps, exits, blinking red disks, lasers). Logic runs at the original
  30 fps with 1/60s tick constants.
- **All the original mechanics**: eating circuit/infotrons/red disks, zonks and
  up-zonks (they fall *up*) with roll-off physics, pushing (zonks, disks,
  mirrors, batteries — gold disks are fire torpedoes), quarks (explode into
  infotrons) and scissors (wall-followers with turn animations), electrified
  bugs, all 7 port types, terminals + yellow disks, red-disk bombs,
  lasers + mirrors, phase hardware, timed levels, the snap/zap move, and
  Oliver's idle/boredom animations (wait 90 seconds sometime).
- **Assets** converted from the originals by `scripts/build-assets.mjs`:
  three world tile atlases (from `../osx/Murphy/*.atlas`), levels from the
  `.lvl` dumps (which keep the time limits and hints the `.mbl` files lost),
  every sound effect, the segment-sequenced music (Infotron's gameplay music
  really is `A BB CC DD EEEE CC BB A`, gaplessly scheduled via WebAudio), and
  the interface art incl. the bitmap fonts (glyphs stored rotated 90°, drawn
  as vertical runs, with the original measured-advance kerning).
- **Recovered content** (`recovered-levels/`): the lost level **In Out In**
  (on the original world map but missing from the repo's level dumps), the cut
  level **Insane Infotrons™**, and the developers' laser **test level** — all
  extracted from the `'MRlv'` resources inside `../legacy/murphy/MR-Dev2.cpt`.
  The level list uses the original campaign order recovered from the same
  archive, and three hints that the old dump tool truncated at a colon are
  restored.

## Faithful vs. adapted

Faithful: tile IDs, piece property table (including the switch fall-throughs),
movement speeds (8 px/frame walk, 4 px/frame push/enemies), cascade order,
explosion timing and chain-reaction frames, enemy AI scan order
(left/straight/right/back), sound events with Manhattan-distance proximity +
stereo panning, the proportional catch-up camera, the pause screen-door dim,
and the "You goofed!" replay dialog.

Adapted for the web: the level list stands in for the original (unfinished)
map-room overworld; arrow keys/WASD replace the 8-4-6-2 defaults (Space =
snap/drop, P or Tab = pause, Esc = suicide); progress and volumes persist in
localStorage; music volume defaults to "Pleasant" rather than the original
default of "Excruciating".
