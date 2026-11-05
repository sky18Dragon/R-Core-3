import { describe, it, expect } from "vitest";
import { SHEET_GAP, customPlacement, framingPlacement, sheetLayout } from "./compareSheet";

describe("compare sheet", () => {
  it("tiles a landscape panel 2×2 with a gutter and no overlap", () => {
    const { width, height, rotate, cells } = sheetLayout(1216, 684, "landscape");
    expect([width, height, rotate]).toEqual([1216, 684, false]);
    expect(cells).toHaveLength(4);
    for (const c of cells) {
      expect(c.x + c.w).toBeLessThanOrEqual(width);
      expect(c.y + c.h).toBeLessThanOrEqual(height);
    }
    expect(cells[1].x - (cells[0].x + cells[0].w)).toBeGreaterThanOrEqual(SHEET_GAP);
    expect(cells[2].y - (cells[0].y + cells[0].h)).toBeGreaterThanOrEqual(SHEET_GAP);
    // Right and bottom tiles end on the panel edge
    expect(cells[3].x + cells[3].w).toBe(width);
    expect(cells[3].y + cells[3].h).toBe(height);
  });

  it("lays a portrait sheet out upright and asks for the quarter turn", () => {
    const { width, height, rotate, cells } = sheetLayout(1216, 684, "portrait");
    expect([width, height, rotate]).toEqual([684, 1216, true]);
    expect(cells[0].h).toBeGreaterThan(cells[0].w);
    expect(sheetLayout(1216, 684, null).rotate).toBe(false);
  });

  it("gives a single setting the whole screen", () => {
    expect(sheetLayout(1216, 684, "landscape", 1).cells).toEqual([{ x: 0, y: 0, w: 1216, h: 684 }]);
    expect(sheetLayout(1216, 684, "portrait", 1).cells).toEqual([{ x: 0, y: 0, w: 684, h: 1216 }]);
  });

  it("places a framing the same way on any tile size", () => {
    // A 400×300 photo on a 200×100 tile: covering needs half size (200×150)
    expect(customPlacement(400, 300, 200, 100)).toEqual({ zoom: 0.5, panX: 0, panY: -25 });
    // Slid to the top edge, then the bottom edge
    expect(customPlacement(400, 300, 200, 100, { y: -1 }).panY + 0).toBe(0);
    expect(customPlacement(400, 300, 200, 100, { y: 1 }).panY).toBe(-50);
    // Twice the cover size, centred
    expect(customPlacement(400, 300, 200, 100, { zoom: 2 })).toEqual({
      zoom: 1,
      panX: -100,
      panY: -100,
    });
    // Fit shows all of it with margins at the sides
    const fit = framingPlacement(400, 300, 200, 100, { scaleMode: "fit" });
    expect(fit.zoom).toBeCloseTo(1 / 3);
    expect(fit.panX).toBeCloseTo(100 / 3);
    expect(fit.panY).toBeCloseTo(0);
    expect(framingPlacement(400, 300, 200, 100, { scaleMode: "cover" })).toEqual(
      customPlacement(400, 300, 200, 100)
    );
  });
});
