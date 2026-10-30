<script setup>
import { t } from "../i18n";
import { ref, watch } from "vue";
import { useAppStore } from "../stores";

const props = defineProps({ modelValue: { type: Boolean, default: false } });
const emit = defineEmits(["update:modelValue", "created"]);

const appStore = useAppStore();
const name = ref("");
const saving = ref(false);
const failed = ref(false);

watch(
  () => props.modelValue,
  (open) => {
    if (open) {
      name.value = "";
      failed.value = false;
    }
  }
);

async function create() {
  const trimmed = name.value.trim();
  if (!trimmed || saving.value) return;
  saving.value = true;
  failed.value = false;
  const ok = await appStore.createAlbum(trimmed);
  saving.value = false;
  if (ok) {
    emit("update:modelValue", false);
    emit("created", trimmed);
  } else {
    failed.value = true;
  }
}
</script>

<template>
  <v-dialog
    :model-value="modelValue"
    max-width="400"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <v-card>
      <v-card-title>{{ t("Create New Album") }}</v-card-title>
      <v-card-text>
        <v-text-field
          v-model="name"
          :label="t('Album Name')"
          autofocus
          :error-messages="
            failed ? [t('Could not create the album. The name may already exist.')] : []
          "
          @keyup.enter="create"
        />
      </v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="emit('update:modelValue', false)">{{ t("Cancel") }}</v-btn>
        <v-btn color="primary" :loading="saving" :disabled="!name.trim()" @click="create">
          {{ t("Create") }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>
