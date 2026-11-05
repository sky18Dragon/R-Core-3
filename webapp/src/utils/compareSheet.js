// Side-by-side comparison: one photo processed with up to four settings,
// tiled 2×2 on a single screen so the differences are judged on the panel
// itself, under the same light, instead of from memory between refreshes.

export const SHEET_LABELS = ["A", "B", "C", "D"];
export const SHEET_GAP = 8; // white gutter between the tiles, in panel pixels

// The keys that make up a processing setting (layout fields are separate).
export const PROCESSING_KEYS = [
  "exposure",
  "saturation",
  "toneMode",
  "contrast",
  "strength",
  "shadowBoost",
  "highlightCompress",
  "midpoint",
  "colorMethod",
  "ditherAlgorithm",
  "compressDynamicRange",
];

// Where the four tiles go. The sheet is laid out the way the viewer sees the
// frame (`orientation`); `rotate` says it must be turned a quarter clockwise
// into the panel's own layout, as processImage does for a whole photo.
// `count` 1 gives one tile filling the screen (a single setting, full size).
export function sheetLayout(width, height, orientation = null, count = 4) {
  const nativeLandscape = width > height;
  const wantLandscape = orientation == null ? nativeLandscape : orientation !== "portrait";
  const rotate = nativeLandscape !== wantLandscape;
  const [w, h] = rotate ? [height, width] : [width, height];
  if (count === 1) return { width: w, height: h, rotate, cells: [{ x: 0, y: 0, w, h }] };
  const cw = Math.floor((w - SHEET_GAP) / 2);
  const ch = Math.floor((h - SHEET_GAP) / 2);
  const cells = [0, 1, 2, 3].map((i) => ({
    x: (i % 2) * (w - cw),
    y: Math.floor(i / 2) * (h - ch),
    w: cw,
    h: ch,
  }));
  return { width: w, height: h, rotate, cells };
}

// Where a photo sits in a tile for a framing chosen relative to the tile, so
// one choice means the same on a full screen and on a quarter of it.
// `zoom` 1 just covers the tile (bigger crops more, smaller leaves margins);
// `x`/`y` from -1 to 1 slide it to show its left/top or right/bottom edge.
// Returns the library's custom-scale values: the scale of the source and the
// top-left corner in tile pixels.
export function customPlacement(srcW, srcH, cellW, cellH, { zoom = 1, x = 0, y = 0 } = {}) {
  const scale = Math.max(cellW / srcW, cellH / srcH) * zoom;
  const w = srcW * scale;
  const h = srcH * scale;
  return { zoom: scale, panX: ((cellW - w) / 2) * (1 + x), panY: ((cellH - h) / 2) * (1 + y) };
}

// The same for any framing mode: "cover" fills and crops centred, "fit" shows
// all of it centred, "custom" uses zoom and position.
export function framingPlacement(srcW, srcH, cellW, cellH, framing = {}) {
  if (framing.scaleMode === "custom") return customPlacement(srcW, srcH, cellW, cellH, framing);
  const scale =
    framing.scaleMode === "fit"
      ? Math.min(cellW / srcW, cellH / srcH)
      : Math.max(cellW / srcW, cellH / srcH);
  return {
    zoom: scale,
    panX: (cellW - srcW * scale) / 2,
    panY: (cellH - srcH * scale) / 2,
  };
}

function makeCanvas(width, height) {
  const canvas = document.createElement("canvas");
  canvas.width = width;
  canvas.height = height;
  return canvas;
}

function whiteCanvas(width, height) {
  const canvas = makeCanvas(width, height);
  const ctx = canvas.getContext("2d", { willReadFrequently: true });
  ctx.fillStyle = "#ffffff";
  ctx.fillRect(0, 0, width, height);
  return canvas;
}

