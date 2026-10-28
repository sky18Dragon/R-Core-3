<script setup>
import { t } from "../i18n";
import { ref, computed, onMounted, onUnmounted } from "vue";
import { HEADER_BYTES, inspectFirmware } from "../utils/otaLocal.js";

const currentVersion = ref(t("Loading..."));
const latestVersion = ref("-");
const otaState = ref("idle");
const progress = ref(0);
const errorMessage = ref("");

let statusPollInterval = null;

const updateAvailable = computed(() => otaState.value === "update_available");
const checking = computed(() => otaState.value === "checking");
const installing = computed(
  () => otaState.value === "downloading" || otaState.value === "installing"
);

const statusMessage = computed(() => {
  switch (otaState.value) {
    case "idle":
      return "";
    case "checking":
      return t("Checking for updates...");
    case "update_available":
      return t("Update available: {version}", { version: latestVersion.value });
    case "downloading":
      return t("Downloading firmware...");
    case "installing":
      return t("Installing firmware...");
    case "success":
      return t("Update successful! Device will reboot...");
    case "error":
      return errorMessage.value || t("Update failed");
    case "up_to_date":
      return t("You're running the latest version.");
    default:
      return "";
  }
});

const statusType = computed(() => {
  switch (otaState.value) {
    case "update_available":
    case "success":
      return "success";
    case "error":
      return "error";
    case "checking":
    case "downloading":
    case "installing":
      return "info";
    default:
      return "info";
  }
});

async function loadOTAStatus() {
  try {
    const response = await fetch("/api/ota/status");
    if (!response.ok) return;

    const data = await response.json();

    currentVersion.value = data.current_version || t("Unknown");
    latestVersion.value = data.latest_version || "-";
    otaState.value = data.state || "idle";
    progress.value = data.progress_percent || 0;
    errorMessage.value = data.error_message || "";

    // If state is idle and we were checking, it means check completed with no update
    // The state should transition appropriately based on the backend response
  } catch (error) {
    console.error("Failed to load OTA status:", error);
  }
}

async function checkForUpdate() {
  try {
    const response = await fetch("/api/ota/check", { method: "POST" });
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    // Start polling for status updates
    startStatusPolling();

    // Immediately reload status
    await loadOTAStatus();
  } catch (error) {
    errorMessage.value = t("Failed to check for updates: ") + error.message;
    otaState.value = "error";
  }
}

async function installUpdate() {
  try {
    const response = await fetch("/api/ota/update", { method: "POST" });
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    // Start polling for status updates
    startStatusPolling();

    // Immediately reload status
    await loadOTAStatus();
  } catch (error) {
    errorMessage.value = t("Failed to install update: ") + error.message;
    otaState.value = "error";
  }
}

function startStatusPolling() {
  // Clear any existing interval
  stopStatusPolling();

  // Poll every second
  statusPollInterval = setInterval(async () => {
    await loadOTAStatus();

    // Stop polling if we're in a terminal state
    if (
      otaState.value === "idle" ||
      otaState.value === "update_available" ||
      otaState.value === "success" ||
      otaState.value === "error" ||
      otaState.value === "up_to_date"
    ) {
      stopStatusPolling();
    }
  }, 1000);
}

function stopStatusPolling() {
  if (statusPollInterval) {
    clearInterval(statusPollInterval);
    statusPollInterval = null;
  }
}

// Read Pico local frame: firmware is uploaded from this page instead of
// downloaded from GitHub. /api/ota/local exists only on that build.
const local = ref(null);
const firmwareFile = ref(null);
const localBusy = ref(false);
const localMessage = ref("");
const localType = ref("info");

async function loadLocalStatus() {
  try {
    const response = await fetch("/api/ota/local");
    if (response.ok) local.value = await response.json();
  } catch (error) {
    console.error("Failed to load local OTA status:", error);
  }
}

async function uploadFirmware() {
  const file = Array.isArray(firmwareFile.value) ? firmwareFile.value[0] : firmwareFile.value;
  if (!file) return;
  const head = new Uint8Array(await file.slice(0, HEADER_BYTES).arrayBuffer());
  const check = inspectFirmware(head, file.size, local.value?.slot_size);
  if (!check.ok) {
    localType.value = "error";
    localMessage.value = t(check.reason);
    return;
  }
  if (!window.confirm(t("Install firmware {version}? The frame restarts afterwards.", { version: check.version })))
    return;
  localBusy.value = true;
  localType.value = "info";
  localMessage.value = t("Uploading firmware...");
  try {
    const response = await fetch("/api/ota/upload", {
      method: "POST",
      headers: { "Content-Type": "application/octet-stream" },
      body: file,
    });
    const data = await response.json().catch(() => ({}));
    if (!response.ok || data.status !== "success") throw new Error(data.message || `HTTP ${response.status}`);
    localType.value = "success";
    localMessage.value = t("Firmware installed. The frame is restarting; reload this page in a minute.");
  } catch (error) {
    localType.value = "error";
    localMessage.value = t("Firmware upload failed: ") + error.message;
  } finally {
    localBusy.value = false;
  }
}

