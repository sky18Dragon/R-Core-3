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

function getBatteryColor(level) {
  if (level < 20) return "error";
  if (level < 50) return "warning";
  return "success";
}

function openSystemSettings() {
  settingsStore.systemSettingsOpen = true;
}
</script>

<template>
  <v-app-bar color="primary" density="comfortable">
    <template #prepend>
      <v-img src="/icon.svg" alt="PhotoFrame" width="40" height="40" class="ml-2" />
    </template>

    <v-app-bar-title class="ml-4">{{ deviceTitle }}</v-app-bar-title>

    <template #append>
      <!-- Show the next image -->
      <v-btn
        icon
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
        variant="flat"
        :color="getBatteryColor(appStore.battery.level)"
        class="text-white mx-2"
      >
        <v-icon :icon="appStore.battery.charging ? 'mdi-battery-charging' : 'mdi-battery'" start />
        {{ appStore.battery.level }}%
        <span v-if="appStore.battery.charging" class="ml-1 d-none d-sm-inline">
          {{ t("Charging") }}
        </span>
      </v-chip>

      <!-- System settings -->
      <v-btn
        icon
        class="mr-1"
        :title="t('System settings')"
        :aria-label="t('System settings')"
        @click="openSystemSettings"
      >
        <v-badge dot color="warning" :model-value="settingsStore.systemDirty">
          <v-icon>mdi-cog</v-icon>
        </v-badge>
      </v-btn>
    </template>

    <!-- Desktop: the two views as tabs under the title (phones use a bottom bar) -->
    <template v-if="!xs" #extension>
      <v-tabs
        :model-value="currentView"
        color="white"
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
