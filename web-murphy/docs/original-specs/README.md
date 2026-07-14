# Original game specs (reverse-engineered)

Implementation-grade documentation of the original 1996 Murphy's Revenge,
extracted from the Mac Toolbox C sources (`../../legacy/murphy/os7code/mr/`),
the Think Pascal Infotron sources, the iPhone/OSX port attempts, and the
`MR-Dev2.cpt` archive of the shipped game folder. The web port in `../../src`
was implemented from these.

- `game-rules.md` — complete gameplay rules with exact tile IDs, timings, and
  the SpriteWorld engine semantics (verified against SpriteWorld 2.x sources)
- `level-format.md` — byte-exact `.mbl`/`.mlv`/`.lvl`/`'MRlv'`/`'MRwd'` formats,
  the recovered levelID -> level campaign mapping, and known data quirks
- `graphics.md` — tile atlases, animation sequences, fonts, interface art
- `app-flow.md` — screens, loop timing, camera, input, HUD, persistence
- `audio.md` — sound event mapping, spatialization, segment-sequenced music
- `infotron-reference.md` — the predecessor game's complete rules (gap filler)

Each file may end with "## Follow-up" sections containing verified corrections;
where a follow-up contradicts the body, the follow-up wins.
