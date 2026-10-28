// Minimal UI localisation: English source strings are the keys; zh.js maps them
// to Simplified Chinese. No dependency; t() reads a reactive locale, so templates
// re-render when the language changes. Missing translations fall back to English.
import { ref } from "vue";
import zh from "./zh.js";

const STORAGE_KEY = "photoframe.locale";
export const LOCALES = ["en", "zh"];

function detect() {
  try {
    const saved = localStorage.getItem(STORAGE_KEY);
    if (LOCALES.includes(saved)) return saved;
  } catch (_e) {
    // Storage may be unavailable (private mode); fall through to the browser language.
  }
  const lang = typeof navigator !== "undefined" ? navigator.language || "" : "";
  return lang.toLowerCase().startsWith("zh") ? "zh" : "en";
}

export const locale = ref(typeof window === "undefined" ? "en" : detect());

const listeners = new Set();
export function onLocaleChange(fn) {
  listeners.add(fn);
  fn(locale.value);
  return () => listeners.delete(fn);
}

export function setLocale(next) {
  if (!LOCALES.includes(next)) return;
  locale.value = next;
  try {
    localStorage.setItem(STORAGE_KEY, next);
  } catch (_e) {
    // Not persisted; the choice still applies to this page.
  }
  if (typeof document !== "undefined")
    document.documentElement.lang = next === "zh" ? "zh-CN" : "en";
  listeners.forEach((fn) => fn(next));
}

// t("Saved {n} images", { n: 3 }) -> translated text with {name} placeholders filled.
export function t(text, params) {
  let out = locale.value === "zh" && zh[text] ? zh[text] : text;
  if (params) out = out.replace(/\{(\w+)\}/g, (m, k) => (k in params ? String(params[k]) : m));
  return out;
}

export function translations() {
  return zh;
}
