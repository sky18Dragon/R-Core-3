import { defineStore } from "pinia";
import { ref, computed, watch } from "vue";
import { t } from "../i18n";
import { useAppStore } from "./app.js";
import { useSettingsStore } from "./settings.js";
import { usePresetsStore } from "./presets.js";
import { autoRotateFill, rotateDir, lastUploadAlbum } from "../utils/uiPrefs";
import {
  fitWithin,
  sourceEdgeCap,
  normalizeDeg,
  rotateCanvas,
  autoRotationFor,
  effectiveRotation,
} from "../utils/imageGeometry";
import { decodeToCanvas, loadImageFromFile } from "../utils/imageLoad";
import { relativePhotoPath, upForUpload } from "../utils/orientation";
import { PROCESSING_KEYS, composeCompareSheet } from "../utils/compareSheet";
import {
  makeBaseName,
  classifyUploadFailure,
  isRetryableFailure,
  isImageFile,
} from "../utils/uploadHelpers";

const THUMB_EDGE = 360; // queue tile preview
const GALLERY_THUMB_EDGE = 400; // thumbnail stored on the device
const RETRY_DELAY_MS = 2500;

// Decoded tile previews live outside reactive state (canvases are large and
// need no tracking); the store keeps only each tile's object URL.
const thumbBases = new Map();

let imageLib = null;
async function getLib() {
  imageLib ??= await import("@aitjcize/epaper-image-convert");
  return imageLib;
}

