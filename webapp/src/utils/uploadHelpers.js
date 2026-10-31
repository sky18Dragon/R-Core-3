// Pure helpers for the batch uploader (no DOM, no network), so they can be unit tested.

// Short unique basename from (source filename + timestamp), so two photos with
// the same name (e.g. IMG_1234.jpg from different sources) don't collide in the
// device album. crypto.subtle is unavailable on the device's plain-HTTP origin,
// so this is a 32-bit FNV-1a hash plus a base-36 millisecond suffix: 12 chars.
export function makeBaseName(fileName, timestamp) {
  const sourceName = String(fileName).replace(/\.[^/.]+$/, "");
  const payload = `${sourceName}:${timestamp}`;
  let fnv = 0x811c9dc5;
  for (let i = 0; i < payload.length; i++) {
    fnv ^= payload.charCodeAt(i);
    fnv = Math.imul(fnv, 0x01000193);
  }
  return (fnv >>> 0).toString(16).padStart(8, "0") + timestamp.toString(36).slice(-4);
}

// Which kind of failure was it? The caller maps the code to a translated message.
// status 0 means the request never got an answer (device off the network, page
// reloaded, Wi-Fi dropped).
export function classifyUploadFailure(status, bodyText = "") {
  const text = String(bodyText).toLowerCase();
  if (status === 0) return "unreachable";
  if (status === 413 || text.includes("oversized")) return "too-large";
  if (status === 415 || text.includes("unsupported") || text.includes("signature mismatch")) {
    return "unsupported";
  }
  if (status === 503 || text.includes("still initializing")) return "busy";
  if (status === 507 || text.includes("no space") || text.includes("storage full")) return "full";
  if (status === 401 || status === 403) return "auth";
  if (status >= 500) return "device";
  return "other";
}

// Failures worth one automatic retry: the device was busy (refreshing the
// panel, initialising), not because of anything wrong with the photo.
export function isRetryableFailure(code) {
  return code === "busy" || code === "device";
}

const IMAGE_NAME = /\.(jpe?g|png|heic|heif|webp|gif|bmp)$/i;

// Keep image files only (some browsers leave `type` empty for HEIC).
export function isImageFile(file) {
  return /^image\//.test(file.type || "") || IMAGE_NAME.test(file.name || "");
}
