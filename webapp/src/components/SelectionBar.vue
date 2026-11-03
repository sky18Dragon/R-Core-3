<script setup>
import { t } from "../i18n";
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { useAppStore } from "../stores";
import { clearSelection, selectedCount, selectedPhotos, selecting } from "../utils/selection";

// Replaces the top bar while photos are picked: how many, and what to do with them.
const appStore = useAppStore();
const confirmDelete = ref(false);
const busy = ref(false);
const notice = ref("");
const noticeShown = computed({
  get: () => !!notice.value,
  set: (value) => {
    if (!value) notice.value = "";
  },
});

async function showOnFrame() {
  const [image] = selectedPhotos.value;
  busy.value = true;
  const ok = await appStore.displayImage(image.album, image.filename);
  busy.value = false;
  notice.value = ok
    ? t("Sent to the frame. The screen is refreshing.")
    : t("Could not show it on the frame");
  if (ok) clearSelection();
}

async function remove() {
  confirmDelete.value = false;
  busy.value = true;
  const list = selectedPhotos.value;
  const deleted = await appStore.deleteImages(list);
  busy.value = false;
  clearSelection();
  notice.value =
    deleted === list.length
      ? t("Deleted {n} photos", { n: deleted })
      : t("Deleted {n} of {total} photos", { n: deleted, total: list.length });
}

function onKey(event) {
  if (event.key === "Escape" && selecting.value && !confirmDelete.value) clearSelection();
}
onMounted(() => window.addEventListener("keydown", onKey));
onBeforeUnmount(() => window.removeEventListener("keydown", onKey));
</script>

<template>
  <v-slide-y-transition>
    <div v-if="selecting" class="selection-bar">
      <v-btn
        icon="mdi-close"
        variant="text"
        :aria-label="t('Clear selection')"
        @click="clearSelection"
      />
      <span class="text-subtitle-1 font-weight-bold ml-1">
        {{ t("{n} selected", { n: selectedCount }) }}
      </span>
      <v-spacer />
      <v-btn
        v-if="selectedCount === 1"
        variant="text"
        :loading="busy"
        :title="t('Show on frame')"
        @click="showOnFrame"
      >
        <v-icon icon="mdi-monitor" start />
        <span class="d-none d-sm-inline">{{ t("Show on frame") }}</span>
      </v-btn>
      <v-btn
        icon="mdi-delete-outline"
        variant="text"
        :disabled="busy"
        :title="t('Delete')"
        :aria-label="t('Delete')"
        @click="confirmDelete = true"
      />
    </div>
  </v-slide-y-transition>

  <v-dialog v-model="confirmDelete" max-width="400">
    <v-card>
      <v-card-title>{{ t("Delete {n} photos?", { n: selectedCount }) }}</v-card-title>
      <v-card-text>{{ t("They are removed from the frame's storage card.") }}</v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="confirmDelete = false">{{ t("Cancel") }}</v-btn>
        <v-btn color="primary" @click="remove">{{ t("Delete") }}</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>

  <v-snackbar v-model="noticeShown" :timeout="3500">{{ notice }}</v-snackbar>
</template>

<style scoped>
.selection-bar {
  position: fixed;
  inset: 0 0 auto 0;
  height: 64px;
  z-index: 1010; /* above the app bar and the side navigation */
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 0 8px;
  background: rgb(var(--v-theme-surface));
  border-bottom: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
}
</style>
