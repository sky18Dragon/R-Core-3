<script setup>
import { t } from "../i18n";
import { computed, onBeforeUnmount, ref, watch } from "vue";
import { useAppStore, useUploadsStore } from "../stores";
import UprightPhoto from "./UprightPhoto.vue";
import { albumLabel } from "../utils/albums";
import { uprightAspect } from "../utils/orientation";

// Full-screen look at one photo, standing upright, with what you can do to it:
// turn it the right way up, put it on the frame, or delete it. Arrow keys /
// swipes move through the list.
const props = defineProps({
  images: { type: Array, required: true },
  modelValue: { type: Number, default: -1 }, // index; -1 = closed
});
const emit = defineEmits(["update:modelValue"]);

const appStore = useAppStore();
const uploads = useUploadsStore();
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

// The photo's upright shape; the box is as large as the stage allows.
const aspect = computed(() => {
  const [w, h] = String(uploads.frameAspect).split("/").map(Number);
  return uprightAspect(image.value?.up, w && h ? w / h : 16 / 9);
});
const boxStyle = computed(() => ({
  aspectRatio: aspect.value,
  width: `min(100%, calc((100vh - 112px) * ${aspect.value}), ${Math.round(900 * Math.min(aspect.value, 1))}px)`,
}));

// Turning: saved at once; "Undo" turns it back while the note shows.
const turned = ref(null); // { image, delta } of the last turn
const turnShown = computed({
  get: () => !!turned.value,
  set: (value) => {
    if (!value) turned.value = null;
  },
});
async function rotate(delta) {
  busy.value = true;
  const target = image.value;
  const ok = (await appStore.rotatePhotos([target], delta)) === 1;
  busy.value = false;
  if (ok) turned.value = { album: target.album, filename: target.filename, delta };
  else notice.value = t("Could not turn the photo");
}
async function undoTurn() {
  const last = turned.value;
  turned.value = null;
  const current = props.images.find((i) => i.album === last.album && i.filename === last.filename);
  if (current) await appStore.rotatePhotos([current], -last.delta);
}

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
        <div v-if="image" class="viewer-title">
          <div>{{ albumLabel(image.album) }}</div>
          <div v-if="image.preset" class="viewer-sub">
            {{ t("Rendered with: {name}", { name: image.preset }) }}
          </div>
        </div>
        <v-spacer />
        <v-btn
          icon="mdi-rotate-right"
          variant="text"
          color="white"
          :disabled="busy"
          :title="t('Rotate right 90°')"
          :aria-label="t('Rotate right 90°')"
          @click="rotate(90)"
        />
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
        <div v-if="image" :key="src" class="viewer-box" :style="boxStyle">
          <UprightPhoto :src="src" :up="image.up || 0" :box-aspect="aspect" :lazy="false" />
        </div>
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

    <v-snackbar v-model="turnShown" :timeout="4000" location="bottom">
      {{ t("Turned. The frame uses the new way round.") }}
      <template #actions>
        <v-btn variant="text" @click="undoTurn">{{ t("Undo") }}</v-btn>
      </template>
    </v-snackbar>
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
  min-width: 0;
  line-height: 1.25;
}
.viewer-sub {
  font-size: 0.75rem;
  font-weight: 400;
  opacity: 0.7;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.viewer-stage {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 64px 16px 24px;
  min-height: 0;
}
/* Sized in script to the photo's upright shape; thumbnails are small, so it
   stops growing at about twice their size. */
.viewer-box {
  position: relative;
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
