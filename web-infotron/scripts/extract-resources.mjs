// extract-resources.mjs — Infotron! asset-extraction pipeline.
//
// Extracts the shipped Infotron 1.1 archive (BinHex .hqx -> Compact Pro) with
// `unar`, parses the classic Mac resource forks out of the AppleDouble files,
// and produces web-ready assets under public/assets/:
//
//   icons/<id>.png     - icl8 (32x32, 8-bit indexed) + ICN# mask -> RGBA PNG
//   icons/index.json   - all icon IDs + mask info
//   levels/NNN.json    - decompressed level boards (see docs/original-specs/)
//   levels/index.json  - id, name, quota, size for all levels
//   sounds/<id>.wav    - 'snd ' resources -> 8-bit unsigned PCM WAV
//   music/<id>.wav     - music 'snd ' resources -> WAV
//   pictures/<id>.png  - PICT resources converted via ImageMagick
//   pictures/index.json
//
// Run via:  node scripts/build-assets.mjs   (or `pnpm assets`)
// Direct:   node scripts/extract-resources.mjs
//
// The raw archive is cached in .assets-cache/ at the repo root; delete that
// directory to force a re-extraction from legacy/infotron/infotron-11.hqx.

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync } from 'node:child_process';
import JimpModule from 'jimp';

const Jimp = JimpModule?.default ?? JimpModule;

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const HQX = path.resolve(ROOT, '../legacy/infotron/infotron-11.hqx');
const CACHE = path.join(ROOT, '.assets-cache');
const OUT = path.join(ROOT, 'public', 'assets');
const DOCS = path.join(ROOT, 'docs', 'original-specs');
const MAGICK = '/opt/homebrew/bin/magick';

// ---------------------------------------------------------------------------
// Step 0: extract the archive (cached)
// ---------------------------------------------------------------------------

function ensureExtracted() {
  const folder = path.join(CACHE, 'Infotron Folder');
  const datafiles = path.join(folder, 'Info Datafiles');
  if (fs.existsSync(datafiles)) {
    log(`Using cached extraction at ${folder}`);
    return folder;
  }
  if (!fs.existsSync(HQX)) throw new Error(`Archive not found: ${HQX}`);
  fs.mkdirSync(CACHE, { recursive: true });
  log(`Extracting ${HQX} ...`);
  // -k hidden: keep resource forks as AppleDouble "._Name" files.
  execFileSync('unar', ['-k', 'hidden', '-q', '-f', '-o', CACHE, HQX], { stdio: 'inherit' });
  if (!fs.existsSync(datafiles)) throw new Error(`Extraction did not produce ${datafiles}`);
  return folder;
}

// ---------------------------------------------------------------------------
// AppleDouble + classic Mac resource-fork parsing
// ---------------------------------------------------------------------------

function readResourceFork(appleDoublePath) {
  const buf = fs.readFileSync(appleDoublePath);
  if (buf.length < 26 || buf.readUInt32BE(0) !== 0x00051607) {
    throw new Error(`Not an AppleDouble file: ${appleDoublePath}`);
  }
  const numEntries = buf.readUInt16BE(24);
  for (let i = 0; i < numEntries; i++) {
    const off = 26 + i * 12;
    const entryId = buf.readUInt32BE(off);
    const dataOff = buf.readUInt32BE(off + 4);
    const len = buf.readUInt32BE(off + 8);
    if (entryId === 2) return buf.subarray(dataOff, dataOff + len); // resource fork
  }
  throw new Error(`No resource-fork entry in ${appleDoublePath}`);
}

// Returns [{type, id, name, data}] for every resource in the fork.
function parseResources(fork) {
  const dataOff = fork.readUInt32BE(0);
  const mapOff = fork.readUInt32BE(4);
  const typeListOff = mapOff + fork.readUInt16BE(mapOff + 24);
  const nameListOff = mapOff + fork.readUInt16BE(mapOff + 26);
  const numTypes = fork.readInt16BE(typeListOff) + 1;
  const out = [];
  for (let t = 0; t < numTypes; t++) {
    const e = typeListOff + 2 + t * 8;
    const typeBytes = fork.subarray(e, e + 4);
    // Type as a string: printable ASCII kept, other bytes as \xNN escapes.
    const type = [...typeBytes]
      .map((b) => (b >= 0x20 && b < 0x7f ? String.fromCharCode(b) : `\\x${b.toString(16).padStart(2, '0')}`))
      .join('');
    const count = fork.readInt16BE(e + 4) + 1;
    const refListOff = typeListOff + fork.readUInt16BE(e + 6);
    for (let r = 0; r < count; r++) {
      const re = refListOff + r * 12;
      const id = fork.readInt16BE(re);
      const nameOff = fork.readInt16BE(re + 2);
      const resDataOff = fork.readUInt32BE(re + 4) & 0xffffff; // attr u8 + off u24
      let name = null;
      if (nameOff !== -1) {
        const n = nameListOff + nameOff;
        name = macRomanToString(fork.subarray(n + 1, n + 1 + fork[n]));
      }
      const len = fork.readUInt32BE(dataOff + resDataOff);
      const data = fork.subarray(dataOff + resDataOff + 4, dataOff + resDataOff + 4 + len);
      out.push({ type, typeBytes: Buffer.from(typeBytes), id, name, data });
    }
  }
  return out;
}

// Minimal MacRoman -> Unicode for the high half (0x80-0xFF).
const MACROMAN_HIGH =
  'ÄÅÇÉÑÖÜáàâäãåçéèêëíìîïñóòôöõúùûü†°¢£§•¶ß®©™´¨≠ÆØ∞±≤≥¥µ∂∑∏π∫ªºΩæø' +
  '¿¡¬√ƒ≈∆«»… ÀÃÕŒœ–—“”‘’÷◊ÿŸ⁄€‹›ﬁﬂ‡·‚„‰ÂÊÁËÈÍÎÏÌÓÔÒÚÛÙıˆ˜¯˘˙˚¸˝˛ˇ';

