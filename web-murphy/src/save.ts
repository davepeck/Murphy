// Progress persistence (the original saved player profiles in a resource file;
// we save after every change instead of only at app exit).

const KEY = "murphy.progress";

interface SaveData {
  passed: string[];
  /** map switches currently toggled (parity list, like the original theThrows) */
  throws: number[];
  /** last map position, [x, y]; null = use the world's start marker */
  mapLocation: [number, number] | null;
}

function load(): SaveData {
  try {
    const d = JSON.parse(localStorage.getItem(KEY) ?? "{}");
    return {
      passed: Array.isArray(d.passed) ? d.passed : [],
      throws: Array.isArray(d.throws) ? d.throws : [],
      mapLocation:
        Array.isArray(d.mapLocation) && d.mapLocation.length === 2 ? d.mapLocation : null,
    };
  } catch {
    return { passed: [], throws: [], mapLocation: null };
  }
}

function store(d: SaveData): void {
  try {
    localStorage.setItem(KEY, JSON.stringify(d));
  } catch {
    // storage unavailable: progress is session-only
  }
}

export function passedLevels(): Set<string> {
  return new Set(load().passed);
}

export function markPassed(file: string): void {
  const d = load();
  if (!d.passed.includes(file)) {
    d.passed.push(file);
    store(d);
  }
}

export function thrownSwitches(): Set<number> {
  return new Set(load().throws);
}

/** Toggle a switch's persisted state (re-throwing removes it, like the original). */
export function toggleThrow(trig: number): void {
  const d = load();
  const i = d.throws.indexOf(trig);
  if (i >= 0) d.throws.splice(i, 1);
  else d.throws.push(trig);
  store(d);
}

export function mapLocation(): [number, number] | null {
  return load().mapLocation;
}

export function setMapLocation(loc: [number, number]): void {
  const d = load();
  d.mapLocation = loc;
  store(d);
}
