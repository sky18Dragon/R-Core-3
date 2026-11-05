<script setup>
import { t } from "../i18n";
import { computed } from "vue";
import { useAppStore, useSettingsStore } from "../stores";
import StorageMeter from "./StorageMeter.vue";
import { currentView, goTo } from "../utils/viewState";

// The sections and storage: always beside the page on wide screens; on phones
// a drawer that ☰ in the top bar slides in.
const props = defineProps({
  temporary: { type: Boolean, default: false },
  modelValue: { type: Boolean, default: false }, // open, when temporary
});
const emit = defineEmits(["update:modelValue"]);

const appStore = useAppStore();
const settingsStore = useSettingsStore();
const deviceTitle = computed(
  () => settingsStore.deviceSettings.deviceName?.trim() || "ESP32 PhotoFrame"
);
const hasStorage = computed(
  () => appStore.systemInfo.sdcard_inserted || appStore.systemInfo.has_flash_storage
);

const items = computed(() => [
  { view: "photos", icon: "mdi-image-outline", title: t("Photos") },
  ...(hasStorage.value ? [{ view: "albums", icon: "mdi-image-album", title: t("Albums") }] : []),
  { view: "slideshow", icon: "mdi-play-box-multiple-outline", title: t("Slideshow") },
  { view: "presets", icon: "mdi-tune-variant", title: t("Presets") },
]);

const open = computed({
  get: () => !props.temporary || props.modelValue,
  set: (value) => emit("update:modelValue", value),
});

function pick(view) {
  goTo(view);
  if (props.temporary) open.value = false;
}
</script>

<template>
  <v-navigation-drawer
    v-model="open"
    :permanent="!temporary"
    :temporary="temporary"
    :border="0"
    width="272"
    class="app-nav"
    :class="{ 'app-nav--temporary': temporary }"
  >
    <div v-if="temporary" class="nav-brand">{{ deviceTitle }}</div>
    <nav class="nav-list" :aria-label="t('Sections')">
      <button
        v-for="item in items"
        :key="item.view"
        type="button"
        class="nav-item"
        :class="{ 'nav-item--active': currentView === item.view }"
        :aria-current="currentView === item.view ? 'page' : undefined"
        @click="pick(item.view)"
      >
        <v-icon :icon="item.icon" size="22" />
        <span>{{ item.title }}</span>
      </button>
    </nav>

    <template v-if="hasStorage" #append>
      <div class="nav-footer">
        <StorageMeter />
      </div>
    </template>
  </v-navigation-drawer>
</template>

<style scoped>
.app-nav {
  background: rgb(var(--v-theme-background)) !important;
}
.app-nav--temporary {
  background: rgb(var(--v-theme-surface)) !important;
  border-radius: 0 16px 16px 0;
}
.nav-brand {
  padding: 20px 32px 8px;
  font-size: 1.1rem;
  font-weight: 600;
}
.nav-list {
  display: flex;
  flex-direction: column;
  gap: 2px;
  padding: 12px 12px 0;
}
.nav-item {
  display: flex;
  align-items: center;
  gap: 20px;
  height: 48px;
  padding: 0 20px;
  border-radius: 24px;
  color: inherit;
  font: inherit;
  font-weight: 500;
  text-align: left;
}
@media (hover: hover) {
  .nav-item:hover {
    background: rgba(var(--v-theme-on-surface), 0.06);
  }
}
.nav-item--active {
  background: rgba(var(--v-theme-on-surface), 0.1);
  font-weight: 700;
}
.nav-footer {
  padding: 12px 12px 20px;
  border-top: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
  margin: 0 12px;
}
</style>