// A letter in a framed white square, reduced to pure black and white so it
// survives the panel's palette mapping (a colour panel turns any other shade
// into white).
function badge(letter, size) {
  const canvas = whiteCanvas(size, size);
  const ctx = canvas.getContext("2d", { willReadFrequently: true });
  const line = Math.max(2, Math.round(size / 14));
  ctx.strokeStyle = "#000000";
  ctx.lineWidth = line;
  ctx.strokeRect(line / 2, line / 2, size - line, size - line);
  ctx.fillStyle = "#000000";
  ctx.font = `bold ${Math.round(size * 0.62)}px sans-serif`;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(letter, size / 2, size / 2 + Math.round(size * 0.04));
  const image = ctx.getImageData(0, 0, size, size);
  const d = image.data;
  for (let i = 0; i < d.length; i += 4) {
    const v = d[i] + d[i + 1] + d[i + 2] < 384 ? 0 : 255;
    d[i] = d[i + 1] = d[i + 2] = v;
    d[i + 3] = 255;
  }
  ctx.putImageData(image, 0, 0);
  return canvas;
}

// Render the sheet. `lib` is the image library, `source` the upright photo,
// `slots` the settings ({params}) in tile order — one slot fills the screen,
// up to four are tiled 2×2 — `labels` their letters (A–D unless given),
// `framing` an optional sheet-wide framing ({scaleMode, zoom, x, y}, see
// framingPlacement), `layout` an optional per-photo
// framing ({scaleMode, backgroundColor, zoom, panX, panY}) from the editor.
// Returns the dithered sheet in panel layout (`canvas`), the same before
// dithering for the thumbnail (`original`), and the dithered sheet the way the
// viewer sees it (`preview`).
export function composeCompareSheet(lib, source, slots, options) {
  const {
    width,
    height,
    orientation,
    palette,
    layout = null,
    framing = null,
    labels = SHEET_LABELS,
  } = options;
  const single = slots.length === 1;
  const sheet = sheetLayout(width, height, orientation, single ? 1 : 4);
  const out = whiteCanvas(sheet.width, sheet.height);
  const orig = whiteCanvas(sheet.width, sheet.height);
  const outCtx = out.getContext("2d", { willReadFrequently: true });
  const origCtx = orig.getContext("2d", { willReadFrequently: true });
  const badgeSize = Math.max(
    28,
    Math.round(Math.min(sheet.cells[0].w, sheet.cells[0].h) * (single ? 0.1 : 0.12))
  );
  const inset = Math.round(badgeSize / 4);

  slots.slice(0, SHEET_LABELS.length).forEach((slot, i) => {
    const cell = sheet.cells[i];
    const p = slot.params;
    const params = {};
    for (const key of PROCESSING_KEYS) params[key] = p[key];
    // A sheet-wide framing (relative, so it works for any tile size) wins
    // over the per-photo layout and the setting's own scale mode
    const custom =
      framing?.scaleMode === "custom"
        ? customPlacement(source.width, source.height, cell.w, cell.h, framing)
        : null;
    const result = lib.processImage(source, {
      displayWidth: cell.w,
      displayHeight: cell.h,
      palette,
      params,
      orientation: null, // each tile is already in the viewer's layout
      scaleMode: framing?.scaleMode ?? layout?.scaleMode ?? p.scaleMode ?? "cover",
      backgroundColor: layout?.backgroundColor ?? p.backgroundColor ?? "white",
      zoom: custom ? custom.zoom : layout?.zoom,
      panX: custom ? custom.panX : layout?.panX,
      panY: custom ? custom.panY : layout?.panY,
      usePerceivedOutput: false,
    });
    outCtx.drawImage(result.canvas, cell.x, cell.y);
    origCtx.drawImage(result.originalCanvas, cell.x, cell.y);
    const mark = badge(labels[i] ?? SHEET_LABELS[i], badgeSize);
    outCtx.drawImage(mark, cell.x + inset, cell.y + inset);
    origCtx.drawImage(mark, cell.x + inset, cell.y + inset);
  });

  return {
    canvas: sheet.rotate ? lib.rotateImage(out, 90) : out,
    original: sheet.rotate ? lib.rotateImage(orig, 90) : orig,
    preview: out,
  };
}
