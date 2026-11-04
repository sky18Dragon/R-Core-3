<script setup>
import { t } from "../i18n";
import { computed, onBeforeUnmount, ref, watch } from "vue";
import { useAppStore } from "../stores";
import { albumLabel } from "../utils/albums";

// Full-screen look at one photo, with the two things you can do to it: put it
// on the frame, or delete it. Arrow keys / swipes move through the list.
const props = defineProps({
  images: { type: Array, required: true },
  modelValue: { type: Number, default: -1 }, // index; -1 = closed
});
const emit = defineEmits(["update:modelValue"]);

const appStore = useAppStore();
const open = computed({
  get: () => props.modelValue >= 0 && props.modelValue < props.images.length,
  set: (value) => {
    if (!value) emit("update:modelValue", -1);
  },
});
const image = computed(() => (open.value ? props.images[props.modelValue] : null));
const src = computed(() =>
  image.value
    ? `/api/image?filepath=${encodeURIComponent(image.value.album + "/" + image.value.thumbnail)}`
    : ""
);

const confirmDelete = ref(false);
const busy = ref(false);
const notice = ref("");
const noticeShown = computed({
  get: () => !!notice.value,
  set: (value) => {
    if (!value) notice.value = "";
  },
});

function step(delta) {
  const next = props.modelValue + delta;
  if (next >= 0 && next < props.images.length) emit("update:modelValue", next);
}

async function showOnFrame() {
  busy.value = true;
  const ok = await appStore.displayImage(image.value.album, image.value.filename);
  busy.value = false;
  notice.value = ok
    ? t("Sent to the frame. The screen is refreshing.")
    : t("Could not show it on the frame");
}

async function remove() {
  confirmDelete.value = false;
  busy.value = true;
  const index = props.modelValue;
  const ok = await appStore.deleteImage(image.value.album, image.value.filename);
  busy.value = false;
  if (!ok) {
    notice.value = t("Could not delete the photo");
    return;
  }
  // The list shrank: stay at the same place (the next photo), or close at the end.
  emit("update:modelValue", Math.min(index, props.images.length - 1));
}

function onKey(event) {
  if (!open.value || confirmDelete.value) return;
  if (event.key === "ArrowRight") step(1);
  else if (event.key === "ArrowLeft") step(-1);
}
watch(open, (value) => {
  if (value) window.addEventListener("keydown", onKey);
  else window.removeEventListener("keydown", onKey);
});
onBeforeUnmount(() => window.removeEventListener("keydown", onKey));

// Swipe left/right on touch screens
let touchX = null;
function touchStart(event) {
  touchX = event.touches.length === 1 ? event.touches[0].clientX : null;
}
function touchEnd(event) {
  if (touchX === null) return;
  const dx = event.changedTouches[0].clientX - touchX;
  touchX = null;
  if (Math.abs(dx) > 50) step(dx < 0 ? 1 : -1);
}
</script>

<template>
  <v-dialog v-model="open" fullscreen :scrim="false" transition="fade-transition">
    <div class="viewer" @touchstart.passive="touchStart" @touchend="touchEnd">
      <div class="viewer-bar">
        <v-btn
          icon="mdi-arrow-left"
          variant="text"
          color="white"
          :aria-label="t('Close')"
          @click="open = false"
        />
        <span v-if="image" class="viewer-title">{{ albumLabel(image.album) }}</span>
        <v-spacer />
        <v-btn variant="text" color="white" :loading="busy" @click="showOnFrame">
          <v-icon icon="mdi-monitor" start />
          <span class="d-none d-sm-inline">{{ t("Show on frame") }}</span>
        </v-btn>
        <v-btn
          icon="mdi-delete-outline"
          variant="text"
          color="white"
          :title="t('Delete')"
          :aria-label="t('Delete')"
          :disabled="busy"
          @click="confirmDelete = true"
        />
      </div>

      <div class="viewer-stage" @click.self="open = false">
        <img v-if="image" :key="src" :src="src" alt="" class="viewer-image" />
        <v-btn
          v-if="modelValue > 0"
          icon="mdi-chevron-left"
          class="viewer-nav viewer-nav--prev d-none d-sm-flex"
          variant="tonal"
          color="white"
          :aria-label="t('Previous')"
          @click="step(-1)"
        />
        <v-btn
          v-if="modelValue < images.length - 1"
          icon="mdi-chevron-right"
          class="viewer-nav viewer-nav--next d-none d-sm-flex"
          variant="tonal"
          color="white"
          :aria-label="t('Next')"
          @click="step(1)"
        />
      </div>
    </div>

    <v-dialog v-model="confirmDelete" max-width="380">
      <v-card>
        <v-card-title>{{ t("Delete this photo?") }}</v-card-title>
        <v-card-text>{{ t("It is removed from the frame's storage card.") }}</v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="confirmDelete = false">{{ t("Cancel") }}</v-btn>
          <v-btn color="primary" @click="remove">{{ t("Delete") }}</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <v-snackbar v-model="noticeShown" :timeout="3500" location="bottom">
      {{ notice }}
    </v-snackbar>
  </v-dialog>
</template>

<style scoped>
.viewer {
  position: fixed;
  inset: 0;
  display: flex;
  flex-direction: column;
  background: #000;
  color: #fff;
}
.viewer-bar {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 8px;
  background: linear-gradient(rgba(0, 0, 0, 0.6), transparent);
  position: absolute;
  inset: 0 0 auto 0;
  z-index: 2;
}
.viewer-title {
  margin-left: 4px;
  font-weight: 600;
}
.viewer-stage {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 64px 16px 24px;
  min-height: 0;
}
.viewer-image {
  max-width: 100%;
  max-height: 100%;
  object-fit: contain;
  image-rendering: auto;
  /* Thumbnails are small; keep them from blowing up past twice their size. */
  width: min(100%, 800px);
}
.viewer-nav {
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
}
.viewer-nav--prev {
  left: 16px;
}
.viewer-nav--next {
  right: 16px;
}
</style>
