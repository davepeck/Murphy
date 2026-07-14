# Murphy's Revenge — Audio Specification (Sound Effects + Music)

Sources read (authoritative):
- `/Users/dave/dev/games/Murphy/legacy/murphy/os7code/mr/MRSound.c`, `MRSound.h` (sound engine)
- `MRResources.c/.h` (resource-file alias mechanism that selects per-world sound banks)
- `MRPrefs.c/.h`, `MRInterface.c/.h` (volume settings / prefs UI)
- Trigger call sites: `MRPlay.c`, `MRPlayCommon.c`, `MRPlaySprite.c`, `MRChore.c`, `MRMap.c`, `MRPlayers.c`, `MRInit.c`, `main.c`, `MRLoop.c`, `MRGfxUtils.c`, `MRPlayConstants.h`
- Assets: `/Users/dave/dev/games/Murphy/legacy/murphy/sounds/{Infotron,Aquatron,Savanatron,Deathtron,Interface}/*.wav`, `sounds/music-info.txt`

Time units: all "ticks" below are classic Mac ticks = 1/60 second.

---

## 1. Architecture: sound banks and the "sound alias"

Sounds are classic Mac `'snd '` resources looked up **by numeric ID inside whichever
resource file is the current "sound alias"** (`res_sndResAlias` in `MRResources.c`).
The same numeric ID means different sounds in different files. This is the key
indirection to reproduce.

Resource files opened at startup (`MRResourcesInit`):

| File | Path | Role |
|---|---|---|
| main | the application itself | contains `snd 128` = time-is-low sting |
| Interface (a.k.a. "general"/"pictures") | `:MR Data:Interface` | all UI sounds, interface music, map-room music |
| Infotron | `:MR Data:Infotron` | Infotron world sfx + music bank |
| Aquatron | `:MR Data:Aquatron` | Aquatron world sfx + music bank |
| Savanatron | `:MR Data:Savanatron` | Savanatron world sfx + music bank |
| Deathtron | `:MR Data:Deathtron` | Deathtron world sfx + music bank |
| Players | `:MR Data:Players` | prefs/player records only (no sounds) |
| World | `:MR Data:World` | levels only (no sounds) |

Alias state machine:

- Default (startup, main menu, map room, player-select, prefs dialog):
  `sndAlias = Interface file` (`MRResourcesInit` sets `res_sndResAlias = res_generalResFile`;
  `MRRestoreSndAlias()` returns to this).
- Entering a level (`MakeLevelCBData`, MRPlay.c ~line 1250):
  `MRSetTerrainAlias(extraInfo.levelWorld)` then `MRTerrainAlias(); MRSetSndAlias();`
  → `sndAlias = world file`. World index mapping (`MRSetTerrainAlias`, and
  `MRPlayConstants.h`): `0=Infotron (default), 1=Aquatron, 2=Savanatron, 3=Deathtron`.
- During a level, two temporary switches occur:
  - Death "play again? yes/no" dialog buttons: switch to Interface file
    (`MRRestoreSndAlias()`), play `kTickSnd`/`kClickSnd`, switch back
    (`MRTerrainAlias(); MRSetSndAlias();`).
  - Time-low sting: switch to **main** application file (`MRMainResFile(); MRSetSndAlias();`),
    play `kNoMoreTimeSnd` (ID 128), switch back to the world file.
- Leaving a level (`RunGameLevel` end): `FlushAllSounds()` then `MRRestoreSndAlias()`
  (alias back to Interface file).

Consequence: pause/unpause, death, explosion, etc. are **per-world sounds** (each world
resource file carries its own copy under the same IDs); tick/click/etc. are **shared**
Interface sounds.

Debug builds: `MRSoundInit` disables both sfx and music entirely unless
`MURPHY_DEBUG == DEBUG_OFF`.

---

## 2. Sound ID → event → file mapping

### 2.1 World sound bank (`snd ` IDs inside each world's resource file)

Constants from `MRSound.h` and `MRPlayConstants.h`. The `.wav` files in
`sounds/<World>/` are the source assets for these resources (mapping by name is
unambiguous):

