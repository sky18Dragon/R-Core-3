<script setup>
import { t } from "../i18n";
import { computed, ref, watch } from "vue";
import { useHotspotStore } from "../stores";
import { hotspotPasswordValid, hotspotSsidValid, idleChoices } from "../utils/hotspot";

// System settings -> Photo hotspot. Saves on its own buttons (not the drawer's
// Save), because each change is one small POST the frame validates itself.
const hotspot = useHotspotStore();
const status = computed(() => hotspot.status || {});

const name = ref("");
const customPassword = ref("");
const confirmOn = ref(false);

watch(
  () => status.value.custom_ssid,
  () => {
    name.value = status.value.custom_ssid ? status.value.ssid : "";
  },
  { immediate: true }
);

const idleItems = computed(() =>
  idleChoices(status.value.never_close_allowed).map((m) => ({
    title: m ? t("{n} min", { n: m }) : t("Never"),
    value: m,
  }))
);
const nameError = computed(() =>
  name.value && !hotspotSsidValid(name.value) ? t("1 to 32 bytes, no control characters") : ""
);
const passwordError = computed(() =>
  customPassword.value && !hotspotPasswordValid(customPassword.value)
    ? t("8 to 63 letters, digits or symbols (no Chinese characters)")
    : ""
);

async function saveName() {
  if (nameError.value) return;
  await hotspot.save({ ssid: name.value });
}
async function savePassword() {
  if (!customPassword.value || passwordError.value) return;
  if (await hotspot.save({ password: customPassword.value })) customPassword.value = "";
}
function setIdle(minutes) {
  hotspot.save({ idle_minutes: minutes });
}
async function turnOn() {
  confirmOn.value = false;
  await hotspot.setOn(true);
}
</script>

<template>
  <div>
    <p class="text-body-2 mb-4">
      {{
        t(
          "Without a router or Internet, a phone can join the frame's own Wi-Fi and use this page to upload photos."
        )
      }}
    </p>

    <div class="d-flex align-center flex-wrap ga-3 mb-4">
      <v-chip :color="status.on ? 'primary' : undefined" variant="tonal">
        {{ status.on ? t("Hotspot on") : t("Hotspot off") }}
      </v-chip>
      <span v-if="status.on" class="text-body-2">
        {{ t("{n} phone(s) connected", { n: status.clients || 0 }) }}
      </span>
      <v-spacer />
      <v-btn
        v-if="!status.on"
        color="primary"
        variant="flat"
        prepend-icon="mdi-access-point-network"
        :loading="hotspot.busy"
        :disabled="status.stop_unconfirmed"
        @click="confirmOn = true"
      >
        {{ t("Turn on hotspot") }}
      </v-btn>
      <v-btn
        v-else
        variant="outlined"
        prepend-icon="mdi-power"
        :loading="hotspot.busy"
        @click="hotspot.setOn(false)"
      >
        {{ status.transfers ? t("Turn off after upload") : t("Turn off hotspot") }}
      </v-btn>
    </div>

    <v-alert
      v-if="status.stop_unconfirmed"
      type="error"
      variant="tonal"
      density="compact"
      class="mb-4"
    >
      {{
        t(
          "The last turn-off was not confirmed by the radio. Turn it off again before turning it on."
        )
      }}
    </v-alert>
    <v-alert v-if="hotspot.error" type="error" variant="tonal" density="compact" class="mb-4">
      {{ hotspot.error }}
    </v-alert>

    <v-table density="compact" class="mb-4">
      <tbody>
        <tr>
          <td class="text-medium-emphasis">{{ t("Wi-Fi name") }}</td>
          <td>{{ status.ssid }}</td>
        </tr>
        <tr>
          <td class="text-medium-emphasis">{{ t("Password") }}</td>
          <td class="font-monospace">{{ status.password }}</td>
        </tr>
        <tr>
          <td class="text-medium-emphasis">{{ t("Web page") }}</td>
          <td>{{ status.url }}</td>
        </tr>
      </tbody>
    </v-table>

    <v-alert
      v-if="status.pending_change"
      type="info"
      variant="tonal"
      density="compact"
      class="mb-4"
    >
      {{ t("New name or password applies the next time the hotspot is turned on.") }}
    </v-alert>

    <v-select
      :model-value="status.idle_minutes"
      :items="idleItems"
      :label="t('Turn off automatically when no phone is connected for')"
      variant="outlined"
      :disabled="hotspot.busy"
      @update:model-value="setIdle"
    />

    <div class="d-flex ga-2 align-start">
      <v-text-field
        v-model="name"
        :label="t('Custom Wi-Fi name')"
        :placeholder="status.default_ssid"
        :hint="t('Leave empty to use {name}', { name: status.default_ssid })"
        persistent-hint
        :error-messages="nameError"
        variant="outlined"
      />
      <v-btn
        class="mt-2"
        variant="outlined"
        :disabled="!!nameError || hotspot.busy"
        @click="saveName"
      >
        {{ t("Save") }}
      </v-btn>
    </div>

    <div class="d-flex ga-2 align-start mt-2">
      <v-text-field
        v-model="customPassword"
        :label="t('Set your own password')"
        :error-messages="passwordError"
        maxlength="63"
        variant="outlined"
        autocomplete="off"
      />
      <v-btn
        class="mt-2"
        variant="outlined"
        :disabled="!customPassword || !!passwordError || hotspot.busy"
        @click="savePassword"
      >
        {{ t("Save") }}
      </v-btn>
    </div>
    <v-btn
      variant="text"
      prepend-icon="mdi-sync"
      :disabled="hotspot.busy"
      @click="hotspot.save({ regenerate_password: true })"
    >
      {{ t("Generate a new password") }}
    </v-btn>

    <v-dialog v-model="confirmOn" max-width="440">
      <v-card>
        <v-card-title>{{ t("Turn on hotspot?") }}</v-card-title>
        <v-card-text>
          {{
            t(
              "The frame leaves your home Wi-Fi and this page will disconnect. Then join the Wi-Fi shown on the frame's screen and open {url}.",
              { url: status.url }
            )
          }}
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="confirmOn = false">{{ t("Cancel") }}</v-btn>
          <v-btn color="primary" variant="flat" @click="turnOn">{{ t("Turn on hotspot") }}</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </div>
</template>
