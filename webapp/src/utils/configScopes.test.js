import { describe, it, expect } from "vitest";
import { IMAGE_CONFIG_KEYS, keysForScope, valuesDiffer, diffConfig } from "./configScopes";

describe("keysForScope", () => {
  const keys = ["auto_rotate", "rotate_cron", "sd_rotation_mode", "wifi_ssid", "timezone"];
  it("splits image and system keys without overlap", () => {
    expect(keysForScope(keys, "image")).toEqual(IMAGE_CONFIG_KEYS);
    expect(keysForScope(keys, "system")).toEqual(["wifi_ssid", "timezone"]);
  });
  it("returns everything for any other scope", () => {
    expect(keysForScope(keys, "all")).toEqual(keys);
  });
});

describe("valuesDiffer", () => {
  it("compares arrays by value", () => {
    expect(valuesDiffer(["0 */12 *"], ["0 */12 *"])).toBe(false);
    expect(valuesDiffer(["0 */12 *"], ["0 */6 *"])).toBe(true);
    expect(valuesDiffer(["a"], undefined)).toBe(true);
  });
  it("compares scalars strictly", () => {
    expect(valuesDiffer(true, true)).toBe(false);
    expect(valuesDiffer(1, "1")).toBe(true);
  });
});

describe("diffConfig", () => {
  const original = {
    auto_rotate: true,
    rotate_cron: ["0 */12 *"],
    sd_rotation_mode: "random",
    wifi_ssid: "home",
    timezone: "UTC0",
  };

  it("is empty when nothing changed", () => {
    expect(diffConfig({ ...original }, original)).toEqual({});
  });

  it("keeps a panel's edits out of the other panel's save", () => {
    const current = { ...original, wifi_ssid: "office", auto_rotate: false };
    expect(diffConfig(current, original, "system")).toEqual({ wifi_ssid: "office" });
    expect(diffConfig(current, original, "image")).toEqual({ auto_rotate: false });
    expect(diffConfig(current, original)).toEqual({ wifi_ssid: "office", auto_rotate: false });
  });

  it("detects schedule edits inside the array", () => {
    const current = { ...original, rotate_cron: ["0 */12 *", "30 8 *"] };
    expect(diffConfig(current, original, "image")).toEqual({ rotate_cron: current.rotate_cron });
  });

  it("treats a missing baseline as everything changed", () => {
    expect(Object.keys(diffConfig(original, undefined))).toHaveLength(5);
  });
});
