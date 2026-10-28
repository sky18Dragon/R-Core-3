// Browser-side pre-check of a Read Pico local-frame firmware image. Mirrors
// main/ota_local.c so an obviously wrong file is refused before upload; the
// device repeats every check and verifies the full image hash.
export const HEADER_BYTES = 112;
const CHIP_ID_ESP32S3 = 9;
const APP_DESC_MAGIC = 0xabcd5432;
const PROJECT_NAME = "esp32-photoframe";
const BOARD_MARKER = "+rplf";

function cString(bytes, offset, length) {
  const slice = bytes.subarray(offset, offset + length);
  const end = slice.indexOf(0);
  if (end < 0) return null; // unterminated
  return new TextDecoder().decode(slice.subarray(0, end));
}

// Returns { ok: true, version } or { ok: false, reason } where reason is an
// English message usable as a t() key.
export function inspectFirmware(bytes, totalSize, slotSize) {
  if (!bytes || bytes.length < HEADER_BYTES || totalSize < HEADER_BYTES)
    return { ok: false, reason: "Not a firmware image" };
  if (slotSize && totalSize > slotSize)
    return { ok: false, reason: "Firmware is larger than the update slot" };
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  if (bytes[0] !== 0xe9) return { ok: false, reason: "Not a firmware image" };
  if (view.getUint16(12, true) !== CHIP_ID_ESP32S3)
    return { ok: false, reason: "Firmware is not for ESP32-S3" };
  if (view.getUint32(32, true) !== APP_DESC_MAGIC)
    return { ok: false, reason: "Not a firmware image" };
  if (cString(bytes, 80, 32) !== PROJECT_NAME)
    return { ok: false, reason: "Firmware is not for this photo frame" };
  const version = cString(bytes, 48, 32);
  if (!version || !version.endsWith(BOARD_MARKER))
    return { ok: false, reason: "Firmware is not a Read Pico local-frame build" };
  return { ok: true, version };
}
