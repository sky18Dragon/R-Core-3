<script setup>
import { t } from "../i18n";
import { ref, computed, watch, onMounted, onUnmounted } from "vue";
import { useSettingsStore, useAppStore } from "../stores";
import PaletteCalibration from "./PaletteCalibration.vue";
import GrayscaleCalibration from "./GrayscaleCalibration.vue";
import BatterySleep from "./BatterySleep.vue";
import LastCrash from "./LastCrash.vue";
import { TIMEZONES } from "../data/timezones";
import {
  APPROXIMATE_ZONES,
  CUSTOM_ZONE,
  FIXED_OFFSET_ZONE,
  TIMEZONE_MAX_BYTES,
  browserTimeZone,
  fixedOffsetLabel,
  ruleForZone,
  validateTimezone,
  zoneForRule,
} from "../utils/timezone";
import { useDisplay } from "vuetify";
import { locale, setLocale } from "../i18n";

const settingsStore = useSettingsStore();
const appStore = useAppStore();

// Time zone picker. The store holds only the POSIX rule the device applies;
// the IANA name shown here is derived from it and never leaves the browser.
const browserZone = browserTimeZone();
const browserZoneKnown = ruleForZone(browserZone) !== null;
const showAdvancedTz = ref(false);

const timezoneRule = computed(() => settingsStore.deviceSettings.timezone);
// Only an edited rule is checked: a value an older firmware let through must
// not block saving unrelated settings (the store sends changed fields only).
const timezoneError = computed(() =>
  timezoneRule.value === settingsStore.savedTimezone ? "" : validateTimezone(timezoneRule.value)
);
const approximateNote = computed(() => APPROXIMATE_ZONES[selectedZone.value] ?? "");

const selectedZone = computed({
  get: () => zoneForRule(timezoneRule.value, browserZone),
  set: (name) => {
    // The two sentinel entries stand for the rule already stored, and
    // clearing the field (null) is not a choice either.
    const rule = ruleForZone(name);
    if (rule !== null) settingsStore.deviceSettings.timezone = rule;
  },
});

const timezoneItems = computed(() => {
  const items = Object.keys(TIMEZONES).map((name) => ({ title: name, value: name }));
  if (selectedZone.value === FIXED_OFFSET_ZONE) {
    const label = t("Fixed offset {offset} (no DST)", {
      offset: fixedOffsetLabel(timezoneRule.value),
    });
    items.unshift({ title: label, value: FIXED_OFFSET_ZONE });
  } else if (selectedZone.value === CUSTOM_ZONE) {
    items.unshift({ title: t("Custom rule"), value: CUSTOM_ZONE });
  }
  return items;
});

// A rule no zone in the table produces can only be edited as text, so open
// the field for it. It stays open (even if typing passes through a rule that
// maps to a zone) until the user collapses it.
watch(
  selectedZone,
  (zone) => {
    if (zone === CUSTOM_ZONE) showAdvancedTz.value = true;
  },
  { immediate: true }
);

const saveBlocker = computed(() => {
  if (timezoneError.value) return t("Fix the time zone rule first");
  return "";
});

// Device time. The device reports its local wall-clock time as text; tick it
// forward from there instead of re-deriving local time from the TZ rule,
// which would need a POSIX DST evaluator in the browser.
const deviceTime = ref("");
const syncingTime = ref(false);
let deviceLocalMs = null; // device wall-clock time, parsed as if it were UTC
let receivedAt = 0; // Date.now() when it was reported
let lastTickHour = null; // hour of the last tick, null right after a report
let tickInterval = null;

function updateDisplayTime() {
  if (deviceLocalMs === null) return;
  const now = new Date(deviceLocalMs + (Date.now() - receivedAt));
  // toISOString() prints in UTC, i.e. the wall-clock numbers we stored
  deviceTime.value = now.toISOString().slice(0, 19).replace("T", " ");
  // A DST rule moves the device's clock on an hour boundary, which ticking
  // forward can't reproduce, so ask the device again whenever the hour rolls
  // over. That also corrects any drift.
  const hour = now.getUTCHours();
  if (lastTickHour !== null && hour !== lastTickHour) fetchDeviceTime();
  lastTickHour = hour;
}

function setDeviceTime(data) {
  // "YYYY-MM-DD HH:MM:SS" in the device's zone
  const wallClock = Date.parse(`${String(data.time ?? "").replace(" ", "T")}Z`);
  deviceLocalMs = Number.isFinite(wallClock) ? wallClock : Number(data.timestamp) * 1000;
  receivedAt = Date.now();
  lastTickHour = null; // a report is authoritative, not a rollover
  updateDisplayTime();
}

async function fetchDeviceTime() {
  try {
    const response = await fetch("/api/time");
    if (response.ok) {
      setDeviceTime(await response.json());
    }
  } catch (error) {
    console.error("Failed to fetch device time:", error);
  }
}

async function syncTime() {
  syncingTime.value = true;
  try {
    const response = await fetch("/api/time/sync", { method: "POST" });
    if (response.ok) {
      const data = await response.json();
      if (data.status === "success") {
        setDeviceTime(data);
      }
    }
  } catch (error) {
    console.error("Failed to sync time:", error);
  } finally {
    syncingTime.value = false;
  }
}

onMounted(() => {
  fetchDeviceTime();
  // Tick every second to update display
  tickInterval = setInterval(updateDisplayTime, 1000);
});