| ID | Constant | Event | Source wav |
|---|---|---|---|
| 128 | `kGotInfotronSnd` | infotron eaten (walk-over or zap-eat) when it is not the last one required | `got-infotron.wav` |
| 129 | `kThudSnd` | falling piece (zonk/infotron/etc.) lands on something solid | `thud.wav` (Aquatron: `water-thud.wav`) |
| 130 | `kPortUseSnd` | Oliver passes through any port tile | `use-port.wav` |
| 131 | `kExplodeSnd` | explosion ignites (incl. red-disk detonation, gold-disk trail) | `explosion.wav` |
| 132 | `kQuarkExplodeSnd` | quark ("plasmoid") explosion ignites | `plasmoid-explosion.wav` |
| 133 | `kGotRedDiskSnd` | red disk collected | `got-red-disk.wav` |
| 134 | `kRedDiskReadySnd` | red disk **drop completes** (disk deposited, starts blinking) | `red-disk-ready.wav` |
| 135 | `kGotLastInfotronSnd` | last required infotron eaten (exits open) | `last-infotron.wav` |
| 136 | — | *no constant / unused gap* | — |
| 137 | `kOliverDiedSnd` | Oliver dies (any cause) | `oliver-died.wav` |
| 138 | `kPauseSnd` | game paused | `pause.wav` |
| 139 | `kUnpauseSnd` | game unpaused | `unpause.wav` |
| 140 | `kBugZapSnd` | a bug tile begins its zapping animation | `bugzap.wav` |
| 141 | `kPushSnd` | Oliver starts pushing **any** pushable piece (push sprite created) — all 10 pushable tiles | `push.wav` (Aquatron: `water-push.wav`) |
| 142 | `kTermUseSnd` | terminal used and ≥1 yellow disk exploded | `terminal-use.wav` |
| 143 | `kTermBadSnd` | terminal used but there were no yellow disks | `terminal-no-yellow.wav` |
| 151 | `kEndLevelSnd` | Oliver steps into the open exit (`kLastExitTile`) | `exit-level.wav` |

Aquatron replaces exactly two effects with water variants (`water-thud.wav`,
`water-push.wav`, same IDs 129/141). All other Aquatron sfx are duplicates of
Infotron's (some at different lengths, e.g. its `explosion.wav` is stereo 1.63 s vs
Infotron's 1.46 s; `oliver-died.wav` 1.63 s vs 0.98 s).

**No "eat base/circuit" sound exists.** `EatPiece` only plays sounds for
`kInfotronTile` and `kRedDiskTile`; walking over/zapping circuit ("base") tiles is
silent. There are also no dedicated Aquatron ambient/water-loop sounds beyond the two
variants above.

**Savanatron and Deathtron ship no sfx wavs** (their folders contain only music). It is
unknown/ambiguous whether their compiled resource files duplicated the Infotron sfx or
whether the sfx were never produced (game is unfinished). A port should fall back to
the Infotron set for those worlds.

### 2.2 Interface sound bank (`snd ` IDs inside the Interface file)

| ID | Constant (file) | Event | Source wav |
|---|---|---|---|
| 128 | `kLevInfoUpSnd` (MRMap.c) | map room: level-info panel pops up | `level-info-pop-up.wav` |
| 129 | `kLevInfoDownSnd` (MRMap.c) | map room: level-info panel dismissed (moving off a level node; also when panel is rebuilt) | `level-info-pop-down.wav` |
| 130 | `kMapSwitchSnd` (MRMap.c) | map room: player steps onto a switch piece (triggers map change) | `map-switch.wav` |
| 144 | `kClickSnd` | button "commit" (mouse-up inside a button / key-up confirming) — used everywhere in UI | `click.wav` |
| 145 | `kTickSnd` | button "press" (mouse-down on a button / key-down highlight) — used everywhere in UI | `tick.wav` |
| 146 | `kDingSnd` | **defined but never played anywhere** | `ding.wav` |
| 150 | `kLogoSnd` | app-startup Q/Redline logo sting (played in `VideoSettingsQuery` just before the logo fade-in) | `q-logo-sound.wav` (7.29 s) |
| 200 | `kPSSoundID` / `kPrefSoundID` | player-select dialog reveal AND preferences dialog reveal (both use the same whoosh) | `player-select-dialog-open.wav` |
| 201 | `kPSKeyPressSnd` / `kPrefKeyPressSnd` / `kPrefBadKeySnd` | typing a character in the player-name editor; ALSO played in prefs when the user tries to assign an already-used key ("bad key" reuses ID 201) | `key-press.wav` |
| 202 | `kPSKeyDeleteSnd` | backspace in player-name editor (when name non-empty) | `key-delete.wav` |
| 203 | `kPSBigNameSnd` | name-too-long (typing past `MAX_PLAYER_NAME_LENGTH`) or backspace on empty name | `name-too-big.wav` |

