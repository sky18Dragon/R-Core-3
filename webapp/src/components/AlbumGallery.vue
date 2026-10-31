<script setup>
import { t } from "../i18n";
import { ref, computed } from "vue";
import { useAppStore, useSettingsStore, useUploadsStore } from "../stores";
import ImageSettings from "./ImageSettings.vue";
import NewAlbumDialog from "./NewAlbumDialog.vue";
import { imageSettingsOpen } from "../utils/uiPrefs";
import { albumLabel } from "../utils/albums";
import { scheduleSummary } from "../utils/slideshow";
import { locale } from "../i18n";

const appStore = useAppStore();
const settingsStore = useSettingsStore();
const uploads = useUploadsStore();

const newAlbumDialog = ref(false);
const deleteAlbumDialog = ref(false);
const albumToDelete = ref(null);
const displayLoading = ref(false);
const displayDialog = ref(false);
const imageToDisplay = ref(null);
const deleteImageDialog = ref(false);
const imageToDelete = ref(null);

function onAlbumCreated(name) {
  appStore.selectAlbum(name);
}

function confirmDeleteAlbum(album) {
  albumToDelete.value = album;
  deleteAlbumDialog.value = true;
}

async function deleteAlbum() {
  if (albumToDelete.value) {
    await appStore.deleteAlbum(albumToDelete.value.name);
    albumToDelete.value = null;
    deleteAlbumDialog.value = false;
  }
}

function confirmDisplayImage(image) {
  imageToDisplay.value = image;
  displayDialog.value = true;
}

async function displayImage() {
  if (imageToDisplay.value) {
    displayDialog.value = false;
    displayLoading.value = true;
    await appStore.displayImage(imageToDisplay.value.album, imageToDisplay.value.filename);
    displayLoading.value = false;
    imageToDisplay.value = null;
  }
}

function confirmDeleteImage(image) {
  imageToDelete.value = image;
  deleteImageDialog.value = true;
}

async function deleteImage() {
  if (imageToDelete.value) {
    await appStore.deleteImage(imageToDelete.value.album, imageToDelete.value.filename);
    imageToDelete.value = null;
    deleteImageDialog.value = false;
  }
}

// Thumbnails are frame-shaped, so tiles are too (no grey bands)
const frameRatio = computed(() => {
  const [w, h] = uploads.frameAspect.split(" / ").map(Number);
  return w && h ? w / h : 16 / 9;
});

const enabledAlbumCount = computed(() => appStore.albums.filter((a) => a.enabled).length);

// One line that says what the slideshow is doing without opening the settings
const slideshowLine = computed(() => {
  const d = settingsStore.deviceSettings;
  if (!d.autoRotate) return t("Auto rotate is off");
  const parts = [scheduleSummary(d.rotateCron, locale.value)];
  parts.push(d.sdRotationMode === "sequential" ? t("In order") : t("Random order"));
  parts.push(t("{n} albums in the slideshow", { n: enabledAlbumCount.value }));
  return parts.join(" · ");
});

function albumCount(album) {
  return Number.isFinite(album.image_count) ? album.image_count : null;
}

function getThumbnailUrl(image) {
  return `/api/image?filepath=${encodeURIComponent(image.album + "/" + image.thumbnail)}`;
}
</script>

