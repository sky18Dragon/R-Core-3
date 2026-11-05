// The user's processing presets (docs/PRD-PRESETS.md): named sets of image
// processing values, kept on the frame as one document
//   {version, rev, default: id | null, presets: [{id, name, source, params, created, updated}]}
// These helpers change a copy of the document; the store sends it.

import { PROCESSING_KEYS } from "./compareSheet";

export const MAX_PRESETS = 20;
export const NAME_MAX_BYTES = 64;

export function emptyDoc() {
  return { version: 1, rev: 0, default: null, presets: [] };
}

// Only the processing values (no layout), the shape a preset stores
export function processingOf(params) {
  const out = {};
  for (const key of PROCESSING_KEYS) if (params[key] !== undefined) out[key] = params[key];
  return out;
}

export function nameFits(name) {
  const trimmed = String(name ?? "").trim();
  return trimmed.length > 0 && new TextEncoder().encode(trimmed).length <= NAME_MAX_BYTES;
}

export function newId(now = Date.now(), random = Math.random) {
  return `p${now.toString(36)}${Math.floor(random() * 36 ** 4)
    .toString(36)
    .padStart(4, "0")}`;
}

function seconds(now) {
  return Math.floor(now / 1000);
}

export function presetById(doc, id) {
  return doc.presets.find((p) => p.id === id) ?? null;
}

// Adds a preset; returns [doc, id] (doc unchanged and id null when full or unnamed)
export function addPreset(doc, { name, params, source = "manual" }, { now = Date.now(), id } = {}) {
  if (doc.presets.length >= MAX_PRESETS || !nameFits(name)) return [doc, null];
  const presetId = id ?? newId(now);
  const preset = {
    id: presetId,
    name: name.trim(),
    source,
    params: processingOf(params),
    created: seconds(now),
    updated: seconds(now),
  };
  return [{ ...doc, presets: [...doc.presets, preset] }, presetId];
}

export function updatePreset(doc, id, { name, params }, { now = Date.now() } = {}) {
  if (name !== undefined && !nameFits(name)) return doc;
  return {
    ...doc,
    presets: doc.presets.map((p) =>
      p.id === id
        ? {
            ...p,
            ...(name !== undefined ? { name: name.trim() } : {}),
            ...(params !== undefined ? { params: processingOf(params) } : {}),
            updated: seconds(now),
          }
        : p
    ),
  };
}

export function duplicateName(doc, name, copyWord) {
  const taken = new Set(doc.presets.map((p) => p.name));
  for (let n = 1; ; n++) {
    const candidate = n === 1 ? `${name} ${copyWord}` : `${name} ${copyWord} ${n}`;
    if (!taken.has(candidate)) return candidate;
  }
}

// The default and the last preset cannot be removed
export function canRemove(doc, id) {
  return doc.presets.length > 1 && doc.default !== id && !!presetById(doc, id);
}

export function removePreset(doc, id) {
  if (!canRemove(doc, id)) return doc;
  return { ...doc, presets: doc.presets.filter((p) => p.id !== id) };
}

export function setDefaultPreset(doc, id) {
  return presetById(doc, id) ? { ...doc, default: id } : doc;
}

// The first preset, made from what the frame already uses, when it has none
export function migratedDoc(doc, params, name, { now = Date.now(), id } = {}) {
  if (doc.presets.length) return doc;
  const [next, presetId] = addPreset(doc, { name, params, source: "migrated" }, { now, id });
  return presetId ? setDefaultPreset(next, presetId) : doc;
}

export function sameProcessingValues(a, b) {
  return PROCESSING_KEYS.every((key) => {
    const x = a?.[key];
    const y = b?.[key];
    if (x === undefined || y === undefined) return true;
    return typeof x === "number" && typeof y === "number" ? Math.abs(x - y) <= 1e-3 : x === y;
  });
}