Note the deliberate ID collisions with the world bank (128/129/130) — resolved purely
by which file the alias points to.

### 2.3 Main application file

| ID | Constant | Event | Source wav |
|---|---|---|---|
| 128 | `kNoMoreTimeSnd` (`MRPlayConstants.h`) | timed level, ≤10 s remaining: played once per second | `time-is-low.wav` (0.5 s) |

Trigger detail (`Level_IdleBack`, MRPlay.c ~445): when the per-second timer fires and
`secs <= 10 && mins == 0 && hours == 0` (including the final `secs == 0` tick when
Oliver is killed by timeout), play with proximity `= secs * 1.5`
(`(short)((double)cb->secs * (15.0/10.0))`) — i.e. it gets **louder** as time runs out:
10 s left → prox 15 (≈32% volume), 0 s → prox 0 (full volume).

---

## 3. Gameplay trigger details (exact conditions)

- **Explosion** (`StartExplosion`, MRPlayCommon.c): plays `kExplodeSnd` only when
  `shouldIgnite == true`, positional stereo
  (`PlaySoundIDStereo(kExplodeSnd, ProximityToOliver(loc), ProximityPanning(loc))`).
  Chain reactions: `IgniteExplosion` calls `StartExplosion` on all 8 neighbors with
  `shouldIgnite = neighbor.isIgnitable`, so a chain can request many overlapping plays
  (most get dropped by the 3-channel limit — see §5).
- **Quark/plasmoid explosion** (`StartQuarkExplosion`): same pattern with
  `kQuarkExplodeSnd`.
- **Gold-disk push trail** (`_move_Push_GoldDisk`, MRPlaySprite.c ~660): each trail cell
  plays `kExplodeSnd` positionally (the `StartExplosion(..., false)` itself is silent;
  the sound is an explicit extra call). The final ignition at end-of-road goes through
  `StartExplosion(..., true)` and sounds via that path.
- **Thud** (`_move_Fall`, MRPlaySprite.c ~555): when a falling sprite can't continue,
  `PlaySoundIDStereo(kThudSnd, ProximityToOliver(loc) / 2, ProximityPanning(loc))` —
  proximity halved: comment says "they're a bit louder than most".
- **Bug zap** (`_process_Bug`, MRChore.c ~284): when a waiting bug flips to its zapping
  animation (waits `kBugZapDelayTime` = 120 ticks + `15 * random(0..16)` ticks between
  zaps), positional stereo `kBugZapSnd`.
- **Death** (`KillOliver`, MRPlayCommon.c ~485): `NextSoundPriority()` then
  `PlaySoundID(kOliverDiedSnd)` — the only sound given channel priority (see §5).
- **Infotron / red disk collection** (`EatPiece`): decrement `infotronCount`; if it hits
  exactly 0 → `kGotLastInfotronSnd` + open exits, else `kGotInfotronSnd`. Red disk →
  `kGotRedDiskSnd` (and >9 disks kills Oliver — the death sound then follows).
  Zap-eating (special+direction) routes through a `Zap` chore that calls
  `EatPiece(origTile)` when the zap animation finishes, so the same sounds play,
  delayed by the zap animation (`kZapDelayTicks` = 3 ticks/frame). Starting a zap
  itself plays **no** sound.
