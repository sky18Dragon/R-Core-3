import { describe, it, expect } from "vitest";
import { isPortraitUp, upForUpload, turnUp, relativePhotoPath, uprightAspect } from "./orientation";

describe("photo orientation", () => {
  it("tells portrait from landscape", () => {
    expect(isPortraitUp(0)).toBe(false);
    expect(isPortraitUp(180)).toBe(false);
    expect(isPortraitUp(90)).toBe(true);
    expect(isPortraitUp(270)).toBe(true);
    expect(isPortraitUp(undefined)).toBe(false);
  });

  it("maps an upload's turn to up, like the firmware", () => {
    expect(upForUpload(90, true)).toBe(270);
    expect(upForUpload(-90, true)).toBe(90);
    expect(upForUpload(270, true)).toBe(90);
    expect(upForUpload(0, true)).toBe(0);
    expect(upForUpload(180, true)).toBe(0);
    expect(upForUpload(90, false)).toBe(0);
  });

  it("turns in quarter steps both ways", () => {
    expect(turnUp(0, 90)).toBe(90);
    expect(turnUp(270, 90)).toBe(0);
    expect(turnUp(0, -90)).toBe(270);
    expect(turnUp(undefined, 90)).toBe(90);
  });

  it("finds the album-relative path and the upright shape", () => {
    expect(relativePhotoPath("/storage/images/Trips/a.epdgz")).toBe("Trips/a.epdgz");
    expect(relativePhotoPath("a.epdgz")).toBe("");
    expect(uprightAspect(0, 16 / 9)).toBeCloseTo(16 / 9);
    expect(uprightAspect(90, 16 / 9)).toBeCloseTo(9 / 16);
  });
});
