// Pure helpers for the photo-transfer hotspot UI (firmware: /api/hotspot).
// Rules mirror main/hotspot_policy.c so the page rejects what the frame would.

export const HOTSPOT_URL = "http://192.168.4.1";
// Device clock further off than this gets a "use this phone's time" prompt.
export const CLOCK_DRIFT_LIMIT_S = 120;

// "9:05" for 545 s; null when no automatic close is scheduled (-1 or missing).
export function formatRemaining(seconds) {
  if (typeof seconds !== "number" || seconds < 0) return null;
  const s = Math.ceil(seconds);
  return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, "0")}`;
}

// Auto-close choices in minutes; 0 (never) only where the frame allows it.
export function idleChoices(neverAllowed) {
  return neverAllowed ? [10, 30, 60, 0] : [10, 30, 60];
}

// WPA2 passphrase: 8..63 printable ASCII characters.
export function hotspotPasswordValid(password) {
  return typeof password === "string" && /^[\x20-\x7e]{8,63}$/.test(password);
}

// 1..32 UTF-8 bytes without control characters.
export function hotspotSsidValid(name) {
  if (typeof name !== "string" || /[\x00-\x1f\x7f]/.test(name)) return false;
  const bytes = new TextEncoder().encode(name).length;
  return bytes >= 1 && bytes <= 32;
}

// Seconds the frame's clock is ahead (+) or behind (-) this browser.
export function clockDriftSeconds(deviceTimestamp, nowMs = Date.now()) {
  if (typeof deviceTimestamp !== "number" || !Number.isFinite(deviceTimestamp)) return null;
  return Math.round(deviceTimestamp - nowMs / 1000);
}

export function clockNeedsSync(driftSeconds) {
  return typeof driftSeconds === "number" && Math.abs(driftSeconds) > CLOCK_DRIFT_LIMIT_S;
}