- **Red disk drop** (`_move_Oliver_SpecialKeyDown`, MRPlaySprite.c ~1127): holding
  special ≥ `kRedDiskDropFinish` (90 ticks; anim starts at `kRedDiskDropDelay` = 30
  ticks) deposits the disk: `redDiskCount -= 1`, `PlaySoundID(kRedDiskReadySnd)`,
  blink chore starts. When the blink chore finishes (12 blinks × 20 ticks) it calls
  `StartExplosion(..., true)` → `kExplodeSnd`.
- **Push** (`MakeSprite_Push`, MRPlaySprite.c:1745): `PlaySoundID(kPushSnd)` is the
  **unconditional** last statement of `MakeSprite_Push`, so it plays for **every**
  successful push — all 10 pushable tiles: zonk (17), up-zonk (18), green disk (3),
  yellow disk (4), gold disk (6), blank chip (38), laser battery (68), fore mirror
  (69), back mirror (70), spent battery (81). Non-positional (full volume). (The
  zonk/up-zonk/gold-disk conditional earlier in the function guards only the
  `SWDrawTile(..., kBlankTile)` cell-blanking, **not** the sound — see Follow-up.)
- **Port** (`_move_Oliver_PassPiece`, ~1289): `PlaySoundIDProx(kPortUseSnd, 8)` — fixed
  proximity 8 → volume factor (22−8)/22 ≈ 0.636.
- **Terminal** (`ExplodeYellowDisks`, MRPlayCommon.c ~863): rate-limited by
  `kTermSoundDelay` = 60 ticks since the last terminal sound (`cb->lastTermSound`).
  If ≥1 yellow disk exploded → `kTermUseSnd`, else → `kTermBadSnd`.
- **Exit** (`_move_Oliver_PassPiece`, ~1298): stepping onto `kLastExitTile` plays
  `kEndLevelSnd`, sets `wonLevel`, removes Oliver.
- **Pause/unpause** (`Level_KeyBack`, MRPlay.c 139/159): pause key-up toggles;
  pause → `kPauseSnd` + `FadeMusicOut()`; unpause → `kUnpauseSnd` + `FadeMusicIn()`.
  Note: music is **not stopped** while paused — it keeps streaming at volume 0
  (`KeepMusicGoing()` runs before the paused check in `Level_IdleBack`). A port should
  mute, not pause, the music.
- **Death dialog / win**: the "play again? yes/no" buttons play Interface
  `kTickSnd`/`kClickSnd` (with the alias juggling described in §1; mouse and Y/N/Return
  keys equally).

---

## 4. Spatial audio model

Constants: `kMaxProximity` = 22 (`MRSound.h`); pan range −10..+10.

- `ProximityToOliver(cb, loc)` = Manhattan distance in tiles:
  `|oliver.h − loc.h| + |oliver.v − loc.v|` (uses `cb->oliverCurLoc`).
- `ProximityPanning(cb, loc)` = `(loc.h − oliver.h) >> 1`, clamped to [−10, +10]
  (arithmetic shift; positive = right of Oliver).

Three play functions (all: no-op if sfx disabled; find a free channel else drop; set
per-play volume; `soundVolume` = current pref 0–255):

1. `PlaySoundID(id)` — both channels at `soundVolume`.
2. `PlaySoundIDProx(id, proximity)` — **returns without playing if
   `proximity > 22`**; both channels at
   `vol = soundVolume * (22 − proximity) / 22` (floating point, truncated to
   unsigned short).
3. `PlaySoundIDStereo(id, proximity, signedPan)` — base
   `vol = soundVolume * (22 − proximity) / 22`; then with `p = (signedPan + 10) / 20`:
   `right = vol * p`, `left = vol * (1 − p)` (constant-power normalization is present
   but commented out — it is plain linear panning). **Bug:** unlike `Prox`, it does NOT
   reject `proximity > 22`; the negative intermediate value wraps through
   unsigned-short casts to a huge volume. A port must clamp `proximity` to [0, 22]
   (equivalently clamp volume to ≥0).

Volume is applied per play with a `volumeCmd` (`param2 = (right << 16) | left`); music
volume uses mono `ampCmd` (0–255). For a web port, treat 0–255 as linear gain 0..1.