function macRomanToString(buf) {
  let s = '';
  for (const b of buf) s += b < 0x80 ? String.fromCharCode(b) : MACROMAN_HIGH[b - 0x80];
  return s;
}

// ---------------------------------------------------------------------------
// Standard Mac 8-bit system palette (clut 8)
// ---------------------------------------------------------------------------
//
// Entries 0-214: the 6x6x6 color cube with component values
// {0xFF,0xCC,0x99,0x66,0x33,0x00}, blue varying fastest, all *descending*,
// so entry 0 = white. The cube's final entry (black) is NOT stored at 215;
// instead entries 215-254 are four 10-step ramps (red, green, blue, gray)
// using the 0x11-multiples that are not 0x33-multiples
// (EE,DD,BB,AA,88,77,55,44,22,11), and entry 255 = black.

function buildMacPalette() {
  const pal = new Array(256);
  const cube = [0xff, 0xcc, 0x99, 0x66, 0x33, 0x00];
  let n = 0;
  for (let r = 0; r < 6; r++) {
    for (let g = 0; g < 6; g++) {
      for (let b = 0; b < 6; b++) {
        if (n < 215) pal[n] = [cube[r], cube[g], cube[b]];
        n++;
      }
    }
  }
  const ramp = [0xee, 0xdd, 0xbb, 0xaa, 0x88, 0x77, 0x55, 0x44, 0x22, 0x11];
  for (let i = 0; i < 10; i++) {
    pal[215 + i] = [ramp[i], 0, 0];
    pal[225 + i] = [0, ramp[i], 0];
    pal[235 + i] = [0, 0, ramp[i]];
    pal[245 + i] = [ramp[i], ramp[i], ramp[i]];
  }
  pal[255] = [0, 0, 0];
  return pal;
}

const MAC_PALETTE = buildMacPalette();

// ---------------------------------------------------------------------------
// Step 1: icons (icl8 + ICN# mask -> RGBA PNG)
// ---------------------------------------------------------------------------

async function extractIcons(resourcesByFile) {
  const pieces = resourcesByFile.get('Info Datafiles/Infotron Pieces');
  const icl8 = new Map();
  const icnMask = new Map();
  for (const r of pieces) {
    if (r.type === 'icl8' && r.data.length === 1024) icl8.set(r.id, r.data);
    if (r.type === 'ICN#' && r.data.length === 256) icnMask.set(r.id, r.data.subarray(128, 256));
  }

  const dir = path.join(OUT, 'icons');
  fs.mkdirSync(dir, { recursive: true });

  const ids = [...icl8.keys()].sort((a, b) => a - b);
  for (const id of ids) {
    const px = icl8.get(id);
    const mask = icnMask.get(id) ?? null;
    const img = new Jimp(32, 32, 0x00000000);
    const bmp = img.bitmap.data;
    for (let y = 0; y < 32; y++) {
      for (let x = 0; x < 32; x++) {
        const idx = px[y * 32 + x];
        const [r, g, b] = MAC_PALETTE[idx];
        let a = 255;
        if (mask) {
          const bit = (mask[y * 4 + (x >> 3)] >> (7 - (x & 7))) & 1;
          a = bit ? 255 : 0;
        }
        const o = (y * 32 + x) * 4;
        bmp[o] = r;
        bmp[o + 1] = g;
        bmp[o + 2] = b;
        bmp[o + 3] = a;
      }
    }
    await img.writeAsync(path.join(dir, `${id}.png`));
  }

  const index = {
    tileSize: 32,
    count: ids.length,
    ids,
    idsWithoutMask: ids.filter((id) => !icnMask.has(id)),
    icnIdsWithoutIcl8: [...icnMask.keys()].filter((id) => !icl8.has(id)).sort((a, b) => a - b),
  };
  fs.writeFileSync(path.join(dir, 'index.json'), JSON.stringify(index, null, 2));
  log(`icons: wrote ${ids.length} PNGs (${index.idsWithoutMask.length} without ICN# mask; ` +
      `${index.icnIdsWithoutIcl8.length} ICN#-only IDs skipped)`);
  return { icl8Ids: ids, icnIds: [...icnMask.keys()].sort((a, b) => a - b), index };
}

// The rules-spec §1.2 icon-ID legend, expanded to explicit graphic IDs.
function specLegendIds() {
  const ids = new Set();
  const add = (a, b = a) => { for (let i = a; i <= b; i++) ids.add(i); };
  add(128); add(129); add(130); add(131); add(1100, 1106); add(132);
  add(133); add(134); add(135); add(139, 214); add(300, 305); add(350, 358);
  add(400, 406); add(450, 456); add(499, 511); add(550, 561); add(600, 609);
  add(650, 657); add(700, 707); add(750, 757); add(1050, 1057); add(850, 853);
  add(900, 952); add(998, 1009); add(1148, 1160); add(1200, 1211);
  return ids;
}

// ---------------------------------------------------------------------------
// Step 2: levels ('l\x9fVl' CompressedBoard resources)
// ---------------------------------------------------------------------------
//
// Resource layout (verified against everything.p DecompressBoard and
// dataIntf.p CompressedBoard; Think Pascal record, big-endian):
//   name        Str255 (256 bytes: length byte + 255)
//   sizeX,sizeY i16
//   murphy,term i16 (packed points: x*256+y, 1-based)
//   numScissors, numQuarks, numInfotrons, numBugs, numComp  i16
//   theBoard    i16[]  - int stream, 0-based indices:
//     indices 0 .. S+Q+B-1   entity locations (scissors, quarks, bugs; packed points)
//     index S+Q+B            SKIPPED by the shipped reader (the RLE while-loop
//                            pre-increments atbn before reading)
//     indices S+Q+B+1 .. numComp   RLE stream: v > 0 -> literal cell (v becomes
//                            lastVal); v < 0 -> repeat lastVal (-v - 1) more
//                            times. lastVal starts 128. Cells fill row-major,
//                            x fastest, into a board pre-filled with 128.
// FixSize then trims sizeX while board[sizeX][2] = 128 and sizeY while
// board[2][sizeY] = 128 (probing row/column 2).

