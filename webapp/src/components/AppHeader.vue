<script setup>
import { ref, computed, watch } from "vue";
import { useDisplay } from "vuetify";
import { useAppStore, useSettingsStore } from "../stores";
import { t } from "../i18n";
import { currentView, goTo } from "../utils/viewState";

const appStore = useAppStore();
const settingsStore = useSettingsStore();
const rotating = ref(false);
const { xs } = useDisplay();

const deviceTitle = computed(
  () => settingsStore.deviceSettings.deviceName?.trim() || "ESP32 PhotoFrame"
);

watch(
  deviceTitle,
  (title) => {
    document.title = title;
  },
  { immediate: true }
);

async function handleRotate() {
  rotating.value = true;
  await appStore.rotateNow();
  rotating.value = false;
}

// Low charge is flagged with a different icon and a word, not a colour.
const batteryLow = computed(() => appStore.battery.level < 20 && !appStore.battery.charging);
const batteryIcon = computed(() => {
  if (appStore.battery.charging) return "mdi-battery-charging";
  return batteryLow.value ? "mdi-battery-alert-variant-outline" : "mdi-battery";
});

function openSystemSettings() {
  settingsStore.systemSettingsOpen = true;
}
</script>

<template>
  <v-app-bar color="surface" flat density="comfortable">
    <template #prepend>
      <!-- Same monochrome frame glyph as the setup page -->
      <svg class="header-glyph ml-3" viewBox="0 0 72 52" aria-hidden="true">
        <rect x="2" y="2" width="68" height="48" rx="6" class="g-frame" />
        <rect x="9" y="9" width="54" height="34" rx="1.5" class="g-paper" />
        <circle cx="24" cy="20" r="4" class="g-mid" />
        <path d="M9 43 L29 26 L41 36 L49 29 L63 43 Z" class="g-ink" />
      </svg>
    </template>

    <v-app-bar-title class="ml-3 font-weight-bold">{{ deviceTitle }}</v-app-bar-title>

    <template #append>
      <!-- Show the next image -->
      <v-btn
        icon
        variant="text"
        :loading="rotating"
        :title="t('Show next image')"
        :aria-label="t('Show next image')"
        @click="handleRotate"
      >
        <v-icon>mdi-skip-next</v-icon>
      </v-btn>

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

      <!-- System settings -->
      <v-btn
        icon
        variant="text"
        class="mr-1"
        :title="t('System settings')"
        :aria-label="t('System settings')"
        @click="openSystemSettings"
      >
        <v-badge dot color="primary" :model-value="settingsStore.systemDirty">
          <v-icon>mdi-cog</v-icon>
        </v-badge>
      </v-btn>
    </template>

    <!-- Desktop: the two views as tabs under the title (phones use a bottom bar) -->
    <template v-if="!xs" #extension>
      <v-tabs
        :model-value="currentView"
        color="primary"
        align-tabs="start"
        class="px-2"
        @update:model-value="goTo"
      >
        <v-tab value="upload">
          <v-icon start>mdi-image-plus</v-icon>
          {{ t("Add photos") }}
        </v-tab>
        <v-tab value="albums">
          <v-icon start>mdi-image-multiple</v-icon>
          {{ t("Albums") }}
        </v-tab>
      </v-tabs>
    </template>
  </v-app-bar>
</template>

<style scoped>
.header-glyph {
  width: 36px;
  height: 26px;
}
.g-frame {
  fill: #fff;
  stroke: #111;
  stroke-width: 4;
}
.g-paper {
  fill: #f0f0f0;
}
.g-mid {
  fill: #c9c9c9;
}
.g-ink {
  fill: #666;
}
.battery-low {
  border-width: 2px;
  font-weight: 700;
}
</style>
