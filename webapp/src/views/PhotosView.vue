<script setup>
import { t } from "../i18n";
import { computed, ref } from "vue";
import { useAppStore, useUploadsStore } from "../stores";
import PhotoGrid from "../components/PhotoGrid.vue";
import PhotoViewer from "../components/PhotoViewer.vue";
import UploadButton from "../components/UploadButton.vue";
import { albumLabel } from "../utils/albums";
import { openAlbum } from "../utils/viewState";
import { selectAll } from "../utils/selection";

// Home: every photo on the frame, one section per album (the frame keeps no
// dates, so albums are the only natural grouping).
const appStore = useAppStore();
const uploads = useUploadsStore();

const sections = computed(() =>
  appStore.sortedAlbums
    .map((album) => ({ album, images: appStore.imagesByAlbum[album.name] || [] }))
    .filter((section) => section.images.length)
);
const allImages = computed(() => sections.value.flatMap((section) => section.images));
const firstLoad = computed(
  () => appStore.loading.allImages && !Object.keys(appStore.imagesByAlbum).length
);

// The viewer walks the whole wall, so a section's index is offset by those above it.
const viewing = ref(-1);
function openAt(sectionIndex, index) {
  let offset = 0;
  for (let i = 0; i < sectionIndex; ++i) offset += sections.value[i].images.length;
  viewing.value = offset + index;
}
</script>

<template>
  <div>
    <!-- No card: the frame shows each photo directly and keeps nothing -->
    <div v-if="appStore.systemInfoLoaded && !uploads.canSaveToAlbum" class="empty-state">
      <v-icon icon="mdi-image-outline" size="56" class="text-medium-emphasis" />
      <p class="text-h6 mt-4">{{ t("Send a photo to the frame") }}</p>
      <p class="text-body-2 text-medium-emphasis">
        {{ t("No storage card: the photo is shown on the frame directly and not saved.") }}
      </p>
      <UploadButton kind="large" :label="t('Choose a photo')" class="mt-4" />
    </div>

    <div v-else-if="firstLoad" class="d-flex justify-center py-16">
      <v-progress-circular indeterminate />
    </div>

    <div v-else-if="!sections.length" class="empty-state">
      <v-icon icon="mdi-image-plus-outline" size="56" class="text-medium-emphasis" />
      <p class="text-h6 mt-4">{{ t("No photos yet") }}</p>
      <p class="text-body-2 text-medium-emphasis">
        {{ t("Upload photos and they appear here and take turns on the frame.") }}
      </p>
      <UploadButton kind="large" :label="t('Upload photos')" class="mt-4" />
      <p class="text-caption text-medium-emphasis mt-3 d-none d-md-block">
        {{ t("You can also drop photos anywhere on this page.") }}
      </p>
    </div>

    <section
      v-for="(section, sectionIndex) in sections"
      :key="section.album.name"
      class="photo-section"
    >
      <div class="section-head">
        <button type="button" class="section-title" @click="openAlbum(section.album.name)">
          {{ albumLabel(section.album.name) }}
        </button>
        <span class="text-body-2 text-medium-emphasis">
          {{ t("{n} photos", { n: section.images.length }) }}
        </span>
        <span v-if="section.album.enabled" class="slideshow-mark">
          <v-icon icon="mdi-play" size="14" />{{ t("In slideshow") }}
        </span>
        <v-spacer />
        <v-btn
          variant="text"
          size="small"
          class="d-none d-sm-flex"
          @click="selectAll(section.images)"
        >
          {{ t("Select all") }}
        </v-btn>
      </div>
      <PhotoGrid :images="section.images" @open="openAt(sectionIndex, $event)" />
    </section>

    <PhotoViewer v-model="viewing" :images="allImages" />
  </div>
</template>

<style scoped>
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  text-align: center;
  padding: 96px 16px;
}
.photo-section + .photo-section {
  margin-top: 40px;
}
.section-head {
  display: flex;
  align-items: baseline;
  flex-wrap: wrap;
  gap: 4px 12px;
  margin-bottom: 12px;
}
.section-title {
  font-size: 1.25rem;
  font-weight: 600;
  color: inherit;
}
.section-title:hover {
  text-decoration: underline;
}
.slideshow-mark {
  display: inline-flex;
  align-items: center;
  gap: 2px;
  font-size: 0.75rem;
  padding: 1px 8px 1px 6px;
  border-radius: 10px;
  border: 1px solid rgba(var(--v-border-color), 0.3);
  color: rgba(var(--v-theme-on-surface), 0.75);
}
</style>