<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon icon="mdi-image-multiple" class="mr-2" />
      {{ t("Albums") }}
      <v-spacer />
      <v-btn color="primary" size="small" variant="flat" @click="newAlbumDialog = true">
        <v-icon icon="mdi-plus" start />
        {{ t("New Album") }}
      </v-btn>
    </v-card-title>

    <v-card-text>
      <!-- Albums: tick = part of the slideshow -->
      <div class="d-flex align-center flex-wrap ga-2">
        <div
          v-for="album in appStore.sortedAlbums"
          :key="album.name"
          class="album-chip d-flex align-center"
          :class="{ 'album-chip--selected': appStore.selectedAlbum === album.name }"
        >
          <v-checkbox-btn
            :model-value="album.enabled"
            color="primary"
            hide-details
            density="compact"
            :title="t('Include in the slideshow')"
            :aria-label="t('Include in the slideshow')"
            @click.stop
            @update:model-value="appStore.toggleAlbumEnabled(album.name, $event)"
          />
          <span class="album-name" @click="appStore.selectAlbum(album.name)">
            {{ albumLabel(album.name) }}
            <span v-if="albumCount(album) !== null" class="text-medium-emphasis">
              {{ albumCount(album) }}
            </span>
          </span>
          <v-btn
            v-if="album.name !== 'Default'"
            icon
            size="x-small"
            variant="text"
            :aria-label="t('Delete Album?')"
            @click.stop="confirmDeleteAlbum(album)"
          >
            <v-icon size="small"> mdi-close </v-icon>
          </v-btn>
        </div>
      </div>
      <div class="text-caption text-medium-emphasis mt-1">
        {{ t("Tick an album to include it in the slideshow") }}
      </div>

      <!-- Slideshow: a summary that opens the settings -->
      <div class="slideshow-row d-flex align-center flex-wrap ga-2 mt-4">
        <v-icon icon="mdi-slideshow" size="small" class="text-medium-emphasis" />
        <span class="text-body-2 slideshow-line">{{ slideshowLine }}</span>
        <v-spacer />
        <v-btn
          variant="tonal"
          size="small"
          :color="imageSettingsOpen ? 'primary' : undefined"
          :aria-expanded="imageSettingsOpen"
          @click="imageSettingsOpen = !imageSettingsOpen"
        >
          <v-badge
            dot
            color="warning"
            :model-value="settingsStore.imageDirty && !imageSettingsOpen"
          >
            <v-icon icon="mdi-tune-variant" start />
          </v-badge>
          {{ t("Slideshow settings") }}
          <v-icon :icon="imageSettingsOpen ? 'mdi-chevron-up' : 'mdi-chevron-down'" end />
        </v-btn>
      </div>

      <v-expand-transition>
        <div v-if="imageSettingsOpen" class="mt-3">
          <ImageSettings />
        </div>
      </v-expand-transition>

      <v-divider class="my-4" />

      <!-- Image Gallery -->
      <div class="d-flex align-center mb-4">
        <h3 class="text-h6">
          {{ albumLabel(appStore.selectedAlbum) }}
        </h3>
        <v-spacer />
        <v-progress-circular v-if="displayLoading" indeterminate size="24" class="mr-2" />
        <span v-if="displayLoading" class="text-body-2 text-grey">
          {{ t("Updating display...") }}
        </span>
      </div>

      <div v-if="appStore.loading.images" class="d-flex justify-center align-center py-12">
        <v-progress-circular indeterminate color="primary" />
      </div>

      <template v-else>
        <v-row v-if="appStore.currentAlbumImages.length > 0" dense>
          <v-col
            v-for="image in appStore.currentAlbumImages"
            :key="image.filename"
            cols="6"
            sm="4"
            md="3"
            lg="2"
          >
            <v-card variant="outlined" class="image-card">
              <v-img
                :src="getThumbnailUrl(image)"
                :alt="image.filename"
                :aspect-ratio="frameRatio"
                cover
                class="cursor-pointer bg-grey-lighten-3"
                @click="confirmDisplayImage(image)"
              >
                <template #placeholder>
                  <div class="d-flex align-center justify-center fill-height">
                    <v-progress-circular indeterminate color="grey-lighten-4" />
                  </div>
                </template>
              </v-img>
              <div class="delete-hotspot">
                <v-btn
                  icon="mdi-delete"
                  size="x-small"
                  color="error"
                  class="delete-overlay"
                  :title="t('Delete image')"
                  :aria-label="t('Delete image')"
                  @click.stop="confirmDeleteImage(image)"
                />
              </div>
            </v-card>
          </v-col>
        </v-row>

        <v-alert v-else type="info" variant="tonal" class="mt-4">
          {{ t("No images in this album. Upload images to get started.") }}
        </v-alert>
      </template>
    </v-card-text>
  </v-card>

  <NewAlbumDialog v-model="newAlbumDialog" @created="onAlbumCreated" />

  <!-- Delete Album Dialog -->
  <v-dialog v-model="deleteAlbumDialog" max-width="400">
    <v-card>
      <v-card-title>{{ t("Delete Album?") }}</v-card-title>
      <v-card-text>
        {{
          t(
            'Are you sure you want to delete "{name}"? This will also delete all images in the album.',
            {
              name: albumLabel(albumToDelete?.name),
            }
          )
        }}
      </v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="deleteAlbumDialog = false"> {{ t("Cancel") }} </v-btn>
        <v-btn color="error" @click="deleteAlbum"> {{ t("Delete") }} </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>

  <!-- Display Image Dialog -->
  <v-dialog v-model="displayDialog" max-width="400">
    <v-card>
      <v-card-title>
        <v-icon icon="mdi-monitor" class="mr-2" />
        {{ t("Display Image?") }}
      </v-card-title>
      <v-card-text>
        <div class="mb-3">{{ t("Show this image on the e-paper display?") }}</div>
        <div class="d-flex justify-center">
          <img
            v-if="imageToDisplay"
            :src="getThumbnailUrl(imageToDisplay)"
            alt=""
            class="confirm-thumb"
          />
        </div>
      </v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="displayDialog = false"> {{ t("Cancel") }} </v-btn>
        <v-btn color="primary" @click="displayImage"> {{ t("Display") }} </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>

  <!-- Delete Image Dialog -->
  <v-dialog v-model="deleteImageDialog" max-width="400">
    <v-card>
      <v-card-title>
        <v-icon icon="mdi-delete" color="error" class="mr-2" />
        {{ t("Delete Image?") }}
      </v-card-title>
      <v-card-text>
        <div class="mb-3">{{ t("Are you sure you want to delete this image?") }}</div>
        <div class="d-flex justify-center">
          <img
            v-if="imageToDelete"
            :src="getThumbnailUrl(imageToDelete)"
            alt=""
            class="confirm-thumb"
          />
        </div>
      </v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="deleteImageDialog = false"> {{ t("Cancel") }} </v-btn>
        <v-btn color="error" @click="deleteImage"> {{ t("Delete") }} </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<style scoped>
