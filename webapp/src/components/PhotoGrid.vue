<script setup>
import { t } from "../i18n";
import { useUploadsStore } from "../stores";
import { isSelected, selecting, toggleSelected } from "../utils/selection";

// Frame-shaped tiles (square on phones), edge to edge. Hover (or a long press on touch) shows a
// check to pick photos; while anything is picked, a tap picks instead of opening.
defineProps({
  images: { type: Array, required: true },
});
const emit = defineEmits(["open"]);

const uploads = useUploadsStore();
const LONG_PRESS_MS = 450;
let pressTimer = null;
let longPressed = false;

function thumbUrl(image) {
  return `/api/image?filepath=${encodeURIComponent(image.album + "/" + image.thumbnail)}`;
}

function activate(image, index) {
  if (longPressed) {
    longPressed = false; // the press already picked it
    return;
  }
  if (selecting.value) toggleSelected(image);
  else emit("open", index);
}

function pressStart(event, image) {
  if (event.pointerType !== "touch") return;
  longPressed = false;
  clearTimeout(pressTimer);
  pressTimer = setTimeout(() => {
    longPressed = true;
    toggleSelected(image);
  }, LONG_PRESS_MS);
}

function pressEnd() {
  clearTimeout(pressTimer);
}
</script>

<template>
  <div class="photo-grid" :class="{ 'photo-grid--selecting': selecting }">
    <div
      v-for="(image, index) in images"
      :key="image.album + '/' + image.filename"
      class="photo-tile"
      :class="{ 'photo-tile--picked': isSelected(image) }"
      :style="{ '--tile-aspect': uploads.frameAspect }"
      role="button"
      tabindex="0"
      :aria-label="selecting ? t('Select photo') : t('Open photo')"
      :aria-pressed="selecting ? isSelected(image) : undefined"
      @click="activate(image, index)"
      @keyup.enter="activate(image, index)"
      @pointerdown="pressStart($event, image)"
      @pointerup="pressEnd"
      @pointercancel="pressEnd"
      @pointerleave="pressEnd"
      @contextmenu.prevent
    >
      <img :src="thumbUrl(image)" alt="" loading="lazy" draggable="false" />
      <button
        type="button"
        class="photo-check"
        :aria-label="isSelected(image) ? t('Deselect photo') : t('Select photo')"
        @click.stop="toggleSelected(image)"
      >
        <v-icon
          :icon="isSelected(image) ? 'mdi-check-circle' : 'mdi-checkbox-blank-circle-outline'"
          size="24"
        />
      </button>
    </div>
  </div>
</template>

<style scoped>
.photo-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
  gap: 4px;
}
/* Phones: edge to edge (the page has a 12px gutter) in square tiles, like a
   camera roll; the full frame-shaped photo shows when one is opened. */
@media (max-width: 600px) {
  .photo-grid {
    grid-template-columns: repeat(3, 1fr);
    gap: 2px;
    margin: 0 -12px;
  }
  .photo-grid .photo-tile {
    aspect-ratio: 1;
  }
}
.photo-tile {
  position: relative;
  aspect-ratio: var(--tile-aspect, 16 / 9);
  overflow: hidden;
  cursor: pointer;
  background: rgba(var(--v-theme-on-surface), 0.06);
  user-select: none;
  -webkit-touch-callout: none;
}
.photo-tile img {
  display: block;
  width: 100%;
  height: 100%;
  object-fit: cover;
  transition: transform 0.12s ease;
}
.photo-tile:focus-visible {
  outline: 2px solid rgb(var(--v-theme-on-surface));
  outline-offset: 2px;
}
/* A soft shade at the top keeps the check readable on bright photos. */
.photo-tile::before {
  content: "";
  position: absolute;
  inset: 0 0 auto 0;
  height: 56px;
  background: linear-gradient(rgba(0, 0, 0, 0.35), transparent);
  opacity: 0;
  transition: opacity 0.12s;
  pointer-events: none;
  z-index: 1;
}
.photo-check {
  position: absolute;
  top: 6px;
  left: 6px;
  z-index: 2;
  display: flex;
  color: #fff;
  opacity: 0;
  transition: opacity 0.12s;
  border-radius: 50%;
}
/* Hover only where there is a pointer that hovers; on touch a long press picks. */
@media (hover: hover) {
  .photo-tile:hover::before,
  .photo-tile:hover .photo-check {
    opacity: 1;
  }
}
.photo-check:focus-visible,
.photo-grid--selecting .photo-check,
.photo-grid--selecting .photo-tile::before {
  opacity: 1;
}
/* Picked: the photo shrinks inside its tile, like a pressed print. */
.photo-tile--picked {
  background: rgba(var(--v-theme-on-surface), 0.12);
}
.photo-tile--picked img {
  transform: scale(0.86);
  border-radius: 6px;
}
.photo-tile--picked .photo-check {
  color: rgb(var(--v-theme-on-surface));
  background: rgb(var(--v-theme-surface));
}
@media (prefers-reduced-motion: reduce) {
  .photo-tile img,
  .photo-check,
  .photo-tile::before {
    transition: none;
  }
}
</style>
