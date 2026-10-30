// GC16 measured ramp (levelsY) and gray code 4, against the project copy of
// epaper-image-convert (third_party/, resolved through the Vite alias).
import { describe, expect, it } from "vitest";
import { createRequire } from "node:module";
import {
  isValidLevelsY,
  makeGrayscale16,
  processImage,
  SPECTRA6,
} from "@aitjcize/epaper-image-convert";

// Native node-canvas installed with the npm copy of the library (test-only).
const require = createRequire(import.meta.url);
const {
  createCanvas,
} = require("../../node_modules/@aitjcize/epaper-image-convert/node_modules/canvas");

// Read Pico, 2026-10-25 white-first wedge, relative to white paper.
const READ_PICO_Y = [
  0.028, 0.034, 0.042, 0.065, 0.095, 0.132, 0.201, 0.253, 0.337, 0.394, 0.47, 0.522, 0.581, 0.61,
  0.643, 0.645,
];
const NEUTRAL = {
  exposure: 1,
  saturation: 1,
  toneMode: "contrast",
  contrast: 1,
  compressDynamicRange: false,
  colorMethod: "rgb",
  ditherAlgorithm: "floyd-steinberg",
};

function solidGray(v) {
  const c = createCanvas(8, 4);
  const ctx = c.getContext("2d");
  ctx.fillStyle = `rgb(${v},${v},${v})`;
  ctx.fillRect(0, 0, 8, 4);
  return c;
}

function outputValues(source, palette) {
  const out = processImage(source, {
    palette,
    params: NEUTRAL,
    displayWidth: 8,
    displayHeight: 4,
    createCanvas,
  });
  const { data } = out.canvas
    ? out.canvas.getContext("2d").getImageData(0, 0, 8, 4)
    : out.getContext("2d").getImageData(0, 0, 8, 4);
  return new Set(Array.from({ length: 32 }, (_, i) => data[i * 4]));
}

describe("isValidLevelsY", () => {
  it("accepts 16 finite non-decreasing values in (0,1]", () => {
    expect(isValidLevelsY(READ_PICO_Y)).toBe(true);
  });
  it("rejects wrong length, non-finite, out of range, decreasing or flat ramps", () => {
    expect(isValidLevelsY(READ_PICO_Y.slice(0, 15))).toBe(false);
    expect(isValidLevelsY([...READ_PICO_Y.slice(0, 15), NaN])).toBe(false);
    expect(isValidLevelsY([0, ...READ_PICO_Y.slice(1)])).toBe(false);
    expect(isValidLevelsY([...READ_PICO_Y.slice(0, 15), 1.2])).toBe(false);
    const dec = [...READ_PICO_Y];
    dec[5] = dec[4] - 0.01;
    expect(isValidLevelsY(dec)).toBe(false);
    expect(isValidLevelsY(new Array(16).fill(0.5))).toBe(false);
    expect(isValidLevelsY(null)).toBe(false);
  });
});

describe("makeGrayscale16 levelsY", () => {
  it("derives each perceived gray from its own measured Y", () => {
    const p = makeGrayscale16({ levelsY: READ_PICO_Y, gamma: 3 });
    const grays = p.perceived.grays.map((g) => g[0]);
    expect(grays).toHaveLength(16);
    // sRGB of Y: 0.645 -> ~210, 0.201 -> ~124, 0.028 -> ~47
    expect(grays[15]).toBeGreaterThan(205);
    expect(grays[15]).toBeLessThan(215);
    expect(grays[6]).toBeGreaterThan(119);
    expect(grays[6]).toBeLessThan(129);
    expect(grays[0]).toBeGreaterThan(42);
    expect(grays[0]).toBeLessThan(52);
    expect(p.perceived.black.r).toBe(grays[0]);
    expect(p.perceived.white.r).toBe(grays[15]);
  });
  it("falls back to the gamma model when levelsY is invalid", () => {
    const base = makeGrayscale16({ blackY: 0.03, whiteY: 0.65, gamma: 0.8 });
    const bad = makeGrayscale16({ blackY: 0.03, whiteY: 0.65, gamma: 0.8, levelsY: [1, 2] });
    expect(bad.perceived.grays).toEqual(base.perceived.grays);
  });
});

describe("gray code 4", () => {
  it("is selectable in grayscale mode", () => {
    const palette = makeGrayscale16({ levelsY: READ_PICO_Y });
    const code4 = palette.perceived.grays[4][0];
    const values = outputValues(solidGray(code4), palette);
    expect(values).toEqual(new Set([palette.theoretical.grays[4][0]]));
  });
  it("is still skipped for the Spectra 6 reserved slot", () => {
    const values = outputValues(solidGray(0), SPECTRA6);
    expect(values.has(0)).toBe(true); // black, never the reserved index
  });
});
