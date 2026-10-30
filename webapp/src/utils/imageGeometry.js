// Pure geometry helpers for the upload editor (no DOM), so they can be unit tested.

// Normalise any multiple of 90 into 0 | 90 | 180 | 270.
export function normalizeDeg(deg) {
  return (((Math.round(deg / 90) * 90) % 360) + 360) % 360;
}

// Size that fits inside a maxEdge x maxEdge box without upscaling.
export function fitWithin(width, height, maxEdge) {
  const longEdge = Math.max(width, height);
  if (longEdge <= maxEdge) return { width, height, scaled: false };
  const ratio = maxEdge / longEdge;
  return {
    width: Math.max(1, Math.round(width * ratio)),
    height: Math.max(1, Math.round(height * ratio)),
    scaled: true,
  };
}

// Long-edge cap for the in-browser source canvas. Phone photos can be tens of
// megapixels; the panel needs a fraction of that, and iOS Safari limits canvas
// area. Keep 2-3x the panel's long edge so cover/custom crops and zoom stay sharp.
export const MIN_SOURCE_EDGE = 2400;
export function sourceEdgeCap(panelWidth, panelHeight) {
  return Math.max(MIN_SOURCE_EDGE, 2 * Math.max(panelWidth || 0, panelHeight || 0));
}

// Squarish images (within this fraction of each other) get no rotation hint.
const SQUARISH = 0.05;

// Which way is the image oriented relative to the frame? Returns "portrait" or
// "landscape" when the image is clearly the opposite of the frame (rotating 90deg
// would fill it better); null when they match or the image is near-square.
export function rotationSuggestion(srcWidth, srcHeight, frameWidth, frameHeight) {
  if (!(srcWidth > 0 && srcHeight > 0 && frameWidth > 0 && frameHeight > 0)) return null;
  if (Math.abs(srcWidth - srcHeight) / Math.max(srcWidth, srcHeight) < SQUARISH) return null;
  if (frameWidth === frameHeight) return null;
  const srcLandscape = srcWidth > srcHeight;
  const frameLandscape = frameWidth > frameHeight;
  if (srcLandscape === frameLandscape) return null;
  return srcLandscape ? "landscape" : "portrait";
}

// Draw `source` rotated clockwise by deg (multiple of 90) onto a new canvas.
// Returns `source` itself for 0 so callers can skip a copy.
export function rotateCanvas(source, deg, createCanvas = () => document.createElement("canvas")) {
  const d = normalizeDeg(deg);
  if (d === 0) return source;
  const swap = d === 90 || d === 270;
  const out = createCanvas();
  out.width = swap ? source.height : source.width;
  out.height = swap ? source.width : source.height;
  const ctx = out.getContext("2d");
  ctx.translate(out.width / 2, out.height / 2);
  ctx.rotate((d * Math.PI) / 180);
  ctx.drawImage(source, -source.width / 2, -source.height / 2);
  return out;
}

// Rotation a queued photo is auto-turned by at add time: toward the remembered
// direction when it is clearly the opposite orientation of the frame, else 0.
export function autoRotationFor(srcWidth, srcHeight, frameWidth, frameHeight, dir) {
  return rotationSuggestion(srcWidth, srcHeight, frameWidth, frameHeight)
    ? dir === "ccw"
      ? 270
      : 90
    : 0;
}

// What a queued photo is actually turned by: the user's explicit choice wins;
// otherwise the auto rotation, when auto-rotate is on.
export function effectiveRotation(userRotation, autoRotation, autoOn) {
  if (userRotation !== null && userRotation !== undefined) return normalizeDeg(userRotation);
  return autoOn ? normalizeDeg(autoRotation || 0) : 0;
}
