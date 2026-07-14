// Progress: current level and the highest level reached.

const KEY = "infotron.progress";

interface SaveData {
  current: number;
  highest: number;
}

function load(): SaveData {
  try {
    const d = JSON.parse(localStorage.getItem(KEY) ?? "{}");
    return {
      current: typeof d.current === "number" ? d.current : 1,
      highest: typeof d.highest === "number" ? d.highest : 1,
    };
  } catch {
    return { current: 1, highest: 1 };
  }
}

function store(d: SaveData): void {
  try {
    localStorage.setItem(KEY, JSON.stringify(d));
  } catch {
    // session-only
  }
}

export function progress(): SaveData {
  return load();
}

export function setCurrentLevel(n: number): void {
  const d = load();
  d.current = n;
  if (n > d.highest) d.highest = n;
  store(d);
}
