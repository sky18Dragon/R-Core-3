<script setup>
import { t } from "../i18n";
import { ref, computed, watch } from "vue";
import { useDisplay } from "vuetify";
import { usePresetsStore, useSettingsStore, useUploadsStore } from "../stores";
import ImageProcessing from "./ImageProcessing.vue";

const props = defineProps({
  // The queued photo being adjusted, or null when closed
  item: { type: Object, default: null },
});
const emit = defineEmits(["close"]);

const settingsStore = useSettingsStore();
const uploads = useUploadsStore();
const presets = usePresetsStore();
const { xs } = useDisplay();

const editor = ref(null);
const showing = ref(false);
const notice = ref(null); // {type, text} from "show only"
const presetId = ref(null); // null: the batch's preset

watch(
  () => props.item,
  (item) => {
    notice.value = null;
    presetId.value = item?.presetId ?? null;
  }
);

// Which preset this photo alone uses (docs/PRD-PRESETS.md)
const presetItems = computed(() => [
  { value: null, title: t("Same as the batch ({name})", { name: uploads.batchName }) },
  ...presets.list.map((p) => ({ value: p.id, title: p.name })),
]);
const previewParams = computed(() => ({
  ...settingsStore.params,
  ...uploads.processingFor(presetId.value),
}));

const initialEdit = computed(() =>
  props.item
    ? {
        rotation: uploads.rotationOf(props.item),
        scaleMode: props.item.edit?.scaleMode,
        backgroundColor: props.item.edit?.backgroundColor,
      }
    : null
);

// What the user set in the editor, in the shape the queue stores. A layout that
// matches the defaults is stored as "no edit", so changing the defaults later
// still applies to this photo.
function collect() {
  const view = editor.value;
  const upload = view.getUploadParams();
  const mode = view.scaleMode;
  const defaultMode = settingsStore.params.scaleMode || "cover";
  const defaultBg = settingsStore.params.backgroundColor || "white";
  const custom = mode === "custom";
  const isDefault = mode === defaultMode && !custom && upload.backgroundColorName === defaultBg;
  return {
    presetId: presetId.value,
    rotation: view.rotation,
    edit: isDefault
      ? null
      : {
          scaleMode: mode,
          backgroundColor: upload.backgroundColorName,
          zoom: custom ? upload.zoom : undefined,
          panX: custom ? upload.panX : undefined,
          panY: custom ? upload.panY : undefined,
        },
  };
}

function done() {
  uploads.applyEdit(props.item, collect());
  emit("close");
}

async function showOnly() {
  showing.value = true;
  notice.value = null;
  const result = await uploads.displayOnly(props.item, collect());
  showing.value = false;
  notice.value = result.ok
    ? { type: "success", text: t("Sent to the frame. The screen is refreshing.") }
    : { type: "error", text: result.message };
}
</script>

<template>
  <v-dialog
    :model-value="!!item"
    :fullscreen="xs"
    max-width="1000"
    scrollable
    @update:model-value="(open) => !open && emit('close')"
  >
    <v-card v-if="item">
      <v-toolbar density="comfortable" color="surface">
        <v-toolbar-title class="text-body-1">{{ t("Adjust this photo") }}</v-toolbar-title>
        <v-btn icon="mdi-close" :aria-label="t('Close')" @click="emit('close')" />
      </v-toolbar>
      <v-divider />
      <v-card-text class="px-2 px-sm-4">
        <v-select
          v-if="presets.available && presets.list.length"
          v-model="presetId"
          :items="presetItems"
          :label="t('Preset for this photo')"
          variant="outlined"
          density="comfortable"
          hide-details
          class="preset-select mb-3"
        />
        <ImageProcessing
          ref="editor"
          :image-file="item.file"
          :params="previewParams"
          :palette="settingsStore.palette"
          :initial-edit="initialEdit"
        />
        <v-alert v-if="notice" :type="notice.type" variant="tonal" density="compact" class="mt-3">
          {{ notice.text }}
        </v-alert>
      </v-card-text>
      <v-divider />
      <v-card-actions class="px-4 py-3 flex-wrap ga-2">
        <v-btn variant="text" @click="emit('close')">{{ t("Cancel") }}</v-btn>
        <v-spacer />
        <v-btn variant="tonal" :loading="showing" @click="showOnly">
          <v-icon icon="mdi-monitor" start />
          {{ t("Show on frame only (not saved)") }}
        </v-btn>
        <v-btn color="primary" variant="flat" @click="done">{{ t("Done") }}</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<style scoped>
.preset-select {
  max-width: 360px;
}
</style>
