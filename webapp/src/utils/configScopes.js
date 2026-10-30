// Which device-config fields belong to which settings panel, and the diff used
// both to decide what a save sends and to tell whether a panel has unsaved edits.

// Image settings (next to the album): the slideshow. Every other config field is
// system settings (the top-right drawer).
export const IMAGE_CONFIG_KEYS = ["auto_rotate", "rotate_cron", "sd_rotation_mode"];

export function keysForScope(allKeys, scope) {
  if (scope === "image") return allKeys.filter((k) => IMAGE_CONFIG_KEYS.includes(k));
  if (scope === "system") return allKeys.filter((k) => !IMAGE_CONFIG_KEYS.includes(k));
  return allKeys;
}

// Arrays (rotate_cron) need a value comparison, not reference equality.
export function valuesDiffer(current, original) {
  return Array.isArray(current) || Array.isArray(original)
    ? JSON.stringify(current) !== JSON.stringify(original)
    : current !== original;
}

// Fields of `current` (restricted to `scope`) that differ from `original`.
export function diffConfig(current, original, scope = "all") {
  const changed = {};
  for (const key of keysForScope(Object.keys(current), scope)) {
    if (valuesDiffer(current[key], original?.[key])) changed[key] = current[key];
  }
  return changed;
}