---

## 5. Channels and priority

- `MAX_SND_CHANS` = 3 stereo sampled-sound channels for sfx, allocated at init, plus
  one dedicated music channel (created per `PlayMusic`).
- Channel selection: `Sound_FindFreeChan` polls busy status and returns the first free
  channel (0,1,2). **If all three are busy the sound is silently dropped.**
- `NextSoundPriority()`: if no channel is free, immediately quiets channel index 2
  (`MAX_SND_CHANS − 1`) so the next play succeeds. Called only before
  `kOliverDiedSnd` — the death sound is guaranteed to play. (A commented-out variant
  inside `Sound_FindFreeChan` that auto-stole the last channel was abandoned:
  "Apparently SM3.0 doesn't like being bombarded with gazillions of immediate
  quietCmds.")
- Memory bookkeeping (`AddSoundID`, up to `MAX_RELEASE_IDS` = 128 ids;
  `FlushAllSounds()` after each level and after leaving the map room) is
  Mac-resource-specific; no behavioral consequence for a port beyond cache eviction.

---

## 6. Music system

### 6.1 Data model

A "music" is a `'STR '` resource (type `kMusicInfoResType`) whose string is a
**play-sequence of letters**; each letter `c` names one `'snd '` segment with resource
ID `baseID + (c − 'A')`. Base IDs (`MRSound.h`):

| Constant | Base ID | Bank (via sound alias at call time) | Segments |
|---|---|---|---|
| `kInterfaceMusic` | 1000 | Interface file | `intro-music.wav` (snd 1000; single 13.58 s stereo segment) |
| `kGamePlayMusic` | 2000 | current world file | per world, below |
| `kMapRoomMusic` | 5000 | Interface file | `map-room-music.wav` (snd 5000; single 15.24 s stereo segment) |

Per-world gameplay segments (letter → `music-N.wav` → snd 2000+N−1):

- Infotron: A–E = `music-1..5.wav` (five 6.85 s mono segments), sequence string
  `AxABBCCDDEEEECCBBA`.
- Savanatron: A–C = `music-1..3.wav` (three 7.56 s stereo segments), sequence string
  `xAABBBBCCCCBB`.
- Aquatron: single `music.wav` (13.71 s stereo) = presumably snd 2000 ('A'); its STR
  sequence is **not in the repo** (lives only in the lost resource fork).
- Deathtron: single `music.wav` (13.34 s stereo) = presumably snd 2000; STR unknown.

`sounds/music-info.txt` contents, verbatim (CR line endings; this is the entire file —
no attribution or original-format info is present):

```
Musics

Infotron: AxABBCCDDEEEECCBBA

Savanatron xAABBBBCCCCBB
```

### 6.2 Loop marker and sequencing semantics (`SetLoopLoc`, `MoveLocations`, `PlayMusic`)

- `loopLoc` = index of the **last** `'x'` in the string (default 1 if none); the `'x'`
  is then removed (characters shift left, length−1). **Quirk/bug:** the removal is
  unconditional — a string with no `'x'` loses its character at index 1.
- `PlayMusic` sets `currentLoc = 0` then advances once (`MoveLocations`) before loading
  the first segment, so **playback starts at index 1** of the post-removal string;
  index 0 is played only if `loopLoc == 0`.
- After playing the segment at `currentLoc`, `currentLoc += 1`; when it reaches the
  string length it wraps to `loopLoc`. Loops forever.

Worked examples:
- Infotron `AxABBCCDDEEEECCBBA` → post-removal `AABBCCDDEEEECCBBA` (len 17), loopLoc 1.
  Playback: indices 1..16 repeating = `A BB CC DD EEEE CC BB A`, i.e. music-1 ×1,
  music-2 ×2, music-3 ×2, music-4 ×2, music-5 ×4, music-3 ×2, music-2 ×2, music-1 ×1,
  then repeat (a palindromic ~109 s loop).
- Savanatron `xAABBBBCCCCBB` → post-removal `AABBBBCCCCBB` (len 12), loopLoc 0.
  First pass starts at index 1; the loop then covers all 12: music-1 ×2, music-2 ×4,
  music-3 ×4, music-2 ×2 (~91 s loop).

### 6.3 Engine behavior worth mimicking

- Gapless double-buffering: the current segment is played with `bufferCmd` followed by
  a queued `callBackCmd`; the callback immediately chains the next (already-loaded)
  segment. `KeepMusicGoing()` must be pumped from every idle loop / busy-wait (it is
  called from the main event loop, all wipe/delay loops, gameplay idle, prefs idle,
  map idle, `DelayMouseTimeMusic`) to pre-load the next segment and run fade-in steps.
  If two consecutive letters are identical the same buffer is reused without reload.
  If a segment fails to load, music halts permanently (`fatalMusicError`).
  (The file header admits: "this version uses a multibuffering approach to music. but
  it still doesn't work right. DERN." — for a web port, seamless segment chaining via
  WebAudio scheduling is the intended behavior.)
- Only one music at a time; `PlayMusic` stops any current music first. No-op when music
  is disabled.
- **Fade-in** (asynchronous, driven by `KeepMusicGoing`): start at volume 0, add
  `delta = max(1, musicVolume / 255)` (integer division ⇒ effectively always 1) every
  `FADE_IN_DELAY_TICKS` = 2 ticks until reaching `musicVolume`. At `musicVolume` 255
  that is ≈8.5 s; at 171 ≈5.7 s; at 85 ≈2.8 s. All in-code `PlayMusic` calls pass
  `fadeIn = true`. (If `fadeIn` were false the channel would just play at its default
  full amplitude.)
- **Fade-out** (`FadeMusicOut`, synchronous/blocking): 15 steps (`FADE_OUT_STEPS`),
  one per `FADE_DELAY_TICKS` = 2 ticks (≈0.5 s total), decrement `max(1, vol/15)` per
  step, pumping `KeepMusicGoing` while waiting; ends by forcing volume 0. It does NOT
  stop playback — callers pair it with `StopMusic()` except for pause (mute only) and
  quit. (Minor original bug: the unsigned volume can wrap for very small starting
  volumes; clamp at 0 in a port.)
- `FadeMusicIn()` re-runs the fade-in ramp on the already-playing channel (used at
  unpause).

### 6.4 When each music plays

| Screen | Call | Music |
|---|---|---|
| App intro (after Redline logo, as the splash appears — `IntroSequence`, main.c 167) | `PlayMusic(kInterfaceMusic, true)` | interface music; continues into the main menu (`RunInterface` deliberately does not restart it) |
| Main menu exit | Start → `FadeMusicOut(); StopMusic();` Quit → `FadeMusicOut()` only; RunInterface also calls `StopMusic()` on loop exit | — |
| Prefs dialog (in-menu) | keeps interface music running; after OK, **only if musicVolume changed**: `FadeMusicOut(); StopMusic();` then if new volume ≠ 0 `SetMusicState(true); PlayMusic(kInterfaceMusic, true)` else `SetMusicState(false)` | interface music |
| Map room (`RunMapRoom`) | `PlayMusic(kMapRoomMusic, true)` after fade-in; on exit `FadeMusicOut(); StopMusic()` | map-room music |
| Gameplay (`RunGameLevel`) | `PlayMusic(kGamePlayMusic, true)` right after the level fades in; on level end (win, death-dialog exit, or replay) `FadeMusicOut(); StopMusic()` | per-world music |
| Pause / unpause | `FadeMusicOut()` (mute, keeps streaming) / `FadeMusicIn()` | — |

(Commented-out code shows map-room music was once started from the interface Start
button instead; the live path is inside `RunMapRoom`.)

---

## 7. Volume model (the four settings)

Preferences (`MRPrefs.h`): `short musicVolume, soundVolume`, persisted in the Players
file (`'Mprf'` id 128). Defaults: both **255**.

Quantized levels (`MRInterface.h`):

| Name | Constant | Value |
|---|---|---|
| Silent | `kSilentVolume` | 0 |
| Audible | `kAudibleVolume` | 85 |
| Pleasant | `kPlesantVolume` (sic; `kPleasantVolume` aliases the typo) | 171 |
| Excruciating | `kExcruciatingVolume` | 255 |

Two independent controls in the prefs dialog — one for music, one for sfx — each an
icon (PICTs 1002–1005: Silent/Audible/Pleasant/Excruciating) with left/right arrow
buttons that **cycle with wraparound** (`PrefLArrow`/`PrefRArrow`):
- Left arrow (decrease): Excruciating → Pleasant → Audible → Silent → Excruciating…
- Right arrow (increase): Silent → Audible → Pleasant → Excruciating → Silent…

What the values control:
- `soundVolume` scales every sfx play (see §4 formulas). 0 additionally maps to
  `SetSoundState(false)` (sfx fully off).
- `musicVolume` is the music channel's target amplitude (fade-in target; 0–255 mono
  gain). 0 maps to `SetMusicState(false)` (music never starts).
