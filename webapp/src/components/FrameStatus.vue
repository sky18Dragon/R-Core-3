<script setup>
import { t, locale } from "../i18n";
import { ref, computed, watch, onMounted, onBeforeUnmount } from "vue";
import { useAppStore, useSettingsStore, useUploadsStore } from "../stores";
import { scheduleSummary } from "../utils/slideshow";
import { albumLabel } from "../utils/albums";
import { goTo } from "../utils/viewState";

const appStore = useAppStore();
const settingsStore = useSettingsStore();
const uploads = useUploadsStore();

const REFRESH_DELAY_MS = 3500; // the panel takes about 2 s to redraw
const MAX_RECENT = 8;

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

onMounted(loadCurrent);
onBeforeUnmount(() => {
  clearTimeout(timer);
  if (currentUrl.value) URL.revokeObjectURL(currentUrl.value);
});

// The picture on the panel changed (next image, shown from an album, ...)
watch(
  () => appStore.displayVersion,
  () => {
    clearTimeout(timer);
    timer = setTimeout(loadCurrent, REFRESH_DELAY_MS);
  }
);

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

const recent = computed(() => uploads.lastBatch.slice(-MAX_RECENT).reverse());
const recentAlbum = computed(() => (uploads.lastBatch.length ? uploads.lastBatch[0].album : ""));

function viewRecent() {
  appStore.selectAlbum(recentAlbum.value);
  goTo("albums");
}
</script>

<template>
  <v-card>
    <v-card-text class="d-flex flex-wrap ga-6">
      <!-- What is on the panel now -->
      <div class="now d-flex ga-4 align-center">
        <div class="now-frame" :style="{ aspectRatio: uploads.frameAspect }">
          <img
            v-if="currentUrl && !currentFailed"
            :src="currentUrl"
            :alt="t('Now showing')"
            @error="currentFailed = true"
          />
          <v-icon v-else icon="mdi-image-off-outline" color="grey" />
        </div>
        <div>
          <div class="text-subtitle-2">{{ t("Now showing") }}</div>
          <div class="text-caption text-medium-emphasis mb-2">{{ schedule }}</div>
          <v-btn size="small" variant="tonal" :loading="rotating" @click="next">
            <v-icon icon="mdi-skip-next" start />
            {{ t("Show next image") }}
          </v-btn>
        </div>
      </div>

      <!-- What was just added -->
      <div v-if="recent.length" class="recent flex-grow-1">
        <div class="d-flex align-center mb-2">
          <span class="text-subtitle-2">{{ t("Just added") }}</span>
          <v-spacer />
          <v-btn size="small" variant="text" append-icon="mdi-arrow-right" @click="viewRecent">
            {{ albumLabel(recentAlbum) }}
          </v-btn>
        </div>
        <div class="d-flex ga-2 flex-wrap">
          <img
            v-for="photo in recent"
            :key="photo.id"
            :src="photo.thumbUrl"
            :alt="photo.name"
            class="recent-thumb"
            :style="{ aspectRatio: uploads.frameAspect }"
          />
        </div>
      </div>
    </v-card-text>
  </v-card>
</template>

<style scoped>
.now {
  min-width: 0;
}
.now-frame {
  width: 150px;
  flex: none;
  border-radius: 4px;
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
.recent {
  min-width: 220px;
}
.recent-thumb {
  height: 48px;
  border-radius: 3px;
  object-fit: cover;
  border: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
}
</style>
