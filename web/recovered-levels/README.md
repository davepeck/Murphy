# Recovered levels

These three `.lvl` files are **not** in `legacy/murphy/levels/` — they were
recovered (July 2026) from the original `'MRlv'` resources inside
`legacy/murphy/MR-Dev2.cpt` (the Compact Pro archive of the shipped 1998 game
folder, resource file `Murphy's Revenge/MR Data/World`), and exported in the
exact `.lvl` text dialect the other dumps use.

- **In Out In.lvl** — levelID 27, a real level on the original world map
  (node at 103,18) that was missing from the 26 dumped levels. Part of the
  campaign.
- **Insane Infotrons (tm).lvl** — levelID 17, cut content: it was the hidden
  level in the 1998 archive's world map, later replaced by Wizard (levelID 25).
- **test level.lvl** — levelID 19, the developers' laser test level; never on
  the world map.

The web asset pipeline (`scripts/build-assets.mjs`) merges these with the
levels from `legacy/murphy/levels/`.

Also recovered from the same archive, for the record: the definitive
levelID → level mapping (campaign order), and the untruncated hints for
"And So It Begins...", "Crystalline", and "Combinations" (the original dump
tool cut hints at the first colon).
