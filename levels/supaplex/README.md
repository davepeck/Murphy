## Supaplex-style Levels

The level files in this directory are intended to make it easy (or easy-ish) to integrate into supaplex-style games, such as https://www.supaplex.online/

The best discussion I'm aware of describing the contents of an old-style supaplex level is found here: http://www.hpautomation.com/SPFIX63a.pdf

Here, we define a simple new JSON format for levels that has some ties to the past format. See the `schema.ts` file for a description of the JSON format in [TypeScript](https://www.typescriptlang.org/).

In the `infotron/` and `murphy/` subdirectories, we have JSON files meeting this specification. Infotron is a Classic MacOS shareware game from the mid-90s; Murphy's Revenge was an intended sequel that never shipped (but got pretty far along and is quite playable in practice).

Both Infotron and Murphy's Revenge had a handful of tile types that are _not_ part of the original
Supaplex specification. Infotron's extra tile types should be trivial to implement. Murphy's Revenge, on the other hand, has a few new tile types that add interesting new potential for designing puzzles. Some Murphy's Revenge levels make use of these new tile types. See `schema.ts` in this directory for details.

-Dave 4/28/2020

