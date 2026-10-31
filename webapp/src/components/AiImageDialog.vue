<script setup>
import { t } from "../i18n";
import { ref, computed, watch } from "vue";
import { useAppStore, useSettingsStore } from "../stores";

const props = defineProps({ modelValue: { type: Boolean, default: false } });
const emit = defineEmits(["update:modelValue", "generated"]);

const appStore = useAppStore();
const settingsStore = useSettingsStore();

const aiPrompt = ref("");
const aiProvider = ref(0);
const aiModel = ref("gpt-image-1.5");
const generating = ref(false);
const error = ref("");

const aiModelOptions = computed(() => {
  if (aiProvider.value === 0) {
    return [
      { title: "GPT Image 1.5", value: "gpt-image-1.5" },
      { title: "GPT Image 1", value: "gpt-image-1" },
      { title: "GPT Image 1 Mini", value: "gpt-image-1-mini" },
    ];
  }
  return [
    { title: "Gemini 3.1 Flash Image", value: "gemini-3.1-flash-image-preview" },
    { title: "Gemini 3 Pro Image", value: "gemini-3-pro-image-preview" },
    { title: "Gemini 2.5 Flash Image", value: "gemini-2.5-flash-image" },
  ];
});

const aiProviderOptions = computed(() => {
  const options = [];
  if (settingsStore.deviceSettings.aiCredentials.openaiApiKey) {
    options.push({ title: "OpenAI", value: 0 });
  }
  if (settingsStore.deviceSettings.aiCredentials.googleApiKey) {
    options.push({ title: "Google Gemini", value: 1 });
  }
  return options;
});

// Reset model to first option when provider changes
watch(aiProvider, (newProvider) => {
  aiModel.value = newProvider === 0 ? "gpt-image-1.5" : "gemini-3.1-flash-image-preview";
});

watch(
  () => props.modelValue,
  (open) => {
    if (!open) return;
    aiPrompt.value = "";
    error.value = "";
    // Default to OpenAI if a key exists, otherwise Google
    aiProvider.value = settingsStore.deviceSettings.aiCredentials.openaiApiKey ? 0 : 1;
    aiModel.value = aiProvider.value === 0 ? "gpt-image-1.5" : "gemini-3.1-flash-image-preview";
  }
);

