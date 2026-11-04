import { describe, it, expect, beforeEach } from "vitest";
import {
  photoKey,
  selectedPhotos,
  selectedCount,
  selecting,
  isSelected,
  toggleSelected,
  selectAll,
  clearSelection,
} from "./selection";

const a = { album: "Default", filename: "a.epdgz" };
const b = { album: "Trips", filename: "a.epdgz" };

describe("photo selection", () => {
  beforeEach(clearSelection);

  it("keys photos by album and file, so the same name in two albums stays apart", () => {
    expect(photoKey(a)).not.toBe(photoKey(b));
    toggleSelected(a);
    expect(isSelected(a)).toBe(true);
    expect(isSelected(b)).toBe(false);
  });

  it("toggles, counts and clears", () => {
    expect(selecting.value).toBe(false);
    toggleSelected(a);
    toggleSelected(b);
    expect(selectedCount.value).toBe(2);
    expect(selecting.value).toBe(true);
    toggleSelected(a);
    expect(selectedPhotos.value).toEqual([b]);
    clearSelection();
    expect(selectedCount.value).toBe(0);
  });

  it("selects a whole group without duplicates", () => {
    toggleSelected(a);
    selectAll([a, b]);
    expect(selectedCount.value).toBe(2);
  });
});
