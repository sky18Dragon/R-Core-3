<script setup>
import { t } from "../i18n";
import { ref, computed, onMounted } from "vue";
import { useAppStore } from "../stores";

const appStore = useAppStore();

// Shown only on the opt-in Read Pico sleep build (/api/power exists there).
const power = ref(null);
const saving = ref(false);
const error = ref("");

const supplyText = computed(() => {
  switch (power.value?.supply) {
    case "battery":
      return t("Battery");
    case "external":
      return t("Charging / external power");
    default:
      return t("Unknown");
  }
});

async function load() {
  try {
    const response = await fetch("/api/power");
    if (response.ok) power.value = await response.json();
  } catch (e) {
    console.error("Failed to load power status:", e);
  }
}

async function setEnabled(enabled) {
  saving.value = true;
  error.value = "";
  try {
    const response = await fetch("/api/power", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ frame_sleep_enabled: enabled }),
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    power.value = await response.json();
  } catch (e) {
    error.value = t("Failed to save: ") + e.message;
    await load();
  } finally {
    saving.value = false;
  }
}

// Put the frame to sleep right now (only offered here, where the sleep build's
// /api/power exists). The page becomes unreachable until the wake key is pressed.
const sleepDialog = ref(false);
async function handleSleep() {
  await appStore.enterSleep();
  sleepDialog.value = false;
}

onMounted(load);
</script>

<template>
  <v-card v-if="power" variant="outlined">
    <v-card-title class="d-flex align-center">
      <v-icon icon="mdi-battery-clock" class="mr-2" />
      {{ t("Battery sleep") }}
    </v-card-title>
    <v-card-text>
      <p class="text-body-2 text-grey mb-4">
        {{
          t(
            "On battery the frame sleeps between rotations and wakes to show the next image. While asleep the web page is unreachable; press the power key to wake it. On USB power it stays awake."
          )
        }}
      </p>
      <v-switch
        :model-value="power.frame_sleep_enabled"
        :loading="saving"
        :label="t('Sleep between rotations on battery')"
        color="primary"
        hide-details
        @update:model-value="setEnabled"
      />
      <div class="text-body-2 mt-2">
        {{ t("Power source") }}: {{ supplyText }}
        <span v-if="power.battery_percent >= 0"> · {{ power.battery_percent }}%</span>
        <span v-if="power.battery_mv > 0"> · {{ power.battery_mv }} mV</span>
      </div>
      <div class="text-caption text-grey">{{ t("Last decision") }}: {{ power.last_decision }}</div>
      <v-alert v-if="error" type="error" variant="tonal" density="compact" class="mt-2">{{
        error
      }}</v-alert>
      <v-btn color="warning" variant="outlined" class="mt-4" @click="sleepDialog = true">
        <v-icon icon="mdi-sleep" start />
        {{ t("Enter Deep Sleep") }}
      </v-btn>
    </v-card-text>

    <v-dialog v-model="sleepDialog" max-width="400">
      <v-card>
        <v-card-title>{{ t("Enter Deep Sleep?") }}</v-card-title>
        <v-card-text>
          {{ t("The device will enter deep sleep mode and stop responding until woken up.") }}
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="sleepDialog = false">{{ t("Cancel") }}</v-btn>
          <v-btn color="primary" @click="handleSleep">{{ t("Sleep") }}</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-card>
</template>
