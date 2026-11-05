<script setup>
import { t } from "../i18n";
import { computed, onMounted, onUnmounted, ref, watch } from "vue";
import { useDisplay } from "vuetify";
import {
  useAppStore,
  useHotspotStore,
  usePresetsStore,
  useSettingsStore,
  useUploadsStore,
} from "../stores";
import AppHeader from "../components/AppHeader.vue";
import AppNav from "../components/AppNav.vue";
import HotspotBanner from "../components/HotspotBanner.vue";
import PhotoUploader from "../components/PhotoUploader.vue";
import SelectionBar from "../components/SelectionBar.vue";
import SystemSettings from "../components/SystemSettings.vue";
import PhotosView from "./PhotosView.vue";
import AlbumsView from "./AlbumsView.vue";
import SlideshowView from "./SlideshowView.vue";
import PresetsView from "./PresetsView.vue";
import { currentAlbum, currentView } from "../utils/viewState";
import { clearSelection } from "../utils/selection";
import { prefersDark } from "../utils/colorScheme";
import "../styles/paper.css";

const appStore = useAppStore();
const settingsStore = useSettingsStore();
const hotspotStore = useHotspotStore();
const presetsStore = usePresetsStore();
const uploads = useUploadsStore();
const { mdAndUp } = useDisplay();
const navOpen = ref(false); // the phone drawer

const hasStorage = computed(
  () => appStore.systemInfo.sdcard_inserted || appStore.systemInfo.has_flash_storage
);

// Each view starts at the top when you switch to it, with nothing picked
watch([currentView, currentAlbum], () => {
  clearSelection();
  window.scrollTo(0, 0);
});

// The photo wall needs every album's list; refetch when albums come or go.
watch(
  () => appStore.albums.map((a) => a.name).join("\n"),
  () => appStore.loadAllImages()
);

// Photos dropped anywhere open the upload dialog (into the open album, if any).
const dragDepth = ref(0);
function hasFiles(event) {
  return [...(event.dataTransfer?.types || [])].includes("Files");
}
function onDragEnter(event) {
  if (!hasFiles(event) || uploads.dialogOpen) return;
  dragDepth.value++;
}
function onDragLeave() {
  if (dragDepth.value > 0) dragDepth.value--;
}
function onDragOver(event) {
  if (hasFiles(event)) event.preventDefault(); // allows the drop
}
function onDrop(event) {
  const wasDragging = dragDepth.value > 0;
  dragDepth.value = 0;
  if (!wasDragging || uploads.running) return;
  event.preventDefault(); // keep the browser from opening the photo
  const album = currentView.value === "albums" ? currentAlbum.value : null;
  uploads.begin(event.dataTransfer.files, album);
}
// On the window, so the whole page takes a drop without wrapping the layout.
onMounted(() => {
  window.addEventListener("dragenter", onDragEnter);
  window.addEventListener("dragover", onDragOver);
  window.addEventListener("dragleave", onDragLeave);
  window.addEventListener("drop", onDrop);
});
onUnmounted(() => {
  window.removeEventListener("dragenter", onDragEnter);
  window.removeEventListener("dragover", onDragOver);
  window.removeEventListener("dragleave", onDragLeave);
  window.removeEventListener("drop", onDrop);
});

onMounted(async () => {
  // Load system info first to check for SD card
  await appStore.loadSystemInfo();

  await Promise.all([
    appStore.loadBatteryStatus(),
    appStore.loadAlbums(),
    settingsStore.loadSettings(),
    settingsStore.loadDeviceSettings(),
    settingsStore.loadPalette(),
    hotspotStore.load(),
    hotspotStore.checkClock(),
  ]);
  // Grayscale frames default to the grayscale preset (if not yet customized).
  if (appStore.isGrayscale) {
    settingsStore.applyGrayscaleDefaultIfUntouched();
  }
  // The preset library; a frame without one gets "My default" from its setting
  if (appStore.systemInfo.sdcard_inserted || appStore.systemInfo.has_flash_storage) {
    await presetsStore.load();
    await presetsStore.ensureMigrated();
  }

  // Refresh battery every 30s
  setInterval(() => appStore.loadBatteryStatus(), 30000);
  // While the hotspot is on, keep its countdown and phone count current.
  setInterval(() => {
    if (hotspotStore.on && document.visibilityState === "visible") hotspotStore.load();
  }, 10000);
});

// Keep the device awake while the webapp is open and focused.
// Pause polling when the tab is hidden, and ping immediately on re-focus.
// If a ping fails the device has gone to deep sleep (unreachable), so we
// surface a hint telling the user which physical button wakes their board.
const KEEP_ALIVE_INTERVAL = 30000;
const WAKE_RETRY_INTERVAL = 5000;
let keepAliveInterval = null;
let wakeRetryInterval = null;

