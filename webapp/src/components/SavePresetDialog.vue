<script setup>
import { t } from "../i18n";
import { computed, ref, watch } from "vue";
import { usePresetsStore } from "../stores";
import { MAX_PRESETS, nameFits } from "../utils/presets";

// Keep a setting as a preset: a new one, or (when `updatable` names one) an
// update of that preset. Optionally made the default at the same time.
const props = defineProps({
  open: { type: Boolean, default: false },
  params: { type: Object, default: null }, // processing values to keep
  suggestedName: { type: String, default: "" },
  source: { type: String, default: "manual" }, // tuning | manual | copy
  updatable: { type: Object, default: null }, // {id, name}
});
const emit = defineEmits(["close", "saved"]);

const presets = usePresetsStore();
const mode = ref("new"); // "new" | "update"
const name = ref("");
const makeDefault = ref(false);
const failed = ref("");

watch(
  () => props.open,
  (open) => {
    if (!open) return;
    mode.value = props.updatable ? "update" : "new";
    name.value = props.suggestedName;
    makeDefault.value = false;
    failed.value = "";
  }
);

const full = computed(() => presets.list.length >= MAX_PRESETS);
const canSave = computed(() =>
  mode.value === "update" ? !!props.updatable : nameFits(name.value) && !full.value
);

async function save() {
  failed.value = "";
  let id = null;
  if (mode.value === "update") {
    const ok = await presets.update(props.updatable.id, { params: props.params });
    id = ok ? props.updatable.id : null;
    if (ok && makeDefault.value) await presets.setDefault(id);
  } else {
    id = await presets.create({
      name: name.value,
      params: props.params,
      source: props.source,
      makeDefault: makeDefault.value,
    });
  }
  if (!id) {
    failed.value = t("Could not save the preset on the frame");
    return;
  }
  emit("saved", id);
}
</script>

<template>
  <v-dialog
    :model-value="open"
    max-width="420"
    @update:model-value="(value) => !value && emit('close')"
  >
    <v-card v-if="open">
      <v-card-title>{{ t("Save as a preset") }}</v-card-title>
      <v-card-text>
        <v-radio-group v-if="updatable" v-model="mode" hide-details class="mb-2">
          <v-radio value="update" :label="t('Update “{name}”', { name: updatable.name })" />
          <v-radio value="new" :label="t('Save as a new preset')" />
        </v-radio-group>
        <v-text-field
          v-if="mode === 'new'"
          v-model="name"
          :label="t('Preset name')"
          :placeholder="t('e.g. Portrait, Landscape, Night')"
          variant="outlined"
          density="comfortable"
          autofocus
          counter="20"
          :error-messages="name && !nameFits(name) ? t('Name too long') : ''"
          @keyup.enter="canSave && save()"
        />
        <v-checkbox
          v-model="makeDefault"
          :label="t('Also make it the default')"
          density="compact"
          hide-details
        />
        <v-alert v-if="full && mode === 'new'" type="info" variant="tonal" density="compact">
          {{ t("You have 20 presets, the most the frame keeps. Delete one first.") }}
        </v-alert>
        <v-alert v-if="failed" type="error" variant="tonal" density="compact" class="mt-2">
          {{ failed }}
        </v-alert>
      </v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="emit('close')">{{ t("Cancel") }}</v-btn>
        <v-btn
          color="primary"
          variant="flat"
          :loading="presets.saving"
          :disabled="!canSave"
          @click="save"
        >
          {{ t("Save") }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>
