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
