import { fitWithin } from "./imageGeometry";

// Decode an image File into an <img> (rejects when the browser can't read it).
export function loadImageFromFile(file) {
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