function decodeLevel(data) {
  const nameLen = data[0];
  const name = macRomanToString(data.subarray(1, 1 + Math.min(nameLen, 255)));
  let o = 256;
  const i16 = () => { const v = data.readInt16BE(o); o += 2; return v; };
  const sizeX0 = i16(), sizeY0 = i16();
  const murphy = i16(), term = i16();
  const numScissors = i16(), numQuarks = i16(), numInfotrons = i16(), numBugs = i16();
  const numComp = i16();
  const streamStart = o;
  const intAt = (idx) => data.readInt16BE(streamStart + idx * 2);
  const availInts = (data.length - streamStart) >> 1;

  const unpack = (v) => ({ x: (v / 256) | 0, y: ((v % 256) + 256) % 256 });
  let atbn = 0;
  const readEntities = (n) => {
    const out = [];
    for (let c = 0; c < n; c++) out.push(unpack(intAt(atbn++)));
    return out;
  };
  const scissors = readEntities(numScissors);
  const quarks = readEntities(numQuarks);
  const bugs = readEntities(numBugs);
  const skippedInt = atbn < availInts && atbn < numComp ? intAt(atbn) : null;

  // Board is 1-based [x][y] up to 120x120, pre-filled with 128.
  const W = 120, H = 120;
  const board = Array.from({ length: W + 1 }, () => new Array(H + 1).fill(128));
  let atx = 1, aty = 1;
  const put = (v) => {
    if (atx <= W && aty <= H) board[atx][aty] = v;
    atx++;
    if (atx > sizeX0) { aty++; atx = 1; }
  };
  let lastVal = 128;
  let cellsWritten = 0;
  while (atbn < numComp) {
    atbn++; // pre-increment, exactly like the shipped reader
    if (atbn >= availInts) break;
    const v = intAt(atbn);
    if (v < 0) {
      for (let q = 1; q <= -v - 1; q++) { put(lastVal); cellsWritten++; }
    } else {
      put(v); lastVal = v; cellsWritten++;
    }
  }

  // FixSize
  let sizeX = sizeX0, sizeY = sizeY0;
  while (sizeX > 1 && board[sizeX][2] === 128) sizeX--;
  while (sizeY > 1 && board[2][sizeY] === 128) sizeY--;

  // Emit rows: rows[y-1][x-1] = board[x][y]
  const rows = [];
  for (let y = 1; y <= sizeY; y++) {
    const row = new Array(sizeX);
    for (let x = 1; x <= sizeX; x++) row[x - 1] = board[x][y];
    rows.push(row);
  }

  return {
    name,
    sizeX, sizeY,
    rawSizeX: sizeX0, rawSizeY: sizeY0,
    murphy: unpack(murphy), term: unpack(term),
    numScissors, numQuarks, numInfotrons, numBugs,
    scissors, quarks, bugs,
    numComp, cellsWritten, skippedInt, availInts,
    board: rows,
  };
}

function extractLevels(resourcesByFile) {
  const levelRes = resourcesByFile
    .get('Info Datafiles/Infotron Levels')
    .filter((r) => r.typeBytes[0] === 0x6c && r.typeBytes[2] === 0x56 && r.typeBytes[3] === 0x6c) // l?Vl
    .sort((a, b) => a.id - b.id);
  if (levelRes.length === 0) throw new Error("No 'l?Vl' level resources found");

  const dir = path.join(OUT, 'levels');
  fs.mkdirSync(dir, { recursive: true });

  const index = [];
  const decoded = [];
  for (const r of levelRes) {
    const lv = decodeLevel(r.data);
    const levelNumber = r.id - 301 + 1; // level N <-> resource N+301
    const nnn = String(levelNumber).padStart(3, '0');
    const json = {
      levelNumber,
      resourceId: r.id,
      resourceName: r.name,
      name: lv.name,
      // Coordinates are 1-based (original engine convention).
      // board[y-1][x-1] = cell at (x, y); cell values are icl8 icon IDs.
      coordinates: '1-based; board[y-1][x-1] = cell (x,y)',
      sizeX: lv.sizeX,
      sizeY: lv.sizeY,
      murphy: lv.murphy,
      term: lv.term,
      infotronQuota: lv.numInfotrons,
      scissors: lv.scissors,
      quarks: lv.quarks,
      bugs: lv.bugs,
      board: lv.board,
    };
    fs.writeFileSync(path.join(dir, `${nnn}.json`), JSON.stringify(json));
    index.push({
      levelNumber,
      resourceId: r.id,
      file: `${nnn}.json`,
      name: lv.name,
      quota: lv.numInfotrons,
      sizeX: lv.sizeX,
      sizeY: lv.sizeY,
      scissors: lv.numScissors,
      quarks: lv.numQuarks,
      bugs: lv.numBugs,
    });
    decoded.push({ ...lv, levelNumber, resourceId: r.id, resourceName: r.name });
  }
  fs.writeFileSync(path.join(dir, 'index.json'), JSON.stringify(index, null, 2));
  log(`levels: wrote ${levelRes.length} level JSONs`);
  return decoded;
}

// ---------------------------------------------------------------------------
// Step 3: sounds ('snd ' -> WAV)
// ---------------------------------------------------------------------------

