<script setup>
import { t, locale } from "../i18n";
import { computed, onBeforeUnmount, ref, watch } from "vue";
import { useAppStore, useSettingsStore, useUploadsStore } from "../stores";
import ImageSettings from "../components/ImageSettings.vue";
import { albumLabel } from "../utils/albums";
import { scheduleSummary } from "../utils/slideshow";
import { currentView } from "../utils/viewState";

// Everything about what the frame shows by itself: the picture now, how often
// it changes, and which albums take part.
const appStore = useAppStore();
const settingsStore = useSettingsStore();
const uploads = useUploadsStore();

const REFRESH_DELAY_MS = 3500; // the panel takes about 2 s to redraw
const currentUrl = ref("");
const currentFailed = ref(false);
const rotating = ref(false);
let timer = null;

async function loadCurrent() {
  try {
    const response = await fetch(`/api/current_image?ts=${Date.now()}`);
    if (!response.ok) throw new Error(String(response.status));
    const blob = await response.blob();
    if (!blob.type.startsWith("image/")) throw new Error("not an image");
    if (currentUrl.value) URL.revokeObjectURL(currentUrl.value);
    currentUrl.value = URL.createObjectURL(blob);
    currentFailed.value = false;
  } catch {
    // Nothing shown yet (fresh boot) or an unreadable format: hide the picture
    currentFailed.value = true;
  }
}

// Fetch when the page is opened, and again after the picture changes.
watch(
  currentView,
  (view) => {
    if (view === "slideshow") loadCurrent();
  },
  { immediate: true }
);
watch(
  () => appStore.displayVersion,
  () => {
    clearTimeout(timer);
    if (currentView.value === "slideshow") timer = setTimeout(loadCurrent, REFRESH_DELAY_MS);
  }
);
onBeforeUnmount(() => {
  clearTimeout(timer);
  if (currentUrl.value) URL.revokeObjectURL(currentUrl.value);
});

async function next() {
  rotating.value = true;
  await appStore.rotateNow();
  rotating.value = false;
}

const schedule = computed(() =>
  settingsStore.deviceSettings.autoRotate
    ? scheduleSummary(settingsStore.deviceSettings.rotateCron, locale.value)
    : t("Auto rotate is off")
);

function photoCount(album) {
  return (appStore.imagesByAlbum[album.name] || []).length;
}
function coverUrl(album) {
  const [first] = appStore.imagesByAlbum[album.name] || [];
  return first
    ? `/api/image?filepath=${encodeURIComponent(first.album + "/" + first.thumbnail)}`
    : "";
}
</script>

<template>
  <div class="slideshow-page">
    <h1 class="page-title">{{ t("Slideshow") }}</h1>

    <!-- What is on the panel now -->
    <section class="now">
      <div class="now-frame" :style="{ aspectRatio: uploads.frameAspect }">
        <img
          v-if="currentUrl && !currentFailed"
          :src="currentUrl"
          :alt="t('Now showing')"
          @error="currentFailed = true"
        />
        <v-icon v-else icon="mdi-image-off-outline" class="text-medium-emphasis" />
      </div>
      <div class="now-text">
        <div class="text-overline text-medium-emphasis">{{ t("Now showing") }}</div>
        <div class="text-body-1 mb-3">{{ schedule }}</div>
        <v-btn variant="tonal" :loading="rotating" @click="next">
          <v-icon icon="mdi-skip-next" start />
          {{ t("Show next image") }}
        </v-btn>
      </div>
    </section>

    <h2 class="section-title">{{ t("Schedule") }}</h2>
    <ImageSettings />

    <template v-if="appStore.albums.length">
      <h2 class="section-title">{{ t("Albums in the slideshow") }}</h2>
      <v-list class="album-list" lines="two" bg-color="transparent">
        <v-list-item
          v-for="album in appStore.sortedAlbums"
          :key="album.name"
          @click="appStore.toggleAlbumEnabled(album.name, !album.enabled)"
        >
          <template #prepend>
            <span class="album-thumb" :style="{ aspectRatio: uploads.frameAspect }">
              <img v-if="coverUrl(album)" :src="coverUrl(album)" alt="" loading="lazy" />
            </span>
          </template>
          <v-list-item-title>{{ albumLabel(album.name) }}</v-list-item-title>
          <v-list-item-subtitle>{{
            t("{n} photos", { n: photoCount(album) })
          }}</v-list-item-subtitle>
          <template #append>
            <v-switch
              :model-value="album.enabled"
              color="primary"
              hide-details
              density="compact"
              :aria-label="t('Include in the slideshow')"
              @click.stop
              @update:model-value="appStore.toggleAlbumEnabled(album.name, $event)"
            />
          </template>
        </v-list-item>
      </v-list>
    </template>
  </div>
</template>

<style scoped>
.slideshow-page {
  max-width: 760px;
}
.page-title {
  font-size: 1.5rem;
  font-weight: 600;
  margin-bottom: 20px;
}
.section-title {
  font-size: 1.05rem;
  font-weight: 600;
  margin: 36px 0 12px;
}
.now {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 20px;
}
.now-frame {
  width: min(100%, 360px);
  flex: none;
  border-radius: 12px;
  overflow: hidden;
  background: rgba(var(--v-theme-on-surface), 0.06);
  display: flex;
  align-items: center;
  justify-content: center;
}
.now-frame img {
  display: block;
  width: 100%;
  height: 100%;
  object-fit: cover;
}
.now-text {
  min-width: 200px;
}
.album-list {
  padding: 0;
}
.album-thumb {
  display: block;
  width: 72px;
  margin-right: 16px;
  border-radius: 6px;
  overflow: hidden;
  background: rgba(var(--v-theme-on-surface), 0.06);
}
.album-thumb img {
  display: block;
  width: 100%;
  height: 100%;
  object-fit: cover;
}
</style>