const deviceAsleep = ref(false);
const wakeupKeyName = computed(() => appStore.systemInfo.wakeup_key_name || t("wake button"));

async function pingKeepAlive() {
  const alive = await appStore.keepAlive();
  // After a hotspot switch the page is expected to lose the frame; the
  // switch dialog already says where to find it, so this is not "asleep".
  deviceAsleep.value = !alive && !hotspotStore.handoff;
  if (alive && hotspotStore.supported) hotspotStore.load();
  if (alive) {
    stopWakeRetry();
  } else {
    startWakeRetry();
  }
  return alive;
}

// While the device is unreachable, poll faster so the hint clears promptly
// once the user presses the wakeup button.
function startWakeRetry() {
  if (wakeRetryInterval) return;
  wakeRetryInterval = setInterval(pingKeepAlive, WAKE_RETRY_INTERVAL);
}

function stopWakeRetry() {
  if (wakeRetryInterval) {
    clearInterval(wakeRetryInterval);
    wakeRetryInterval = null;
  }
}

function startKeepAlive() {
  if (keepAliveInterval) return;
  pingKeepAlive();
  keepAliveInterval = setInterval(pingKeepAlive, KEEP_ALIVE_INTERVAL);
}

function stopKeepAlive() {
  if (keepAliveInterval) {
    clearInterval(keepAliveInterval);
    keepAliveInterval = null;
  }
}

function handleVisibilityChange() {
  if (document.visibilityState === "visible") {
    startKeepAlive();
  } else {
    // Don't poll while hidden/unfocused.
    stopKeepAlive();
    stopWakeRetry();
  }
}

onMounted(() => {
  document.addEventListener("visibilitychange", handleVisibilityChange);
  if (document.visibilityState === "visible") {
    startKeepAlive();
  }
});

onUnmounted(() => {
  document.removeEventListener("visibilitychange", handleVisibilityChange);
  stopKeepAlive();
  stopWakeRetry();
});
</script>

<template>
  <v-app :theme="prefersDark ? 'paperDark' : 'paper'">
    <AppHeader @toggle-nav="navOpen = !navOpen" />
    <AppNav v-model="navOpen" :temporary="!mdAndUp" />
    <SelectionBar />
    <SystemSettings />

    <v-main class="bg-background">
      <v-container fluid class="page px-3 px-sm-6 pt-2 pb-10">
        <v-alert
          v-if="
            appStore.systemInfo.has_sdcard &&
            !appStore.systemInfo.sdcard_inserted &&
            !appStore.systemInfo.has_flash_storage
          "
          type="info"
          variant="tonal"
          class="mb-6"
          :title="t('SD Card Not Detected')"
          :text="
            t(
              'This device supports an SD card. To upload images and create multiple albums, please insert one and restart the device.'
            )
          "
        ></v-alert>

        <HotspotBanner />

        <PhotosView v-show="currentView === 'photos'" />
        <AlbumsView v-if="hasStorage" v-show="currentView === 'albums'" />
        <SlideshowView v-show="currentView === 'slideshow'" />
        <PresetsView v-show="currentView === 'presets'" />
      </v-container>
    </v-main>

    <PhotoUploader />

    <!-- Dropping photos anywhere -->
    <div v-if="dragDepth > 0" class="drop-overlay" aria-hidden="true">
      <v-icon icon="mdi-upload" size="40" />
      <span class="text-h6 mt-2">{{ t("Drop to upload") }}</span>
    </div>

    <!-- Device-asleep hint: shown when the device stops responding to keep-alive -->
    <v-dialog v-model="deviceAsleep" max-width="440" persistent>
      <v-card>
        <v-card-title class="d-flex align-center">
          <v-icon icon="mdi-power-sleep" class="mr-2" />
          {{ t("Device is asleep") }}
        </v-card-title>
        <v-card-text>
          {{
            t(
              "Your photo frame has gone to sleep to save power and is no longer responding. Press the"
            )
          }}
          <strong>{{ wakeupKeyName }}</strong>
          {{ t("on the device to wake it, then tap Try again.") }}
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn color="primary" variant="text" @click="pingKeepAlive">
            {{ t("Try again") }}
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-app>
</template>

<style scoped>
.page {
  max-width: 1600px;
}
.drop-overlay {
  position: fixed;
  inset: 12px;
  z-index: 3000;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  border: 2px dashed rgb(var(--v-theme-on-surface));
  border-radius: 16px;
  background: rgba(var(--v-theme-background), 0.88);
  pointer-events: none;
}
</style>

<!-- Touch screens: small icon buttons get a 44px hit area (the thumbnail delete
     overlay keeps its own, thumbnail-sized target). -->
<style>
@media (pointer: coarse) {
  .v-btn--icon.v-btn--size-x-small:not(.delete-overlay) {
    min-width: 44px;
    min-height: 44px;
  }
}
</style>