onUnmounted(() => {
  if (tickInterval) {
    clearInterval(tickInterval);
  }
});

const orientationOptions = computed(() => {
  const width = appStore.systemInfo.width || 800;
  const height = appStore.systemInfo.height || 480;
  const maxDim = Math.max(width, height);
  const minDim = Math.min(width, height);

  return [
    { title: t("Landscape ({w}×{h})", { w: maxDim, h: minDim }), value: "landscape" },
    { title: t("Portrait ({w}×{h})", { w: minDim, h: maxDim }), value: "portrait" },
  ];
});

// 90/270 would swap the panel's logical dimensions, which the streaming
// pipeline and dimensionless .epdgz payloads can't represent; portrait
// mounting is handled by the orientation setting instead
const rotationOptions = [
  { title: "0°", value: 0 },
  { title: "180°", value: 180 },
];

const rotationModeOptions = computed(() => {
  const options = [{ title: t("URL - Fetch image from URL"), value: "url" }];
  if (appStore.systemInfo.sdcard_inserted || appStore.systemInfo.has_flash_storage) {
    options.unshift({ title: t("Storage - Rotate through images"), value: "storage" });
  }
  return options;
});

const saving = ref(false);
const saveSuccess = ref(false);

const saveMessage = ref("");
const saveError = ref(false);

// Format the storage card: the one action that erases every photo, so it lives
// here (not next to the upload button) and asks for a typed confirmation.
const showFormatDialog = ref(false);
const formatConfirm = ref("");
const formatting = ref(false);
const hasStorage = computed(
  () => appStore.systemInfo.sdcard_inserted || appStore.systemInfo.has_flash_storage
);
const photoCount = computed(() =>
  appStore.albums.reduce((sum, a) => sum + (Number.isFinite(a.image_count) ? a.image_count : 0), 0)
);
const formatWord = computed(() => t("FORMAT"));

function openFormatDialog() {
  formatConfirm.value = "";
  showFormatDialog.value = true;
}

async function performFormat() {
  formatting.value = true;
  try {
    const response = await fetch("/api/format-storage", { method: "POST" });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    saveSuccess.value = true;
    saveError.value = false;
    saveMessage.value = t("Storage formatted successfully");
    setTimeout(() => (saveSuccess.value = false), 4000);
    await appStore.loadSystemInfo();
    await appStore.loadAlbums();
    appStore.selectAlbum("Default");
  } catch (error) {
    saveError.value = true;
    saveSuccess.value = false;
    saveMessage.value = t("Format failed: {msg}", { msg: error.message });
    setTimeout(() => (saveError.value = false), 6000);
  } finally {
    formatting.value = false;
    showFormatDialog.value = false;
  }
}

const aboutRows = computed(() => {
  const info = appStore.systemInfo;
  return [
    [t("Firmware"), info.project_name],
    [t("Build"), [info.version, info.compile_date, info.compile_time].filter(Boolean).join(" ")],
    ["ESP-IDF", info.idf_version],
    [t("Board"), info.board_name],
    [t("Device ID"), info.device_id],
    [t("Screen"), info.width && info.height ? `${info.width}×${info.height}` : ""],
  ].filter(([, value]) => value);
});

const showFactoryResetDialog = ref(false);
const resetting = ref(false);
const showImportDialog = ref(false);
const importData = ref(null);
const importFileName = ref("");

async function exportConfig() {
  try {
    const [configRes, processingRes, paletteRes] = await Promise.all([
      fetch("/api/config"),
      fetch("/api/settings/processing"),
      fetch("/api/settings/palette"),
    ]);

    const exported = {};

    if (configRes.ok) {
      const config = await configRes.json();
      // Remove sensitive fields
      delete config.wifi_password;
      exported.config = config;
    }
    if (processingRes.ok) exported.processing = await processingRes.json();
    if (paletteRes.ok) exported.palette = await paletteRes.json();

    const blob = new Blob([JSON.stringify(exported, null, 2)], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    const deviceName = settingsStore.deviceSettings.deviceName || "photoframe";
    a.download = `${deviceName.toLowerCase().replace(/\s+/g, "-")}-config.json`;
    a.click();
    URL.revokeObjectURL(url);
  } catch (error) {
    console.error("Failed to export config:", error);
  }
}

const downloadingLog = ref(false);

async function downloadDebugLog() {
  downloadingLog.value = true;
  try {
    const response = await fetch("/api/debug/log");
    if (!response.ok) {
      saveError.value = true;
      saveMessage.value = t("No debug logs available");
      setTimeout(() => (saveError.value = false), 5000);
      return;
    }
    const blob = await response.blob();
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    const deviceName = settingsStore.deviceSettings.deviceName || "photoframe";
    a.download = `${deviceName.toLowerCase().replace(/\s+/g, "-")}-debug.log`;
    a.click();
    URL.revokeObjectURL(url);
  } catch (error) {
    console.error("Failed to download debug log:", error);
    saveError.value = true;
    saveMessage.value = t("Failed to download debug logs");
    setTimeout(() => (saveError.value = false), 5000);
  } finally {
    downloadingLog.value = false;
  }
}

const clearingLog = ref(false);

async function clearDebugLog() {
  clearingLog.value = true;
  try {
    const response = await fetch("/api/debug/log", { method: "DELETE" });
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }
    saveSuccess.value = true;
    saveMessage.value = t("Debug logs cleared");
    setTimeout(() => (saveSuccess.value = false), 3000);
  } catch (error) {
    console.error("Failed to clear debug logs:", error);
    saveError.value = true;
    saveMessage.value = t("Failed to clear debug logs");
    setTimeout(() => (saveError.value = false), 5000);
  } finally {
    clearingLog.value = false;
  }
}

