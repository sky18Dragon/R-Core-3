<script setup>
import { t } from "../i18n";
import { computed, nextTick, onMounted, ref, watch } from "vue";
import { useUploadsStore } from "../stores";
import { framingPlacement } from "../utils/compareSheet";
import { isImageFile } from "../utils/uploadHelpers";
import {
  hasTestPhoto,
  setTestFraming,
  setTestPhoto,
  testFraming,
  testPhotoName,
  testPhotoVersion,
  testSource,
  turnTestPhoto,
} from "../utils/testPhoto";

// The photo presets are judged with: pick it, see how it sits on a screen-
// shaped preview, and turn, fill, fit or zoom it. Every setting uses this
// same framing, so they compare like for like.
defineProps({
  // A note under the controls (e.g. that changes show from the next setting)
  note: { type: String, default: "" },
  // Only the photo, without the framing controls
  compact: { type: Boolean, default: false },
});
const emit = defineEmits(["error"]);

const uploads = useUploadsStore();
const PREVIEW_WIDTH = 320;

const fileInput = ref(null);
const loading = ref(false);
const loaded = computed(() => testPhotoVersion.value >= 0 && hasTestPhoto());

async function onPhoto(event) {
  const [file] = event.target.files || [];
  event.target.value = "";
  if (!file || !isImageFile(file)) return;
  loading.value = true;
  try {
    setTestPhoto(await uploads.sourceFromFile(file), file.name);
  } catch {
    emit("error", t("Could not read this photo"));
  } finally {
    loading.value = false;
  }
}

const scaleModes = computed(() => [
  { value: "cover", title: t("Fill (crop)") },
  { value: "fit", title: t("Whole photo") },
  { value: "custom", title: t("Custom") },
]);

// A quick look at the framing (no processing)
const preview = ref(null);
function drawPreview() {
  const canvas = preview.value;
  const source = testSource();
  if (!canvas) return;
  const { w, h } = uploads.frameSize();
  canvas.width = PREVIEW_WIDTH;
  canvas.height = Math.round((PREVIEW_WIDTH * h) / w);
  const ctx = canvas.getContext("2d");
  ctx.fillStyle = "#ffffff";
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  if (!source) return;
  const place = framingPlacement(
    source.width,
    source.height,
    canvas.width,
    canvas.height,
    testFraming.value
  );
  ctx.imageSmoothingQuality = "high";
  ctx.drawImage(
    source,
    place.panX,
    place.panY,
    source.width * place.zoom,
    source.height * place.zoom
  );
}
watch([testFraming, testPhotoVersion, () => uploads.frameAspect], () => nextTick(drawPreview), {
  deep: true,
});
onMounted(drawPreview);
</script>

<template>
  <div>
    <div class="photo-row">
      <div class="photo-preview" :style="{ aspectRatio: uploads.frameAspect }">
        <canvas v-show="loaded" ref="preview" />
        <v-icon v-if="!loaded" icon="mdi-image-outline" class="text-medium-emphasis" />
      </div>
      <div class="photo-side">
        <div v-if="testPhotoName" class="text-body-2 mb-2 text-truncate">{{ testPhotoName }}</div>
        <div v-else class="text-body-2 text-medium-emphasis mb-2">
          {{ t("Pick a photo that is typical of what the frame shows.") }}
        </div>
        <v-btn variant="tonal" :loading="loading" @click="fileInput.click()">
          <v-icon icon="mdi-image-plus" start />
          {{ testPhotoName ? t("Choose another photo") : t("Choose a photo") }}
        </v-btn>
        <input ref="fileInput" type="file" accept="image/*" hidden @change="onPhoto" />
      </div>
    </div>

    <div v-if="loaded && !compact" class="framing">
      <div class="framing-row">
        <span class="framing-label">{{ t("Turn") }}</span>
        <v-btn
          icon="mdi-rotate-left"
          variant="tonal"
          size="small"
          :title="t('Rotate left 90°')"
          :aria-label="t('Rotate left 90°')"
          @click="turnTestPhoto(-90)"
        />
        <v-btn
          icon="mdi-rotate-right"
          variant="tonal"
          size="small"
          :title="t('Rotate right 90°')"
          :aria-label="t('Rotate right 90°')"
          @click="turnTestPhoto(90)"
        />
      </div>
      <div class="framing-row">
        <span class="framing-label">{{ t("Size") }}</span>
        <v-btn-toggle
          :model-value="testFraming.scaleMode"
          mandatory
          divided
          variant="outlined"
          density="comfortable"
          @update:model-value="setTestFraming({ scaleMode: $event })"
        >
          <v-btn v-for="m in scaleModes" :key="m.value" :value="m.value">{{ m.title }}</v-btn>
        </v-btn-toggle>
      </div>
      <template v-if="testFraming.scaleMode === 'custom'">
        <v-slider
          :model-value="testFraming.zoom"
          :min="0.5"
          :max="3"
          :step="0.05"
          :label="t('Zoom')"
          hide-details
          class="framing-slider"
          @update:model-value="setTestFraming({ zoom: $event })"
        >
          <template #append>
            <span class="text-body-2 slider-value">{{ Math.round(testFraming.zoom * 100) }}%</span>
          </template>
        </v-slider>
        <v-slider
          :model-value="testFraming.x"
          :min="-1"
          :max="1"
          :step="0.05"
          :label="t('Left / right')"
          hide-details
          class="framing-slider"
          @update:model-value="setTestFraming({ x: $event })"
        />
        <v-slider
          :model-value="testFraming.y"
          :min="-1"
          :max="1"
          :step="0.05"
          :label="t('Up / down')"
          hide-details
          class="framing-slider"
          @update:model-value="setTestFraming({ y: $event })"
        />
      </template>
      <div v-if="note" class="text-caption text-medium-emphasis">{{ note }}</div>
    </div>
  </div>
</template>

<style scoped>
.photo-row {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 16px;
}
.photo-preview {
  width: min(100%, 320px);
  flex: none;
  border-radius: 8px;
  overflow: hidden;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(var(--v-theme-on-surface), 0.06);
  border: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
}
.photo-preview canvas {
  display: block;
  width: 100%;
  height: 100%;
}
.photo-side {
  min-width: 0;
  max-width: 280px;
}
.framing {
  margin-top: 16px;
  max-width: 520px;
  display: grid;
  gap: 12px;
}
.framing-row {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px;
}
.framing-label {
  width: 48px;
  font-size: 0.875rem;
}
.framing-slider :deep(.v-label) {
  min-width: 64px;
}
.slider-value {
  min-width: 44px;
  text-align: right;
}
@media (max-width: 600px) {
  .photo-side {
    max-width: 100%;
  }
}
</style>
