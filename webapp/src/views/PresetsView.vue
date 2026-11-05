<script setup>
import { t } from "../i18n";
import { computed, ref } from "vue";
import { getDitherOptions } from "@aitjcize/epaper-image-convert";
import { useAppStore, usePresetsStore } from "../stores";
import PresetEditDialog from "../components/PresetEditDialog.vue";
import PresetTuning from "../components/PresetTuning.vue";
import { MAX_PRESETS, canRemove } from "../utils/presets";
import { describeDiff } from "../utils/tuning";

// The preset library (docs/PRD-PRESETS.md): named processing settings kept on
// the frame. One is the default (★), used for new uploads and for pictures
// the frame makes by itself. New presets come from tuning on the panel, a
// copy, or by hand.
const appStore = useAppStore();
const presets = usePresetsStore();

const tuning = ref(null); // {baseId, updatingId} while tuning
const editing = ref(null); // {presetId} while the editor is open
const deleting = ref(null); // preset to confirm
const busyId = ref("");

const full = computed(() => presets.list.length >= MAX_PRESETS);

// ---- Describing a preset ---------------------------------------------------
const ditherTitles = Object.fromEntries(getDitherOptions().map((o) => [o.value, o.title]));

function keyTitle(key) {
  return {
    exposure: t("Exposure"),
    saturation: t("Saturation"),
    toneMode: t("Tone Mapping"),
    contrast: t("Contrast"),
    strength: t("Strength"),
    shadowBoost: t("Shadow Boost"),
    highlightCompress: t("Highlight Compress"),
    midpoint: t("Midpoint"),
    colorMethod: t("Color Matching"),
    ditherAlgorithm: t("Dithering Algorithm"),
    compressDynamicRange: t("Compress Dynamic Range"),
  }[key];
}

function valueText(key, value) {
  if (key === "toneMode") return value === "scurve" ? t("S-Curve") : t("Contrast");
  if (key === "ditherAlgorithm") return ditherTitles[value] ?? value;
  if (key === "colorMethod") return String(value).toUpperCase();
  if (typeof value === "boolean") return value ? t("on") : t("off");
  if (typeof value === "number") return value.toFixed(2);
  return String(value);
}

// The default reads as its main values; the others as what differs from it
const NEUTRAL = {
  exposure: 1,
  saturation: 1,
  toneMode: "contrast",
  contrast: 1,
  colorMethod: "rgb",
  ditherAlgorithm: "floyd-steinberg",
  compressDynamicRange: true,
};
function summaryOf(preset) {
  const isDefault = preset.id === presets.defaultId;
  const reference = isDefault || !presets.defaultPreset ? NEUTRAL : presets.defaultPreset.params;
  const diff = describeDiff(preset.params, reference, { grayscale: appStore.isGrayscale });
  if (!diff.length) return isDefault ? t("Standard values") : t("Same as the default");
  const text = diff.map((d) => `${keyTitle(d.key)} ${valueText(d.key, d.value)}`).join(" · ");
  return isDefault ? text : t("Differs from the default: {text}", { text });
}

function sourceText(source) {
  return (
    {
      tuning: t("From tuning"),
      manual: t("By hand"),
      copy: t("Copy"),
      migrated: t("Earlier default"),
    }[source] ?? ""
  );
}

// ---- Actions ---------------------------------------------------------------
const notice = ref({ text: "", color: "success" });
const noticeShown = computed({
  get: () => !!notice.value.text,
  set: (value) => {
    if (!value) notice.value = { text: "", color: "success" };
  },
});
function notify(ok, text) {
  notice.value = ok
    ? { text, color: "success" }
    : { text: t("Could not save the preset on the frame"), color: "error" };
}

async function makeDefault(preset) {
  busyId.value = preset.id;
  const ok = await presets.setDefault(preset.id);
  busyId.value = "";
  notify(ok, t("“{name}” is now the default", { name: preset.name }));
}