onMounted(() => {
  loadOTAStatus();
  loadLocalStatus();
});

onUnmounted(() => {
  stopStatusPolling();
});
</script>

<template>
  <v-card style="overflow: visible">
    <v-card-title class="d-flex align-center">
      <v-icon icon="mdi-cellphone-arrow-down" class="mr-2" />
      {{ t("Firmware Update (OTA)") }}
    </v-card-title>

    <v-card-text v-if="local">
      <p class="text-body-2 text-grey mb-4">
        {{ t("Upload a Read Pico local-frame firmware file (.bin). The factory firmware stays as a fallback.") }}
      </p>
      <v-list-item class="px-0 mb-2">
        <v-list-item-title class="text-body-2 text-grey">{{ t("Current Version") }}</v-list-item-title>
        <v-list-item-subtitle class="text-h6">{{ local.version }} ({{ local.running }})</v-list-item-subtitle>
      </v-list-item>
      <v-alert v-if="!local.supported" type="warning" variant="tonal" density="compact">
        {{ t("This device does not have the update partitions yet; install them over USB first.") }}
      </v-alert>
      <div v-else class="d-flex flex-wrap align-center gap-2">
        <v-file-input
          v-model="firmwareFile"
          accept=".bin"
          :label="t('Firmware file')"
          density="compact"
          hide-details
          prepend-icon="mdi-chip"
          style="min-width: 260px"
          :disabled="localBusy"
        />
        <v-btn color="primary" :loading="localBusy" :disabled="!firmwareFile" @click="uploadFirmware">
          <v-icon icon="mdi-upload" start />
          {{ t("Upload and install") }}
        </v-btn>
      </div>
      <v-alert v-if="localMessage" :type="localType" variant="tonal" class="mt-4" density="compact">
        {{ localMessage }}
      </v-alert>
    </v-card-text>

    <v-card-text v-else>
      <p class="text-body-2 text-grey mb-4">
        {{ t("Check for and install firmware updates from GitHub releases.") }}
      </p>

      <v-row align="center">
        <v-col cols="12" md="4">
          <v-list-item>
            <v-list-item-title class="text-body-2 text-grey">
              {{ t("Current Version") }}
            </v-list-item-title>
            <v-list-item-subtitle class="text-h6">
              {{ currentVersion }}
            </v-list-item-subtitle>
          </v-list-item>
        </v-col>

        <v-col cols="12" md="4">
          <v-list-item>
            <v-list-item-title class="text-body-2 text-grey">
              {{ t("Latest Version") }}
            </v-list-item-title>
            <v-list-item-subtitle class="text-h6">
              <span :class="{ 'text-success': updateAvailable }">
                {{ latestVersion }}
              </span>
            </v-list-item-subtitle>
          </v-list-item>
        </v-col>

        <v-col cols="12" md="4" class="d-flex flex-wrap justify-end gap-2">
          <v-btn variant="outlined" :loading="checking" class="mr-2" @click="checkForUpdate">
            <v-icon icon="mdi-refresh" start />
            {{ t("Check") }}
          </v-btn>

          <v-btn
            v-if="updateAvailable"
            color="primary"
            :loading="installing"
            @click="installUpdate"
          >
            <v-icon icon="mdi-download" start />
            {{ t("Install") }}
          </v-btn>
        </v-col>
      </v-row>

      <!-- Progress Bar -->
      <v-expand-transition>
        <div v-if="installing" class="mt-4">
          <v-progress-linear :model-value="progress" color="primary" height="8" rounded />
          <p class="text-body-2 text-center mt-2">{{ progress }}%</p>
        </div>
      </v-expand-transition>

      <!-- Status Message -->
      <v-alert
        v-if="statusMessage"
        :type="statusType"
        variant="tonal"
        class="mt-4"
        density="compact"
      >
        {{ statusMessage }}
      </v-alert>
    </v-card-text>
  </v-card>
</template>