function decodeSnd(data) {
  const format = data.readUInt16BE(0);
  let o;
  if (format === 1) {
    const nMods = data.readUInt16BE(2);
    o = 4 + nMods * 6; // {modNumber u16, modInit u32} per modifier
  } else if (format === 2) {
    o = 4; // format u16, refCount u16
  } else {
    throw new Error(`unknown 'snd ' format ${format}`);
  }
  const nCmds = data.readUInt16BE(o);
  o += 2;
  let headerOff = null;
  for (let c = 0; c < nCmds; c++) {
    const cmd = data.readUInt16BE(o) & 0x7fff; // strip dataOffsetFlag
    const param2 = data.readUInt32BE(o + 4);
    // bufferCmd = 81, soundCmd = 80: param2 = offset of SampledSoundHeader
    if (cmd === 81 || cmd === 80) headerOff = param2;
    o += 8;
  }
  if (headerOff === null) throw new Error("no bufferCmd/soundCmd in 'snd '");

  const h = headerOff;
  const sampleRate = data.readUInt32BE(h + 8) / 65536; // Fixed 16.16
  const encode = data[h + 20];
  if (encode === 0x00) {
    // stdSH: 8-bit unsigned mono; u32 at h+4 is the byte length.
    const length = data.readUInt32BE(h + 4);
    const samples = data.subarray(h + 22, h + 22 + length);
    if (samples.length < length) throw new Error(`sample data truncated (${samples.length}/${length})`);
    return { format, sampleRate, samples, channels: 1, bitsPerSample: 8 };
  }
  if (encode === 0xff) {
    // extSH: u32 at h+4 is numChannels; numFrames u32 at h+22;
    // sampleSize u16 at h+48; sample data starts at h+64.
    const channels = data.readUInt32BE(h + 4);
    const numFrames = data.readUInt32BE(h + 22);
    const bitsPerSample = data.readUInt16BE(h + 48);
    if (bitsPerSample !== 8 && bitsPerSample !== 16) {
      throw new Error(`unsupported extSH sampleSize ${bitsPerSample}`);
    }
    const byteLen = numFrames * channels * (bitsPerSample / 8);
    const samples = data.subarray(h + 64, h + 64 + byteLen);
    if (samples.length < byteLen) throw new Error(`sample data truncated (${samples.length}/${byteLen})`);
    return { format, sampleRate, samples, channels, bitsPerSample };
  }
  throw new Error(`unsupported sample encoding 0x${encode.toString(16)} (compressed?)`);
}

function writeWav(filePath, samples, sampleRate, channels = 1, bitsPerSample = 8) {
  const rate = Math.round(sampleRate);
  let payload = samples;
  if (bitsPerSample === 16) {
    // Mac 'snd ' 16-bit samples are signed big-endian; WAV wants little-endian.
    payload = Buffer.from(samples);
    payload.swap16();
  }
  const blockAlign = channels * (bitsPerSample / 8);
  const header = Buffer.alloc(44);
  header.write('RIFF', 0);
  header.writeUInt32LE(36 + payload.length, 4);
  header.write('WAVE', 8);
  header.write('fmt ', 12);
  header.writeUInt32LE(16, 16); // fmt chunk size
  header.writeUInt16LE(1, 20); // PCM
  header.writeUInt16LE(channels, 22);
  header.writeUInt32LE(rate, 24);
  header.writeUInt32LE(rate * blockAlign, 28); // byte rate
  header.writeUInt16LE(blockAlign, 32);
  header.writeUInt16LE(bitsPerSample, 34);
  header.write('data', 36);
  header.writeUInt32LE(payload.length, 40);
  fs.writeFileSync(filePath, Buffer.concat([header, payload]));
}

function extractSounds(resourcesByFile, sourceKey, outSubdir) {
  const dir = path.join(OUT, outSubdir);
  fs.mkdirSync(dir, { recursive: true });
  const table = [];
  for (const r of resourcesByFile.get(sourceKey).filter((r) => r.type === 'snd ')) {
    try {
      const { format, sampleRate, samples, channels, bitsPerSample } = decodeSnd(r.data);
      writeWav(path.join(dir, `${r.id}.wav`), samples, sampleRate, channels, bitsPerSample);
      const frames = samples.length / (channels * (bitsPerSample / 8));
      table.push({
        id: r.id, name: r.name, format, channels, bitsPerSample,
        sampleRate: Math.round(sampleRate * 100) / 100,
        frames,
        seconds: Math.round((frames / sampleRate) * 100) / 100,
      });
    } catch (e) {
      table.push({ id: r.id, name: r.name, error: e.message });
    }
  }
  table.sort((a, b) => a.id - b.id);
  fs.writeFileSync(path.join(dir, 'index.json'), JSON.stringify(table, null, 2));
  const failed = table.filter((t) => t.error);
  log(`${outSubdir}: wrote ${table.length - failed.length} WAVs` +
      (failed.length ? `, ${failed.length} FAILED: ${failed.map((f) => f.id).join(', ')}` : ''));
  return table;
}

// ---------------------------------------------------------------------------
// Step 4: pictures (PICT -> PNG)
// ---------------------------------------------------------------------------
//
// ImageMagick's PICT coder silently mis-decodes many of these resources
// (17 of 28 come out solid black), and modern macOS `sips` has lost PICT
// support entirely. All 28 shipped PICTs are simple QuickDraw version-2
// pictures containing a single PackBitsRgn (8-bit indexed + color table) or
// DirectBitsRgn (16-bit RGB555) blit, so we decode them natively and only
// fall back to ImageMagick / sips for anything the native decoder rejects.

