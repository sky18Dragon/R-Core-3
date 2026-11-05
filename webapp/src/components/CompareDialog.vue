<script setup>
import { t } from "../i18n";
import { computed, onBeforeUnmount, ref, watch } from "vue";
import { useDisplay } from "vuetify";
import { usePresetsStore, useSettingsStore, useUploadsStore } from "../stores";
import ProcessingControls from "./ProcessingControls.vue";
import SavePresetDialog from "./SavePresetDialog.vue";
import { SHEET_LABELS } from "../utils/compareSheet";
import { processingOf } from "../utils/presets";
import { startList } from "../utils/tuning";

// The same photo under four processing settings, tiled 2×2 and shown on the
// frame together, so the best one is picked on the panel itself.
const props = defineProps({
  // The queued photo to compare with, or null when closed
  item: { type: Object, default: null },
});
const emit = defineEmits(["close"]);

const settingsStore = useSettingsStore();
const uploads = useUploadsStore();
const presets = usePresetsStore();
const { xs } = useDisplay();

const PREVIEW_DELAY_MS = 400;

const slots = ref([]);
const active = ref(0);
const previewUrl = ref("");
const rendering = ref(false);
const sending = ref(false);
const saving = ref(false); // the save-as-preset dialog is open
const notice = ref(null); // {type, text}
let source = null;
let sheet = null;
let timer = null;
let renderId = 0;

function slotTitle(s) {
  return s.name ?? t("Adjusted");
}

// Fresh slots and a decoded photo each time the dialog opens
watch(
  () => props.item,
  async (item) => {
    clearTimeout(timer);
    notice.value = null;
    sheet = null;
    source = null;
    if (!item) return;
    active.value = 0;
    // A: what this batch uses now; then the other presets; then variations
    const base = { ...settingsStore.params, ...uploads.batchProcessing };
    const others = presets.list
      .filter((p) => p.id !== uploads.batchPreset?.id)
      .map((p) => ({ name: p.name, params: p.params }));
    slots.value = startList(base, uploads.batchName || null, others, 4).map((c) => ({
      name: c.name,
      params: c.params,
    }));
    rendering.value = true;
    try {
      source = await uploads.compareSource(item);
    } catch {
      rendering.value = false;
      notice.value = { type: "error", text: t("Could not read this photo") };
      return;
    }
    render();
  },
  { immediate: true }
);

// Re-render shortly after the last change, so dragging a slider stays smooth
watch(
  slots,
  () => {
    if (!source) return;
    clearTimeout(timer);
    timer = setTimeout(render, PREVIEW_DELAY_MS);
  },
  { deep: true }
);

async function render() {
  if (!source || !props.item) return;
  const id = ++renderId;
  rendering.value = true;
  // Let the spinner paint before the (synchronous) processing starts
  await new Promise((resolve) => setTimeout(resolve, 0));
  if (id !== renderId) return;
  try {
    const result = await uploads.renderCompare(props.item, source, slots.value);
    if (id !== renderId) return;
    sheet = result;
    const blob = await new Promise((resolve) => result.preview.toBlob(resolve, "image/png"));
    if (id !== renderId) return;
    if (previewUrl.value) URL.revokeObjectURL(previewUrl.value);
    previewUrl.value = URL.createObjectURL(blob);
  } catch {
    notice.value = { type: "error", text: t("Could not prepare the comparison") };
  } finally {
    if (id === renderId) rendering.value = false;
  }
}

onBeforeUnmount(() => {
  clearTimeout(timer);
  if (previewUrl.value) URL.revokeObjectURL(previewUrl.value);
});

const slot = computed(() => slots.value[active.value]);

function onParams(params) {
  const s = slot.value;
  s.params = { ...s.params, ...params };
  s.name = null; // changed: no longer the preset it came from
}

async function showOnFrame() {
  if (rendering.value) await render();
  if (!sheet) return;
  sending.value = true;
  notice.value = null;
  const result = await uploads.displayCompare(sheet);
  sending.value = false;
  notice.value = result.ok
    ? { type: "success", text: t("Sent to the frame. Compare A–D on the panel.") }
    : { type: "error", text: result.message };
}

function onSaved(id) {
  saving.value = false;
  const label = SHEET_LABELS[active.value];
  notice.value = {
    type: "success",
    text: t("Saved {label} as “{name}”", { label, name: presets.byId(id)?.name ?? "" }),
  };
}
</script>

