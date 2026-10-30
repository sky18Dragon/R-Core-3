import { describe, it, expect } from "vitest";
import {
  normalizeDeg,
  fitWithin,
  sourceEdgeCap,
  rotationSuggestion,
  rotateCanvas,
} from "./imageGeometry";

describe("normalizeDeg", () => {
  it("wraps into 0/90/180/270", () => {
    expect(normalizeDeg(0)).toBe(0);
    expect(normalizeDeg(90)).toBe(90);
    expect(normalizeDeg(360)).toBe(0);
    expect(normalizeDeg(-90)).toBe(270);
    expect(normalizeDeg(450)).toBe(90);
    expect(normalizeDeg(-360)).toBe(0);
  });
});

describe("fitWithin", () => {
  it("never upscales", () => {
    expect(fitWithin(800, 480, 2400)).toEqual({ width: 800, height: 480, scaled: false });
  });
  it("scales the long edge down and keeps the ratio", () => {
    expect(fitWithin(8000, 6000, 2400)).toEqual({ width: 2400, height: 1800, scaled: true });
    expect(fitWithin(3000, 4000, 2000)).toEqual({ width: 1500, height: 2000, scaled: true });
  });
  it("keeps at least one pixel", () => {
    expect(fitWithin(10000, 1, 100).height).toBe(1);
  });
});

describe("sourceEdgeCap", () => {
  it("has a floor and follows large panels", () => {
    expect(sourceEdgeCap(800, 480)).toBe(2400);
    expect(sourceEdgeCap(1600, 1200)).toBe(3200);
    expect(sourceEdgeCap(undefined, undefined)).toBe(2400);
  });
});

describe("rotationSuggestion", () => {
  it("suggests when image and frame orientations differ", () => {
    expect(rotationSuggestion(3000, 4000, 800, 480)).toBe("portrait");
    expect(rotationSuggestion(4000, 3000, 480, 800)).toBe("landscape");
  });
  it("is silent when they match", () => {
    expect(rotationSuggestion(4000, 3000, 800, 480)).toBeNull();
    expect(rotationSuggestion(3000, 4000, 480, 800)).toBeNull();
  });
  it("is silent for near-square images or frames and bad input", () => {
    expect(rotationSuggestion(1000, 980, 800, 480)).toBeNull();
    expect(rotationSuggestion(3000, 4000, 600, 600)).toBeNull();
    expect(rotationSuggestion(0, 4000, 800, 480)).toBeNull();
  });
});

// Minimal fake canvas that records the transform calls.
function fakeCanvas(width, height) {
  const calls = [];
  return {
    width,
    height,
    calls,
    getContext: () => ({
      translate: (...a) => calls.push(["translate", ...a]),
      rotate: (...a) => calls.push(["rotate", ...a]),
      drawImage: (...a) => calls.push(["drawImage", ...a.slice(1)]),
    }),
  };
}

describe("rotateCanvas", () => {
  it("returns the source untouched for 0", () => {
    const src = fakeCanvas(30, 20);
    expect(rotateCanvas(src, 0)).toBe(src);
    expect(rotateCanvas(src, 360)).toBe(src);
  });
  it("swaps dimensions for quarter turns and keeps them for 180", () => {
    const src = fakeCanvas(30, 20);
    const make = () => fakeCanvas(0, 0);
    const r90 = rotateCanvas(src, 90, make);
    expect([r90.width, r90.height]).toEqual([20, 30]);
    const r270 = rotateCanvas(src, -90, make);
    expect([r270.width, r270.height]).toEqual([20, 30]);
    const r180 = rotateCanvas(src, 180, make);
    expect([r180.width, r180.height]).toEqual([30, 20]);
  });
  it("rotates clockwise about the centre", () => {
    const src = fakeCanvas(30, 20);
    const out = rotateCanvas(src, 90, () => fakeCanvas(0, 0));
    expect(out.calls[0]).toEqual(["translate", 10, 15]);
    expect(out.calls[1][0]).toBe("rotate");
    expect(out.calls[1][1]).toBeCloseTo(Math.PI / 2);
    expect(out.calls[2]).toEqual(["drawImage", -15, -10]);
  });
});
