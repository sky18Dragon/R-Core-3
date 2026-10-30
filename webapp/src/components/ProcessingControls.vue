<script setup>
import { t } from "../i18n";
import { computed, ref } from "vue";
import { useDisplay } from "vuetify";
import { getDitherOptions, getPresetOptions } from "@aitjcize/epaper-image-convert";
import { useAppStore } from "../stores";

const appStore = useAppStore();
const { xs } = useDisplay();

// Basic controls are always visible; the rest (dithering, tone mapping, S-curve)
// sit in a collapsed section so a quick tweak is one screen on a phone.
const advancedOpen = ref([]);

const props = defineProps({
  params: {
    type: Object,
    required: true,
  },
  preset: {
    type: String,
    default: "custom",
  },
});

const emit = defineEmits(["update:params", "update:preset", "preset-change"]);

const presetOptions = computed(() => [
  ...getPresetOptions().map((p) => ({ ...p, title: t(p.title) })),
  { value: "custom", title: t("Custom"), description: t("Manually adjusted parameters") },
]);

const ditherOptions = getDitherOptions();

const toneModeOptions = computed(() => [
  { title: t("Contrast"), value: "contrast" },
  { title: t("S-Curve"), value: "scurve" },
]);

const colorMethodOptions = [
  { title: "RGB", value: "rgb" },
  { title: "LAB", value: "lab" },
];

const scaleModeOptions = computed(() => [
  { title: t("Cover (crop to fill)"), value: "cover" },
  { title: t("Fit (letterbox)"), value: "fit" },
]);

const backgroundColorOptions = computed(() => [
  { title: t("White"), value: "white" },
  { title: t("Black"), value: "black" },
]);

const isScurveMode = computed(() => props.params.toneMode === "scurve");

function onPresetChange(value) {
  emit("update:preset", value);
  emit("preset-change", value);
}

function updateParam(key, value) {
  emit("update:params", { ...props.params, [key]: value });
}
</script>