// Decode a PICT *resource* (no 512-byte file header) to {width, height, rgba}.
function decodePict(d) {
  let o = 0;
  const u8 = () => d[o++];
  const u16 = () => { const v = d.readUInt16BE(o); o += 2; return v; };
  const i16 = () => { const v = d.readInt16BE(o); o += 2; return v; };
  const u32 = () => { const v = d.readUInt32BE(o); o += 4; return v; };
  const rect = () => ({ t: i16(), l: i16(), b: i16(), r: i16() });
  const skipRegion = () => { const size = u16(); o += size - 2; };

  u16(); // picSize (legacy 16-bit; unreliable, ignored)
  const frame = rect();
  const width = frame.r - frame.l;
  const height = frame.b - frame.t;
  if (width <= 0 || height <= 0) throw new Error(`bad picFrame ${width}x${height}`);
  const rgba = Buffer.alloc(width * height * 4, 0xff); // white opaque canvas

  const putPixel = (x, y, r, g, b) => {
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    const p = (y * width + x) * 4;
    rgba[p] = r; rgba[p + 1] = g; rgba[p + 2] = b; rgba[p + 3] = 255;
  };

  // Unpack one PackBits-compressed scanline worth of bytes.
  const unpackRow = (packed) => {
    const out = [];
    let i = 0;
    while (i < packed.length) {
      const c = packed.readInt8(i++);
      if (c >= 0) { for (let k = 0; k <= c; k++) out.push(packed[i++]); }
      else if (c !== -128) { const v = packed[i++]; for (let k = 0; k < 1 - c; k++) out.push(v); }
    }
    return out;
  };

  function bitsOp(direct, hasRegion) {
    if (direct) u32(); // baseAddr (0x000000FF)
    const rowBytesRaw = u16();
    const rowBytes = rowBytesRaw & 0x3fff;
    const isPixMap = (rowBytesRaw & 0x8000) !== 0;
    if (!isPixMap) throw new Error('1-bit BitMap blit not supported');
    const bounds = rect();
    u16(); // pmVersion
    const packType = u16();
    u32(); // packSize
    o += 8; // hRes, vRes
    u16(); // pixelType
    const pixelSize = u16();
    const cmpCount = u16();
    u16(); // cmpSize
    o += 12; // planeBytes, pmTable, pmReserved
    let ct = null;
    if (!direct) {
      u32(); u16(); // ctSeed, ctFlags
      const ctSize = u16();
      ct = new Array(ctSize + 1);
      for (let i = 0; i <= ctSize; i++) {
        u16(); // value (positional tables in practice)
        ct[i] = [u16() >> 8, u16() >> 8, u16() >> 8];
      }
    }
    const src = rect();
    const dst = rect();
    u16(); // transfer mode (srcCopy assumed)
    if (hasRegion) skipRegion();

    const bh = bounds.b - bounds.t;
    const readRowPacked = () => {
      const n = rowBytes > 250 ? u16() : u8();
      const packed = d.subarray(o, o + n);
      o += n;
      return packed;
    };

    // Destination offset of bounds-relative pixel (x,y), in canvas coords.
    const dx0 = dst.l - frame.l + (bounds.l - src.l);
    const dy0 = dst.t - frame.t + (bounds.t - src.t);

    for (let y = 0; y < bh; y++) {
      if (!direct) {
        if (pixelSize !== 8) throw new Error(`indexed pixelSize ${pixelSize} not supported`);
        const row = rowBytes < 8 ? [...d.subarray(o, (o += rowBytes))] : unpackRow(readRowPacked());
        for (let x = 0; x < row.length; x++) {
          const c = ct[row[x]] ?? [0, 0, 0];
          putPixel(dx0 + x, dy0 + y, c[0], c[1], c[2]);
        }
      } else if (packType === 3 && pixelSize === 16) {
        // RLE by 16-bit pixel, RGB555.
        const packed = readRowPacked();
        let i = 0, x = 0;
        const put16 = (v) => {
          const r5 = (v >> 10) & 0x1f, g5 = (v >> 5) & 0x1f, b5 = v & 0x1f;
          putPixel(dx0 + x, dy0 + y, (r5 << 3) | (r5 >> 2), (g5 << 3) | (g5 >> 2), (b5 << 3) | (b5 >> 2));
          x++;
        };
        while (i < packed.length) {
          const c = packed.readInt8(i++);
          if (c >= 0) { for (let k = 0; k <= c; k++) { put16(packed.readUInt16BE(i)); i += 2; } }
          else if (c !== -128) { const v = packed.readUInt16BE(i); i += 2; for (let k = 0; k < 1 - c; k++) put16(v); }
        }
      } else if (packType === 4 && pixelSize === 32) {
        // Byte-RLE, then component planes (A)RGB.
        const row = unpackRow(readRowPacked());
        const w = Math.floor(row.length / cmpCount);
        const base = cmpCount === 4 ? w : 0; // skip alpha/pad plane
        for (let x = 0; x < w; x++) {
          putPixel(dx0 + x, dy0 + y, row[base + x], row[base + w + x], row[base + 2 * w + x]);
        }
      } else {
        throw new Error(`direct packType ${packType}/pixelSize ${pixelSize} not supported`);
      }
    }
  }

  loop: while (o < d.length - 1) {
    if (o & 1) o++; // v2 opcodes are word-aligned
    const op = u16();
    switch (op) {
      case 0x0000: break; // NOP
      case 0x0011: o += 2; break; // VersionOp
      case 0x0c00: o += 24; break; // HeaderOp
      case 0x001e: break; // DefHilite
      case 0x001f: o += 6; break; // OpColor
      case 0x0001: skipRegion(); break; // Clip
      case 0x00a0: o += 2; break; // ShortComment
      case 0x00a1: { u16(); const n = u16(); o += n; break; } // LongComment
      case 0x0098: bitsOp(false, false); break; // PackBitsRect
      case 0x0099: bitsOp(false, true); break; // PackBitsRgn
      case 0x009a: bitsOp(true, false); break; // DirectBitsRect
      case 0x009b: bitsOp(true, true); break; // DirectBitsRgn
      case 0x00ff: break loop; // end of picture
      default:
        throw new Error(`unsupported PICT opcode 0x${op.toString(16).padStart(4, '0')} at ${o - 2}`);
    }
  }
  return { width, height, rgba };
}

