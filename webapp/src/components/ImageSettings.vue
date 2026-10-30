<script setup>
import { t } from "../i18n";
import { ref, computed } from "vue";
import { useSettingsStore, useAppStore } from "../stores";
import ProcessingControls from "./ProcessingControls.vue";
import RotationSchedule from "./RotationSchedule.vue";
import { isValidCron } from "../utils/cron";

const settingsStore = useSettingsStore();
const appStore = useAppStore();

// The device rejects the entire config request when any schedule rule is
// invalid, empty or over the 7-rule budget — gate saving on the same checks.
const scheduleValid = computed(() => {
  const rules = settingsStore.deviceSettings.rotateCron || [];
  return rules.length >= 1 && rules.length <= 7 && rules.every((r) => isValidCron(r));
});

const saveBlocker = computed(() =>
  scheduleValid.value ? "" : t("Fix the rotation schedule first (invalid or too many rules)")
);

const sdRotationModeOptions = [
  { title: t("Random - Shuffle images"), value: "random" },
  { title: t("Sequential - In sequence"), value: "sequential" },
];

const hasStorage = computed(
  () => appStore.systemInfo.sdcard_inserted || appStore.systemInfo.has_flash_storage
);

function onPresetChange(preset) {
  if (preset !== "custom") {
    settingsStore.applyPreset(preset);
  }
}

function onParamsUpdate(newParams) {
  Object.assign(settingsStore.params, newParams);
}

const saving = ref(false);
const saveSuccess = ref(false);
const saveError = ref(false);
const saveMessage = ref("");

async function save() {
  saving.value = true;
  const result = await settingsStore.saveImageSettings();
  saving.value = false;

  if (result.success) {
    saveSuccess.value = true;
    saveError.value = false;
    saveMessage.value = result.message || t("Settings saved!");
    setTimeout(() => (saveSuccess.value = false), 3000);
  } else {
    saveError.value = true;
    saveSuccess.value = false;
    saveMessage.value = result.message || t("Failed to save settings");
    setTimeout(() => (saveError.value = false), 5000);
  }
}
</script>

<template>
  <v-sheet border rounded class="image-settings pa-4">
    <div class="text-subtitle-1 mb-2">
      <v-icon icon="mdi-slideshow" size="small" class="mr-1" />
      {{ t("Slideshow") }}
    </div>

    <v-alert
      v-if="settingsStore.deviceSettings.rotationMode === 'url'"
      type="info"
      variant="tonal"
      density="compact"
      class="mb-3"
    >
      {{
        t(
          "The frame is set to a remote image source (System settings → Experimental). The order below applies to local storage only."
        )
      }}
    </v-alert>

    <v-switch
      v-model="settingsStore.deviceSettings.autoRotate"
      :label="t('Enable Auto-Rotate')"
      color="primary"
      hide-details
      class="mb-2"
    />

    <div class="ml-md-10">
      <RotationSchedule
        v-model="settingsStore.deviceSettings.rotateCron"
        :disabled="!settingsStore.deviceSettings.autoRotate"
      />

      <v-select
        v-if="hasStorage"
        v-model="settingsStore.deviceSettings.sdRotationMode"
        :items="sdRotationModeOptions"
        item-title="title"
        item-value="value"
        :label="t('Storage Rotation Logic')"
        variant="outlined"
        hide-details
        class="mt-6"
        :disabled="!settingsStore.deviceSettings.autoRotate"
      />
    </div>

    <v-divider class="my-4" />

    <v-expansion-panels variant="accordion" elevation="0">
      <v-expansion-panel elevation="0">
        <v-expansion-panel-title>
          <v-icon icon="mdi-image-edit-outline" class="mr-3" />
          {{ t("Upload defaults") }}
          <span class="text-caption text-medium-emphasis ml-3">
            {{ t("Applied to every new photo; adjust a single photo while uploading") }}
          </span>
        </v-expansion-panel-title>
        <v-expansion-panel-text>
          <ProcessingControls
            :params="settingsStore.params"
            :preset="settingsStore.preset"
            @update:params="onParamsUpdate"
            @update:preset="settingsStore.preset = $event"
            @preset-change="onPresetChange"
          />
        </v-expansion-panel-text>
      </v-expansion-panel>
    </v-expansion-panels>

    <div class="d-flex align-center flex-wrap ga-2 mt-4">
      <v-chip v-if="settingsStore.imageDirty" color="warning" variant="tonal" size="small">
        <v-icon icon="mdi-circle-edit-outline" start />
        {{ t("Unsaved changes") }}
      </v-chip>
      <v-spacer />
      <v-fade-transition>
        <v-chip v-if="saveSuccess" color="success" variant="tonal">
          <v-icon icon="mdi-check" start />
          {{ saveMessage || t("Settings saved!") }}
        </v-chip>
        <v-chip v-else-if="saveError" color="error" variant="tonal">
          <v-icon icon="mdi-alert-circle" start />
          {{ saveMessage || t("Failed to save settings") }}
        </v-chip>
      </v-fade-transition>
      <v-tooltip :text="saveBlocker" location="top" :disabled="!saveBlocker">
        <template #activator="{ props: tooltipProps }">
          <span v-bind="tooltipProps">
            <v-btn
              color="primary"
              :loading="saving"
              :disabled="!!saveBlocker || !settingsStore.imageDirty"
              @click="save"
            >
              <v-icon icon="mdi-content-save" start />
              {{ t("Save Settings") }}
            </v-btn>
          </span>
        </template>
      </v-tooltip>
    </div>
  </v-sheet>
</template>