<template>
  <v-dialog
    :model-value="!!item"
    :fullscreen="xs"
    max-width="1100"
    scrollable
    @update:model-value="(open) => !open && emit('close')"
  >
    <v-card v-if="item">
      <v-toolbar density="comfortable" color="surface">
        <v-toolbar-title class="text-body-1">{{ t("Compare side by side") }}</v-toolbar-title>
        <v-btn icon="mdi-close" :aria-label="t('Close')" @click="emit('close')" />
      </v-toolbar>
      <v-divider />
      <v-card-text class="px-2 px-sm-4">
        <p class="text-body-2 text-medium-emphasis mb-3">
          {{
            t(
              "The same photo with four settings on one screen. Show it on the frame, compare under the same light, then keep the best one."
            )
          }}
        </p>
        <div class="compare-layout">
          <div class="sheet" :style="{ aspectRatio: uploads.frameAspect }">
            <img v-if="previewUrl" :src="previewUrl" :alt="t('Comparison preview')" />
            <div v-if="rendering" class="sheet-busy">
              <v-progress-circular indeterminate size="32" />
            </div>
          </div>
          <div class="controls">
            <v-btn-toggle
              v-model="active"
              mandatory
              divided
              variant="outlined"
              density="comfortable"
              class="slot-tabs mb-4"
            >
              <v-btn v-for="(s, i) in slots" :key="i" :value="i" class="slot-tab">
                <span class="font-weight-bold mr-1">{{ SHEET_LABELS[i] }}</span>
                <span class="text-caption text-truncate">{{ slotTitle(s) }}</span>
              </v-btn>
            </v-btn-toggle>
            <div v-if="active === 0" class="text-caption text-medium-emphasis mb-2">
              {{ t("A starts as what this upload uses now.") }}
            </div>
            <ProcessingControls
              v-if="slot"
              :params="slot.params"
              :show-presets="false"
              :show-layout="false"
              @update:params="onParams"
            />
          </div>
        </div>
        <v-alert v-if="notice" :type="notice.type" variant="tonal" density="compact" class="mt-3">
          {{ notice.text }}
        </v-alert>
      </v-card-text>
      <v-divider />
      <v-card-actions class="px-4 py-3 flex-wrap ga-2">
        <v-btn variant="text" @click="emit('close')">{{ t("Close") }}</v-btn>
        <v-spacer />
        <v-btn v-if="presets.available" variant="text" :disabled="!slot" @click="saving = true">
          {{ t("Keep {label} as a preset", { label: SHEET_LABELS[active] }) }}
        </v-btn>
        <v-btn color="primary" variant="flat" :loading="sending" @click="showOnFrame">
          <v-icon icon="mdi-monitor" start />
          {{ t("Show on frame") }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
  <SavePresetDialog
    :open="saving"
    :params="slot ? processingOf(slot.params) : null"
    :suggested-name="slot?.name ?? ''"
    source="tuning"
    @close="saving = false"
    @saved="onSaved"
  />
</template>

<style scoped>
.compare-layout {
  display: grid;
  grid-template-columns: minmax(0, 3fr) minmax(0, 2fr);
  gap: 20px;
  align-items: start;
}
@media (max-width: 760px) {
  .compare-layout {
    grid-template-columns: minmax(0, 1fr);
  }
}
@media (min-width: 761px) {
  /* Keep the sheet in view while scrolling through the controls */
  .sheet {
    position: sticky;
    top: 0;
    z-index: 1;
  }
}
.sheet {
  position: relative;
  width: 100%;
  max-height: 70vh;
  border-radius: 8px;
  overflow: hidden;
  background: rgba(var(--v-theme-on-surface), 0.06);
  border: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
}
.sheet img {
  display: block;
  width: 100%;
  height: 100%;
  object-fit: contain;
  background: #fff;
}
.sheet-busy {
  position: absolute;
  inset: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(var(--v-theme-surface), 0.5);
}
.controls {
  min-width: 0;
}
.slot-tabs {
  width: 100%;
  height: auto !important;
}
.slot-tab {
  flex: 1 1 0;
  min-width: 0;
  height: 48px !important;
  text-transform: none;
}
/* The controls' grid follows the window width; in this column they stack */
.controls :deep([class*="v-col"]) {
  flex: 0 0 100%;
  max-width: 100%;
}
.controls :deep(.v-card .v-btn-toggle) {
  flex-wrap: wrap;
  height: auto;
}
</style>