async function extractPictures(resourcesByFile) {
  const dir = path.join(OUT, 'pictures');
  const tmp = path.join(CACHE, 'picts');
  fs.mkdirSync(dir, { recursive: true });
  fs.mkdirSync(tmp, { recursive: true });
  const table = [];
  const promises = [];
  for (const r of resourcesByFile.get('Info Datafiles/Infotron Pictures').filter((r) => r.type === 'PICT')) {
    const pictPath = path.join(tmp, `${r.id}.pict`);
    const pngPath = path.join(dir, `${r.id}.png`);
    // A PICT *file* is the resource data with a 512-byte header prepended.
    // (Dumped for reference / external tools even when we decode natively.)
    fs.writeFileSync(pictPath, Buffer.concat([Buffer.alloc(512), r.data]));

    const entry = { id: r.id, name: r.name, bytes: r.data.length };
    let converted = false;
    try {
      const { width, height, rgba } = decodePict(r.data);
      const img = new Jimp(width, height);
      rgba.copy(img.bitmap.data);
      promises.push(img.writeAsync(pngPath));
      entry.converter = 'native';
      entry.width = width;
      entry.height = height;
      converted = true;
    } catch (e) {
      entry.nativeError = e.message;
    }
    if (!converted) {
      // Fallbacks. NOTE: ImageMagick's PICT coder produces solid-black output
      // for many of these resources without erroring, so it is only trusted
      // as a fallback when the native decoder cannot parse the picture.
      try {
        execFileSync(MAGICK, [pictPath, pngPath], { stdio: 'pipe' });
        entry.converter = 'magick';
        converted = true;
      } catch {
        try {
          execFileSync('sips', ['-s', 'format', 'png', pictPath, '--out', pngPath], { stdio: 'pipe' });
          entry.converter = 'sips';
          converted = true;
        } catch { /* fall through */ }
      }
      if (converted) {
        try {
          const idOut = execFileSync(MAGICK, ['identify', '-format', '%w %h', pngPath], { encoding: 'utf8' });
          const [w, h] = idOut.trim().split(' ').map(Number);
          entry.width = w;
          entry.height = h;
        } catch { /* leave dimensions unset */ }
      } else {
        entry.error = `conversion failed (native: ${entry.nativeError}; magick and sips also failed)`;
      }
    }
    table.push(entry);
  }
  await Promise.all(promises);
  table.sort((a, b) => a.id - b.id);
  fs.writeFileSync(path.join(dir, 'index.json'), JSON.stringify(table, null, 2));
  const failed = table.filter((t) => t.error);
  log(`pictures: converted ${table.length - failed.length}/${table.length} PICTs` +
      (failed.length ? `, FAILED: ${failed.map((f) => f.id).join(', ')}` : ''));
  return table;
}

// ---------------------------------------------------------------------------
// Inventory document
// ---------------------------------------------------------------------------

function fmtRange(ids) {
  if (ids.length === 0) return '(none)';
  const parts = [];
  let start = ids[0], prev = ids[0];
  for (let i = 1; i <= ids.length; i++) {
    const v = ids[i];
    if (v === prev + 1) { prev = v; continue; }
    parts.push(start === prev ? `${start}` : `${start}–${prev}`);
    start = prev = v;
  }
  return parts.join(', ');
}

