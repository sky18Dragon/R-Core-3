<script setup>
import { t } from "../i18n";
import { ref, computed, watch, onMounted, onBeforeUnmount } from "vue";
import { useDisplay } from "vuetify";
import { useAppStore, usePresetsStore, useSettingsStore, useUploadsStore } from "../stores";
import ProcessingControls from "./ProcessingControls.vue";
import SavePresetDialog from "./SavePresetDialog.vue";
import PhotoEditDialog from "./PhotoEditDialog.vue";
import CompareDialog from "./CompareDialog.vue";
import NewAlbumDialog from "./NewAlbumDialog.vue";
import AiImageDialog from "./AiImageDialog.vue";
import { autoRotateFill } from "../utils/uiPrefs";
import { albumLabel } from "../utils/albums";
import { goTo } from "../utils/viewState";
import { isPortraitUp, upForUpload } from "../utils/orientation";

// The upload dialog: opened by any "+" or a drop with photos already chosen.
const appStore = useAppStore();
const { xs } = useDisplay();
const settingsStore = useSettingsStore();
const uploads = useUploadsStore();
const presets = usePresetsStore();

const fileInput = ref(null);
const dragging = ref(false);
const editing = ref(null); // the queued photo open in the editor
const newAlbumOpen = ref(false);
const albumMenu = ref(false);
const aiOpen = ref(false);
const confirmDiscard = ref(false);

const snackbar = ref(false);
const snackbarText = ref("");
const snackbarColor = ref("info");
function notify(text, color = "info") {
  snackbarText.value = text;
  snackbarColor.value = color;
  snackbar.value = true;
}

// --- choosing photos -------------------------------------------------------

function openPicker() {
  fileInput.value?.click();
}

async function addFiles(files) {
  if (!files || !files.length) return;
  const { skipped } = await uploads.addFiles(files);
  if (skipped > 0 && uploads.canSaveToAlbum) {
    notify(t("{n} files were not images and were skipped", { n: skipped }), "warning");
  } else if (!uploads.canSaveToAlbum && files.length > 1) {
    notify(t("Without a storage card only one photo can be shown at a time"), "warning");
  }
}

async function onPicked(event) {
  const files = [...(event.target.files || [])];
  event.target.value = ""; // so the same photo can be chosen again
  await addFiles(files);
}

function onDrop(event) {
  dragging.value = false;
  addFiles(event.dataTransfer?.files);
}

// --- album target ----------------------------------------------------------

const albumItems = computed(() =>
  appStore.sortedAlbums.map((a) => ({ title: albumLabel(a.name), value: a.name }))
);

// The remembered album may have been deleted since
watch(
  () => appStore.albums,
  (albums) => {
    if (albums.length && !albums.some((a) => a.name === uploads.targetAlbum)) {
      uploads.targetAlbum = "Default";
    }
  },
  { immediate: true, deep: true }
);

function onAlbumCreated(name) {
  uploads.targetAlbum = name;
}

function newAlbumFromMenu() {
  albumMenu.value = false;
  newAlbumOpen.value = true;
}

// --- queue state -----------------------------------------------------------

const queueTotal = computed(() => uploads.items.filter((i) => !i.unreadable).length);
const processed = computed(
  () =>
    uploads.items.filter((i) => i.status === "done" || (i.status === "error" && !i.unreadable))
      .length
);
const progress = computed(() =>
  queueTotal.value ? (processed.value / queueTotal.value) * 100 : 0
);

const sendLabel = computed(() => {
  if (!uploads.canSaveToAlbum) return t("Show on frame");
  // Failed photos are sent again with the rest, so they count too
  if (uploads.pendingCount > 0) {
    return t("Add to album ({n})", { n: uploads.pendingCount + uploads.failedCount });
  }
  return t("Retry failed ({n})", { n: uploads.failedCount });
});
const canSend = computed(
  () => !uploads.running && (uploads.pendingCount > 0 || uploads.failedCount > 0)
);