function onImportFileSelected(event) {
  const file = event.target.files?.[0];
  if (!file) return;

  importFileName.value = file.name;
  const reader = new FileReader();
  reader.onload = (e) => {
    try {
      importData.value = JSON.parse(e.target.result);
      showImportDialog.value = true;
    } catch {
      saveError.value = true;
      saveMessage.value = t("Invalid JSON file");
      setTimeout(() => (saveError.value = false), 5000);
    }
  };
  reader.readAsText(file);
  // Reset input so the same file can be selected again
  event.target.value = "";
}

async function performImport() {
  if (!importData.value) return;

  showImportDialog.value = false;
  saving.value = true;

  try {
    // One at a time, config last: a config that sets the device password
    // turns authentication on, and any request still in flight without
    // credentials would then be refused with a 401.
    const requests = [];
    if (importData.value.processing) {
      requests.push(["/api/settings/processing", "POST", importData.value.processing]);
    }
    if (importData.value.palette) {
      requests.push(["/api/settings/palette", "POST", importData.value.palette]);
    }
    if (importData.value.config) {
      requests.push(["/api/config", "PATCH", importData.value.config]);
    }
    for (const [url, method, body] of requests) {
      const response = await fetch(url, {
        method,
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body),
      });
      if (!response.ok) {
        throw new Error(`${method} ${url} failed with HTTP ${response.status}`);
      }
    }

    // Reload all settings from device
    await Promise.all([
      settingsStore.loadDeviceSettings(),
      settingsStore.loadSettings(),
      settingsStore.loadPalette(),
    ]);

    // The device password is write-only: an export records only whether one
    // was set, and http_auth_enabled is informational to the firmware. So an
    // import can neither restore a password nor, deliberately, drop one --
    // silently opening a protected frame is the worse surprise. Compare what
    // the file says against what the device reports now and say so if they
    // differ, rather than claim the import reproduced the exported state.
    const importedAuth = importData.value.config?.http_auth_enabled;
    const deviceAuth = settingsStore.deviceSettings.httpAuthEnabled;
    let authNote = "";
    if (typeof importedAuth === "boolean" && importedAuth !== deviceAuth) {
      authNote = importedAuth
        ? " " +
          t(
            "Exports never include the device password: set it again under Device & network → Advanced network settings to require one."
          )
        : " " +
          t(
            "The device password was left in place: turn it off under Device & network → Advanced network settings if you want the frame open."
          );
    }

    saveSuccess.value = true;
    saveError.value = false;
    saveMessage.value = authNote
      ? t("Config imported.") + authNote
      : t("Config imported successfully!");
    setTimeout(() => (saveSuccess.value = false), authNote ? 10000 : 3000);
  } catch (error) {
    console.error("Failed to import config:", error);
    saveError.value = true;
    saveMessage.value = t("Failed to import config");
    setTimeout(() => (saveError.value = false), 5000);
  } finally {
    saving.value = false;
    importData.value = null;
  }
}

async function saveSettings() {
  saving.value = true;
  const deviceResult = await settingsStore.saveDeviceSettings("system");
  saving.value = false;

  if (deviceResult.success) {
    saveSuccess.value = true;
    saveError.value = false;
    saveMessage.value = deviceResult.message || t("Settings saved!");
    setTimeout(() => (saveSuccess.value = false), 3000);

    // Refresh device time in case timezone changed
    await fetchDeviceTime();
  } else {
    saveError.value = true;
    saveSuccess.value = false;
    saveMessage.value = deviceResult.message || t("Failed to save settings");
    setTimeout(() => (saveError.value = false), 5000);
  }
}

async function performFactoryReset() {
  resetting.value = true;
  const result = await settingsStore.factoryReset();
  resetting.value = false;
  showFactoryResetDialog.value = false;

  if (result.success) {
    saveSuccess.value = true;
    saveError.value = false;
    saveMessage.value = result.message;
    setTimeout(() => (saveSuccess.value = false), 3000);
  } else {
    saveError.value = true;
    saveSuccess.value = false;
    saveMessage.value = result.message;
    setTimeout(() => (saveError.value = false), 5000);
  }
}
// Drawer: full width on phones, a side sheet otherwise.
const display = useDisplay();
const drawerWidth = computed(() => (display.xs.value ? display.width.value : 520));

// Closing with unsaved edits asks first; the edits live in the shared store, so
// discarding means reloading them from the device (image-panel edits are kept).
const showDiscardDialog = ref(false);

function requestClose() {
  if (settingsStore.systemDirty) {
    showDiscardDialog.value = true;
  } else {
    settingsStore.systemSettingsOpen = false;
  }
}

async function discardAndClose() {
  showDiscardDialog.value = false;
  await settingsStore.discardSystemEdits();
  settingsStore.systemSettingsOpen = false;
}

function onDrawerToggle(open) {
  if (open) settingsStore.systemSettingsOpen = true;
  else requestClose();
}
</script>