export const useUploadsStore = defineStore("uploads", () => {
  const appStore = useAppStore();
  const settingsStore = useSettingsStore();
  const presets = usePresetsStore();

  // Queue of photos waiting to go to the frame. Item fields:
  //   id, file, name, status: "pending" | "working" | "done" | "error",
  //   unreadable (browser can't decode it), error (text), thumbUrl,
  //   width/height (original), autoRotation (set when added),
  //   rotation (null = automatic, else explicit degrees clockwise),
  //   edit (null | {scaleMode, backgroundColor, zoom, panX, panY}) from the editor,
  //   presetId (null = the batch's preset) chosen for this photo alone
  const items = ref([]);

  // The processing preset for the whole batch (docs/PRD-PRESETS.md): null
  // means the default. `batchAdjusted` holds values fine-tuned from it for
  // this batch only; the preset itself is not changed.
  const batchPresetId = ref(null);
  const batchAdjusted = ref(null);
  const batchPreset = computed(
    () => presets.byId(batchPresetId.value) ?? presets.defaultPreset ?? null
  );
  const batchProcessing = computed(
    () => batchAdjusted.value ?? presets.paramsFor(batchPreset.value?.id ?? null)
  );
  const batchName = computed(() => {
    const name = batchPreset.value?.name ?? "";
    return batchAdjusted.value && name ? t("{name} (adjusted)", { name }) : name;
  });

  function chooseBatchPreset(id) {
    batchPresetId.value = id;
    batchAdjusted.value = null;
  }

  function adjustBatch(params) {
    const next = { ...batchProcessing.value };
    for (const key of PROCESSING_KEYS) if (params[key] !== undefined) next[key] = params[key];
    batchAdjusted.value = next;
  }

  function resetBatch() {
    batchAdjusted.value = null;
  }

  // What a photo is rendered with: its own preset, else the batch's
  function processingFor(presetId) {
    return presetId && presets.byId(presetId) ? presets.paramsFor(presetId) : batchProcessing.value;
  }

  function presetNameFor(presetId) {
    return presetId && presets.byId(presetId) ? presets.byId(presetId).name : batchName.value;
  }
  const running = ref(false);
  const summary = ref(null); // {album, done, failed, cancelled} after a run
  const lastBatch = ref([]); // [{id, name, album, thumbUrl}] of the latest run
  let cancelRequested = false;
  let nextId = 1;

  // The upload dialog: opened with photos from any "+" button or a drop
  const dialogOpen = ref(false);

  // Adds photos and opens the dialog; album (when given) becomes the target,
  // e.g. "+" inside an album uploads to that album.
  async function begin(files, album = null) {
    if (album && !running.value) targetAlbum.value = album;
    const result = await addFiles(files);
    if (items.value.length) dialogOpen.value = true;
    return result;
  }

  const targetAlbum = ref(lastUploadAlbum.value);
  watch(targetAlbum, (name) => {
    lastUploadAlbum.value = name;
  });

  const canSaveToAlbum = computed(
    () => appStore.systemInfo.sdcard_inserted || appStore.systemInfo.has_flash_storage
  );

  // Photos that still need sending: waiting, or failed and retryable
  const pendingCount = computed(() => items.value.filter((i) => i.status === "pending").length);
  const failedCount = computed(
    () => items.value.filter((i) => i.status === "error" && !i.unreadable).length
  );
  const doneCount = computed(() => items.value.filter((i) => i.status === "done").length);
  const hasUnsent = computed(() =>
    items.value.some((i) => i.status === "pending" || i.status === "working")
  );

  // Frame size as the user sees it (the saved orientation applied)
  function frameSize() {
    let w = appStore.systemInfo.width || 800;
    let h = appStore.systemInfo.height || 480;
    const orientation = settingsStore.appliedOrientation;
    if (orientation === "portrait" && w > h) [w, h] = [h, w];
    else if (orientation === "landscape" && w < h) [w, h] = [h, w];
    return { w, h };
  }

  const frameAspect = computed(() => {
    const { w, h } = frameSize();
    return `${w} / ${h}`;
  });

  function rotationOf(item) {
    return effectiveRotation(item.rotation, item.autoRotation, autoRotateFill.value);
  }

  // Tile preview = the small base image turned by what will really be uploaded
  async function refreshThumb(item) {
    const base = thumbBases.get(item.id);
    if (!base) return;
    const canvas = rotateCanvas(base, rotationOf(item));
    const blob = await new Promise((resolve) => canvas.toBlob(resolve, "image/jpeg", 0.8));
    if (!blob) return;
    if (item.thumbUrl) URL.revokeObjectURL(item.thumbUrl);
    item.thumbUrl = URL.createObjectURL(blob);
  }

  watch(autoRotateFill, () => {
    for (const item of items.value) {
      if (item.status !== "done") refreshThumb(item);
    }
  });

  // Add photos to the queue. Returns {added, skipped} (skipped = not images).
  async function addFiles(fileList) {
    const all = [...fileList];
    let files = all.filter(isImageFile);
    // Without storage the frame can only show one photo directly
    if (!canSaveToAlbum.value) files = files.slice(0, 1);
    const skipped = all.length - files.length;
    summary.value = null;
    if (!canSaveToAlbum.value) clear();
    // Finished photos are already on the frame; their previews live on in lastBatch
    items.value = items.value.filter((i) => i.status !== "done");

    const created = files.map((file) => {
      const item = {
        id: nextId++,
        file,
        name: file.name,
        status: "pending",
        unreadable: false,
        error: "",
        thumbUrl: "",
        width: 0,
        height: 0,
        autoRotation: 0,
        presetId: null,
        rotation: null,
        edit: null,
      };
      items.value.push(item);
      // Work on the reactive proxy so later changes render
      return items.value[items.value.length - 1];
    });

    // Decode one at a time: a phone photo is tens of megapixels
    for (const item of created) {
      try {
        const img = await loadImageFromFile(item.file);
        const size = fitWithin(img.width, img.height, THUMB_EDGE);
        const base = document.createElement("canvas");
        base.width = size.width;
        base.height = size.height;
        base.getContext("2d").drawImage(img, 0, 0, size.width, size.height);
        thumbBases.set(item.id, base);
        item.width = img.width;
        item.height = img.height;
        const frame = frameSize();
        item.autoRotation = autoRotationFor(
          img.width,
          img.height,
          frame.w,
          frame.h,
          rotateDir.value
        );
        await refreshThumb(item);
      } catch {
        item.status = "error";
        item.unreadable = true;
        item.error = t("This browser cannot read this image");
      }
    }
    return { added: created.length, skipped };
  }

  function rotateItem(item, delta) {
    item.rotation = normalizeDeg(rotationOf(item) + delta);
    rotateDir.value = delta > 0 ? "cw" : "ccw";
    // A custom crop is tied to the old orientation
    item.edit = null;
    refreshThumb(item);
  }

  // Apply what the single-photo editor produced
  function applyEdit(item, { rotation, edit, presetId = item.presetId ?? null }) {
    item.rotation = normalizeDeg(rotation);
    item.edit = edit;
    item.presetId = presetId;
    refreshThumb(item);
  }

  function removeItem(item) {
    if (item.thumbUrl && !lastBatch.value.some((b) => b.id === item.id)) {
      URL.revokeObjectURL(item.thumbUrl);
    }
    thumbBases.delete(item.id);
    items.value = items.value.filter((i) => i.id !== item.id);
  }

  // Empty the queue (finished photos keep their preview for the "just added" strip)
  function clear() {
    for (const item of items.value) {
      if (item.thumbUrl && !lastBatch.value.some((b) => b.id === item.id)) {
        URL.revokeObjectURL(item.thumbUrl);
      }
      thumbBases.delete(item.id);
    }
    items.value = [];
    summary.value = null;
  }

  function failureMessage(code) {
    switch (code) {
      case "unreachable":
        return t("Cannot reach the frame. Check that it is on and on the same Wi-Fi.");
      case "too-large":
        return t("The photo is too large for the frame (limit 5 MB after processing).");
      case "unsupported":
        return t("The frame does not accept this image format.");
      case "busy":
        return t("The frame is busy updating its screen. Try again in a moment.");
      case "full":
        return t("The frame's storage is full.");
      case "auth":
        return t("The frame asked for a password. Reload the page and sign in.");
      case "device":
        return t("The frame could not save the photo (storage busy or write failed).");
      default:
        return t("Upload failed.");
    }
  }

  // The processing values of a parameter set (layout fields are separate)
  function processingParams(p) {
    const params = {};
    for (const key of PROCESSING_KEYS) params[key] = p[key];
    return params;
  }

  // Same palette the previews dither with (device-calibrated)
  function paletteFor(lib) {
    return appStore.isGrayscale
      ? lib.makeGrayscale16({
          blackY: settingsStore.palette?.black_y ?? 0.009,
          whiteY: settingsStore.palette?.white_y ?? 0.65,
          gamma: settingsStore.palette?.gamma ?? 1.42,
          levelsY: settingsStore.palette?.levels_y ?? null,
        })
      : {
          ...lib.SPECTRA6,
          perceived:
            settingsStore.palette && Object.keys(settingsStore.palette).length > 0
              ? settingsStore.palette
              : lib.SPECTRA6.perceived,
        };
  }

  // Process one queued photo into the frame's format; returns the form to send.
  // `override` ({rotation, edit}) lets the editor test unsaved changes.
  async function buildForm(item, override = null) {
    const lib = await getLib();
    const width = appStore.systemInfo.width;
    const height = appStore.systemInfo.height;

    const { canvas: base } = await decodeToCanvas(item.file, sourceEdgeCap(width, height));
    const rotation = override ? normalizeDeg(override.rotation) : rotationOf(item);
    const source = rotateCanvas(base, rotation);
    const edit = override ? override.edit : item.edit;
    const presetId = override && "presetId" in override ? override.presetId : item.presetId;

    const p = settingsStore.params; // layout defaults
    const params = processingParams({ ...p, ...processingFor(presetId) });
    const palette = paletteFor(lib);

    const result = lib.processImage(source, {
      displayWidth: width,
      displayHeight: height,
      palette,
      params,
      orientation: settingsStore.appliedOrientation,
      scaleMode: edit?.scaleMode ?? p.scaleMode ?? "cover",
      backgroundColor: edit?.backgroundColor ?? p.backgroundColor ?? "white",
      zoom: edit?.zoom,
      panX: edit?.panX,
      panY: edit?.panY,
      usePerceivedOutput: false,
    });

    const compressed = await lib.createEPDGZ(result.canvas, { grayscale: appStore.isGrayscale });
    const rawBlob = new Blob([compressed], { type: "application/gzip" });

    const baseName = makeBaseName(item.file.name, Date.now());
    // Gallery thumbnail from the post-layout, pre-dither canvas, so it shows
    // what the frame will show (crop, fit, background all reflected)
    const thumbCanvas = lib.generateThumbnail(result.originalCanvas, GALLERY_THUMB_EDGE);
    const thumbBlob = await new Promise((resolve) =>
      thumbCanvas.toBlob(resolve, "image/jpeg", 0.85)
    );

    const form = new FormData();
    form.append("image", rawBlob, `${baseName}.epdgz`);
    form.append("thumbnail", thumbBlob, `${baseName}.jpg`);
    return form;
  }

  async function post(url, form) {
    try {
      const response = await fetch(url, { method: "POST", body: form });
      if (response.ok) {
        const body = await response.json().catch(() => ({}));
        return { ok: true, filepath: body.filepath || "" };
      }
      const text = await response.text().catch(() => "");
      return { ok: false, code: classifyUploadFailure(response.status, text) };
    } catch {
      return { ok: false, code: classifyUploadFailure(0) };
    }
  }

  // Send one item, retrying once when the device was merely busy
  async function sendToAlbum(item, album) {
    const form = await buildForm(item);
    let result = await post(`/api/upload?album=${encodeURIComponent(album)}`, form);
    if (!result.ok && isRetryableFailure(result.code)) {
      await new Promise((resolve) => setTimeout(resolve, RETRY_DELAY_MS));
      // The form's streams were consumed by the first attempt: build it again
      result = await post(`/api/upload?album=${encodeURIComponent(album)}`, await buildForm(item));
    }
    return result;
  }

  // Upload everything pending (and retry failed items) into `album`, one at a time
  async function start(album) {
    if (running.value) return null;
    running.value = true;
    cancelRequested = false;
    summary.value = null;
    targetAlbum.value = album;

    for (const old of lastBatch.value) URL.revokeObjectURL(old.thumbUrl);
    lastBatch.value = [];

    const queue = items.value.filter(
      (i) => !i.unreadable && (i.status === "pending" || i.status === "error")
    );
    let done = 0;
    let failed = 0;
    let stoppedBy = "";

    for (const item of queue) {
      if (cancelRequested) break;
      item.status = "working";
      item.error = "";
      try {
        const result = await sendToAlbum(item, album);
        if (result.ok) {
          // A portrait photo is stored on its side to fill the panel: record
          // which way is up so the page and the frame can stand it upright.
          // Also record the preset it was made with (shown with the photo).
          const up = upForUpload(rotationOf(item), item.height > item.width);
          const preset = presets.available ? presetNameFor(item.presetId) : "";
          const relative = relativePhotoPath(result.filepath);
          if ((up || preset) && relative) {
            const [photoAlbum, filename] = relative.split("/");
            await appStore.setPhotoMeta(
              { album: photoAlbum, filename },
              { ...(up ? { up } : {}), ...(preset ? { preset } : {}) }
            );
          }
          item.status = "done";
          done++;
          lastBatch.value.push({ id: item.id, name: item.name, album, thumbUrl: item.thumbUrl });
        } else {
          item.status = "error";
          item.error = failureMessage(result.code);
          failed++;
          // If the frame is gone, the rest would fail the same way
          if (result.code === "unreachable") {
            stoppedBy = result.code;
            break;
          }
        }
      } catch (err) {
        item.status = "error";
        // The error type is the only clue when this happens on a phone without a console.
        item.error = `${t("Could not process this image.")} (${err?.name || "Error"})`;
        console.error("Upload processing failed", err);
        failed++;
      }
    }

    running.value = false;
    // Report the whole list, not just this run: a retry should not make the
    // earlier successes disappear from the summary
    done = items.value.filter((i) => i.status === "done").length;
    failed = items.value.filter((i) => i.status === "error" && !i.unreadable).length;
    const cancelled = cancelRequested;
    const notTried = queue.filter((i) => i.status === "pending").length;
    summary.value = { album, done, failed, cancelled, notTried, stoppedBy };

    await appStore.loadSystemInfo();
    await appStore.loadAlbums();
    if (appStore.selectedAlbum === album) await appStore.loadImages(album);
    await appStore.reloadAlbumImages(album);
    return summary.value;
  }

  function cancel() {
    cancelRequested = true;
  }

  // Show one photo on the panel without saving it to an album
  async function displayOnly(item, override = null) {
    const result = await post("/api/display-image", await buildForm(item, override));
    if (result.ok) appStore.displayVersion++;
    return result.ok ? { ok: true } : { ok: false, message: failureMessage(result.code) };
  }

  // Side-by-side comparison and tuning: the photo decoded and turned once,
  // then rendered under one setting (full screen) or up to four (tiled 2×2)
  // and shown without saving.
  async function compareSource(item) {
    const { width, height } = appStore.systemInfo;
    const { canvas } = await decodeToCanvas(item.file, sourceEdgeCap(width, height));
    return rotateCanvas(canvas, rotationOf(item));
  }

  // A photo picked outside the queue, decoded upright
  async function sourceFromFile(file) {
    const { width, height } = appStore.systemInfo;
    const { canvas } = await decodeToCanvas(file, sourceEdgeCap(width, height));
    return canvas;
  }

  async function renderSheet(source, slots, { labels, layout = null, framing = null } = {}) {
    const lib = await getLib();
    return composeCompareSheet(lib, source, slots, {
      width: appStore.systemInfo.width,
      height: appStore.systemInfo.height,
      orientation: settingsStore.appliedOrientation,
      palette: paletteFor(lib),
      layout,
      framing,
      labels,
    });
  }

  function renderCompare(item, source, slots) {
    return renderSheet(source, slots, { layout: item.edit });
  }

  // Send a rendered sheet; waits out a screen that is still refreshing
  async function displaySheet(sheet, { busyRetries = 4, retryDelayMs = RETRY_DELAY_MS } = {}) {
    const lib = await getLib();
    const compressed = await lib.createEPDGZ(sheet.canvas, { grayscale: appStore.isGrayscale });
    const thumbCanvas = lib.generateThumbnail(sheet.original, GALLERY_THUMB_EDGE);
    const thumbBlob = await new Promise((resolve) =>
      thumbCanvas.toBlob(resolve, "image/jpeg", 0.85)
    );
    const baseName = makeBaseName("compare", Date.now());
    let result;
    for (let attempt = 0; ; attempt++) {
      const form = new FormData();
      form.append(
        "image",
        new Blob([compressed], { type: "application/gzip" }),
        `${baseName}.epdgz`
      );
      form.append("thumbnail", thumbBlob, `${baseName}.jpg`);
      result = await post("/api/display-image", form);
      if (result.ok || result.code !== "busy" || attempt >= busyRetries) break;
      await new Promise((resolve) => setTimeout(resolve, retryDelayMs));
    }
    if (result.ok) appStore.displayVersion++;
    return result.ok ? { ok: true } : { ok: false, message: failureMessage(result.code) };
  }

  const displayCompare = displaySheet;

  return {
    items,
    running,
    dialogOpen,
    begin,
    summary,
    lastBatch,
    targetAlbum,
    canSaveToAlbum,
    pendingCount,
    failedCount,
    frameAspect,
    frameSize,
    doneCount,
    hasUnsent,
    rotationOf,
    addFiles,
    rotateItem,
    applyEdit,
    removeItem,
    clear,
    start,
    cancel,
    displayOnly,
    compareSource,
    batchPresetId,
    batchPreset,
    batchAdjusted,
    batchProcessing,
    batchName,
    chooseBatchPreset,
    adjustBatch,
    resetBatch,
    processingFor,
    presetNameFor,
    sourceFromFile,
    renderSheet,
    displaySheet,
    renderCompare,
    displayCompare,
  };
});
