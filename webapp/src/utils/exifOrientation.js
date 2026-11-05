// Phones store a portrait photo's pixels on their side and add an EXIF
// "Orientation" tag (1–8) saying how to turn them. Current browsers apply it
// when they load the image; some in-app browsers (e.g. a chat app's built-in
// page viewer) do not, and a portrait photo then looks like a sideways
// landscape one. These helpers read the tag and turn the pixels ourselves when
// the browser has not (docs/PRD-PHOTO-ORIENTATION.md).

const SCAN_BYTES = 256 * 1024; // EXIF sits in the first JPEG segments

// Orientation tag of a JPEG (1 when absent, unreadable or not a JPEG).
export function exifOrientationFromBytes(buffer) {
  const view = new DataView(buffer);
  if (view.byteLength < 4 || view.getUint16(0) !== 0xffd8) return 1;
  let offset = 2;
  while (offset + 4 <= view.byteLength) {
    const marker = view.getUint16(offset);
    if ((marker & 0xff00) !== 0xff00 || marker === 0xffda) return 1; // image data
    const length = view.getUint16(offset + 2);
    if (length < 2) return 1;
    if (marker === 0xffe1 && offset + 10 <= view.byteLength) {
      const tiff = offset + 10; // after "Exif\0\0"
      if (view.getUint32(offset + 4) === 0x45786966 && tiff + 8 <= view.byteLength) {
        const little = view.getUint16(tiff) === 0x4949;
        const ifd = tiff + view.getUint32(tiff + 4, little);
        if (ifd + 2 > view.byteLength) return 1;
        const count = view.getUint16(ifd, little);
        for (let i = 0; i < count; i++) {
          const entry = ifd + 2 + i * 12;
          if (entry + 10 > view.byteLength) return 1;
          if (view.getUint16(entry, little) === 0x0112) {
            const value = view.getUint16(entry + 8, little);
            return value >= 1 && value <= 8 ? value : 1;
          }
        }
        return 1;
      }
    }
    offset += 2 + length;
  }
  return 1;
}

export async function readExifOrientation(file) {
  try {
    return exifOrientationFromBytes(await file.slice(0, SCAN_BYTES).arrayBuffer());
  } catch {
    return 1;
  }
}

// Orientations 5–8 swap width and height.
export function swapsSides(orientation) {
  return orientation >= 5 && orientation <= 8;
}

// Canvas transform (a, b, c, d, e, f) that draws the stored w×h pixels upright.
export function orientationTransform(orientation, w, h) {
  switch (orientation) {
    case 2:
      return [-1, 0, 0, 1, w, 0];
    case 3:
      return [-1, 0, 0, -1, w, h];
    case 4:
      return [1, 0, 0, -1, 0, h];
    case 5:
      return [0, 1, 1, 0, 0, 0];
    case 6:
      return [0, 1, -1, 0, h, 0];
    case 7:
      return [0, -1, -1, 0, h, w];
    case 8:
      return [0, -1, 1, 0, 0, w];
    default:
      return [1, 0, 0, 1, 0, 0];
  }
}

// A tiny JPEG whose pixels are 2×1 with Orientation 6 (turn a quarter): a
// browser that applies EXIF reports it as 1×2. Built from a canvas JPEG with an
// EXIF segment spliced in, so no binary test file is needed.
let appliesPromise = null;
export function browserAppliesExifOrientation() {
  if (!appliesPromise) appliesPromise = probe().catch(() => true);
  return appliesPromise;
}

async function probe() {
  const canvas = document.createElement("canvas");
  canvas.width = 2;
  canvas.height = 1;
  const jpeg = new Uint8Array(
    await (await new Promise((resolve) => canvas.toBlob(resolve, "image/jpeg"))).arrayBuffer()
  );
  // APP1: "Exif\0\0", big-endian TIFF, one IFD entry: Orientation (SHORT) = 6
  const app1 = [
    0xff, 0xe1, 0x00, 0x22, 0x45, 0x78, 0x69, 0x66, 0x00, 0x00, 0x4d, 0x4d, 0x00, 0x2a, 0x00, 0x00,
    0x00, 0x08, 0x00, 0x01, 0x01, 0x12, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
  ];
  const bytes = new Uint8Array(jpeg.length + app1.length);
  bytes.set(jpeg.subarray(0, 2));
  bytes.set(app1, 2);
  bytes.set(jpeg.subarray(2), 2 + app1.length);
  const url = URL.createObjectURL(new Blob([bytes], { type: "image/jpeg" }));
  try {
    const img = await new Promise((resolve, reject) => {
      const image = new Image();
      image.onload = () => resolve(image);
      image.onerror = reject;
      image.src = url;
    });
    return img.width === 1 && img.height === 2;
  } finally {
    URL.revokeObjectURL(url);
  }
}

// The image turned upright on a canvas (long edge at most maxEdge, so a
// 12-megapixel photo does not need a full-size canvas).
export function orientUpright(img, orientation, maxEdge) {
  const w = img.width;
  const h = img.height;
  const scale = Math.min(1, maxEdge / Math.max(w, h));
  const sw = Math.max(1, Math.round(w * scale));
  const sh = Math.max(1, Math.round(h * scale));
  const canvas = document.createElement("canvas");
  canvas.width = swapsSides(orientation) ? sh : sw;
  canvas.height = swapsSides(orientation) ? sw : sh;
  const ctx = canvas.getContext("2d");
  ctx.imageSmoothingEnabled = true;
  ctx.imageSmoothingQuality = "high";
  ctx.setTransform(...orientationTransform(orientation, sw, sh));
  ctx.drawImage(img, 0, 0, sw, sh);
  return canvas;
}
