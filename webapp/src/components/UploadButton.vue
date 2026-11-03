<script setup>
import { t } from "../i18n";
import { ref } from "vue";
import { useUploadsStore } from "../stores";

// "+": picks photos and opens the upload dialog with them. The file input
// lives here so the picker opens straight from the click (iOS requires that).
const props = defineProps({
  album: { type: String, default: null }, // upload target; null keeps the last one
  kind: { type: String, default: "text" }, // "icon" | "text" | "large"
  label: { type: String, default: "" },
});

const uploads = useUploadsStore();
const input = ref(null);

function open() {
  input.value?.click();
}

async function picked(event) {
  const files = [...(event.target.files || [])];
  event.target.value = ""; // so the same photo can be chosen again
  if (files.length) await uploads.begin(files, props.album);
}

defineExpose({ open });
</script>

<template>
  <input
    ref="input"
    type="file"
    :multiple="uploads.canSaveToAlbum"
    accept=".jpg,.jpeg,.png,.heic,.heif,.webp,.gif,.bmp"
    class="d-none"
    @change="picked"
  />
  <v-btn
    v-if="kind === 'icon'"
    icon="mdi-plus"
    variant="text"
    :title="label || t('Upload photos')"
    :aria-label="label || t('Upload photos')"
    :disabled="uploads.running"
    @click="open"
  />
  <v-btn
    v-else
    :variant="kind === 'large' ? 'flat' : 'text'"
    :color="kind === 'large' ? 'primary' : undefined"
    :size="kind === 'large' ? 'large' : undefined"
    :disabled="uploads.running"
    @click="open"
  >
    <v-icon icon="mdi-upload" start />
    {{ label || t("Upload") }}
  </v-btn>
</template>
