<script setup>
import { t } from "../i18n";
import { computed, ref } from "vue";
import { useHotspotStore } from "../stores";
import { formatRemaining } from "../utils/hotspot";

// Top of the page: hotspot status while the phone is on the frame's Wi-Fi, the
// "clock is off" prompt for offline frames, and where to go after a switch.
const hotspot = useHotspotStore();
const status = computed(() => hotspot.status || {});
const remaining = computed(() => formatRemaining(status.value.remaining_s));
const syncing = ref(false);
const syncError = ref("");

async function syncClock() {
  syncing.value = true;
  syncError.value = "";
  try {
    await hotspot.syncClock();
  } catch (e) {
    syncError.value = String(e?.message || e);
  } finally {
    syncing.value = false;
  }
}

const handoffOpen = computed({
  get: () => !!hotspot.handoff,
  set: (open) => {
    if (!open) hotspot.handoff = null;
  },
});
</script>

<template>
  <div>
    <v-alert
      v-if="status.on"
      type="info"
      variant="tonal"
      density="compact"
      icon="mdi-access-point-network"
      class="mb-4"
    >
      <div class="d-flex align-center flex-wrap ga-2">
        <span>
          {{ t("Photo hotspot on") }} ·
          {{ t("{n} phone(s) connected", { n: status.clients || 0 }) }}
          <template v-if="remaining">
            · {{ t("turns off in {time} without a phone", { time: remaining }) }}
          </template>
        </span>
        <v-spacer />
        <v-btn
          size="small"
          variant="outlined"
          :loading="hotspot.busy"
          @click="hotspot.setOn(false)"
        >
          {{ status.transfers ? t("Turn off after upload") : t("Turn off hotspot") }}
        </v-btn>
      </div>
    </v-alert>

    <v-alert
      v-if="hotspot.clockOff"
      type="warning"
      variant="tonal"
      density="compact"
      icon="mdi-clock-outline"
      class="mb-4"
    >
      <div class="d-flex align-center flex-wrap ga-2">
        <span>{{ t("The frame's clock is off by more than 2 minutes.") }}</span>
        <v-spacer />
        <v-btn size="small" variant="outlined" :loading="syncing" @click="syncClock">
          {{ t("Use this phone's time") }}
        </v-btn>
      </div>
      <div v-if="syncError" class="text-caption mt-1">{{ syncError }}</div>
    </v-alert>

    <v-dialog v-model="handoffOpen" max-width="460" persistent>
      <v-card v-if="hotspot.handoff === 'to-hotspot'">
        <v-card-title>{{ t("Hotspot is turning on") }}</v-card-title>
        <v-card-text>
          <p class="mb-3">
            {{ t("This page disconnects now. On your phone, join the frame's Wi-Fi:") }}
          </p>
          <v-table density="compact" class="mb-3">
            <tbody>
              <tr>
                <td class="text-medium-emphasis">{{ t("Wi-Fi name") }}</td>
                <td>{{ hotspot.handoffInfo.ssid }}</td>
              </tr>
              <tr>
                <td class="text-medium-emphasis">{{ t("Password") }}</td>
                <td class="font-monospace">{{ hotspot.handoffInfo.password }}</td>
              </tr>
            </tbody>
          </v-table>
          <p>
            {{
              t("Then open {url}. The frame's screen shows both as QR codes.", { url: status.url })
            }}
          </p>
          <p class="text-caption mt-2">
            {{
              t(
                "If Android says the network has no Internet, choose to stay connected; if the page still does not open, turn off mobile data for a moment."
              )
            }}
          </p>
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="handoffOpen = false">{{ t("Close") }}</v-btn>
        </v-card-actions>
      </v-card>
      <v-card v-else>
        <v-card-title>{{ t("Hotspot is turning off") }}</v-card-title>
        <v-card-text>
          {{
            t(
              "The frame is rejoining your home Wi-Fi and this page disconnects now. Switch your phone back to your home Wi-Fi to reach the frame again."
            )
          }}
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="handoffOpen = false">{{ t("Close") }}</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </div>
</template>
