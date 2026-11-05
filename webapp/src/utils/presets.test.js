import { describe, it, expect } from "vitest";
import {
  MAX_PRESETS,
  addPreset,
  canRemove,
  duplicateName,
  emptyDoc,
  migratedDoc,
  nameFits,
  newId,
  processingOf,
  removePreset,
  sameProcessingValues,
  setDefaultPreset,
  updatePreset,
} from "./presets";

const PARAMS = {
  exposure: 0.97,
  contrast: 1.21,
  toneMode: "contrast",
  ditherAlgorithm: "stucki",
  scaleMode: "fit",
  backgroundColor: "white",
};
const NOW = 1793800000000;

describe("presets", () => {
  it("keeps processing values only, never the layout", () => {
    expect(processingOf(PARAMS)).toEqual({
      exposure: 0.97,
      contrast: 1.21,
      toneMode: "contrast",
      ditherAlgorithm: "stucki",
    });
  });

  it("checks names by trimmed UTF-8 length", () => {
    expect(nameFits("人像")).toBe(true);
    expect(nameFits("   ")).toBe(false);
    expect(nameFits("字".repeat(21))).toBe(true); // 63 bytes
    expect(nameFits("字".repeat(22))).toBe(false); // 66 bytes
  });

  it("makes short unique-looking ids", () => {
    expect(newId(NOW, () => 0.5)).toMatch(/^p[0-9a-z]+$/);
    expect(newId(NOW, () => 0.1)).not.toBe(newId(NOW, () => 0.2));
  });

  it("migrates the frame's current setting into the first, default preset", () => {
    const doc = migratedDoc(emptyDoc(), PARAMS, "我的默认", { now: NOW, id: "p1" });
    expect(doc.default).toBe("p1");
    expect(doc.presets).toEqual([
      {
        id: "p1",
        name: "我的默认",
        source: "migrated",
        params: processingOf(PARAMS),
        created: 1793800000,
        updated: 1793800000,
      },
    ]);
    // Nothing happens once there are presets
    expect(migratedDoc(doc, { exposure: 2 }, "x")).toBe(doc);
  });

  it("adds, renames, changes and duplicates", () => {
    let [doc, id] = addPreset(
      emptyDoc(),
      { name: " 人像 ", params: PARAMS, source: "tuning" },
      { now: NOW, id: "a" }
    );
    expect(id).toBe("a");
    expect(doc.presets[0]).toMatchObject({ name: "人像", source: "tuning" });
    doc = updatePreset(
      doc,
      "a",
      { name: "夜景", params: { ...PARAMS, exposure: 1.2 } },
      { now: NOW + 5000 }
    );
    expect(doc.presets[0]).toMatchObject({
      name: "夜景",
      updated: 1793800005,
      created: 1793800000,
    });
    expect(doc.presets[0].params.exposure).toBe(1.2);
    // A bad name changes nothing
    expect(updatePreset(doc, "a", { name: "" })).toBe(doc);
    expect(duplicateName(doc, "夜景", "副本")).toBe("夜景 副本");
    [doc] = addPreset(doc, { name: "夜景 副本", params: PARAMS }, { id: "b" });
    expect(duplicateName(doc, "夜景", "副本")).toBe("夜景 副本 2");
  });

  it("stops at the limit", () => {
    let doc = emptyDoc();
    for (let i = 0; i < MAX_PRESETS; i++) [doc] = addPreset(doc, { name: `p${i}`, params: PARAMS });
    const [same, id] = addPreset(doc, { name: "one more", params: PARAMS });
    expect(id).toBeNull();
    expect(same).toBe(doc);
  });

  it("never removes the default or the last preset", () => {
    let [doc] = addPreset(emptyDoc(), { name: "a", params: PARAMS }, { id: "a" });
    doc = setDefaultPreset(doc, "a");
    expect(canRemove(doc, "a")).toBe(false);
    [doc] = addPreset(doc, { name: "b", params: PARAMS }, { id: "b" });
    expect(canRemove(doc, "a")).toBe(false);
    expect(canRemove(doc, "b")).toBe(true);
    doc = removePreset(doc, "b");
    expect(doc.presets.map((p) => p.id)).toEqual(["a"]);
    expect(setDefaultPreset(doc, "missing")).toBe(doc);
  });

  it("compares processing values with the frame's float tolerance", () => {
    expect(sameProcessingValues({ contrast: 1.2100000381 }, { contrast: 1.21 })).toBe(true);
    expect(sameProcessingValues({ contrast: 1.25 }, { contrast: 1.21 })).toBe(false);
  });
});
