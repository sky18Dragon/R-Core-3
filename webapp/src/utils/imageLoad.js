import { fitWithin } from "./imageGeometry";
import {
  browserAppliesExifOrientation,
  orientUpright,
  readExifOrientation,
} from "./exifOrientation";

// Long edge of the canvas used when we turn a photo upright ourselves: enough
// for every later step (processing uses at most twice the panel's long edge).
const UPRIGHT_MAX_EDGE = 4096;

// Decode an image File the way it should be seen: an <img>, or a canvas turned
// upright when the photo's EXIF orientation is something the browser ignored
// (rejects when the browser can't read it). Both have width/height and draw.
export async function loadImageFromFile(file) {
  const img = await loadRawImage(file);
  const orientation = await readExifOrientation(file);
  if (orientation === 1 || (await browserAppliesExifOrientation())) return img;
  return orientUpright(img, orientation, UPRIGHT_MAX_EDGE);
}

function loadRawImage(file) {
  return new Promise((resolve, reject) => {
    const url = URL.createObjectURL(file);
    const img = new Image();
    img.onload = () => {
      URL.revokeObjectURL(url);
      resolve(img);
    };
    img.onerror = (error) => {
      URL.revokeObjectURL(url);
      reject(error);
    };
    img.src = url;
  });
}

// Decode a File onto a canvas whose long edge is at most maxEdge (never upscaled).
export async function decodeToCanvas(file, maxEdge) {
  const img = await loadImageFromFile(file);
  const size = fitWithin(img.width, img.height, maxEdge);
  const canvas = document.createElement("canvas");
  canvas.width = size.width;
  canvas.height = size.height;
  const ctx = canvas.getContext("2d");
  if (size.scaled) {
    ctx.imageSmoothingEnabled = true;
    ctx.imageSmoothingQuality = "high";
  }
  ctx.drawImage(img, 0, 0, size.width, size.height);
  return { canvas, naturalWidth: img.width, naturalHeight: img.height };
}
