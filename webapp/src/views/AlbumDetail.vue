<script setup>
import { t } from "../i18n";
import { computed, ref } from "vue";
import { useAppStore } from "../stores";
import PhotoGrid from "../components/PhotoGrid.vue";
import PhotoViewer from "../components/PhotoViewer.vue";
import UploadButton from "../components/UploadButton.vue";
import { albumLabel } from "../utils/albums";
import { goTo } from "../utils/viewState";
import { selectAll } from "../utils/selection";

// One album: its photos, "+" that uploads here, and the album's own settings.
const props = defineProps({ name: { type: String, required: true } });

const appStore = useAppStore();
const album = computed(() => appStore.albums.find((a) => a.name === props.name) || null);
const images = computed(() => appStore.imagesByAlbum[props.name] || []);
const viewing = ref(-1);

const confirmDelete = ref(false);
const deleting = ref(false);
async function removeAlbum() {
  deleting.value = true;
  const ok = await appStore.deleteAlbum(props.name);
  deleting.value = false;
  confirmDelete.value = false;
  if (ok) goTo("albums");
}

function toggleSlideshow() {
  appStore.toggleAlbumEnabled(props.name, !album.value.enabled);
}
</script>

<template>
  <div>
    <div class="album-head">
      <v-btn
        icon="mdi-arrow-left"
        variant="text"
        :aria-label="t('All albums')"
        :title="t('All albums')"
        @click="goTo('albums')"
      />
      <div class="album-heading">
        <h1 class="album-title">{{ albumLabel(name) }}</h1>
        <div v-if="album" class="text-body-2 text-medium-emphasis">
          {{ t("{n} photos", { n: images.length }) }} ·
          {{ album.enabled ? t("In slideshow") : t("Not in slideshow") }}
        </div>
      </div>
      <v-spacer />
      <template v-if="album">
        <UploadButton :album="name" :label="t('Add photos')" class="d-none d-sm-flex" />
        <UploadButton :album="name" kind="icon" :label="t('Add photos')" class="d-sm-none" />
        <v-menu location="bottom end">
          <template #activator="{ props: menuProps }">
            <v-btn
              v-bind="menuProps"
              icon="mdi-dots-vertical"
              variant="text"
              :aria-label="t('More')"
            />
          </template>
          <v-list density="comfortable" min-width="220">
            <v-list-item @click="toggleSlideshow">
              <template #prepend>
                <v-icon
                  :icon="album.enabled ? 'mdi-checkbox-marked' : 'mdi-checkbox-blank-outline'"
                />
              </template>
              <v-list-item-title>{{ t("Include in the slideshow") }}</v-list-item-title>
            </v-list-item>
            <v-list-item
              v-if="images.length"
              prepend-icon="mdi-check-circle"
              :title="t('Select all')"
              @click="selectAll(images)"
            />
            <v-list-item
              v-if="name !== 'Default'"
              prepend-icon="mdi-delete-outline"
              :title="t('Delete album')"
              @click="confirmDelete = true"
            />
          </v-list>
        </v-menu>
      </template>
    </div>

    <div v-if="!album && !appStore.loading.albums" class="empty-state">
      <p class="text-h6">{{ t("This album no longer exists") }}</p>
      <v-btn variant="text" class="mt-2" @click="goTo('albums')">{{ t("All albums") }}</v-btn>
    </div>

    <div v-else-if="album && !images.length && !appStore.loading.allImages" class="empty-state">
      <v-icon icon="mdi-image-plus-outline" size="56" class="text-medium-emphasis" />
      <p class="text-h6 mt-4">{{ t("This album is empty") }}</p>
      <UploadButton :album="name" kind="large" :label="t('Add photos')" class="mt-4" />
    </div>

    <PhotoGrid v-else :images="images" @open="viewing = $event" />
    <PhotoViewer v-model="viewing" :images="images" />

    <v-dialog v-model="confirmDelete" max-width="400">
      <v-card>
        <v-card-title>{{ t("Delete Album?") }}</v-card-title>
        <v-card-text>
          {{
            t(
              'Are you sure you want to delete "{name}"? This will also delete all images in the album.',
              { name: albumLabel(name) }
            )
          }}
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="confirmDelete = false">{{ t("Cancel") }}</v-btn>
          <v-btn color="primary" :loading="deleting" @click="removeAlbum">{{ t("Delete") }}</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </div>
</template>

<style scoped>
.album-head {
  display: flex;
  align-items: center;
  gap: 8px;
  margin: -4px 0 20px -8px;
}
.album-heading {
  min-width: 0;
}
.album-title {
  font-size: 1.5rem;
  font-weight: 600;
  line-height: 1.3;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  text-align: center;
  padding: 80px 16px;
}
</style>
