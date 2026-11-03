<script setup>
import { t } from "../i18n";
import { computed, ref } from "vue";
import { useAppStore } from "../stores";

// Storage as a bar: the numbers only on hover or a tap, so they do not compete
// with the photos. Words appear only when space is nearly gone.
const appStore = useAppStore();
const showNumbers = ref(false);

const total = computed(() => appStore.systemInfo.storage_total || 0);
const used = computed(() => Math.min(appStore.systemInfo.storage_used || 0, total.value));
const percent = computed(() => (total.value ? (used.value / total.value) * 100 : 0));
const nearlyFull = computed(() => percent.value >= 90);

function size(bytes) {
  const gb = bytes / 1024 / 1024 / 1024;
  return gb >= 1 ? `${gb.toFixed(1)} GB` : `${Math.round(bytes / 1024 / 1024)} MB`;
}
const detail = computed(() =>
  t("{used} of {total} used", { used: size(used.value), total: size(total.value) })
);
</script>

<template>
  <button
    v-if="total"
    type="button"
    class="storage-meter"
    :title="detail"
    :aria-label="`${t('Storage')}: ${detail}`"
    @click="showNumbers = !showNumbers"
  >
    <span class="d-flex align-center text-body-2">
      <v-icon icon="mdi-sd" size="small" class="mr-2" />
      {{ t("Storage") }}
      <span v-if="nearlyFull" class="ml-auto text-caption font-weight-bold">
        {{ t("Almost full") }}
      </span>
    </span>
    <span class="storage-track" :class="{ 'storage-track--full': nearlyFull }">
      <span class="storage-fill" :style="{ width: `${Math.max(percent, 1.5)}%` }" />
    </span>
    <span v-if="showNumbers" class="d-block text-caption text-medium-emphasis mt-1">
      {{ detail }}
    </span>
  </button>
</template>

<style scoped>
.storage-meter {
  display: block;
  width: 100%;
  text-align: left;
  padding: 8px 12px;
  border-radius: 10px;
  color: inherit;
  font: inherit;
}
.storage-meter:hover {
  background: rgba(var(--v-theme-on-surface), 0.05);
}
.storage-track {
  display: block;
  height: 4px;
  margin-top: 8px;
  border-radius: 2px;
  background: rgba(var(--v-theme-on-surface), 0.12);
  overflow: hidden;
}
.storage-fill {
  display: block;
  height: 100%;
  border-radius: 2px;
  background: rgba(var(--v-theme-on-surface), 0.55);
}
.storage-track--full .storage-fill {
  background: rgb(var(--v-theme-on-surface));
}
</style>