async function send() {
  if (!uploads.canSaveToAlbum) {
    const item = uploads.items.find((i) => !i.unreadable);
    if (!item) return;
    item.status = "working";
    const result = await uploads.displayOnly(item);
    item.status = result.ok ? "done" : "error";
    item.error = result.ok ? "" : result.message;
    notify(
      result.ok ? t("Sent to the frame. The screen is refreshing.") : result.message,
      result.ok ? "success" : "error"
    );
    return;
  }
  await uploads.start(uploads.targetAlbum);
}

function viewAlbum() {
  const album = uploads.summary.album;
  finish();
  goTo("albums", album);
}

// Closing keeps nothing behind: finished photos are on the frame already, and
// photos not sent yet are dropped after a confirmation.
function finish() {
  uploads.clear();
  uploads.dialogOpen = false;
}
function requestClose() {
  if (uploads.running) return;
  if (uploads.hasUnsent) confirmDiscard.value = true;
  else finish();
}
function discard() {
  confirmDiscard.value = false;
  finish();
}

const dialogModel = computed({
  get: () => uploads.dialogOpen,
  set: (value) => {
    if (!value) requestClose();
  },
});

const dialogTitle = computed(() => {
  if (!uploads.canSaveToAlbum) return t("Show a photo on the frame");
  const n = uploads.items.filter((i) => !i.unreadable).length;
  return n ? t("Upload {n} photos", { n }) : t("Upload photos");
});

// How the frame will file this photo (portrait photos take turns when it
// stands up), so a wrong guess can be turned before it is sent.
function shapeOf(item) {
  if (!item.width || !item.height) return "";
  const up = upForUpload(uploads.rotationOf(item), item.height > item.width);
  return isPortraitUp(up) ? t("Portrait photo") : t("Landscape photo");
}

// What the tile preview should look like for the layout this photo will get
function tileFit(item) {
  const mode = item.edit?.scaleMode ?? settingsStore.params.scaleMode ?? "cover";
  return mode === "cover" ? "cover" : "contain";
}
function tileBackground(item) {
  const bg = item.edit?.backgroundColor ?? settingsStore.params.backgroundColor ?? "white";
  return bg === "black" ? "#000" : "#fff";
}

const summaryAlert = computed(() => {
  const s = uploads.summary;
  if (!s) return null;
  const album = albumLabel(s.album);
  if (s.stoppedBy === "unreachable") {
    return {
      type: "error",
      text: t("Lost contact with the frame. {done} added; the rest are still in the list.", {
        done: s.done,
      }),
    };
  }
  if (s.cancelled) {
    return {
      type: "info",
      text: t("Stopped. {done} added to {album}; {left} not sent.", {
        done: s.done,
        album,
        left: s.notTried + s.failed,
      }),
    };
  }
  if (s.failed > 0) {
    return {
      type: "warning",
      text: t("{done} added to {album}; {failed} failed.", {
        done: s.done,
        album,
        failed: s.failed,
      }),
    };
  }
  return { type: "success", text: t("{done} added to {album}.", { done: s.done, album }) };
});

const allDone = computed(
  () =>
    uploads.summary &&
    uploads.summary.failed === 0 &&
    !uploads.summary.cancelled &&
    uploads.pendingCount === 0
);

// A clean run closes the dialog and leaves a card at the bottom of the page,
// like a phone's backup notice; failures keep the dialog open for a retry.
const RESULT_MS = 8000;
const result = ref(null); // { album, done, thumb }
let resultTimer = null;
watch(
  () => uploads.summary,
  (summary) => {
    if (!summary || uploads.running || !allDone.value || !uploads.canSaveToAlbum) return;
    result.value = {
      album: summary.album,
      done: summary.done,
      thumb: uploads.lastBatch[0]?.thumbUrl || "",
    };
    clearTimeout(resultTimer);
    resultTimer = setTimeout(() => (result.value = null), RESULT_MS);
    finish();
  }
);
function openResultAlbum() {
  const album = result.value.album;
  result.value = null;
  goTo("albums", album);
}
onBeforeUnmount(() => clearTimeout(resultTimer));

