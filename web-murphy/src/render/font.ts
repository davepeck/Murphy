// The original bitmap lettering (MRLettering / CalculateLetterRect):
// font sheets are 3 columns x 26 rows of 17x17 glyph cells in a 1-px grid;
// glyphs are stored rotated 90°, and text is drawn as a vertical run (reads
// sideways, like a book spine). Advance is measured per glyph: the first
// all-black row inside the cell marks the glyph's extent.

const CELL = 17;
const GRID = 18; // cell + 1px grid line
const DEFAULT_ADVANCE = 12; // kLetterBoxSize - 5

const PUNCT = "!@#$%^&*()'\".,-";

function glyphCell(ch: string): { col: number; row: number } | null {
  const c = ch.charCodeAt(0);
  if (c >= 65 && c <= 90) return { col: 2, row: c - 65 }; // A-Z
  if (c >= 97 && c <= 122) return { col: 1, row: c - 97 }; // a-z
  if (c >= 49 && c <= 57) return { col: 0, row: c - 49 }; // 1-9
  if (ch === "0") return { col: 0, row: 9 };
  const p = PUNCT.indexOf(ch);
  if (p >= 0) return { col: 0, row: 10 + p };
  return null; // unknown / space: black cell
}

export class BitmapFont {
  private advances = new Map<string, number>();

  constructor(private sheet: HTMLImageElement) {
    // Measure advances once, like MRCalcOffset.
    const cv = document.createElement("canvas");
    cv.width = sheet.width;
    cv.height = sheet.height;
    const cx = cv.getContext("2d");
    if (!cx) return;
    cx.drawImage(sheet, 0, 0);
    const data = cx.getImageData(0, 0, cv.width, cv.height).data;
    const chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" + PUNCT;
    for (const ch of chars) {
      const cell = glyphCell(ch);
      if (!cell) continue;
      const x0 = cell.col * GRID + 1;
      const y0 = cell.row * GRID + 1;
      let advance = DEFAULT_ADVANCE;
      for (let r = 0; r < CELL; r++) {
        let allBlack = true;
        for (let c = 1; c < CELL; c++) {
          const i = ((y0 + r) * cv.width + (x0 + c)) * 4;
          if ((data[i] ?? 0) > 24 || (data[i + 1] ?? 0) > 24 || (data[i + 2] ?? 0) > 24) {
            allBlack = false;
            break;
          }
        }
        if (allBlack) {
          advance = Math.min(r + 2, CELL);
          break;
        }
      }
      this.advances.set(ch, advance);
    }
  }

  /** Draws text as a vertical run starting at (x, y); returns the end y. */
  drawVertical(g: CanvasRenderingContext2D, text: string, x: number, y: number, maxY = Infinity): number {
    for (const ch of text) {
      if (y + CELL > maxY) break;
      const cell = glyphCell(ch);
      if (cell) {
        g.drawImage(this.sheet, cell.col * GRID + 1, cell.row * GRID + 1, CELL, CELL, x, y, CELL, CELL);
        y += this.advances.get(ch) ?? DEFAULT_ADVANCE;
      } else {
        y += DEFAULT_ADVANCE; // space / unknown
      }
    }
    return y;
  }
}