function writeInventory({ resourcesByFile, icons, levels, sounds, music, pictures }) {
  const lines = [];
  const push = (s = '') => lines.push(s);

  push('# Infotron 1.1 — Resource Inventory');
  push();
  push('Generated by `scripts/extract-resources.mjs`. Do not edit by hand;');
  push('re-run `node scripts/build-assets.mjs` (or `pnpm assets`) to regenerate');
  push('this file and everything under `public/assets/`.');
  push();
  push('Source archive: `legacy/infotron/infotron-11.hqx` (BinHex → Compact Pro),');
  push('extracted with `unar -k hidden` into `.assets-cache/` (cached; delete to');
  push('force re-extraction). Resource forks live in AppleDouble `._Name` files.');
  push();

  push('## Resource types per file');
  push();
  for (const [file, resources] of resourcesByFile) {
    const byType = new Map();
    for (const r of resources) {
      if (!byType.has(r.type)) byType.set(r.type, []);
      byType.get(r.type).push(r);
    }
    push(`### ${file}`);
    push();
    push('| Type | Count | IDs | Total bytes |');
    push('|---|---|---|---|');
    for (const [type, list] of byType) {
      const ids = list.map((r) => r.id).sort((a, b) => a - b);
      const bytes = list.reduce((s, r) => s + r.data.length, 0);
      push(`| \`${type}\` | ${list.length} | ${fmtRange(ids)} | ${bytes} |`);
    }
    push();
  }

  push('## icl8 icon inventory vs. the rules-spec legend (§1.2)');
  push();
  push(`icl8 IDs present (${icons.icl8Ids.length}): ${fmtRange(icons.icl8Ids)}`);
  push();
  const legend = specLegendIds();
  const present = new Set(icons.icl8Ids);
  const missing = [...legend].filter((id) => !present.has(id)).sort((a, b) => a - b);
  const extra = icons.icl8Ids.filter((id) => !legend.has(id));
  push(`Legend IDs with NO icl8 graphic (${missing.length}): ${fmtRange(missing)}`);
  push();
  push('(The legend includes classifier *acceptance* ranges wider than the shipped');
  push('artwork — e.g. 139–214 Murphy, 650–657 quark, 700–707 zonk, 900–952 ports —');
  push('so gaps here are expected: those IDs never occur as graphics.)');
  push();
  push(`icl8 IDs NOT in the legend (${extra.length}): ${fmtRange(extra)}`);
  push();
  push(`icl8 IDs without an ICN# mask (exported fully opaque): ${fmtRange(icons.index.idsWithoutMask)}`);
  push();
  push(`ICN#-only IDs (no icl8; not exported): ${fmtRange(icons.index.icnIdsWithoutIcl8)}`);
  push();
  push('Observations:');
  push('- Scissor artwork only exists for the four even (orthogonal) directions:');
  push('  base 1000–1007, open/medium 1010/1012/1014/1016 and 1020/1022/1024/1026,');
  push('  turning 1150/1152/1154/1156. The spec ranges 998–1009/1148–1160 are');
  push('  classifier acceptance ranges, not shipped graphics.');
  push('- Ports ship as 900/902/904/906 (one-way, `icon mod 8` = direction), plus');
  push('  132 (4-way), 950 (vertical), 952 (horizontal). 901–949 odd IDs are unused.');
  push('- Chips ship as 131 + 1100/1102/1104/1105/1106 (1105 = dropped red disk).');
  push('- Extra graphics beyond the legend: 136–137 (app/doc icons), 250 (Murphy');
  push('  extra), 407/457/610/658 (one frame past each animation range — 610 and');
  push('  658 ARE checked by code per spec §18.2), 758–760 (hardware variants used');
  push('  in shipped levels).');
  push('- The game only ever draws `icl8` icons (GetResource(\'icl8\', …) in');
  push('  everything.p); the ICN#-only IDs look like leftovers of an older B/W set,');
  push('  and ICN# masks are used here solely for PNG transparency.');
  push();

  push('## Levels');
  push();
  push("Type `l\\x9fVl` ('l', 0x9F, 'V', 'l'; 0x9F = MacRoman 'ü'), resource ID =");
  push('level number + 300 (level 1 = 301). The game looks levels up by *name*');
  push('(`GetNamedResource`) but derives names from IDs (`ResourceName(N + 301)`).');
  push('Decoded per `DecompressBoard` in `everything.p`, including its quirk of');
  push('skipping one int between the entity list and the RLE stream, then FixSize');
  push('trimming (probe row/col 2 for 128).');
  push();
  push('| # | Res ID | Name | Quota | Size (trimmed) | Raw size | Scissors | Quarks | Bugs |');
  push('|---|---|---|---|---|---|---|---|---|');
  for (const lv of levels) {
    push(`| ${lv.levelNumber} | ${lv.resourceId} | ${lv.name} | ${lv.numInfotrons} | ` +
         `${lv.sizeX}×${lv.sizeY} | ${lv.rawSizeX}×${lv.rawSizeY} | ` +
         `${lv.numScissors} | ${lv.numQuarks} | ${lv.numBugs} |`);
  }
  push();
  const cellVals = new Set();
  for (const lv of levels) for (const row of lv.board) for (const v of row) cellVals.add(v);
  const sortedVals = [...cellVals].sort((a, b) => a - b);
  push(`Distinct cell values used across all 51 boards: ${fmtRange(sortedVals)}`);
  push();
  push('Notes:');
  push('- Every board decodes with Murphy walk icon **198** stored at the exact');
  push('  `murphy` header position (a strong end-to-end validation of the decoder).');
  push('- Levels use hardware variants **758–760**, which are *outside* the');
  push('  classifier’s 750–757 hardware range — they classify as the inert generic');
  push('  piece, which is behaviorally identical (indestructible, not rounded).');
  push('- Level 31 “Seeing Yellow” really is 120×120: its designer painted the full');
  push('  board, and FixSize cannot trim because row/col 2 end in empty (129), not');
  push('  base (128).');
  push('- The raw (pre-FixSize) size is 120×120 for every level; the editor always');
  push('  saved full boards and the engine trims at load.');
  push('- Level 31 is also the only level whose border ring is not fully hardware:');
  push('  its extreme bottom-right corner cell (120,120) is base 128 (destructible');
  push('  by an adjacent explosion). Murphy can never occupy a corner, but a port');
  push('  that reads out of bounds instead of trusting borders should know the ring');
  push('  is not literally solid there.');
  push();

  const soundPurpose = {
    128: 'got infotron', 129: 'thud (landing)', 130: 'win-jingle note instrument',
    131: 'explosion', 132: 'quark explosion', 133: 'got/used red disk',
    134: 'red disk ready (held 1.5 s)', 135: 'got LAST infotron', 136: 'about box',
    137: 'Murphy died', 138: 'pause', 139: 'unpause', 140: 'bug zap', 141: 'push',
    143: 'logo', 144: 'help-tutorial UI click', 145: 'help-tutorial UI tick',
    146: 'help-tutorial UI ding', 150: '[1.1] boot Q-logo (binary-only usage)',
    151: '[1.1] end-of-level (binary-only usage, unverified)',
  };
  push('## Sounds');
  push();
  push('| ID | Name | Purpose (spec §13) | Ch×bits | Rate (Hz) | Frames | Duration (s) |');
  push('|---|---|---|---|---|---|---|');
  for (const s of sounds) {
    push(`| ${s.id} | ${s.name ?? ''} | ${soundPurpose[s.id] ?? ''} | ${s.error ? `ERROR: ${s.error}` : `${s.channels}×${s.bitsPerSample}`} | ` +
         `${s.sampleRate ?? ''} | ${s.frames ?? ''} | ${s.seconds ?? ''} |`);
  }
  push();
  push('Missing vs. spec §13: IDs **130** (win-jingle note instrument), **136**');
  push('(about-box) and **143** (logo) are referenced by the code but absent from');
  push('the shipped v1.1 Sounds file (GetResource returns nil → the game plays');
  push('nothing). The app binary carries no \'snd \' resources of its own.');
  push('IDs 144–151 (Click, Tick, Ding!, QLogoSnd, endLevel) are shipped but were');
  push('not listed in that spec; see graphics-infotron.md §8 and');
  push('app-flow-infotron.md §11.1 for their verified/presumed triggers.');
  push();
  push('## Music');
  push();
  push('| ID | Name | Ch×bits | Rate (Hz) | Frames | Duration (s) |');
  push('|---|---|---|---|---|---|');
  for (const s of music) {
    push(`| ${s.id} | ${s.name ?? ''} | ${s.error ? `ERROR: ${s.error}` : `${s.channels}×${s.bitsPerSample}`} | ` +
         `${s.sampleRate ?? ''} | ${s.frames ?? ''} | ${s.seconds ?? ''} |`);
  }
  push();
  push('(Spec §13 said 3 music tracks 1000–1002; the shipped 1.1 archive contains 2:');
  push('1000 and 1001.)');
  push();

  // Usage per app-flow-infotron.md §4.1 (verified against InfoWindows.p/main.p).
  const pictPurpose = {
    128: 'startup shareware-notice art in DLOG 2000 — "Q Software" dog-on-hydrant logo, 16-bit',
    129: 'in-game status bar / HUD strip ("Infotron™" + counters)',
    130: 'Choose Level dialog background (Choose / Skip / Cancel)',
    131: 'Open Player dialog background (OK / Cancel + list box)',
    132: 'Set Keys dialog background (gray machine frame)',
    133: 'New Player name dialog background (OK / Cancel)',
    134: 'Rename Player dialog background (variant of 133)',
    135: 'scroll-bar thumb ("piece"), 12×12',
    136: 'scroll-bar thumb eraser ("blank"), 12×12',
    137: 'error banner window background (sad-Murphy strip, WIND 137)',
    138: 'scroll-bar thumb while dragging, 12×12',
    139: 'suspended-app 55×55 mini-window badge',
    143: 'Help part 1: "THE CAST" piece-gallery background (icons composited at runtime)',
    144: 'Help part 2: "INFOTRON TUTORIAL" background',
    145: 'Delete Player dialog background (same frame as 131)',
    146: '"Are you sure?" yes/no dialog background',
    149: 'About box background',
    150: 'About: scrolling credits column #1 (shareware-pitch text)',
    151: 'About: scrolling credits column #2',
    3000: 'boot splash / title screen, 640×480',
  };
  for (let i = 500; i <= 507; i++) {
    pictPurpose[i] = '[1.1] Set Keys key-slot highlight overlay, 27×18 (replaces 1.0 PICTs 303–308; binary-only usage)';
  }
  push('## Pictures (PICT)');
  push();
  push('All PICTs are QuickDraw v2, one PackBitsRgn (8-bit indexed) or DirectBitsRgn');
  push('(16-bit RGB555) blit, decoded natively — ImageMagick mis-decodes 17 of the 28');
  push('to solid black without erroring, and macOS `sips` no longer reads PICT.');
  push();
  push('| ID | Name | Bytes | Size (px) | Converter | Purpose (app-flow spec §4.1) |');
  push('|---|---|---|---|---|---|');
  for (const p of pictures) {
    const dim = p.width ? `${p.width}×${p.height}` : (p.error ?? '?');
    let guess = pictPurpose[p.id] ?? '';
    if (!guess && p.name) guess = `“${p.name}”`;
    push(`| ${p.id} | ${p.name ?? ''} | ${p.bytes} | ${dim} | ${p.converter ?? 'FAILED'} | ${guess} |`);
  }
  push();

  push('## Re-running the pipeline');
  push();
  push('```sh');
  push('cd web-infotron');
  push('pnpm assets            # = node scripts/build-assets.mjs');
  push('# or directly:');
  push('node scripts/extract-resources.mjs');
  push('```');
  push();
  push('Requires: `unar` (`brew install unar`), ImageMagick at');
  push('`/opt/homebrew/bin/magick` (with `sips` as fallback), and the repo dev');
  push('dependencies (`pnpm install`; uses `jimp`). Outputs go to `public/assets/`;');
  push('the extraction cache lives in `.assets-cache/` at the web-infotron root.');
  push();

  fs.mkdirSync(DOCS, { recursive: true });
  fs.writeFileSync(path.join(DOCS, 'resources-inventory.md'), lines.join('\n'));
  log(`inventory: wrote docs/original-specs/resources-inventory.md`);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

function log(...args) {
  console.log('[assets]', ...args);
}

export async function main() {
  const folder = ensureExtracted();

  const files = [
    'Info Datafiles/Infotron Levels',
    'Info Datafiles/Infotron Music',
    'Info Datafiles/Infotron Pictures',
    'Info Datafiles/Infotron Pieces',
    'Info Datafiles/Infotron Players',
    'Info Datafiles/Infotron Sounds',
  ];
  const resourcesByFile = new Map();
  for (const f of files) {
    const p = path.join(folder, path.dirname(f), `._${path.basename(f)}`);
    resourcesByFile.set(f, parseResources(readResourceFork(p)));
  }
  // Extra forks, parsed for the inventory only (not used for asset output):
  // the app binary and the older icon-editing copy of the Pieces file.
  for (const f of ['Infotron™ 1.1', 'Pieces/Infotron Pieces']) {
    try {
      const p = path.join(folder, path.dirname(f), `._${path.basename(f)}`);
      resourcesByFile.set(`${f} (inventory only)`, parseResources(readResourceFork(p)));
    } catch { /* optional */ }
  }

  fs.mkdirSync(OUT, { recursive: true });
  const icons = await extractIcons(resourcesByFile);
  const levels = extractLevels(resourcesByFile);
  const sounds = extractSounds(resourcesByFile, 'Info Datafiles/Infotron Sounds', 'sounds');
  const music = extractSounds(resourcesByFile, 'Info Datafiles/Infotron Music', 'music');
  const pictures = await extractPictures(resourcesByFile);
  writeInventory({ resourcesByFile, icons, levels, sounds, music, pictures });
  log('done.');
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main().catch((e) => {
    console.error(e);
    process.exit(1);
  });
}