- At startup (`InitMurphysRevenge`): `soundVolume == 0` → sounds off;
  `musicVolume == 0` → music off.
- After the prefs dialog: `SetSoundState(soundVolume != 0)`; music state handled as in
  §6.4 (restart with fade only if the value changed). While the prefs dialog is open,
  sfx are force-enabled (`SetSoundState(true)` "hack") so the user hears UI feedback.
- SFX volume changes take effect immediately (each play reads current prefs); a music
  volume change takes effect via the music restart on OK.

Vestigial: `DoTempPrefs` (an old System-7 modal dialog, **never called**) treated both
as booleans (`255 − v` toggling, `SetSoundState(v == 255)`); ignore it.

---

## 8. UI sound conventions (tick/click pattern)

Everywhere in the interface, a two-phase button sound is used from the Interface bank:
- `kTickSnd` (145) on press/highlight (mouse-down entering the button, or key-down of
  an accelerator);
- `kClickSnd` (144) on successful commit (mouse-up still inside / matching key-up).
  Aborted tracks play only the tick.

Specific sites: main-menu items (mouse hover uses no sound; press/commit tick/click;
keyboard accelerators q/i/c/h/s/p and Return mirror this), prefs checkboxes and
arrows and Ok/Cancel and key-capture boxes (bad key → snd 201; accepted key →
`kClickSnd`), map room back-arrow / switch-player / level-info "Play" button /
Return-to-play, player-select dialog buttons and Return handling, in-level death
dialog Yes/No. The player-select confirm-delete dialog uses the same tick/click.