<template>
  <div class="processing-controls">
    <!-- Preset Selection -->
    <v-row>
      <v-col cols="12">
        <v-card variant="outlined" class="mb-6">
          <v-card-subtitle class="pt-3"> {{ t("Processing Preset") }} </v-card-subtitle>
          <v-card-text>
            <v-select
              v-if="xs"
              :model-value="preset"
              :items="presetOptions"
              item-title="title"
              item-value="value"
              variant="outlined"
              density="compact"
              hide-details
              :aria-label="t('Processing Preset')"
              @update:model-value="onPresetChange"
            />
            <v-btn-toggle
              v-else
              :model-value="preset"
              mandatory
              color="primary"
              variant="outlined"
              @update:model-value="onPresetChange"
            >
              <v-btn v-for="p in presetOptions" :key="p.value" :value="p.value">
                {{ p.title }}
              </v-btn>
            </v-btn-toggle>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Layout: scale mode first; background only matters in fit mode -->
    <v-row>
      <v-col cols="12" md="4">
        <v-select
          :model-value="params.scaleMode || 'cover'"
          :items="scaleModeOptions"
          item-title="title"
          item-value="value"
          :label="t('Scale Mode')"
          variant="outlined"
          density="compact"
          @update:model-value="updateParam('scaleMode', $event)"
        />
      </v-col>

      <v-col v-if="(params.scaleMode || 'cover') === 'fit'" cols="12" md="4">
        <v-select
          :model-value="params.backgroundColor || 'white'"
          :items="backgroundColorOptions"
          item-title="title"
          item-value="value"
          :label="t('Background Color')"
          variant="outlined"
          density="compact"
          @update:model-value="updateParam('backgroundColor', $event)"
        />
      </v-col>
    </v-row>

    <v-row>
      <!-- Exposure -->
      <v-col cols="12" md="4">
        <v-slider
          :model-value="params.exposure"
          :min="0.5"
          :max="2.0"
          :step="0.01"
          :label="t('Exposure')"
          :hint="t('Brightness multiplier in linear light: below 1 darker, above 1 brighter.')"
          persistent-hint
          thumb-label
          color="primary"
          @update:model-value="updateParam('exposure', $event)"
        >
          <template #append>
            <span class="text-body-2">{{ params.exposure.toFixed(2) }}</span>
          </template>
        </v-slider>
      </v-col>

      <!-- Saturation (hidden on grayscale panels) -->
      <v-col v-if="!appStore.isGrayscale" cols="12" md="4">
        <v-slider
          :model-value="params.saturation"
          :min="0.5"
          :max="2.0"
          :step="0.01"
          :label="t('Saturation')"
          thumb-label
          color="primary"
          @update:model-value="updateParam('saturation', $event)"
        >
          <template #append>
            <span class="text-body-2">{{ params.saturation.toFixed(2) }}</span>
          </template>
        </v-slider>
      </v-col>

      <!-- Contrast (for contrast mode) -->
      <v-col v-if="!isScurveMode" cols="12" md="4">
        <v-slider
          :model-value="params.contrast"
          :min="0.5"
          :max="2.0"
          :step="0.01"
          :label="t('Contrast')"
          :hint="
            t('Stretches tones around mid-gray: above 1 deepens shadows and lifts highlights.')
          "
          persistent-hint
          thumb-label
          color="primary"
          @update:model-value="updateParam('contrast', $event)"
        >
          <template #append>
            <span class="text-body-2">{{ params.contrast.toFixed(2) }}</span>
          </template>
        </v-slider>
      </v-col>
    </v-row>

    <!-- Advanced: dithering, colour matching, tone mapping, S-curve -->
    <v-expansion-panels v-model="advancedOpen" multiple variant="accordion" elevation="0">
      <v-expansion-panel value="advanced">
        <v-expansion-panel-title>
          <v-icon icon="mdi-tune-vertical" class="mr-3" />
          {{ t("Advanced") }}
        </v-expansion-panel-title>
        <v-expansion-panel-text>
          <v-row>
            <!-- Dither Algorithm -->
            <v-col cols="12" md="4">
              <v-select
                :model-value="params.ditherAlgorithm"
                :items="ditherOptions"
                item-title="title"
                item-value="value"
                :label="t('Dithering Algorithm')"
                :hint="
                  t(
                    'How the leftover error between the image and the 16 grays is spread to neighbouring pixels.'
                  )
                "
                persistent-hint
                variant="outlined"
                density="compact"
                @update:model-value="updateParam('ditherAlgorithm', $event)"
              />
            </v-col>

            <v-col cols="12" md="4">
              <v-select
                :model-value="params.colorMethod"
                :items="colorMethodOptions"
                item-title="title"
                item-value="value"
                :label="t('Color Matching')"
                :hint="t('Nearest-gray choice: RGB distance or perceptual LAB distance.')"
                persistent-hint
                variant="outlined"
                density="compact"
                @update:model-value="updateParam('colorMethod', $event)"
              />
            </v-col>

            <!-- Compress Dynamic Range -->
            <v-col cols="12" md="4">
              <v-checkbox
                :model-value="params.compressDynamicRange"
                :label="t('Compress Dynamic Range')"
                :hint="
                  t(
                    'Squeeze the full image range into the panel\'s real black-to-white range: accurate but flatter. Off: deeper blacks.'
                  )
                "
                persistent-hint
                color="primary"
                @update:model-value="updateParam('compressDynamicRange', $event)"
              />
            </v-col>
          </v-row>

          <!-- Tone Mode -->
          <v-row>
            <v-col cols="12" md="4">
              <v-select
                :model-value="params.toneMode"
                :items="toneModeOptions"
                item-title="title"
                item-value="value"
                :label="t('Tone Mapping')"
                :hint="
                  t(
                    'Contrast: one slider around mid-gray. S-Curve: separate shadow/highlight control.'
                  )
                "
                persistent-hint
                variant="outlined"
                density="compact"
                @update:model-value="updateParam('toneMode', $event)"
              />
            </v-col>
          </v-row>

          <!-- S-Curve Controls -->
          <v-expand-transition>
            <v-card v-if="isScurveMode" variant="tonal" class="mt-4">
              <v-card-subtitle class="pt-3"> {{ t("S-Curve Parameters") }} </v-card-subtitle>
              <v-card-text>
                <v-row>
                  <v-col cols="12" md="6">
                    <v-slider
                      :model-value="params.strength"
                      :min="0"
                      :max="1"
                      :step="0.01"
                      :label="t('Strength')"
                      :hint="t('How strongly the S-curve is applied (0 = off).')"
                      persistent-hint
                      thumb-label
                      color="primary"
                      @update:model-value="updateParam('strength', $event)"
                    >
                      <template #append>
                        <span class="text-body-2">{{ params.strength.toFixed(2) }}</span>
                      </template>
                    </v-slider>
                  </v-col>

                  <v-col cols="12" md="6">
                    <v-slider
                      :model-value="params.shadowBoost"
                      :min="0"
                      :max="1"
                      :step="0.01"
                      :label="t('Shadow Boost')"
                      :hint="
                        t('Lifts the dark half of the curve; higher shows more shadow detail.')
                      "
                      persistent-hint
                      thumb-label
                      color="primary"
                      @update:model-value="updateParam('shadowBoost', $event)"
                    >
                      <template #append>
                        <span class="text-body-2">{{ params.shadowBoost.toFixed(2) }}</span>
                      </template>
                    </v-slider>
                  </v-col>

                  <v-col cols="12" md="6">
                    <v-slider
                      :model-value="params.highlightCompress"
                      :min="0.5"
                      :max="5"
                      :step="0.01"
                      :label="t('Highlight Compress')"
                      :hint="t('Rolls off bright tones; higher keeps more highlight detail.')"
                      persistent-hint
                      thumb-label
                      color="primary"
                      @update:model-value="updateParam('highlightCompress', $event)"
                    >
                      <template #append>
                        <span class="text-body-2">{{ params.highlightCompress.toFixed(2) }}</span>
                      </template>
                    </v-slider>
                  </v-col>

                  <v-col cols="12" md="6">
                    <v-slider
                      :model-value="params.midpoint"
                      :min="0.3"
                      :max="0.7"
                      :step="0.01"
                      :label="t('Midpoint')"
                      :hint="t('Where the curve pivots between shadows and highlights.')"
                      persistent-hint
                      thumb-label
                      color="primary"
                      @update:model-value="updateParam('midpoint', $event)"
                    >
                      <template #append>
                        <span class="text-body-2">{{ params.midpoint.toFixed(2) }}</span>
                      </template>
                    </v-slider>
                  </v-col>
                </v-row>
              </v-card-text>
            </v-card>
          </v-expand-transition>
        </v-expansion-panel-text>
      </v-expansion-panel>
    </v-expansion-panels>
  </div>
</template>
