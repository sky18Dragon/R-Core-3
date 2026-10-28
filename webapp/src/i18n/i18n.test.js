// Every literal passed to t() in the device UI must have a Chinese entry, and
// t() must interpolate and fall back correctly.
import { describe, expect, it } from "vitest";
import { readdirSync, readFileSync, statSync } from "node:fs";
import { join } from "node:path";
import zh from "./zh.js";
import { locale, setLocale, t } from "./index.js";

const SRC = join(__dirname, "..");
// LandingPage (GitHub Pages demo) and PaletteCalibration (colour panels only) are not translated.
const SKIP = new Set(["LandingPage.vue", "PaletteCalibration.vue"]);

function sources(dir) {
  return readdirSync(dir).flatMap((name) => {
    const path = join(dir, name);
    if (statSync(path).isDirectory()) return name === "i18n" ? [] : sources(path);
    return /\.(vue|js)$/.test(name) && !name.endsWith(".test.js") && !SKIP.has(name) ? [path] : [];
  });
}

function literals(text) {
  const keys = [];
  const re = /\bt\(\s*(?:"((?:[^"\\]|\\.)*)"|'((?:[^'\\]|\\.)*)')/g;
  for (const m of text.matchAll(re)) keys.push((m[1] ?? m[2]).replace(/\\(['"])/g, "$1"));
  return keys;
}

describe("i18n", () => {
  it("has a Chinese translation for every t() literal", () => {
    const missing = [];
    for (const file of sources(SRC)) {
      for (const key of literals(readFileSync(file, "utf8"))) {
        if (!(key in zh)) missing.push(`${file.slice(SRC.length + 1)}: ${key}`);
      }
    }
    expect(missing).toEqual([]);
  });

  it("switches language, interpolates and falls back to English", () => {
    setLocale("en");
    expect(t("Settings")).toBe("Settings");
    setLocale("zh");
    expect(locale.value).toBe("zh");
    expect(t("Settings")).toBe("设置");
    expect(t("Schedule {n}", { n: 2 })).toBe("计划 2");
    expect(t("Not in the dictionary")).toBe("Not in the dictionary");
    setLocale("xx");
    expect(locale.value).toBe("zh");
    setLocale("en");
  });
});