Other interface events: prefs dialog reveal & player-select reveal → snd 200 whoosh;
map-room level-info pop up/down → snd 128/129; map switch → snd 130; name editing →
snd 201/202/203; startup logo → snd 150. `kDingSnd` (146) is never triggered.

---

## 9. Asset inventory (all 44100 Hz, 16-bit PCM WAV)

Durations (seconds) for reference. Mono unless noted.

Infotron: bugzap 0.53, exit-level 0.73, explosion 1.46 (stereo), got-infotron 0.057,
got-red-disk 0.11, last-infotron 0.34, music-1..5 6.85 each, oliver-died 0.98,
pause/unpause 0.78, plasmoid-explosion 1.18, push 0.16, red-disk-ready 0.55,
terminal-no-yellow 0.21, terminal-use 0.49, thud 0.11, use-port 0.44.

Aquatron: as Infotron except explosion 1.63 (stereo), oliver-died 1.63,
water-push 0.34, water-thud 0.34, music.wav 13.71 (stereo); no push/thud.

Savanatron: music-1..3 7.56 each (stereo) only. Deathtron: music.wav 13.34 (stereo) only.

Interface: click 0.11, tick 0.084 (stereo), ding 0.60 (unused), intro-music 13.58
(stereo), key-delete 0.085, key-press 0.022, level-info-pop-down 0.059,
level-info-pop-up 0.067, map-room-music 15.24 (stereo), map-switch 0.48,
name-too-big 0.086, player-select-dialog-open 0.68, q-logo-sound 7.29,
time-is-low 0.50.

---

## 10. Ambiguities / unfinished areas (do not guess in the port)

