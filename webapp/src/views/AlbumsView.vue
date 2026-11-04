<script setup>
import { t } from "../i18n";
import { ref } from "vue";
import { useAppStore, useUploadsStore } from "../stores";
import NewAlbumDialog from "../components/NewAlbumDialog.vue";
import AlbumDetail from "./AlbumDetail.vue";
import { albumLabel } from "../utils/albums";
import { currentAlbum, openAlbum } from "../utils/viewState";

// The album list as covers; one album opens on its own page.
const appStore = useAppStore();
const uploads = useUploadsStore();
const newAlbumOpen = ref(false);

function photosOf(album) {
  return appStore.imagesByAlbum[album.name] || [];
}
function coverUrl(album) {
  const [first] = photosOf(album);
  return first
    ? `/api/image?filepath=${encodeURIComponent(first.album + "/" + first.thumbnail)}`
    : "";
}
</script>

<template>
  <AlbumDetail v-if="currentAlbum" :name="currentAlbum" />

  <div v-else>
    <h1 class="page-title">{{ t("Albums") }}</h1>

    <div class="album-grid">
      <button type="button" class="album-card album-card--new" @click="newAlbumOpen = true">
        <span class="album-cover album-cover--new" :style="{ aspectRatio: uploads.frameAspect }">
          <v-icon icon="mdi-plus" size="32" />
        </span>
        <span class="album-name">{{ t("New album") }}</span>
      </button>

      <button
        v-for="album in appStore.sortedAlbums"
        :key="album.name"
        type="button"
        class="album-card"
        @click="openAlbum(album.name)"
      >
        <span class="album-cover" :style="{ aspectRatio: uploads.frameAspect }">
          <img v-if="coverUrl(album)" :src="coverUrl(album)" alt="" loading="lazy" />
          <v-icon v-else icon="mdi-image-outline" size="32" class="text-medium-emphasis" />
        </span>
        <span class="album-name">{{ albumLabel(album.name) }}</span>
        <span class="album-meta text-caption text-medium-emphasis">
          {{ t("{n} photos", { n: photosOf(album).length }) }}
          <template v-if="album.enabled"> · {{ t("In slideshow") }}</template>
        </span>
      </button>
    </div>

    <NewAlbumDialog v-model="newAlbumOpen" @created="openAlbum" />
  </div>
</template>

<style scoped>
.page-title {
  font-size: 1.5rem;
  font-weight: 600;
  margin-bottom: 20px;
}
.album-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
  gap: 24px 16px;
}
@media (max-width: 600px) {
  .album-grid {
    grid-template-columns: repeat(2, 1fr);
    gap: 16px 12px;
  }
}
.album-card {
  display: flex;
  flex-direction: column;
  text-align: left;
  color: inherit;
  font: inherit;
  border-radius: 12px;
}
.album-card:focus-visible {
  outline: 2px solid rgb(var(--v-theme-on-surface));
  outline-offset: 4px;
}
.album-cover {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  border-radius: 12px;
  overflow: hidden;
  background: rgba(var(--v-theme-on-surface), 0.06);
}
.album-cover img {
  width: 100%;
  height: 100%;
  object-fit: cover;
  transition: transform 0.2s ease;
}
@media (hover: hover) {
  .album-card:hover .album-cover img {
    transform: scale(1.03);
  }
  .album-card--new:hover .album-cover--new {
    background: rgba(var(--v-theme-on-surface), 0.05);
  }
}
.album-cover--new {
  border: 1.5px dashed rgba(var(--v-theme-on-surface), 0.3);
  background: transparent;
}
.album-name {
  margin-top: 10px;
  font-weight: 600;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.album-meta {
  margin-top: 2px;
}
@media (prefers-reduced-motion: reduce) {
  .album-cover img {
    transition: none;
  }
}
</style>
