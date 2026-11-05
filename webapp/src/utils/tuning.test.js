import { describe, it, expect } from "vitest";
import {
  LETTERS,
  STARTER_PATCHES,
  applyPatch,
  describeDiff,
  nextRound,
  previousRound,
  reletter,
  remainingLetters,
  startList,
  starterCandidates,
  toggleLetter,
} from "./tuning";

const BASE = {
  exposure: 1,
  saturation: 1,
  toneMode: "contrast",
  contrast: 1,
  strength: 0.5,
  shadowBoost: 0,
  highlightCompress: 0,
  midpoint: 0.5,
  colorMethod: "rgb",
  ditherAlgorithm: "floyd-steinberg",
  compressDynamicRange: true,
  scaleMode: "cover",
  backgroundColor: "white",
};

describe("tuning candidates", () => {
  it("starts with the base as A, then the other presets, then variations", () => {
    const others = [
      { name: "夜景", params: { exposure: 1.2 } },
      { name: "同默认", params: {} }, // looks exactly like the base: skipped
    ];
    const list = starterCandidates(BASE, "人像", others);
    expect(list[0]).toMatchObject({ letter: "A", isBase: true, name: "人像", params: BASE });
    expect(list[1]).toMatchObject({ letter: "B", isBase: false, name: "夜景" });
    expect(list[1].params).toEqual({ ...BASE, exposure: 1.2 });
    expect(list.map((c) => c.letter)).toEqual(LETTERS.slice(0, list.length));
    expect(list.length).toBeLessThanOrEqual(16);
    // Floyd-Steinberg is already the base's dithering: that variation is skipped
    expect(list.filter((c) => c.params.ditherAlgorithm === "floyd-steinberg").length).toBe(
      list.length - 3
    );
    // Layout fields come along unchanged
    expect(list.every((c) => c.params.scaleMode === "cover")).toBe(true);
  });

  it("caps the list and still covers the variations", () => {
    expect(startList(BASE, null, [], 4)).toHaveLength(4);
    expect(startList(BASE, null, [], 99).length).toBe(STARTER_PATCHES.length); // one duplicate skipped
  });

  it("flips a toggled switch", () => {
    expect(applyPatch(BASE, { compressDynamicRange: "toggle" }).compressDynamicRange).toBe(false);
    expect(applyPatch(BASE, { exposure: 0.9 }).exposure).toBe(0.9);
  });

  it("re-letters without gaps", () => {
    const list = reletter([{ letter: "A" }, { letter: "C" }, { letter: "F" }]);
    expect(list.map((c) => c.letter)).toEqual(["A", "B", "C"]);
  });

  it("describes what differs from the default", () => {
    const keys = (p, opts) => describeDiff(p, BASE, opts).map((d) => d.key);
    expect(keys(BASE)).toEqual([]);
    expect(keys({ ...BASE, contrast: 1.25 })).toEqual(["contrast"]);
    // Switching to the S-curve lists the curve's values, not contrast
    expect(keys({ ...BASE, toneMode: "scurve", strength: 0.5 })).toEqual([
      "toneMode",
      "strength",
      "shadowBoost",
      "highlightCompress",
      "midpoint",
    ]);
    expect(keys({ ...BASE, saturation: 0, colorMethod: "lab" }, { grayscale: true })).toEqual([
      "colorMethod",
    ]);
  });
});

describe("tuning rounds", () => {
  it("plays what was not dropped next, in the original order", () => {
    const history = [["A", "B", "C", "D", "E"]];
    expect(remainingLetters(history, ["C", "A", "E"])).toEqual(["B", "D"]);
    expect(nextRound(history, ["E", "C", "A"])).toEqual([
      ["A", "B", "C", "D", "E"],
      ["B", "D"],
    ]);
    // Dropping nothing or everything does not make a new round
    expect(nextRound(history, [])).toBeNull();
    expect(nextRound(history, ["A", "B", "C", "D", "E"])).toBeNull();
    // A letter not in this round is ignored
    expect(
      nextRound(
        [
          ["A", "B", "C"],
          ["B", "C"],
        ],
        ["A", "B"]
      )
    ).toEqual([["A", "B", "C"], ["B", "C"], ["C"]]);
  });

  it("goes back a round but never past the first", () => {
    expect(previousRound([["A", "B"], ["B"]])).toEqual([["A", "B"]]);
    expect(previousRound([["A", "B"]])).toEqual([["A", "B"]]);
  });

  it("toggles a letter", () => {
    expect(toggleLetter(["A"], "B")).toEqual(["A", "B"]);
    expect(toggleLetter(["A", "B"], "A")).toEqual(["B"]);
  });
});
