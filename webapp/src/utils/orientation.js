// Which way is up for a stored photo (docs/PRD-PHOTO-ORIENTATION.md). `up` is how
// far the stored frame-shaped image turns clockwise to stand upright: 0 / 180
// landscape photos, 90 / 270 portrait photos stored on their side. Mirrors the
// firmware's photo_meta.c.

export const UP_VALUES = [0, 90, 180, 270];

function normalize(degrees) {
  return (((degrees || 0) % 360) + 360) % 360;
}

export function isPortraitUp(up) {
  return normalize(up) % 180 === 90;
}

// An upload turned the source `rotation` degrees clockwise. A quarter turn of a
// portrait source filled the panel and is undone by turning back; any other turn
// was a correction, so the stored photo is already upright.
export function upForUpload(rotation, portraitSource) {
  const turned = normalize(rotation);
  if (portraitSource && (turned === 90 || turned === 270)) return normalize(360 - turned);
  return 0;
}

// Turning a photo `delta` degrees clockwise (±90 per press).
export function turnUp(up, delta) {
  return normalize(normalize(up) + delta);
}

// "<album>/<name>" from the firmware's full path ("/storage/images/<album>/<name>").
export function relativePhotoPath(fullPath) {
  const parts = String(fullPath || "")
    .split("/")
    .filter(Boolean);
  return parts.length >= 2 ? parts.slice(-2).join("/") : "";
}

// Width / height of a photo standing upright, for a frame of `aspect` (w / h).
export function uprightAspect(up, aspect) {
  return isPortraitUp(up) ? 1 / aspect : aspect;
}
