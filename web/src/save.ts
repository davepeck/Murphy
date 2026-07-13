// Progress persistence (the original saved player profiles in a resource file;
// we save after every level completion instead of only at app exit).

const KEY = "murphy.progress";

interface SaveData {
  passed: string[];
}

function load(): SaveData {
  try {
    const d = JSON.parse(localStorage.getItem(KEY) ?? "{}");
    return { passed: Array.isArray(d.passed) ? d.passed : [] };
  } catch {
    return { passed: [] };
  }
}

export function passedLevels(): Set<string> {
  return new Set(load().passed);
}

export function markPassed(file: string): void {
  const d = load();
  if (!d.passed.includes(file)) {
    d.passed.push(file);
    localStorage.setItem(KEY, JSON.stringify(d));
  }
}
