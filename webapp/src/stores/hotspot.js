import { defineStore } from "pinia";
import { computed, ref } from "vue";
import { clockDriftSeconds, clockNeedsSync } from "../utils/hotspot";

// Photo-transfer hotspot (firmware /api/hotspot). The frame applies "on"/"off"
// about 1.5 s after answering, then this page loses its connection: `handoff`
// tells the user where to find the frame next instead of a generic error.
export const useHotspotStore = defineStore("hotspot", () => {
  const status = ref(null); // null until loaded; { supported: false } on older firmware
  const busy = ref(false);
  const error = ref("");
  // null | "to-hotspot" (phone must join the frame's Wi-Fi) | "to-home" (frame rejoins home Wi-Fi)
  const handoff = ref(null);
  const handoffInfo = ref({ ssid: "", password: "" });
  const clockDrift = ref(null);

  const supported = computed(() => !!status.value?.supported);
  const on = computed(() => !!status.value?.on);

  async function load() {
    try {
      const response = await fetch("/api/hotspot", { cache: "no-store" });
      if (!response.ok) {
        if (response.status === 404) status.value = { supported: false };
        return;
      }
      status.value = await response.json();
    } catch (_e) {
      // Unreachable while the radio switches; keep the last known status.
    }
  }

  async function post(body) {
    busy.value = true;
    error.value = "";
    try {
      const response = await fetch("/api/hotspot", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body),
      });
      const text = await response.text();
      if (!response.ok) {
        let message = text;
        try {
          message = JSON.parse(text).error || text;
        } catch (_e) {
          // Plain-text error body.
        }
        error.value = message || `HTTP ${response.status}`;
        return false;
      }
      status.value = JSON.parse(text);
      return true;
    } catch (e) {
      error.value = String(e?.message || e);
      return false;
    } finally {
      busy.value = false;
    }
  }

  async function setOn(next) {
    const wasOn = on.value;
    const ok = await post({ on: next });
    if (ok && next !== wasOn) {
      handoffInfo.value = { ssid: status.value.ssid, password: status.value.password };
      handoff.value = next ? "to-hotspot" : "to-home";
    }
    return ok;
  }

  function save(changes) {
    return post(changes);
  }

  async function checkClock() {
    try {
      const response = await fetch("/api/time", { cache: "no-store" });
      if (!response.ok) return;
      const data = await response.json();
      clockDrift.value = clockDriftSeconds(data.timestamp);
    } catch (_e) {
      // Leave the previous reading.
    }
  }

  async function syncClock() {
    const response = await fetch("/api/time", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ timestamp: Math.floor(Date.now() / 1000) }),
    });
    if (!response.ok) throw new Error(await response.text());
    const data = await response.json();
    clockDrift.value = clockDriftSeconds(data.timestamp);
  }

  const clockOff = computed(() => clockNeedsSync(clockDrift.value));

  return {
    status,
    busy,
    error,
    handoff,
    handoffInfo,
    clockDrift,
    clockOff,
    supported,
    on,
    load,
    setOn,
    save,
    checkClock,
    syncClock,
  };
});