.slideshow-line {
  min-width: 0;
}
.album-chip {
  border: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
  border-radius: 20px;
  padding: 4px 4px 4px 4px;
  background: rgb(var(--v-theme-surface));
  cursor: pointer;
  margin-right: 8px;
  margin-bottom: 8px;
}
.album-chip--selected {
  border-color: rgb(var(--v-theme-primary));
  background: rgb(var(--v-theme-primary) / 0.08);
}
.album-name {
  cursor: pointer;
  user-select: none;
  padding: 0 4px;
}
.image-card:hover {
  border-color: rgb(var(--v-theme-primary));
}
.cursor-pointer {
  cursor: pointer;
}
.delete-hotspot {
  position: absolute;
  top: 0;
  right: 0;
  width: 48px;
  height: 48px;
  z-index: 1;
}
.delete-overlay {
  position: absolute;
  top: 4px;
  right: 4px;
  opacity: 0;
  transition: opacity 0.2s;
}
/* Reveal the button as soon as the thumbnail itself is hovered or focused,
   not only its top-right corner (which nobody found). */
.image-card:hover .delete-overlay,
.image-card:focus-within .delete-overlay,
.delete-overlay:focus-visible {
  opacity: 1;
}
/* Touch screens have no hover: keep the button visible, with a larger target. */
@media (hover: none) {
  .image-card .delete-overlay {
    opacity: 0.92;
    width: 40px;
    height: 40px;
  }
}
/* Album chips: taller rows and a bigger remove button for fingers */
@media (pointer: coarse) {
  .album-chip {
    min-height: 44px;
  }
}
.confirm-thumb {
  max-width: 100%;
  max-height: 60vh;
  display: block;
}
</style>