async function duplicate(preset) {
  busyId.value = preset.id;
  const id = await presets.duplicate(preset.id);
  busyId.value = "";
  notify(!!id, t("Copied “{name}”", { name: preset.name }));
}

async function confirmDelete() {
  const preset = deleting.value;
  deleting.value = null;
  busyId.value = preset.id;
  const ok = await presets.remove(preset.id);
  busyId.value = "";
  notify(ok, t("Deleted “{name}”", { name: preset.name }));
}

function startTuning(preset = null) {
  tuning.value = { baseId: preset?.id ?? presets.defaultId, updatingId: preset?.id ?? null };
}

function onTuned(id) {
  tuning.value = null;
  const preset = presets.byId(id);
  notify(true, t("Saved “{name}”", { name: preset?.name ?? "" }));
}

function onEdited(id) {
  editing.value = null;
  const preset = presets.byId(id);
  notify(true, t("Saved “{name}”", { name: preset?.name ?? "" }));
}

// A round left half-way in this browser
const sessionCleared = ref(0); // bumps when the session is thrown away
const unfinished = computed(() => {
  // Read again when tuning closes or the session is thrown away
  if (tuning.value || sessionCleared.value < 0) return false;
  try {
    const saved = JSON.parse(localStorage.getItem("photoframe.tuning") || "null");
    return Array.isArray(saved?.history) && saved.history.length > 0 ? saved.base : false;
  } catch {
    return false;
  }
});

function discardSession() {
  try {
    localStorage.removeItem("photoframe.tuning");
  } catch {
    // Nothing stored
  }
  sessionCleared.value++;
}
</script>

