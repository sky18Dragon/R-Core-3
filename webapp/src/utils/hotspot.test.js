import { describe, expect, it } from "vitest";
import {
  clockDriftSeconds,
  clockNeedsSync,
  formatRemaining,
  hotspotPasswordValid,
  hotspotSsidValid,
  idleChoices,
} from "./hotspot.js";

describe("hotspot helpers", () => {
  it("formats the time left before the hotspot closes", () => {
    expect(formatRemaining(545)).toBe("9:05");
    expect(formatRemaining(0)).toBe("0:00");
    expect(formatRemaining(59.2)).toBe("1:00");
    expect(formatRemaining(-1)).toBeNull();
    expect(formatRemaining(undefined)).toBeNull();
  });

  it("offers 'never' only when the frame allows it", () => {
    expect(idleChoices(true)).toEqual([10, 30, 60, 0]);
    expect(idleChoices(false)).toEqual([10, 30, 60]);
  });

  it("validates passwords like the firmware", () => {
    expect(hotspotPasswordValid("K7MP4TQX")).toBe(true);
    expect(hotspotPasswordValid("a".repeat(63))).toBe(true);
    expect(hotspotPasswordValid("short")).toBe(false);
    expect(hotspotPasswordValid("a".repeat(64))).toBe(false);
    expect(hotspotPasswordValid("密码密码密码密码")).toBe(false);
    expect(hotspotPasswordValid("tab\there1")).toBe(false);
  });

  it("validates names by UTF-8 bytes", () => {
    expect(hotspotSsidValid("PhotoFrame - 3F2A1")).toBe(true);
    expect(hotspotSsidValid("相框".repeat(5))).toBe(true); // 30 bytes
    expect(hotspotSsidValid("相框".repeat(6))).toBe(false); // 36 bytes
    expect(hotspotSsidValid("")).toBe(false);
    expect(hotspotSsidValid("a\nb")).toBe(false);
  });

  it("measures clock drift against this browser", () => {
    expect(clockDriftSeconds(1000, 1_000_000)).toBe(0);
    expect(clockDriftSeconds(700, 1_000_000)).toBe(-300);
    expect(clockDriftSeconds(undefined, 0)).toBeNull();
    expect(clockNeedsSync(-300)).toBe(true);
    expect(clockNeedsSync(120)).toBe(false);
    expect(clockNeedsSync(null)).toBe(false);
  });
});
