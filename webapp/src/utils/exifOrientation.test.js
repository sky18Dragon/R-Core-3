import { describe, it, expect } from "vitest";
import { exifOrientationFromBytes, orientationTransform, swapsSides } from "./exifOrientation";

// A minimal JPEG: SOI, optional APP0, APP1 EXIF with one Orientation entry, SOS.
function jpeg({ orientation = 6, little = false, app0 = true, exif = true } = {}) {
  const u16 = (v) => (little ? [v & 0xff, v >> 8] : [v >> 8, v & 0xff]);
  const u32 = (v) =>
    little ? [v & 0xff, (v >> 8) & 0xff, 0, 0] : [0, 0, (v >> 8) & 0xff, v & 0xff];
  const tiff = [
    ...(little ? [0x49, 0x49] : [0x4d, 0x4d]),
    ...u16(42),
    ...u32(8),
    ...u16(1),
    ...u16(0x0112),
    ...u16(3),
    ...u32(1),
    ...u16(orientation),
    0,
    0,
    ...u32(0),
  ];
  const app1Body = [0x45, 0x78, 0x69, 0x66, 0, 0, ...tiff];
  const bytes = [0xff, 0xd8];
  if (app0) bytes.push(0xff, 0xe0, 0x00, 0x04, 0x4a, 0x46);
  if (exif)
    bytes.push(0xff, 0xe1, (app1Body.length + 2) >> 8, (app1Body.length + 2) & 0xff, ...app1Body);
  bytes.push(0xff, 0xda, 0x00, 0x02);
  return new Uint8Array(bytes).buffer;
}

describe("EXIF orientation", () => {
  it("reads the tag in either byte order, after other segments", () => {
    expect(exifOrientationFromBytes(jpeg({ orientation: 6 }))).toBe(6);
    expect(exifOrientationFromBytes(jpeg({ orientation: 8, little: true }))).toBe(8);
    expect(exifOrientationFromBytes(jpeg({ orientation: 3, app0: false }))).toBe(3);
  });

  it("falls back to upright for anything else", () => {
    expect(exifOrientationFromBytes(jpeg({ exif: false }))).toBe(1);
    expect(exifOrientationFromBytes(jpeg({ orientation: 9 }))).toBe(1);
    expect(exifOrientationFromBytes(new Uint8Array([0x89, 0x50, 0x4e, 0x47]).buffer)).toBe(1);
    expect(exifOrientationFromBytes(new Uint8Array([0xff, 0xd8, 0xff]).buffer)).toBe(1);
    expect(exifOrientationFromBytes(new ArrayBuffer(0))).toBe(1);
  });

  it("knows which orientations swap the sides", () => {
    expect([1, 2, 3, 4].map(swapsSides)).toEqual([false, false, false, false]);
    expect([5, 6, 7, 8].map(swapsSides)).toEqual([true, true, true, true]);
  });

  it("maps each orientation to a transform that lands the image on the canvas", () => {
    // Corners of a w×h image must land inside the (possibly swapped) canvas.
    const w = 4;
    const h = 3;
    for (let o = 1; o <= 8; o++) {
      const [a, b, c, d, e, f] = orientationTransform(o, w, h);
      const cw = swapsSides(o) ? h : w;
      const ch = swapsSides(o) ? w : h;
      for (const [x, y] of [
        [0, 0],
        [w, 0],
        [0, h],
        [w, h],
      ]) {
        const X = a * x + c * y + e;
        const Y = b * x + d * y + f;
        expect(X).toBeGreaterThanOrEqual(0);
        expect(X).toBeLessThanOrEqual(cw);
        expect(Y).toBeGreaterThanOrEqual(0);
        expect(Y).toBeLessThanOrEqual(ch);
      }
    }
    // 6 (turn a quarter clockwise): the image's top-left goes to the top-right.
    const [a, b, c, d, e, f] = orientationTransform(6, w, h);
    expect([a * 0 + c * 0 + e, b * 0 + d * 0 + f]).toEqual([h, 0]);
  });
});