<template>
  <div class="presets-page">
    <PresetTuning
      v-if="tuning"
      :base-id="tuning.baseId"
      :updating-id="tuning.updatingId"
      @close="tuning = null"
      @saved="onTuned"
    />

    <template v-else>
      <div class="head">
        <h1 class="page-title">{{ t("Presets") }}</h1>
        <v-spacer />
        <v-menu location="bottom end">
          <template #activator="{ props: menu }">
            <v-btn
              v-bind="menu"
              color="primary"
              variant="flat"
              :disabled="!presets.available || full"
            >
              <v-icon icon="mdi-plus" start />{{ t("New preset") }}
            </v-btn>
          </template>
          <v-list density="comfortable">
            <v-list-item
              prepend-icon="mdi-view-grid-outline"
              :title="t('Find by tuning on the frame')"
              :subtitle="t('Recommended')"
              @click="startTuning()"
            />
            <v-list-item
              prepend-icon="mdi-tune"
              :title="t('Adjust by hand')"
              @click="editing = { presetId: null }"
            />
          </v-list>
        </v-menu>
      </div>
      <p class="text-body-2 text-medium-emphasis intro">
        {{
          t(
            "Presets are named processing settings kept on the frame. Pick one when uploading; the ★ default is used when you do not."
          )
        }}
      </p>

      <v-alert v-if="!presets.available" type="info" variant="tonal" class="mb-4">
        {{ t("Insert a storage card to keep presets.") }}
      </v-alert>
      <v-alert v-if="full" type="info" variant="tonal" density="compact" class="mb-4">
        {{ t("You have 20 presets, the most the frame keeps. Delete one first.") }}
      </v-alert>
      <v-alert
        v-if="unfinished && presets.byId(unfinished)"
        type="info"
        variant="tonal"
        density="compact"
        class="mb-4"
      >
        <div class="d-flex align-center flex-wrap ga-2">
          <span>{{ t("A tuning session is unfinished.") }}</span>
          <v-spacer />
          <v-btn
            size="small"
            variant="text"
            @click="tuning = { baseId: unfinished, updatingId: null }"
          >
            {{ t("Continue") }}
          </v-btn>
          <v-btn size="small" variant="text" @click="discardSession">
            {{ t("Discard") }}
          </v-btn>
        </div>
      </v-alert>

      <div class="preset-list">
        <div v-for="preset in presets.list" :key="preset.id" class="preset-card">
          <div
            class="preset-main"
            role="button"
            tabindex="0"
            @click="editing = { presetId: preset.id }"
          >
            <div class="preset-title">
              <span class="text-subtitle-1 font-weight-medium">{{ preset.name }}</span>
              <v-chip
                v-if="preset.id === presets.defaultId"
                size="x-small"
                variant="flat"
                color="primary"
                class="ml-2"
              >
                ★ {{ t("Default") }}
              </v-chip>
              <v-chip
                v-if="preset.id === presets.defaultId && presets.defaultModified"
                size="x-small"
                variant="outlined"
                class="ml-1"
                :title="
                  t('The frame’s default was changed elsewhere; set it again to use this preset.')
                "
              >
                {{ t("changed") }}
              </v-chip>
            </div>
            <div class="text-body-2 text-medium-emphasis">{{ summaryOf(preset) }}</div>
            <div class="text-caption text-medium-emphasis">{{ sourceText(preset.source) }}</div>
          </div>
          <div class="preset-actions">
            <v-btn
              v-if="preset.id !== presets.defaultId || presets.defaultModified"
              size="small"
              variant="text"
              :loading="busyId === preset.id"
              @click="makeDefault(preset)"
            >
              {{ t("Make default") }}
            </v-btn>
            <v-menu location="bottom end">
              <template #activator="{ props: menu }">
                <v-btn
                  v-bind="menu"
                  icon="mdi-dots-vertical"
                  variant="text"
                  size="small"
                  :aria-label="t('More')"
                />
              </template>
              <v-list density="compact">
                <v-list-item
                  prepend-icon="mdi-view-grid-outline"
                  :title="t('Tune on the frame')"
                  @click="startTuning(preset)"
                />
                <v-list-item
                  prepend-icon="mdi-pencil-outline"
                  :title="t('Edit')"
                  @click="editing = { presetId: preset.id }"
                />
                <v-list-item
                  prepend-icon="mdi-content-copy"
                  :title="t('Duplicate')"
                  :disabled="full"
                  @click="duplicate(preset)"
                />
                <v-list-item
                  prepend-icon="mdi-delete-outline"
                  :title="t('Delete')"
                  :disabled="!canRemove(presets.doc, preset.id)"
                  @click="deleting = preset"
                />
              </v-list>
            </v-menu>
          </div>
        </div>
      </div>
      <div v-if="presets.list.length" class="text-caption text-medium-emphasis mt-3">
        {{ t("The default cannot be deleted; make another one the default first.") }}
      </div>
    </template>

    <PresetEditDialog
      :open="!!editing"
      :preset-id="editing?.presetId ?? null"
      @close="editing = null"
      @saved="onEdited"
    />

    <v-dialog :model-value="!!deleting" max-width="400" @update:model-value="deleting = null">
      <v-card v-if="deleting">
        <v-card-title>{{ t("Delete “{name}”?", { name: deleting.name }) }}</v-card-title>
        <v-card-text>
          {{ t("Photos already uploaded with it stay as they are.") }}
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="deleting = null">{{ t("Cancel") }}</v-btn>
          <v-btn color="primary" variant="flat" @click="confirmDelete">{{ t("Delete") }}</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <v-snackbar v-model="noticeShown" :color="notice.color" :timeout="3500">
      {{ notice.text }}
    </v-snackbar>
  </div>
</template>

<style scoped>
.presets-page {
  max-width: 820px;
}
.head {
  display: flex;
  align-items: center;
  gap: 8px;
}
.page-title {
  font-size: 1.5rem;
  font-weight: 600;
}
.intro {
  max-width: 640px;
  margin: 8px 0 20px;
}
.preset-list {
  display: grid;
  gap: 12px;
}
.preset-card {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px 8px 12px 16px;
  border-radius: 12px;
  border: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
}
.preset-main {
  flex: 1;
  min-width: 0;
  cursor: pointer;
}
.preset-title {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
}
.preset-actions {
  display: flex;
  align-items: center;
  flex: none;
}
</style>
