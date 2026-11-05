import { ref, watch } from "vue";
import { normalizeDeg, rotateCanvas } from "./imageGeometry";

// The photo presets are judged with (docs/PRD-PRESETS.md), shared by the
// tuning rounds and the preset editor: the decoded photo, its quarter turns,
// and how it sits on the screen (the same for every setting). The framing is
// remembered in this browser; the photo itself is not.

const FRAMING_KEY = "photoframe.testFraming";
const MODES = ["cover", "fit", "custom"];

function loadFraming() {
  try {
    const saved = JSON.parse(localStorage.getItem(FRAMING_KEY) || "null");
    if (saved && MODES.includes(saved.scaleMode)) return { zoom: 1, x: 0, y: 0, ...saved };
  } catch {
    // Storage blocked or empty
  }
  return { scaleMode: "cover", zoom: 1, x: 0, y: 0 };
}

export const testPhotoName = ref("");
export const testRotation = ref(0);
export const testFraming = ref(loadFraming());
export const testPhotoVersion = ref(0); // bumps whenever the picture changes

let upright = null;
let turned = null;

watch(
  testFraming,
  (value) => {
    try {
      localStorage.setItem(FRAMING_KEY, JSON.stringify(value));
    } catch {
      // Not remembered, still works
    }
  },
  { deep: true }
);

export function hasTestPhoto() {
  return !!turned;
}

// The photo to render: upright as decoded, turned by testRotation
export function testSource() {
  return turned;
}

// Upright to start with: filling the screen crops it rather than laying it on
// its side; turning is the user's choice
export function setTestPhoto(canvas, name) {
  upright = canvas;
  testRotation.value = 0;
  turned = upright;
  testPhotoName.value = name;
  testPhotoVersion.value++;
}

export function turnTestPhoto(delta) {
  if (!upright) return;
  testRotation.value = normalizeDeg(testRotation.value + delta);
  turned = rotateCanvas(upright, testRotation.value);
  testPhotoVersion.value++;
}

export function setTestFraming(patch) {
  testFraming.value = { ...testFraming.value, ...patch };
}