<template>
  <v-navigation-drawer
    :model-value="settingsStore.systemSettingsOpen"
    location="right"
    temporary
    :width="drawerWidth"
    @update:model-value="onDrawerToggle"
  >
    <div class="d-flex flex-column fill-height">
      <div class="d-flex align-center px-4 py-3">
        <v-icon icon="mdi-cog" class="mr-2" />
        <span class="text-h6">{{ t("System settings") }}</span>
        <v-spacer />
        <v-btn-toggle
          :model-value="locale"
          mandatory
          density="compact"
          variant="outlined"
          divided
          class="mr-2"
          @update:model-value="setLocale"
        >
          <v-btn value="zh" size="small" :aria-label="t('Switch language')">中文</v-btn>
          <v-btn value="en" size="small" :aria-label="t('Switch language')">EN</v-btn>
        </v-btn-toggle>
        <v-btn icon="mdi-close" variant="text" :aria-label="t('Close')" @click="requestClose" />
      </div>
      <v-divider />

      <div class="flex-grow-1 overflow-y-auto">
        <v-expansion-panels v-model="settingsStore.systemSection" variant="accordion" elevation="0">
          <v-expansion-panel value="device">
            <v-expansion-panel-title>
              <v-icon icon="mdi-wifi" class="mr-3" />
              {{ t("Device & network") }}
            </v-expansion-panel-title>
            <v-expansion-panel-text>
              <v-row class="mt-2">
                <v-col cols="12">
                  <v-text-field
                    v-model="settingsStore.deviceSettings.deviceName"
                    :label="t('Device Name')"
                    variant="outlined"
                    :hint="
                      t(
                        'Used for mDNS hostname (e.g., \'Living Room Frame\' → living-room-frame.local)'
                      )
                    "
                    persistent-hint
                  />
                </v-col>
              </v-row>
              <v-row>
                <v-col cols="12">
                  <v-text-field
                    v-model="settingsStore.deviceSettings.wifiSsid"
                    :label="t('WiFi SSID')"
                    variant="outlined"
                    :hint="t('Network name to connect to')"
                    persistent-hint
                  />
                </v-col>
                <v-col cols="12">
                  <v-text-field
                    v-model="settingsStore.deviceSettings.wifiPassword"
                    :label="t('WiFi Password')"
                    type="password"
                    variant="outlined"
                    :hint="t('Leave empty to keep current password')"
                    persistent-hint
                    placeholder="••••••••"
                  />
                </v-col>
              </v-row>
              <v-expansion-panels class="mt-2" variant="accordion">
                <v-expansion-panel :title="t('Advanced network settings')" elevation="0">
                  <v-expansion-panel-text>
                    <v-row>
                      <v-col cols="12">
                        <v-text-field
                          v-model="settingsStore.deviceSettings.ntpServer"
                          :label="t('NTP Server')"
                          variant="outlined"
                          :hint="t('e.g., pool.ntp.org, cn.pool.ntp.org, or a local IP')"
                          persistent-hint
                        />
                      </v-col>
                      <v-col cols="12">
                        <v-select
                          v-model="settingsStore.deviceSettings.ipMode"
                          :items="[
                            { title: 'Automatic (DHCP)', value: 'dhcp' },
                            { title: 'Static IP', value: 'static' },
                          ]"
                          :label="t('IP Configuration')"
                          variant="outlined"
                          :hint="t('Applied on the next boot / wake')"
                          persistent-hint
                        />
                      </v-col>
                    </v-row>
                    <v-row v-if="settingsStore.deviceSettings.ipMode === 'static'">
                      <v-col cols="12" md="4">
                        <v-text-field
                          v-model="settingsStore.deviceSettings.staticIp"
                          :label="t('IP Address')"
                          variant="outlined"
                          placeholder="192.168.1.50"
                        />
                      </v-col>
                      <v-col cols="12" md="4">
                        <v-text-field
                          v-model="settingsStore.deviceSettings.staticNetmask"
                          :label="t('Netmask')"
                          variant="outlined"
                        />
                      </v-col>
                      <v-col cols="12" md="4">
                        <v-text-field
                          v-model="settingsStore.deviceSettings.staticGateway"
                          :label="t('Gateway')"
                          variant="outlined"
                          placeholder="192.168.1.1"
                        />
                      </v-col>
                    </v-row>
                    <v-row>
                      <v-col cols="12">
                        <v-text-field
                          v-model="settingsStore.deviceSettings.dnsServer"
                          :label="t('DNS Server')"
                          variant="outlined"
                          :hint="
                            settingsStore.deviceSettings.ipMode === 'static'
                              ? 'Leave empty to use the gateway'
                              : 'Optional override; leave empty to use DHCP-provided DNS'
                          "
                          persistent-hint
                        />
                      </v-col>
                    </v-row>
                    <v-switch
                      v-model="settingsStore.deviceSettings.httpAuthEnabled"
                      :label="t('Require a password for this device\'s web interface')"
                      color="primary"
                      class="mt-6"
                      hide-details
                    />
                    <div class="text-caption text-medium-emphasis mb-2">
                      {{
                        t(
                          "Off by default. Most frames sit on a trusted home network, where this is unnecessary."
                        )
                      }}
                    </div>
                    <v-text-field
                      v-if="settingsStore.deviceSettings.httpAuthEnabled"
                      v-model="settingsStore.deviceSettings.httpPassword"
                      :label="
                        settingsStore.deviceSettings.httpAuthEnabled &&
                        settingsStore.deviceSettings.httpPassword === '' &&
                        settingsStore.deviceSettings.httpAuthWasEnabled
                          ? t('Password (set — leave blank to keep)')
                          : t('Password')
                      "
                      type="password"
                      maxlength="63"
                      variant="outlined"
                      :hint="t('Any username is accepted; the password is the whole credential.')"
                      persistent-hint
                      class="mt-2"
                    />
                    <v-alert
                      v-if="settingsStore.deviceSettings.httpAuthEnabled"
                      type="warning"
                      variant="tonal"
                      density="compact"
                      class="mt-3"
                    >
                      {{
                        t(
                          "Enter the same password in the photoframe server, the Home Assistant integration and the mobile app, or they will stop syncing with this frame; older versions of them cannot send it at all. It is also sent unencrypted over plain HTTP — it guards against casual access on a shared network, not against someone who can capture your traffic."
                        )
                      }}
                    </v-alert>
                  </v-expansion-panel-text>
                </v-expansion-panel>
              </v-expansion-panels>
            </v-expansion-panel-text>
          </v-expansion-panel>

          <v-expansion-panel value="time">
            <v-expansion-panel-title>
              <v-icon icon="mdi-clock-outline" class="mr-3" />
              {{ t("Time") }}
            </v-expansion-panel-title>
            <v-expansion-panel-text>
              <v-row>
                <v-col cols="12">
                  <v-text-field
                    :model-value="deviceTime || 'Loading...'"
                    :label="t('Device Time')"
                    variant="outlined"
                    readonly
                    :hint="t('Click sync to update from NTP server')"
                    persistent-hint
                  >
                    <template #append-inner>
                      <v-btn
                        icon
                        variant="text"
                        size="small"
                        :loading="syncingTime"
                        @click="syncTime"
                      >
                        <v-icon>mdi-sync</v-icon>
                        <v-tooltip activator="parent" location="top">{{ t("Sync NTP") }}</v-tooltip>
                      </v-btn>
                    </template>
                  </v-text-field>
                </v-col>
                <v-col cols="12">
                  <v-autocomplete
                    v-model="selectedZone"
                    :items="timezoneItems"
                    :label="t('Time zone')"
                    variant="outlined"
                    auto-select-first
                    :hint="t('The rotation schedule (Auto Rotate) runs in this time zone')"
                    persistent-hint
                  />
                  <div class="text-caption text-medium-emphasis mt-1">
                    {{ t("POSIX TZ rule:") }} <code>{{ timezoneRule }}</code>
                  </div>
                  <div v-if="approximateNote" class="text-caption text-warning mt-1">
                    {{ approximateNote }}
                  </div>
                  <div class="d-flex flex-wrap ga-2 mt-1">
                    <v-btn
                      v-if="browserZoneKnown"
                      size="small"
                      variant="text"
                      prepend-icon="mdi-web"
                      :disabled="selectedZone === browserZone"
                      @click="selectedZone = browserZone"
                    >
                      {{ t("Use this browser's time zone ({zone})", { zone: browserZone }) }}
                    </v-btn>
                    <v-btn
                      size="small"
                      variant="text"
                      :prepend-icon="showAdvancedTz ? 'mdi-chevron-up' : 'mdi-chevron-down'"
                      @click="showAdvancedTz = !showAdvancedTz"
                    >
                      {{ t("Advanced: POSIX TZ rule") }}
                    </v-btn>
                  </div>
                  <v-expand-transition>
                    <v-text-field
                      v-if="showAdvancedTz"
                      v-model="settingsStore.deviceSettings.timezone"
                      :label="t('POSIX TZ rule')"
                      variant="outlined"
                      density="compact"
                      class="mt-2"
                      :maxlength="TIMEZONE_MAX_BYTES"
                      :error-messages="timezoneError ? [timezoneError] : []"
                      :hint="
                        t(
                          'Any rule tzset() accepts, e.g. EST5EDT,M3.2.0,M11.1.0. A fixed offset is UTC-8 for eight hours ahead of UTC (POSIX inverts the sign).'
                        )
                      "
                      persistent-hint
                    />
                  </v-expand-transition>
                </v-col>
              </v-row>
            </v-expansion-panel-text>
          </v-expansion-panel>

          <v-expansion-panel value="display">
            <v-expansion-panel-title>
              <v-icon icon="mdi-monitor" class="mr-3" />
              {{ t("Screen") }}
            </v-expansion-panel-title>
            <v-expansion-panel-text>
              <v-alert type="info" variant="tonal" density="compact" class="mb-4">
                {{
                  t(
                    "How the frame is mounted. To turn a single photo, use the rotate buttons when uploading."
                  )
                }}
              </v-alert>
              <v-row>
                <v-col cols="12">
                  <v-select
                    v-model="settingsStore.deviceSettings.displayOrientation"
                    :items="orientationOptions"
                    item-title="title"
                    item-value="value"
                    :label="t('Display Orientation')"
                    variant="outlined"
                  />
                </v-col>
                <v-col cols="12">
                  <v-select
                    v-model="settingsStore.deviceSettings.displayRotationDeg"
                    :items="rotationOptions"
                    item-title="title"
                    item-value="value"
                    :label="t('Display Rotation (deg)')"
                    variant="outlined"
                  />
                </v-col>
              </v-row>
            </v-expansion-panel-text>
          </v-expansion-panel>

          <v-expansion-panel value="power">
            <v-expansion-panel-title>
              <v-icon icon="mdi-power" class="mr-3" />
              {{ t("Power") }}
            </v-expansion-panel-title>
            <v-expansion-panel-text>
              <v-switch
                v-model="settingsStore.deviceSettings.deepSleepEnabled"
                :label="t('Enable Deep Sleep')"
                color="primary"
                class="mb-4"
              />

              <v-expand-transition>
                <v-alert
                  v-if="!settingsStore.deviceSettings.deepSleepEnabled"
                  type="warning"
                  variant="tonal"
                >
                  <strong>{{ t("Power Consumption Notice") }}</strong
                  ><br />
                  {{
                    t(
                      "Disabling deep sleep keeps the HTTP server accessible but significantly increases power consumption. Only disable if permanently powered via USB."
                    )
                  }}
                </v-alert>
              </v-expand-transition>
              <BatterySleep class="mt-4" />
            </v-expansion-panel-text>
          </v-expansion-panel>

          <v-expansion-panel value="calibration">
            <v-expansion-panel-title>
              <v-icon icon="mdi-tune" class="mr-3" />
              {{ t("Calibration") }}
            </v-expansion-panel-title>
            <v-expansion-panel-text>
              <GrayscaleCalibration v-if="appStore.isGrayscale" />
              <PaletteCalibration v-else />
            </v-expansion-panel-text>
          </v-expansion-panel>

          <v-expansion-panel value="maintenance">
            <v-expansion-panel-title>
              <v-icon icon="mdi-wrench" class="mr-3" />
              {{ t("Maintenance") }}
            </v-expansion-panel-title>
            <v-expansion-panel-text>
              <div class="text-subtitle-1 mt-2 mb-4">{{ t("Config Backup") }}</div>
              <v-row>
                <v-col cols="12">
                  <v-btn variant="outlined" class="mr-2" @click="exportConfig">
                    <v-icon start>mdi-download</v-icon>
                    {{ t("Export Config") }}
                  </v-btn>
                  <v-btn variant="outlined" @click="$refs.importInput.click()">
                    <v-icon start>mdi-upload</v-icon>
                    {{ t("Import Config") }}
                  </v-btn>
                  <input
                    ref="importInput"
                    type="file"
                    accept=".json"
                    style="display: none"
                    @change="onImportFileSelected"
                  />
                </v-col>
              </v-row>

              <v-divider class="my-6" />

              <div class="text-subtitle-1 mb-4">{{ t("Debug Logging") }}</div>
              <v-row>
                <v-col cols="12">
                  <v-switch
                    v-model="settingsStore.deviceSettings.debugLogEnabled"
                    :label="t('Save console logs to storage')"
                    color="primary"
                    hide-details
                    class="mb-2"
                  />
                  <v-expand-transition>
                    <v-alert
                      v-if="settingsStore.deviceSettings.debugLogEnabled"
                      type="info"
                      variant="tonal"
                      density="compact"
                      class="mb-4"
                    >
                      {{
                        t(
                          "Serial console output is mirrored to the SD card, keeping only the most recent lines. Takes effect after saving."
                        )
                      }}
                    </v-alert>
                  </v-expand-transition>
                  <v-btn
                    variant="outlined"
                    class="mr-2"
                    :loading="downloadingLog"
                    @click="downloadDebugLog"
                  >
                    <v-icon start>mdi-download</v-icon>
                    {{ t("Download Logs") }}
                  </v-btn>
                  <v-btn variant="outlined" :loading="clearingLog" @click="clearDebugLog">
                    <v-icon start>mdi-delete</v-icon>
                    {{ t("Clear Logs") }}
                  </v-btn>
                </v-col>
              </v-row>

              <v-divider class="my-6" />

              <LastCrash />

              <div class="text-subtitle-1 mb-4">{{ t("Factory Reset") }}</div>
              <v-row>
                <v-col cols="12">
                  <v-btn color="error" variant="outlined" @click="showFactoryResetDialog = true">
                    <v-icon start>mdi-restore-alert</v-icon>
                    {{ t("Factory Reset Device") }}
                  </v-btn>
                </v-col>
              </v-row>

              <v-divider class="my-6" />

              <div class="text-subtitle-1 mb-2">{{ t("Format storage") }}</div>
              <div class="text-body-2 text-medium-emphasis mb-3">
                {{ t("Erases every photo and album on the card. This cannot be undone.") }}
              </div>
              <v-btn
                color="error"
                variant="outlined"
                :disabled="!hasStorage"
                @click="openFormatDialog"
              >
                <v-icon start>mdi-delete-sweep</v-icon>
                {{ t("Format storage") }}
              </v-btn>
            </v-expansion-panel-text>
          </v-expansion-panel>

          <v-expansion-panel value="experimental">
            <v-expansion-panel-title>
              <v-icon icon="mdi-flask-outline" class="mr-3" />
              {{ t("Experimental") }}
            </v-expansion-panel-title>
            <v-expansion-panel-text>
              <v-alert type="warning" variant="tonal" density="compact" class="mb-4">
                {{
                  t(
                    "Experimental features. The current local-frame firmware does not use them, and the device may reject or ignore these settings."
                  )
                }}
              </v-alert>
              <div class="text-subtitle-1 mb-2">Home Assistant</div>
              <v-text-field
                v-model="settingsStore.deviceSettings.haUrl"
                :label="t('Home Assistant URL')"
                variant="outlined"
                placeholder="http://homeassistant.local:8123"
                :hint="t('Configure for dynamic image serving and battery level reporting')"
                persistent-hint
              />
              <v-divider class="my-4" />
              <div class="text-subtitle-1 mb-2">{{ t("AI Generation") }}</div>
              <v-alert type="info" variant="tonal" density="compact" class="mt-2 mb-4">
                {{
                  t("API keys are used for client-side AI image generation when uploading images.")
                }}
              </v-alert>

              <v-text-field
                v-model="settingsStore.deviceSettings.aiCredentials.openaiApiKey"
                :label="t('OpenAI API Key')"
                variant="outlined"
                type="password"
                hint="sk-..."
                persistent-hint
                class="mb-2"
              />
              <div class="text-caption text-grey ml-2 mb-4">
                {{ t("Get your API key at") }}
                <a
                  href="https://platform.openai.com/api-keys"
                  target="_blank"
                  class="text-primary text-decoration-none"
                  >platform.openai.com</a
                >
              </div>

              <v-text-field
                v-model="settingsStore.deviceSettings.aiCredentials.googleApiKey"
                :label="t('Google Gemini API Key')"
                variant="outlined"
                type="password"
                class="mb-2"
              />
              <div class="text-caption text-grey ml-2 mb-4">
                {{ t("Get your API key at") }}
                <a
                  href="https://aistudio.google.com/app/apikey"
                  target="_blank"
                  class="text-primary text-decoration-none"
                  >aistudio.google.com</a
                >
              </div>
              <v-divider class="my-4" />
              <div class="text-subtitle-1 mb-2">{{ t("Remote image source") }}</div>
              <v-select
                v-model="settingsStore.deviceSettings.rotationMode"
                :items="rotationModeOptions"
                item-title="title"
                item-value="value"
                :label="t('Rotation Mode')"
                variant="outlined"
                class="mt-4 mb-4"
              />
              <v-expand-transition>
                <v-card
                  v-if="settingsStore.deviceSettings.rotationMode === 'url'"
                  variant="tonal"
                  class="mb-4"
                >
                  <v-card-text>
                    <v-text-field
                      v-model="settingsStore.deviceSettings.imageUrl"
                      :label="t('Image URL')"
                      variant="outlined"
                      hide-details
                      class="mb-4"
                    />

                    <div
                      v-if="settingsStore.deviceSettings.caCertSet"
                      class="mb-4 d-flex flex-column ga-1"
                    >
                      <v-chip
                        color="success"
                        size="small"
                        variant="tonal"
                        style="align-self: flex-start"
                      >
                        <v-icon start>mdi-check-circle</v-icon>
                        {{ t("Certificate Pinned") }}
                      </v-chip>
                      <div class="text-caption text-medium-emphasis">
                        {{
                          t(
                            "The TLS certificate for this HTTPS URL is pinned. It will re-pin automatically when you change the URL."
                          )
                        }}
                      </div>
                    </div>

                    <v-alert
                      v-if="settingsStore.deviceSettings.lastFetchError"
                      type="error"
                      variant="tonal"
                      density="compact"
                      class="mb-4"
                    >
                      {{ t("Last fetch error:") }} {{ settingsStore.deviceSettings.lastFetchError }}
                    </v-alert>

                    <v-checkbox
                      v-if="
                        appStore.systemInfo.sdcard_inserted || appStore.systemInfo.has_flash_storage
                      "
                      v-model="settingsStore.deviceSettings.saveDownloadedImages"
                      :label="t('Save downloaded images to Downloads album')"
                      color="primary"
                      class="mb-8"
                      hide-details
                    />

                    <v-text-field
                      v-model="settingsStore.deviceSettings.accessToken"
                      :label="t('Access Token (Optional)')"
                      variant="outlined"
                      :hint="t('Sets Authorization: Bearer header')"
                      persistent-hint
                      class="mt-4"
                    />

                    <v-row class="mt-4">
                      <v-col cols="12" md="6">
                        <v-text-field
                          v-model="settingsStore.deviceSettings.httpHeaderKey"
                          :label="t('Custom Header Name')"
                          variant="outlined"
                          :placeholder="t('e.g., X-API-Key')"
                        />
                      </v-col>
                      <v-col cols="12" md="6">
                        <v-text-field
                          v-model="settingsStore.deviceSettings.httpHeaderValue"
                          :label="t('Custom Header Value')"
                          variant="outlined"
                        />
                      </v-col>
                    </v-row>
                  </v-card-text>
                </v-card>
              </v-expand-transition>
            </v-expansion-panel-text>
          </v-expansion-panel>
          <v-expansion-panel value="about">
            <v-expansion-panel-title>
              <v-icon icon="mdi-information-outline" class="mr-3" />
              {{ t("About") }}
            </v-expansion-panel-title>
            <v-expansion-panel-text>
              <v-table density="compact">
                <tbody>
                  <tr v-for="[label, value] in aboutRows" :key="label">
                    <td class="text-medium-emphasis">{{ label }}</td>
                    <td class="text-break">{{ value }}</td>
                  </tr>
                </tbody>
              </v-table>
            </v-expansion-panel-text>
          </v-expansion-panel>
        </v-expansion-panels>
      </div>

      <v-divider />
      <div class="d-flex align-center flex-wrap ga-2 px-4 py-3">
        <v-chip v-if="settingsStore.systemDirty" color="warning" variant="tonal" size="small">
          <v-icon icon="mdi-circle-edit-outline" start />
          {{ t("Unsaved changes") }}
        </v-chip>
        <v-spacer />
        <v-fade-transition>
          <v-chip v-if="saveSuccess" color="success" variant="tonal">
            <v-icon icon="mdi-check" start />
            {{ saveMessage || t("Settings saved!") }}
          </v-chip>
          <v-chip v-else-if="saveError" color="error" variant="tonal">
            <v-icon icon="mdi-alert-circle" start />
            {{ saveMessage || t("Failed to save settings") }}
          </v-chip>
        </v-fade-transition>
        <v-tooltip :text="saveBlocker" location="top" :disabled="!saveBlocker">
          <template #activator="{ props: tooltipProps }">
            <span v-bind="tooltipProps">
              <v-btn
                color="primary"
                :loading="saving"
                :disabled="!!saveBlocker || !settingsStore.systemDirty"
                @click="saveSettings"
              >
                <v-icon icon="mdi-content-save" start />
                {{ t("Save Settings") }}
              </v-btn>
            </span>
          </template>
        </v-tooltip>
      </div>
    </div>

    <!-- Format storage: type the word to confirm -->
    <v-dialog v-model="showFormatDialog" max-width="440">
      <v-card>
        <v-card-title class="text-error">
          <v-icon icon="mdi-alert" class="mr-2" />
          {{ t("Format storage") }}
        </v-card-title>
        <v-card-text>
          <v-alert type="error" variant="tonal" class="mb-4">
            {{
              t(
                "This will erase all {n} photos on the card and every album. It cannot be undone.",
                {
                  n: photoCount,
                }
              )
            }}
          </v-alert>
          <v-text-field
            v-model="formatConfirm"
            :label="t('Type {word} to confirm', { word: formatWord })"
            variant="outlined"
            density="compact"
            autocomplete="off"
            hide-details
          />
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="showFormatDialog = false">{{ t("Cancel") }}</v-btn>
          <v-btn
            color="error"
            variant="flat"
            :loading="formatting"
            :disabled="formatConfirm.trim().toUpperCase() !== formatWord.toUpperCase()"
            @click="performFormat"
          >
            {{ t("Format") }}
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- Unsaved-changes dialog -->
    <v-dialog v-model="showDiscardDialog" max-width="400">
      <v-card>
        <v-card-title>{{ t("Discard unsaved changes?") }}</v-card-title>
        <v-card-text>
          {{ t("Your edits in System settings have not been saved.") }}
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="showDiscardDialog = false">{{ t("Keep editing") }}</v-btn>
          <v-btn color="error" variant="flat" @click="discardAndClose">{{ t("Discard") }}</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- Factory Reset Confirmation Dialog -->
    <v-dialog v-model="showFactoryResetDialog" max-width="500">
      <v-card>
        <v-card-title class="text-h5 text-error">
          <v-icon icon="mdi-alert" class="mr-2" />
          {{ t("Confirm Factory Reset") }}
        </v-card-title>
        <v-card-text>
          <v-alert type="error" variant="tonal" class="mb-4">
            <div class="text-subtitle-2 mb-2">{{ t("This action is irreversible!") }}</div>
            <div class="text-body-2">
              {{ t("All device settings will be permanently erased, including:") }}
            </div>
            <ul class="mt-2">
              <li>{{ t("WiFi credentials") }}</li>
              <li>{{ t("Image processing settings") }}</li>
              <li>{{ t("Device configuration") }}</li>
              <li>{{ t("All custom settings") }}</li>
            </ul>
          </v-alert>
          <div class="text-body-1 mb-3">
            {{
              t(
                "The device will restart and return to factory defaults. Are you sure you want to continue?"
              )
            }}
          </div>
          <v-alert type="info" variant="tonal" density="compact">
            <div class="text-body-2">
              <strong>{{ t("After reset:") }}</strong>
              {{
                t(
                  'The device will create a WiFi access point named "PhotoFrame". Connect to it from your device to restart the provisioning process.'
                )
              }}
            </div>
          </v-alert>
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="showFactoryResetDialog = false">{{ t("Cancel") }}</v-btn>
          <v-btn color="error" variant="flat" :loading="resetting" @click="performFactoryReset">
            {{ t("Reset Device") }}
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
    <!-- Import Config Confirmation Dialog -->
    <v-dialog v-model="showImportDialog" max-width="500">
      <v-card>
        <v-card-title>
          <v-icon icon="mdi-upload" class="mr-2" />
          {{ t("Import Config") }}
        </v-card-title>
        <v-card-text>
          <v-alert type="warning" variant="tonal" class="mb-4">
            {{ t("This will overwrite your current settings with the imported config.") }}
          </v-alert>
          <div class="text-body-2 mb-2">
            {{ t("File:") }} <strong>{{ importFileName }}</strong>
          </div>
          <div v-if="importData" class="text-body-2">
            {{ t("Sections to import:") }}
            <ul class="mt-1 ml-4">
              <li v-if="importData.config">{{ t("Device settings") }}</li>
              <li v-if="importData.processing">{{ t("Processing settings") }}</li>
              <li v-if="importData.palette">{{ t("Palette calibration") }}</li>
            </ul>
          </div>
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="showImportDialog = false">{{ t("Cancel") }}</v-btn>
          <v-btn color="primary" variant="flat" @click="performImport"> {{ t("Import") }} </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-navigation-drawer>
</template>

<style scoped></style>
