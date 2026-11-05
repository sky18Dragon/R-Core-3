// Parameter tuning by elimination: every candidate setting gets a letter and is
// shown full screen on the frame in turn; the viewer drops the letters they do
// not want, the rest play again, and so on until four are left for a 2×2
// side-by-side.

import { PROCESSING_KEYS } from "./compareSheet";

export const LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ".split("");
export const MAX_CANDIDATES = LETTERS.length;
export const FINAL_COUNT = 4; // at or below this, compare on one screen

// Variations tried around the base, as changes to it: brightness, contrast,
// tone curve, colour matching, dithering and dynamic-range compression.
export const STARTER_PATCHES = [
  { exposure: 0.9 },
  { exposure: 1.1 },
  { toneMode: "contrast", contrast: 0.85 },
  { toneMode: "contrast", contrast: 1.25 },
  { toneMode: "scurve", strength: 0.5, shadowBoost: 0.1, highlightCompress: 1.0, midpoint: 0.5 },
  { toneMode: "scurve", strength: 0.9, shadowBoost: 0.3, highlightCompress: 2.0, midpoint: 0.5 },
  { colorMethod: "lab" },
  { colorMethod: "lab", toneMode: "contrast", contrast: 1.1 },
  { ditherAlgorithm: "floyd-steinberg" },
  { ditherAlgorithm: "stucki" },
  { ditherAlgorithm: "sierra" },
  { ditherAlgorithm: "burkes" },
  { compressDynamicRange: "toggle" },
];

function sameValue(a, b) {
  return typeof a === "number" && typeof b === "number" ? Math.abs(a - b) <= 1e-3 : a === b;
}

export function sameProcessing(a, b) {
  return PROCESSING_KEYS.every((key) => sameValue(a[key], b[key]));
}

export function applyPatch(base, patch) {
  const params = { ...base };
  for (const [key, value] of Object.entries(patch)) {
    params[key] = value === "toggle" ? !base[key] : value;
  }
  return params;
}

// Letters follow list order: A, B, C… (re-lettered whenever the list changes
// before the first round, so there are never gaps).
export function reletter(candidates) {
  return candidates.map((c, i) => ({ ...c, letter: LETTERS[i] }));
}

// Settings to try, in order: the base (A), the user's other presets
// (`others`: [{name, params}]), then variations of the base. Anything that
// would look exactly like an earlier one is skipped. `limit` caps the list.
export function startList(base, baseName, others = [], limit = 16) {
  const list = [{ params: { ...base }, name: baseName ?? null, isBase: true }];
  const push = (params, name) => {
    if (list.length >= limit || list.some((c) => sameProcessing(c.params, params))) return;
    list.push({ params, name, isBase: false });
  };
  for (const other of others) push({ ...base, ...other.params }, other.name);
  for (const patch of STARTER_PATCHES) push(applyPatch(base, patch), null);
  return list;
}

export function starterCandidates(base, baseName, others = []) {
  return reletter(startList(base, baseName, others, 16));
}

// What sets a candidate apart from the default, as [{key, value}] for the
// keys that matter for its tone mode (and not saturation on a gray panel).
export function describeDiff(params, base, { grayscale = false } = {}) {
  const scurveKeys = ["strength", "shadowBoost", "highlightCompress", "midpoint"];
  // Switching to the S-curve makes all its values new, even ones equal to the
  // (unused) values stored with the default.
  const modeChanged = params.toneMode !== base.toneMode;
  const out = [];
  for (const key of PROCESSING_KEYS) {
    if (grayscale && key === "saturation") continue;
    if (params.toneMode === "scurve" && key === "contrast") continue;
    if (params.toneMode !== "scurve" && scurveKeys.includes(key)) continue;
    if (!sameValue(params[key], base[key]) || (modeChanged && scurveKeys.includes(key))) {
      out.push({ key, value: params[key] });
    }
  }
  return out;
}

// Rounds: `history` is the list of pools (letters shown in each round), the
// last one being the round in progress; `dropped` the letters struck out in
// it. The next round is what is left; null when that would be nothing, or the
// same round again.
export function remainingLetters(history, dropped) {
  const pool = history[history.length - 1] ?? [];
  return pool.filter((letter) => !dropped.includes(letter));
}

export function nextRound(history, dropped) {
  const pool = history[history.length - 1] ?? [];
  const next = remainingLetters(history, dropped);
  if (next.length === 0 || next.length === pool.length) return null;
  return [...history, next];
}

export function previousRound(history) {
  return history.length > 1 ? history.slice(0, -1) : history;
}

export function toggleLetter(letters, letter) {
  return letters.includes(letter) ? letters.filter((l) => l !== letter) : [...letters, letter];
}