// --- processing defaults ---------------------------------------------------

// The batch renders with a preset (docs/PRD-PRESETS.md); fine-tuning changes
// this batch only and can be kept as a new preset.
const presetItems = computed(() =>
  presets.list.map((p) => ({
    value: p.id,
    title: p.id === presets.defaultId ? `★ ${p.name}` : p.name,
  }))
);
const showPresets = computed(() => presets.available && presets.list.length > 0);
const fineParams = computed(() => ({ ...settingsStore.params, ...uploads.batchProcessing }));
const savingPreset = ref(false);

function onPresetSaved(id) {
  savingPreset.value = false;
  uploads.chooseBatchPreset(id);
  notify(t("Saved “{name}”", { name: presets.byId(id)?.name ?? "" }), "success");
}

const makingDefault = ref(false);
async function makeBatchDefault() {
  makingDefault.value = true;
  const ok = await presets.setDefault(uploads.batchPreset.id);
  makingDefault.value = false;
  notify(
    ok
      ? t("“{name}” is now the default", { name: uploads.batchPreset.name })
      : t("Could not save the preset on the frame"),
    ok ? "success" : "error"
  );
}

// Layout is not part of a preset: a change here becomes the frame's default
const scaleModeOptions = computed(() => [
  { title: t("Cover (crop to fill)"), value: "cover" },
  { title: t("Fit (letterbox)"), value: "fit" },
]);
const backgroundOptions = computed(() => [
  { title: t("White"), value: "white" },
  { title: t("Black"), value: "black" },
]);
async function setLayout(key, value) {
  settingsStore.params[key] = value;
  if (!(await settingsStore.saveSettings())) notify(t("Failed to save settings"), "error");
}

// Each photo can use a preset of its own; the tile shows which
function tilePreset(item) {
  return item.presetId && presets.byId(item.presetId) ? presets.byId(item.presetId).name : "";
}

// Side-by-side comparison uses the first photo in the queue
const comparing = ref(null);
const compareItem = computed(() => uploads.items.find((i) => !i.unreadable) ?? null);

// --- misc ------------------------------------------------------------------

const aiAvailable = computed(
  () =>
    !!settingsStore.deviceSettings.aiCredentials.openaiApiKey ||
    !!settingsStore.deviceSettings.aiCredentials.googleApiKey
);

// Leaving the page would lose photos that have not been sent yet
function warnBeforeLeaving(event) {
  if (uploads.hasUnsent) {
    event.preventDefault();
    event.returnValue = "";
  }
}
onMounted(() => window.addEventListener("beforeunload", warnBeforeLeaving));
onBeforeUnmount(() => window.removeEventListener("beforeunload", warnBeforeLeaving));
</script>

