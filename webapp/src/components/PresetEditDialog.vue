<script setup>
import { t } from "../i18n";
import { computed, ref, watch } from "vue";
import { useDisplay } from "vuetify";
import { usePresetsStore, useSettingsStore, useUploadsStore } from "../stores";
import ProcessingControls from "./ProcessingControls.vue";
import TestPhotoPanel from "./TestPhotoPanel.vue";
import { MAX_PRESETS, nameFits, processingOf } from "../utils/presets";
import { hasTestPhoto, testFraming, testPhotoVersion, testSource } from "../utils/testPhoto";

// Edit one preset by hand (or make a new one from the default): its name and
// processing values, with a full-screen look on the frame using the test photo.
const props = defineProps({
  open: { type: Boolean, default: false },
  presetId: { type: String, default: null }, // null: a new preset
});
const emit = defineEmits(["close", "saved"]);

const presets = usePresetsStore();
const settingsStore = useSettingsStore();
const uploads = useUploadsStore();
const { xs } = useDisplay();

const name = ref("");
const params = ref({});
const notice = ref(null); // {type, text}
const showing = ref(false);

watch(
  () => props.open,
  (open) => {
    if (!open) return;
    const preset = presets.byId(props.presetId);
    const from = preset ?? presets.defaultPreset;
    name.value = preset ? preset.name : "";
    params.value = { ...settingsStore.params, ...(from ? from.params : {}) };
    notice.value = null;
  }
);

const isNew = computed(() => !props.presetId);
const canSave = computed(
  () => nameFits(name.value) && (!isNew.value || presets.list.length < MAX_PRESETS)
);
const hasPhoto = computed(() => testPhotoVersion.value >= 0 && hasTestPhoto());

async function showOnFrame() {
  const source = testSource();
  if (!source) return;
  showing.value = true;
  notice.value = null;
  try {
    const mark = [...(name.value.trim() || "?")][0];
    const sheet = await uploads.renderSheet(source, [{ params: params.value }], {
      labels: [mark],
      framing: testFraming.value,
    });
    const result = await uploads.displaySheet(sheet);
    notice.value = result.ok
      ? { type: "success", text: t("Sent to the frame. The screen is refreshing.") }
      : { type: "error", text: result.message };
  } catch {
    notice.value = { type: "error", text: t("Could not prepare the comparison") };
  } finally {
    showing.value = false;
  }
}

async function save() {
  let id = props.presetId;
  const values = processingOf(params.value);
  const ok = isNew.value
    ? !!(id = await presets.create({ name: name.value, params: values, source: "manual" }))
    : await presets.update(id, { name: name.value, params: values });
  if (!ok) {
    notice.value = { type: "error", text: t("Could not save the preset on the frame") };
    return;
  }
  emit("saved", id);
}
</script>

<template>
  <v-dialog
    :model-value="open"
    :fullscreen="xs"
    max-width="760"
    scrollable
    @update:model-value="(value) => !value && emit('close')"
  >
    <v-card v-if="open">
      <v-toolbar density="comfortable" color="surface">
        <v-toolbar-title class="text-body-1">
          {{ isNew ? t("New preset") : t("Edit preset") }}
        </v-toolbar-title>
        <v-btn icon="mdi-close" :aria-label="t('Close')" @click="emit('close')" />
      </v-toolbar>
      <v-divider />
      <v-card-text class="px-3 px-sm-5">
        <v-text-field
          v-model="name"
          :label="t('Preset name')"
          :placeholder="t('e.g. Portrait, Landscape, Night')"
          variant="outlined"
          density="comfortable"
          counter="20"
          :error-messages="name && !nameFits(name) ? t('Name too long') : ''"
        />
        <ProcessingControls
          :params="params"
          :show-presets="false"
          :show-layout="false"
          @update:params="params = { ...params, ...$event }"
        />
        <h3 class="text-subtitle-2 mt-4 mb-2">{{ t("Look on the frame") }}</h3>
        <TestPhotoPanel compact @error="notice = { type: 'error', text: $event }" />
        <v-alert v-if="notice" :type="notice.type" variant="tonal" density="compact" class="mt-3">
          {{ notice.text }}
        </v-alert>
      </v-card-text>
      <v-divider />
      <v-card-actions class="px-4 py-3 flex-wrap ga-2">
        <v-btn variant="text" @click="emit('close')">{{ t("Cancel") }}</v-btn>
        <v-spacer />
        <v-btn variant="tonal" :loading="showing" :disabled="!hasPhoto" @click="showOnFrame">
          <v-icon icon="mdi-monitor" start />
          {{ t("Show on frame") }}
        </v-btn>
        <v-btn
          color="primary"
          variant="flat"
          :loading="presets.saving"
          :disabled="!canSave"
          @click="save"
        >
          {{ t("Save") }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>