1. **Aquatron & Deathtron music sequence strings are unknown** (single segment each;
   the STR 2000 resources lived in the resource forks, which are not in this repo).
   Looping the single file is the only reasonable port behavior.
2. **Interface and map-room music sequence strings are unknown** (also single files);
   loop the files.
3. **Savanatron/Deathtron sfx banks are missing** — fall back to the Infotron set.
4. The music engine is self-described as buggy ("still doesn't work right"); its
   observable *intent* — gapless segment chaining with letter-sequence looping — is
   what should be ported, not its timing glitches.
5. `PlaySoundIDStereo` volume wrap for proximity > 22, `SetLoopLoc` char-deletion when
   no `'x'` exists, and the fade-out unsigned wrap are original bugs; a port should
   clamp/guard instead of reproducing them.
6. Sound ID 136 is an unexplained gap; IDs 147–149 and 152+ (below 200) are unused.
7. `kDefaultSoundProx` (= 6) is defined but never used.
8. Whether interface music should restart when returning to the main menu from the map
   room: `RunInterface` never calls `PlayMusic` (relies on the intro having started
   it), so on re-entry from the map room the menu is **silent** in the code as
   written. This looks like an oversight (the call is commented out at
   MRInterface.c:1716); flag for a product decision.

---

## Follow-up: kPushSnd plays on EVERY push, not just zonk/up-zonk/gold-disk

**Resolved 2026-07-13.** The earlier §3 "Push" bullet (and the ID-141 table row)
claimed `kPushSnd` sounded only for zonk, up-zonk, or gold-disk pushes. That is
**wrong**. In `MakeSprite_Push` (legacy/murphy/os7code/mr/MRPlaySprite.c, function
starts at line 1677; line numbers per CR-converted file), the sound call is the
unconditional final statement of the function, line 1745:

```c
	// now move it along somewhat.
	SWOffsetSprite( pushSprite, pushSprite->horizMoveDelta, pushSprite->vertMoveDelta );

	// play the push sound
	PlaySoundID( kPushSnd );
}
```

There is no early `return` anywhere in `MakeSprite_Push`, so every call reaches
line 1745. `MakeSprite_Push` is invoked from exactly one place:
`_move_Oliver_PushPiece` (MRPlaySprite.c:1228), which itself runs only after
`newKind.isPushable && SafeToPush(...)` succeeds (game-rules.md §7 step /
`_move_Oliver` dispatch). Therefore **all 10 pushable tiles sound**:

| Tile | ID | Push directions |
|---|---|---|
| kGreenDiskTile | 3 | horizontal only |
| kYellowDiskTile | 4 | 4-dir |
| kGoldDiskTile | 6 | 4-dir |
| kZonkTile | 17 | horizontal only |
| kUpZonkTile | 18 | horizontal only |
| kBlankChipTile | 38 | 4-dir |
| kLaserPowerTile | 68 | 4-dir |
| kForeMirrorTile | 69 | 4-dir |
| kBackMirrorTile | 70 | 4-dir |
| kLaserEmptyPiece | 81 | 4-dir |

**Likely origin of the error:** `MakeSprite_Push` does contain a
zonk/up-zonk/gold-disk conditional, but it guards only the blanking of the vacated
cell, ~10 lines above the sound (MRPlaySprite.c:1734–1737):

```c
	if (( pushTag->info.push.isGoldDisk ) ||
		( pushTag->tileID == kZonkTile ) ||
		( pushTag->tileID == kUpZonkTile ))
		SWDrawTile( cbData->sw, pushTag->loc.v, pushTag->loc.h, kBlankTile );
```

That `SWDrawTile` conditional has nothing to do with audio.

**Port requirement:** play `kPushSnd` (snd ID 141, `push.wav`; Aquatron
`water-push.wav`) once at push-sprite creation for every successful push of any of
the 10 pushable pieces, non-positional at full volume (plain `PlaySoundID`, no
proximity/panning). One sound per push initiation — a continued push into the next
cell is a new `MakeSprite_Push` call and sounds again. This matches game-rules.md
§10, which was already correct.