<template>
  <v-dialog
    v-model="dialogModel"
    :fullscreen="xs"
    max-width="980"
    scrollable
    :persistent="uploads.running"
  >
    <v-card
      class="photo-uploader"
      :class="{ 'photo-uploader--drag': dragging }"
      @dragover.prevent="dragging = true"
      @dragleave.self="dragging = false"
      @drop.prevent="onDrop"
    >
      <input
        ref="fileInput"
        type="file"
        :multiple="uploads.canSaveToAlbum"
        accept=".jpg,.jpeg,.png,.heic,.heif,.webp,.gif,.bmp"
        class="d-none"
        @change="onPicked"
      />

      <v-card-title class="d-flex align-center">
        <v-btn
          icon="mdi-close"
          variant="text"
          class="mr-2 ml-n2"
          :disabled="uploads.running"
          :aria-label="t('Close')"
          @click="requestClose"
        />
        {{ dialogTitle }}
      </v-card-title>

      <v-card-text>
        <!-- Result of the last run -->
        <v-alert v-if="summaryAlert" :type="summaryAlert.type" variant="tonal" class="mb-4">
          <div>{{ summaryAlert.text }}</div>
          <div class="d-flex flex-wrap ga-2 mt-3">
            <v-btn
              v-if="uploads.summary.done > 0"
              size="small"
              variant="flat"
              color="primary"
              @click="viewAlbum"
            >
              {{ t("View album") }}
            </v-btn>
            <v-btn v-if="allDone" size="small" variant="outlined" @click="openPicker">
              {{ t("Add more") }}
            </v-btn>
          </div>
        </v-alert>

        <v-alert
          v-if="appStore.systemInfoLoaded && !uploads.canSaveToAlbum"
          type="info"
          variant="tonal"
          density="compact"
          class="mb-4"
        >
          {{ t("No storage card: the photo is shown on the frame directly and not saved.") }}
        </v-alert>

        <!-- Everything removed: pick again -->
        <v-sheet
          v-if="!uploads.items.length"
          class="dropzone d-flex flex-column align-center justify-center pa-6"
          rounded
          border
          role="button"
          tabindex="0"
          :aria-label="t('Choose photos')"
          @click="openPicker"
          @keyup.enter="openPicker"
        >
          <v-icon icon="mdi-image-plus-outline" size="48" />
          <p class="text-body-1 mt-3 text-center">{{ t("Choose photos") }}</p>
        </v-sheet>

        <!-- Chosen photos -->
        <v-row v-else dense>
          <v-col v-for="item in uploads.items" :key="item.id" cols="6" sm="4" md="3">
            <v-card variant="outlined" class="photo-tile" :class="`photo-tile--${item.status}`">
              <div
                class="tile-frame"
                :style="{ aspectRatio: uploads.frameAspect, background: tileBackground(item) }"
              >
                <img
                  v-if="item.thumbUrl"
                  :src="item.thumbUrl"
                  :alt="item.name"
                  :style="{ objectFit: tileFit(item) }"
                />
                <div v-else class="tile-empty">
                  <v-icon v-if="item.unreadable" icon="mdi-image-broken-variant" />
                  <v-progress-circular v-else indeterminate size="22" width="2" />
                </div>
                <span v-if="!item.unreadable && shapeOf(item)" class="tile-shape">
                  {{ shapeOf(item) }}
                </span>
                <span v-if="!item.unreadable && tilePreset(item)" class="tile-preset">
                  {{ tilePreset(item) }}
                </span>
                <div v-if="item.status !== 'pending' && !item.unreadable" class="tile-status">
                  <v-progress-circular v-if="item.status === 'working'" indeterminate size="26" />
                  <v-icon
                    v-else-if="item.status === 'done'"
                    icon="mdi-check-circle"
                    color="success"
                    size="30"
                  />
                  <v-icon v-else icon="mdi-alert-circle" color="error" size="30" />
                </div>
              </div>
              <div v-if="item.error" class="text-caption text-error px-2 pt-1">
                {{ item.error }}
              </div>
              <div
                v-if="!uploads.running && item.status !== 'done'"
                class="tile-actions d-flex px-1"
              >
                <template v-if="!item.unreadable">
                  <v-btn
                    icon="mdi-rotate-left"
                    size="small"
                    variant="text"
                    :aria-label="t('Rotate left 90°')"
                    @click="uploads.rotateItem(item, -90)"
                  />
                  <v-btn
                    icon="mdi-rotate-right"
                    size="small"
                    variant="text"
                    :aria-label="t('Rotate right 90°')"
                    @click="uploads.rotateItem(item, 90)"
                  />
                  <v-btn
                    icon="mdi-pencil-outline"
                    size="small"
                    variant="text"
                    :aria-label="t('Adjust this photo')"
                    @click="editing = item"
                  />
                </template>
                <v-spacer />
                <v-btn
                  icon="mdi-close"
                  size="small"
                  variant="text"
                  :aria-label="t('Remove')"
                  @click="uploads.removeItem(item)"
                />
              </div>
            </v-card>
          </v-col>
          <v-col v-if="uploads.canSaveToAlbum && !uploads.running" cols="6" sm="4" md="3">
            <v-sheet
              class="tile-add d-flex flex-column align-center justify-center"
              :style="{ aspectRatio: uploads.frameAspect }"
              rounded
              border
              role="button"
              tabindex="0"
              :aria-label="t('Add more')"
              @click="openPicker"
              @keyup.enter="openPicker"
            >
              <v-icon icon="mdi-plus" size="28" />
              <span class="text-caption">{{ t("Add more") }}</span>
            </v-sheet>
          </v-col>
        </v-row>

        <!-- Where they go, and how they are prepared -->
        <div v-if="uploads.items.length" class="options mt-5">
          <v-select
            v-if="uploads.canSaveToAlbum"
            v-model="uploads.targetAlbum"
            v-model:menu="albumMenu"
            :items="albumItems"
            :label="t('Put in album')"
            variant="outlined"
            density="comfortable"
            hide-details
            class="album-select"
            :disabled="uploads.running"
          >
            <template #append-item>
              <v-divider class="my-1" />
              <v-list-item
                prepend-icon="mdi-plus"
                :title="t('New album')"
                @click="newAlbumFromMenu"
              />
            </template>
          </v-select>

          <v-expansion-panels variant="accordion" elevation="0" class="mt-3">
            <v-expansion-panel>
              <v-expansion-panel-title>
                <v-icon icon="mdi-tune" class="mr-3" />
                {{ t("Processing") }}
                <span class="text-caption text-medium-emphasis ml-3 text-truncate">
                  {{ uploads.batchName }}
                </span>
              </v-expansion-panel-title>
              <v-expansion-panel-text>
                <v-switch
                  v-model="autoRotateFill"
                  :label="t('Turn photos to fill the screen')"
                  color="primary"
                  density="compact"
                  hide-details
                  class="mb-2"
                />
                <v-select
                  v-if="showPresets"
                  :model-value="uploads.batchPreset?.id"
                  :items="presetItems"
                  :label="t('Preset')"
                  :hint="
                    uploads.batchAdjusted
                      ? t('Adjusted for this upload only')
                      : t('For every photo here')
                  "
                  persistent-hint
                  variant="outlined"
                  density="comfortable"
                  class="mb-5"
                  @update:model-value="uploads.chooseBatchPreset($event)"
                />
                <div class="layout-row">
                  <v-select
                    :model-value="settingsStore.params.scaleMode || 'cover'"
                    :items="scaleModeOptions"
                    :label="t('Scale Mode')"
                    variant="outlined"
                    density="compact"
                    hide-details
                    @update:model-value="setLayout('scaleMode', $event)"
                  />
                  <v-select
                    v-if="(settingsStore.params.scaleMode || 'cover') === 'fit'"
                    :model-value="settingsStore.params.backgroundColor || 'white'"
                    :items="backgroundOptions"
                    :label="t('Background Color')"
                    variant="outlined"
                    density="compact"
                    hide-details
                    @update:model-value="setLayout('backgroundColor', $event)"
                  />
                </div>
                <v-expansion-panels variant="accordion" elevation="0" class="mt-3 fine-tune">
                  <v-expansion-panel>
                    <v-expansion-panel-title class="px-3">
                      {{ t("Fine-tune") }}
                    </v-expansion-panel-title>
                    <v-expansion-panel-text>
                      <ProcessingControls
                        :params="fineParams"
                        :show-presets="false"
                        :show-layout="false"
                        @update:params="uploads.adjustBatch($event)"
                      />
                    </v-expansion-panel-text>
                  </v-expansion-panel>
                </v-expansion-panels>
                <div v-if="uploads.batchAdjusted" class="adjusted-row">
                  <span class="text-body-2">{{ t("Adjusted for this upload only.") }}</span>
                  <v-btn size="small" variant="text" @click="uploads.resetBatch()">
                    {{ t("Undo changes") }}
                  </v-btn>
                  <v-btn
                    v-if="presets.available"
                    size="small"
                    variant="text"
                    @click="savingPreset = true"
                  >
                    {{ t("Save as a new preset") }}
                  </v-btn>
                </div>
                <div class="d-flex justify-end flex-wrap ga-1 mt-2">
                  <v-btn
                    variant="text"
                    size="small"
                    :disabled="!compareItem || uploads.running"
                    :title="t('Try four settings on the frame at once')"
                    @click="comparing = compareItem"
                  >
                    <v-icon icon="mdi-view-grid-outline" start />
                    {{ t("Compare side by side") }}
                  </v-btn>
                  <v-btn
                    v-if="
                      showPresets &&
                      !uploads.batchAdjusted &&
                      uploads.batchPreset &&
                      (uploads.batchPreset.id !== presets.defaultId || presets.defaultModified)
                    "
                    variant="text"
                    size="small"
                    :loading="makingDefault"
                    @click="makeBatchDefault"
                  >
                    {{ t("Make this preset the default") }}
                  </v-btn>
                </div>
              </v-expansion-panel-text>
            </v-expansion-panel>
          </v-expansion-panels>

          <v-btn v-if="aiAvailable" variant="text" size="small" class="mt-1" @click="aiOpen = true">
            <v-icon icon="mdi-magic-staff" start />
            {{ t("Generate with AI") }}
          </v-btn>
        </div>
      </v-card-text>

      <v-divider v-if="uploads.items.length" />
      <v-card-actions v-if="uploads.items.length" class="send-bar px-4 py-3 flex-wrap ga-2">
        <div v-if="uploads.running" class="send-progress flex-grow-1">
          <div class="text-body-2 mb-1">
            {{ t("Uploading {done} / {total}", { done: processed, total: queueTotal }) }}
          </div>
          <v-progress-linear :model-value="progress" color="primary" height="6" rounded />
        </div>
        <v-btn v-if="uploads.running" variant="text" @click="uploads.cancel()">
          {{ t("Stop after this photo") }}
        </v-btn>
        <template v-else-if="allDone">
          <v-spacer />
          <v-btn color="primary" variant="flat" @click="finish">{{ t("Done") }}</v-btn>
        </template>
        <template v-else>
          <v-spacer />
          <v-btn variant="text" @click="requestClose">{{ t("Cancel") }}</v-btn>
          <v-btn color="primary" variant="flat" :disabled="!canSend" @click="send">
            <v-icon :icon="uploads.canSaveToAlbum ? 'mdi-upload' : 'mdi-monitor'" start />
            {{ sendLabel }}
          </v-btn>
        </template>
      </v-card-actions>

      <PhotoEditDialog :item="editing" @close="editing = null" />
      <CompareDialog :item="comparing" @close="comparing = null" />
      <SavePresetDialog
        :open="savingPreset"
        :params="uploads.batchProcessing"
        :suggested-name="uploads.batchPreset?.name ?? ''"
        source="manual"
        @close="savingPreset = false"
        @saved="onPresetSaved"
      />
      <NewAlbumDialog v-model="newAlbumOpen" @created="onAlbumCreated" />
      <AiImageDialog v-model="aiOpen" @generated="(file) => addFiles([file])" />

      <v-dialog v-model="confirmDiscard" max-width="380">
        <v-card>
          <v-card-title>{{ t("Discard these photos?") }}</v-card-title>
          <v-card-text>{{ t("Photos not uploaded yet are removed from the list.") }}</v-card-text>
          <v-card-actions>
            <v-spacer />
            <v-btn variant="text" @click="confirmDiscard = false">{{ t("Keep editing") }}</v-btn>
            <v-btn color="primary" @click="discard">{{ t("Discard") }}</v-btn>
          </v-card-actions>
        </v-card>
      </v-dialog>

      <v-snackbar v-model="snackbar" :color="snackbarColor" :timeout="4000">
        {{ snackbarText }}
        <template #actions>
          <v-btn variant="text" @click="snackbar = false">{{ t("Close") }}</v-btn>
        </template>
      </v-snackbar>
    </v-card>
  </v-dialog>

  <v-slide-y-reverse-transition>
    <div v-if="result" class="upload-result" role="status">
      <div class="result-text">
        <div class="text-body-1">
          {{
            t("Added {n} photos to {album}", { n: result.done, album: albumLabel(result.album) })
          }}
        </div>
        <v-btn variant="text" class="mt-2 ml-n3" @click="openResultAlbum">
          <v-icon icon="mdi-image-album" start />
          {{ t("View album") }}
        </v-btn>
      </div>
      <div class="result-thumb">
        <img v-if="result.thumb" :src="result.thumb" alt="" />
        <button type="button" class="result-close" :aria-label="t('Close')" @click="result = null">
          <v-icon icon="mdi-close" size="18" />
        </button>
      </div>
    </div>
  </v-slide-y-reverse-transition>
