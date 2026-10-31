import { ref, watch } from "vue";

// Per-browser UI preferences, persisted to localStorage so they survive reloads.

const WIDE_EDIT_KEY = "photoframe.wideEdit";
const ROTATE_DIR_KEY = "photoframe.rotateDir";

function loadBool(key) {
  try {
    return localStorage.getItem(key) === "true";
  } catch {
    return false;
  }
}

// Wide side-by-side editing layout: when on, the upload/preview card breaks out
// to ~80vw and the image-processing controls sit next to the preview.
export const wideEdit = ref(loadBool(WIDE_EDIT_KEY));

watch(wideEdit, (value) => {
  try {
    localStorage.setItem(WIDE_EDIT_KEY, value ? "true" : "false");
  } catch {
    // localStorage can be unavailable (private mode / disabled); ignore.
  }
});

// Direction the user last rotated an image ("cw" | "ccw"); the rotate hint
// recommends it first next time, since it reflects how the frame is mounted.
function loadRotateDir() {
  try {
    return localStorage.getItem(ROTATE_DIR_KEY) === "ccw" ? "ccw" : "cw";
  } catch {
    return "cw";
  }
}

export const rotateDir = ref(loadRotateDir());

watch(rotateDir, (value) => {
  try {
    localStorage.setItem(ROTATE_DIR_KEY, value);
  } catch {
    // Not persisted; the choice still applies to this page.
  }
});

// Whether the image-settings panel beside the album list is expanded.
const IMAGE_SETTINGS_KEY = "photoframe.imageSettingsOpen";
export const imageSettingsOpen = ref(loadBool(IMAGE_SETTINGS_KEY));

watch(imageSettingsOpen, (value) => {
  try {
    localStorage.setItem(IMAGE_SETTINGS_KEY, value ? "true" : "false");
  } catch {
    // Not persisted; the choice still applies to this page.
  }
});

// Turn portrait photos to fill a landscape frame (and vice versa) automatically
// when adding them; the user can still rotate any photo by hand.
const AUTO_ROTATE_KEY = "photoframe.autoRotateFill";
function loadBoolDefault(key, fallback) {
  try {
    const v = localStorage.getItem(key);
    return v === null ? fallback : v === "true";
  } catch {
    return fallback;
  }
}
export const autoRotateFill = ref(loadBoolDefault(AUTO_ROTATE_KEY, true));

watch(autoRotateFill, (value) => {
  try {
    localStorage.setItem(AUTO_ROTATE_KEY, value ? "true" : "false");
  } catch {
    // Not persisted; the choice still applies to this page.
  }
});

// The album the last batch went to, so the next one defaults to it.
const UPLOAD_ALBUM_KEY = "photoframe.uploadAlbum";
function loadString(key, fallback) {
  try {
    return localStorage.getItem(key) || fallback;
  } catch {
    return fallback;
  }
}
export const lastUploadAlbum = ref(loadString(UPLOAD_ALBUM_KEY, "Default"));

watch(lastUploadAlbum, (value) => {
  try {
    localStorage.setItem(UPLOAD_ALBUM_KEY, value);
  } catch {
    // Not persisted; the choice still applies to this page.
  }
});
