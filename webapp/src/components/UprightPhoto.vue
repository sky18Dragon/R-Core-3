<script setup>
import { computed } from "vue";

// A stored photo turned to stand upright inside a box of `boxAspect` (width /
// height). Thumbnails are frame-shaped and portrait photos are stored on their
// side, so a quarter turn swaps the image's width and height: the image is sized
// to the box's height × width before turning, then covers the box.
const props = defineProps({
  src: { type: String, required: true },
  up: { type: Number, default: 0 },
  boxAspect: { type: Number, default: 16 / 9 },
  lazy: { type: Boolean, default: true },
});

const style = computed(() => {
  const up = (((props.up || 0) % 360) + 360) % 360;
  if (up % 180 === 0) return up ? { transform: "rotate(180deg)" } : {};
  const r = props.boxAspect || 1;
  return {
    position: "absolute",
    left: "50%",
    top: "50%",
    width: `${100 / r}%`,
    height: `${r * 100}%`,
    transform: `translate(-50%, -50%) rotate(${up}deg)`,
  };
});
</script>

<template>
  <span class="upright">
    <img :src="src" alt="" :loading="lazy ? 'lazy' : undefined" draggable="false" :style="style" />
  </span>
</template>

<style scoped>
.upright {
  position: absolute;
  inset: 0;
  overflow: hidden;
}
.upright img {
  display: block;
  width: 100%;
  height: 100%;
  object-fit: cover;
}
</style>