</template>

<style scoped>
.photo-uploader {
  transition: box-shadow 0.15s;
}

.upload-result {
  position: fixed;
  left: 16px;
  bottom: 16px;
  z-index: 2400;
  display: flex;
  align-items: stretch;
  gap: 16px;
  width: min(420px, calc(100vw - 32px));
  padding: 16px;
  border-radius: 16px;
  background: rgb(var(--v-theme-surface));
  color: rgb(var(--v-theme-on-surface));
  border: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
  box-shadow: 0 6px 24px rgba(0, 0, 0, 0.25);
}
.result-text {
  flex: 1;
  min-width: 0;
}
.result-thumb {
  position: relative;
  width: 96px;
  height: 96px;
  flex: none;
  border-radius: 8px;
  overflow: hidden;
  background: rgba(var(--v-theme-on-surface), 0.06);
}
.result-thumb img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}
.result-close {
  position: absolute;
  top: 4px;
  right: 4px;
  display: flex;
  padding: 3px;
  border-radius: 50%;
  color: #fff;
  background: rgba(0, 0, 0, 0.55);
}
@media (max-width: 600px) {
  .upload-result {
    left: 8px;
    right: 8px;
    bottom: 8px;
    width: auto;
  }
}
.photo-uploader--drag {
  outline: 2px dashed rgb(var(--v-theme-primary));
  outline-offset: -2px;
}