async function generate() {
  generating.value = true;
  error.value = "";
  try {
    const provider = aiProvider.value;
    const apiKey =
      provider === 0
        ? settingsStore.deviceSettings.aiCredentials.openaiApiKey
        : settingsStore.deviceSettings.aiCredentials.googleApiKey;

    const isPortrait = settingsStore.deviceSettings.displayOrientation === "portrait";
    let src = null;

    if (provider === 0) {
      // OpenAI
      const isDalle3 = aiModel.value.includes("dall-e-3");
      const isDalle2 = aiModel.value.includes("dall-e-2");
      let size = "1024x1024";

      if (isDalle3) {
        size = isPortrait ? "1024x1792" : "1792x1024";
      } else if (isDalle2) {
        size = "1024x1024";
      } else {
        // GPT Image models (1.5, 1, etc) often support 1024x1536 (3:4) but not 1792 (16:9)
        size = isPortrait ? "1024x1536" : "1536x1024";
      }

      const body = {
        model: aiModel.value,
        prompt: aiPrompt.value,
        n: 1,
        size: size,
      };

      if (isDalle3) {
        // DALL-E 3: quality ("standard" or "hd"), style ("vivid" or "natural")
        body.quality = "hd";
        body.style = "vivid";
        body.response_format = "b64_json";
      } else if (isDalle2) {
        // DALL-E 2: no quality/style params, only response_format
        body.response_format = "b64_json";
      } else {
        // GPT Image models: quality ("low", "medium", "high"), output_format, output_compression
        // Note: GPT Image models return b64_json by default, no response_format needed
        body.quality = "high";
      }

      const response = await fetch("https://api.openai.com/v1/images/generations", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          Authorization: `Bearer ${apiKey}`,
        },
        body: JSON.stringify(body),
      });

      if (!response.ok) {
        const errorText = await response.text();
        throw new Error(`API Error: ${response.status} - ${errorText}`);
      }

      const data = await response.json();

      if (data.data?.[0]?.b64_json) {
        // All models return b64_json when requested (DALL-E) or by default (GPT Image)
        src = `data:image/png;base64,${data.data[0].b64_json}`;
      } else if (data.data?.[0]?.url) {
        // Fallback: handle URL response if returned
        const urlRes = await fetch(data.data[0].url);
        if (!urlRes.ok) throw new Error("Failed to download image from OpenAI URL");
        const blob = await urlRes.blob();
        src = URL.createObjectURL(blob);
      } else {
        throw new Error("Invalid response from AI API: missing image data");
      }
    } else {
      // Google Gemini
      // Build imageConfig - imageSize only supported by Gemini 3 Pro
      const imageConfig = {
        aspectRatio: isPortrait ? "3:4" : "4:3",
      };

      if (aiModel.value.includes("gemini-3")) {
        // Select imageSize based on display resolution
        // 1K (~1024px), 2K (~2048px), 4K (~4096px)
        const maxDim = Math.max(appStore.systemInfo.width, appStore.systemInfo.height);
        if (maxDim > 2048) {
          imageConfig.imageSize = "4K";
        } else if (maxDim > 1024) {
          imageConfig.imageSize = "2K";
        } else {
          imageConfig.imageSize = "1K";
        }
      }

      const response = await fetch(
        `https://generativelanguage.googleapis.com/v1beta/models/${aiModel.value}:generateContent?key=${apiKey}`,
        {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            contents: [{ parts: [{ text: aiPrompt.value }] }],
            generationConfig: {
              responseModalities: ["Image"],
              imageConfig: imageConfig,
            },
          }),
        }
      );

      if (!response.ok) {
        const errorText = await response.text();
        throw new Error(`API Error: ${response.status} - ${errorText}`);
      }

      const data = await response.json();
      const b64 = data.candidates?.[0]?.content?.parts?.[0]?.inlineData?.data;
      if (!b64) {
        throw new Error("Invalid response from Google API: missing inlineData");
      }
      src = `data:image/jpeg;base64,${b64}`;
    }

    const res = await fetch(src);
    const blob = await res.blob();
    const timestamp = new Date().toISOString().replace(/[:.]/g, "-");
    const file = new File([blob], `ai-generated-${timestamp}.jpg`, { type: "image/jpeg" });

    emit("update:modelValue", false);
    emit("generated", file);
  } catch (e) {
    error.value = t("Generation failed: {msg}", { msg: e.message });
  } finally {
    generating.value = false;
  }
}
</script>

<template>
  <v-dialog
    :model-value="modelValue"
    max-width="500"
    @update:model-value="emit('update:modelValue', $event)"
  >
    <v-card>
      <v-card-title>{{ t("Generate Image") }}</v-card-title>
      <v-card-text>
        <v-select
          v-model="aiProvider"
          :items="aiProviderOptions"
          item-title="title"
          item-value="value"
          :label="t('Provider')"
          variant="outlined"
          class="mb-4"
        />
        <v-select
          v-model="aiModel"
          :items="aiModelOptions"
          item-title="title"
          item-value="value"
          :label="t('Model')"
          variant="outlined"
          class="mb-4"
        />
        <v-textarea
          v-model="aiPrompt"
          :label="t('Prompt')"
          variant="outlined"
          rows="3"
          auto-grow
          :hint="t('Describe the image you want to generate')"
          persistent-hint
        />
        <v-alert v-if="error" type="error" variant="tonal" density="compact" class="mt-4">
          {{ error }}
        </v-alert>
      </v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="emit('update:modelValue', false)">{{ t("Cancel") }}</v-btn>
        <v-btn color="primary" :loading="generating" @click="generate">{{ t("Generate") }}</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>
