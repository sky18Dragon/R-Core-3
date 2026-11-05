<script setup>
import { computed, ref, watch } from "vue";
import { useDisplay } from "vuetify";
import { useAppStore, useSettingsStore } from "../stores";
import { t } from "../i18n";
import UploadButton from "./UploadButton.vue";
import { currentView, goTo } from "../utils/viewState";

// Wide screens: frame name on the left, upload and settings on the right; the
// sections live in the side bar. Phones: ☰ (the drawer), the section's name,
// "+" and ⋮ -- the sections live in the drawer.
const emit = defineEmits(["toggle-nav"]);

const appStore = useAppStore();
const settingsStore = useSettingsStore();
const { mdAndUp } = useDisplay();

const deviceTitle = computed(
  () => settingsStore.deviceSettings.deviceName?.trim() || "ESP32 PhotoFrame"
);
const sectionTitle = computed(
  () =>
    ({ albums: t("Albums"), slideshow: t("Slideshow"), presets: t("Presets") })[
      currentView.value
    ] || t("Photos")
);

watch(
  deviceTitle,
  (title) => {
    document.title = title;
  },
  { immediate: true }
);

// Low charge is flagged with a different icon and a word, not a colour.
const batteryLow = computed(() => appStore.battery.level < 20 && !appStore.battery.charging);
const batteryIcon = computed(() => {
  if (appStore.battery.charging) return "mdi-battery-charging";
  return batteryLow.value ? "mdi-battery-alert-variant-outline" : "mdi-battery";
});

function openSystemSettings() {
  settingsStore.systemSettingsOpen = true;
}

const notice = ref(false);
async function showNext() {
  notice.value = false;
  const ok = await appStore.rotateNow();
  if (ok) notice.value = true;
}
</script>

<template>
  <v-app-bar color="background" flat density="comfortable" class="app-header">
    <template #prepend>
      <template v-if="mdAndUp">
        <button type="button" class="brand" :aria-label="t('Photos')" @click="goTo('photos')">
          <!-- Same monochrome frame glyph as the setup page -->
          <svg class="header-glyph" viewBox="0 0 72 52" aria-hidden="true">
            <rect x="2" y="2" width="68" height="48" rx="6" class="g-frame" />
            <rect x="9" y="9" width="54" height="34" rx="1.5" class="g-paper" />
            <circle cx="24" cy="20" r="4" class="g-mid" />
            <path d="M9 43 L29 26 L41 36 L49 29 L63 43 Z" class="g-ink" />
          </svg>
          <span class="brand-title">{{ deviceTitle }}</span>
        </button>
      </template>
      <template v-else>
        <v-btn
          icon="mdi-menu"
          variant="text"
          :aria-label="t('Menu')"
          :title="t('Menu')"
          @click="emit('toggle-nav')"
        />
        <span class="section-title">{{ sectionTitle }}</span>
      </template>
    </template>

    <template #append>
      <!-- Battery: only on frames that have one -->
      <v-chip
        v-if="appStore.battery.connected"
        variant="outlined"
        class="mx-2"
        :class="{ 'battery-low': batteryLow }"
        :title="batteryLow ? t('Low') : undefined"
      >
        <v-icon :icon="batteryIcon" start />
        {{ appStore.battery.level }}%
        <span v-if="appStore.battery.charging" class="ml-1 d-none d-sm-inline">
          {{ t("Charging") }}
        </span>
        <span v-else-if="batteryLow" class="ml-1 d-none d-sm-inline">{{ t("Low") }}</span>
      </v-chip>

      <UploadButton :kind="mdAndUp ? 'text' : 'icon'" :label="t('Upload')" />

      <v-btn
        v-if="mdAndUp"
        icon
        variant="text"
        class="mr-1"
        :title="t('System settings')"
        :aria-label="t('System settings')"
        @click="openSystemSettings"
      >
        <v-badge dot color="primary" :model-value="settingsStore.systemDirty">
          <v-icon>mdi-cog-outline</v-icon>
        </v-badge>
      </v-btn>

      <v-menu v-else location="bottom end">
        <template #activator="{ props: menuProps }">
          <v-btn v-bind="menuProps" icon variant="text" class="mr-1" :aria-label="t('More')">
            <v-badge dot color="primary" :model-value="settingsStore.systemDirty">
              <v-icon>mdi-dots-vertical</v-icon>
            </v-badge>
          </v-btn>
        </template>
        <v-list density="comfortable" min-width="200">
          <v-list-item
            prepend-icon="mdi-skip-next"
            :title="t('Show next image')"
            @click="showNext"
          />
          <v-list-item
            prepend-icon="mdi-cog-outline"
            :title="t('System settings')"
            @click="openSystemSettings"
          />
        </v-list>
      </v-menu>
    </template>
  </v-app-bar>

  <v-snackbar v-model="notice" :timeout="3000">
    {{ t("Sent to the frame. The screen is refreshing.") }}
  </v-snackbar>
</template>

<style scoped>
.app-header {
  border-bottom: 0 !important;
}
.brand {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-left: 16px;
  color: inherit;
  font: inherit;
  border-radius: 8px;
}
.brand-title {
  font-size: 1.25rem;
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  max-width: 50vw;
}
.section-title {
  margin-left: 12px;
  font-size: 1.375rem;
  font-weight: 500;
}
.header-glyph {
  width: 34px;
  height: 25px;
  flex: none;
}
.g-frame {
  fill: rgb(var(--v-theme-surface));
  stroke: rgb(var(--v-theme-on-surface));
  stroke-width: 4;
}
.g-paper {
  fill: rgba(var(--v-theme-on-surface), 0.08);
}
.g-mid {
  fill: rgba(var(--v-theme-on-surface), 0.3);
}
.g-ink {
  fill: rgba(var(--v-theme-on-surface), 0.6);
}
.battery-low {
  border-width: 2px;
  font-weight: 700;
}
</style>