.dropzone {
  cursor: pointer;
  min-height: 200px;
  transition: background-color 0.2s;
}
.dropzone:hover,
.dropzone:focus-visible {
  background-color: rgba(var(--v-theme-primary), 0.05);
}

.photo-tile {
  overflow: hidden;
}
.tile-frame {
  position: relative;
  width: 100%;
  overflow: hidden;
}
.tile-frame img {
  display: block;
  width: 100%;
  height: 100%;
}
.tile-empty {
  position: absolute;
  inset: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(var(--v-theme-on-surface), 0.06);
}
.tile-preset {
  z-index: 1;
  position: absolute;
  right: 6px;
  bottom: 6px;
  max-width: calc(100% - 12px);
  padding: 1px 6px;
  border-radius: 4px;
  font-size: 0.7rem;
  line-height: 1.4;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  color: #fff;
  background: rgba(0, 0, 0, 0.6);
}
.layout-row {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
  gap: 12px;
}
.fine-tune {
  border: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
  border-radius: 8px;
}
.adjusted-row {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 4px;
  margin-top: 8px;
}
.tile-shape {
  position: absolute;
  top: 6px;
  left: 6px;
  z-index: 1;
  padding: 1px 8px;
  border-radius: 10px;
  font-size: 0.75rem;
  color: #fff;
  background: rgba(0, 0, 0, 0.6);
}
.tile-status {
  position: absolute;
  inset: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(255, 255, 255, 0.55);
}
.tile-name {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.photo-tile--done {
  opacity: 0.85;
}
.photo-tile--error {
  border-color: rgb(var(--v-theme-error));
}

.tile-add {
  cursor: pointer;
  border-style: dashed;
  color: rgba(var(--v-theme-on-surface), 0.6);
}
.tile-add:hover,
.tile-add:focus-visible {
  background: rgba(var(--v-theme-primary), 0.05);
}

.album-select {
  max-width: 360px;
}

@media (max-width: 600px) {
  .send-bar .v-btn {
    min-height: 44px;
  }
}
</style>
