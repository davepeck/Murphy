// build-assets.mjs — regenerate everything under public/assets/ from the
// original Infotron 1.1 archive. Thin wrapper around extract-resources.mjs
// so `pnpm assets` / `node scripts/build-assets.mjs` is the one entry point.

import { main } from './extract-resources.mjs';

main().catch((e) => {
  console.error(e);
  process.exit(1);
});
