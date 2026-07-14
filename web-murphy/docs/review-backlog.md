# Review backlog — unverified findings

A multi-agent review produced these claims, but their adversarial
verification pass was cut short (session limits). Each needs to be
checked against `docs/original-specs/` and the original C code before
being treated as a real defect — reviewers over-report by design, and
46 other claims from the same run were refuted on verification.

- **high** `web/src/engine/game.ts` — Quark ignition does not convert its 3x3 neighborhood into infotrons
- **high** `web/src/engine/game.ts` — Mover creation leaves the quark/scissor level tile on the grid (no phantom/blank rewrite)
- **high** `web/src/engine/sprites.ts` — Push never re-phantomizes Oliver's destination cell (pushed piece's old cell)
- **medium** `web/src/engine/game.ts` — Sprites created by the current tail sprite run on their creation frame, so vertical zonk cascades fall in lockstep instead of the original 1-frame-per-link stagger
- **medium** `web/src/engine/game.ts` — Laser beam terminates at explodable/ignitable pieces instead of blasting through and continuing
- **medium** `web/src/engine/game.ts` — hitBottomCheck (and Oliver's blocked-by-mover path) explode at the mover's loc instead of the checked phantom cell
- **medium** `web/src/engine/game.ts` — Crushed/bumped movers explode at m.loc instead of at the checked phantom cell
- **medium** `web/src/engine/sprites.ts` — Quark blanks its trailing cell before the direction scan, eliminating the original dead-end stall and facing spin
- **medium** `web/src/engine/sprites.ts` — PushSprite takes 9 frames per tile instead of 8 (double hesitation)
- **medium** `web/src/engine/sprites.ts` — After dropping a red disk Oliver stays frozen while special is held; original frees him immediately
- **medium** `web/src/engine/sprites.ts` — Deny-after-failed-zap only clears on special-key release, not direction-key release
- **medium** `web/src/engine/sprites.ts` — Special key: red-disk drop timer never resets after a zap (premature/instant drops)
- **medium** `web/src/engine/sprites.ts` — push() never phantomizes Oliver's destination cell for gold/zonk/up-zonk pushes
- **medium** `web/src/engine/sprites.ts` — RollSprite and PushSprite lack the original's creation-time pixel offset; every roll and push completes one frame late
- **medium** `web/src/engine/sprites.ts` — Bumping a mover's phantom explodes at the mover's loc, not the bumped cell
- **medium** `web/src/engine/sprites.ts` — Red-disk drop timer not reset after zapping: releasing the direction key can instantly drop a red disk
- **medium** `web/src/engine/sprites.ts` — Boxed-in scissor idles instead of rotating in place through the turn state machine
- **medium** `web/src/engine/sprites.ts` — Push: pushed piece's origin cell left Blank instead of Phantom (gold/zonk/up-zonk)
- **medium** `web/src/main.ts` — applySwitch has no failure path: a rejected Screen.enter() leaves the game on a permanent black screen
- **low** `web/src/audio.ts` — Default music/sound volumes are 171 (Pleasant) instead of the original 255 (Excruciating)
- **low** `web/src/engine/game.ts` — findMoverAt does not reproduce FindMoverAtLocation's first-sprite-wins NULL masking
- **low** `web/src/engine/game.ts` — Laser beams stop at spent batteries; the original explodes through them
- **low** `web/src/engine/game.ts` — Zap and Exit chores start one delay period late (original first step fires immediately)
- **low** `web/src/engine/game.ts` — Battery landing and emitter activation scan neighbors in a different order than the original
- **low** `web/src/engine/game.ts` — Laser beams are stopped by spent batteries (81); the original only stops at fresh batteries (68)
- **low** `web/src/engine/game.ts` — portPassable(0,0) returns false; original GetPortPassable returns passable for zero deltas
- **low** `web/src/engine/game.ts` — Exit and Zap chores skip the original's immediate first step (everything runs 4 ticks late)
- **low** `web/src/engine/sprites.ts` — Mover's start cell keeps its editor tile (20-24) until the first move completes: 8-frame double-render and stale solid cell
- **low** `web/src/engine/sprites.ts` — Push path skips blanking Oliver's origin on port/RD-blink tiles; the original blanks unconditionally there
- **low** `web/src/engine/sprites.ts` — Rolling zonk animation advances at half the original rate (66ms vs 33ms)
- **low** `web/src/engine/sprites.ts` — Gold disk keeps Oliver in the push state for its whole flight and grants a pushHack frame the original suppresses
- **low** `web/src/engine/sprites.ts` — RollSprite commits one frame late (firstCall guard instead of creation offset)
- **low** `web/src/engine/sprites.ts` — Push path skips blanking Oliver's origin on port/rd-blink cells; original blanks unconditionally
- **low** `web/src/engine/sprites.ts` — Landing thud proximity is capped before halving, so distant thuds play at a constant half volume
- **low** `web/src/engine/sprites.ts` — Gold disk stops with a 3x3 blast on mover phantoms; original plows through them
- **low** `web/src/engine/sprites.ts` — Rolling zonk animation runs at half the original rate
- **low** `web/src/engine/sprites.ts` — Scissor turn-resume is missing the extra 4px nudge (post-turn tile crossing takes 8 frames instead of 7)
- **low** `web/src/engine/sprites.ts` — Phantom written at Oliver's cell during special-key freeze (original returns before the write)
- **low** `web/src/engine/sprites.ts` — Zap denial only clears on special-key release, not on releasing the offending direction key
- **low** `web/src/engine/tiles.ts` — Dead export: oliverFrameName is never used
- **low** `web/src/input.ts` — held is a Set of Actions, so releasing one of two physical keys bound to the same action drops the action while the other key is still held
- **low** `web/src/main.ts` — Input/pointer edge queues accumulate during async screen switches and fire on the new screen's first tick
- **low** `web/src/ui/select.ts` — Unguarded localStorage access can throw inside enter()/tick(), freezing the game
