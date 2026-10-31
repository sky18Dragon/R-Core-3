<script setup>
import { t, locale, setLocale } from "../i18n";
import { ref, computed, nextTick, onMounted, onUnmounted } from "vue";

// Wi-Fi setup page served by the frame's own hotspot (/provision). The phone has
// no internet here, so everything is inline: no icon font, no web font, no CDN.
// Strictly black / white / grey; state is always carried by an icon and a sentence,
// never by colour alone.

const ssid = ref("");
const password = ref("");
const deviceName = ref("PhotoFrame");
const showPassword = ref(false);
const manualEntry = ref(false);
const attempted = ref(false);

// 'form' | 'connecting' | 'saved'
const phase = ref("form");
const failure = ref("");
const savedHostname = ref("");
const showNextSteps = ref(false);
const notice = ref(null);

const networks = ref([]);
const scanning = ref(false);
const scanFailed = ref(false);

// Advanced network settings (#43): static IP + DNS override for networks
// without DHCP. Collapsed by default — almost everyone uses DHCP.
const showAdvanced = ref(false);
const ipMode = ref("dhcp");
const staticIp = ref("");
const staticNetmask = ref("255.255.255.0");
const staticGateway = ref("");
const dnsServer = ref("");

const busy = computed(() => phase.value === "connecting");

// Signal strength -> 1..4 filled bars plus a spoken label.
function signalLevel(rssi) {
  if (rssi >= -50) return 4;
  if (rssi >= -60) return 3;
  if (rssi >= -70) return 2;
  return 1;
}

function signalLabel(level) {
  return ["", t("Weak signal"), t("Fair signal"), t("Good signal"), t("Strong signal")][level];
}

function authLabel(auth) {
  return auth === "Open" ? t("Open network") : auth;
}

async function scanNetworks() {
  scanning.value = true;
  scanFailed.value = false;
  try {
    // A scan that never answers must not leave the list spinning with Rescan disabled.
    // (AbortController rather than AbortSignal.timeout(): older phone browsers lack it.)
    const abort = new AbortController();
    const timer = setTimeout(() => abort.abort(), 20000);
    let response;
    try {
      response = await fetch("/api/wifi/scan", { signal: abort.signal });
    } finally {
      clearTimeout(timer);
    }
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const data = await response.json();
    networks.value = data.map((n) => {
      const level = signalLevel(n.rssi);
      return {
        ssid: n.ssid,
        level,
        secured: n.auth !== "Open",
        detail: `${signalLabel(level)} · ${authLabel(n.auth)}`,
      };
    });
  } catch (e) {
    console.error("WiFi scan failed:", e);
    scanFailed.value = true;
  } finally {
    scanning.value = false;
  }
}

const selectedNetwork = computed(() =>
  manualEntry.value ? null : networks.value.find((n) => n.ssid === ssid.value) || null
);

// A scanned open network needs no password; everything else (including a typed name) may.
const needsPassword = computed(() => !selectedNetwork.value || selectedNetwork.value.secured);

function chooseNetwork(n) {
  if (busy.value) return;
  manualEntry.value = false;
  ssid.value = n.ssid;
  if (!n.secured) password.value = "";
}

function chooseOther() {
  if (busy.value) return;
  if (!manualEntry.value) ssid.value = "";
  manualEntry.value = true;
}

// Mirrors the firmware's wifi_text_credentials_valid(): SSID 1-32 bytes, password empty
// or 8-64 printable ASCII characters.
const ssidError = computed(() => {
  const bytes = new TextEncoder().encode(ssid.value).length;
  if (bytes === 0) return t("Choose a network or enter its name.");
  if (bytes > 32) return t("The network name is too long (32 bytes at most).");
  return "";
});

const passwordError = computed(() => {
  if (!needsPassword.value) return "";
  const p = password.value;
  if (p === "") {
    return selectedNetwork.value ? t("Enter the network password.") : "";
  }
  if (p.length < 8 || p.length > 64 || /[^\x20-\x7e]/.test(p)) {
    return t("The password must be 8 to 64 standard keyboard characters.");
  }
  return "";
});

const formError = computed(() => (attempted.value ? ssidError.value || passwordError.value : ""));

let keepAliveInterval = null;
let restoreChrome = null;

onMounted(() => {
  scanNetworks();
  keepAliveInterval = setInterval(() => {
    fetch("/api/keep_alive").catch(() => {});
  }, 60000);

  // The page background is set on <body> so overscroll matches; undone on leave so
  // the other routes are untouched.
  const prevBg = document.body.style.background;
  const prevTitle = document.title;
  document.body.style.background = "#f7f7f7";
  document.title = t("PhotoFrame Setup");
  restoreChrome = () => {
    document.body.style.background = prevBg;
    document.title = prevTitle;
  };
});

onUnmounted(() => {
  if (keepAliveInterval) {
    clearInterval(keepAliveInterval);
    keepAliveInterval = null;
  }
  if (restoreChrome) restoreChrome();
});

// The notice sits by the button, which may be off-screen on a phone: bring it into view.
function revealNotice() {
  nextTick(() => notice.value?.scrollIntoView({ block: "center", behavior: "smooth" }));
}

function failureMessage(status, text) {
  if (status === 409) return t("This setup session has expired. Reload the page and try again.");
  if (text.includes("Connection Failed") || text.includes("connect")) {
    return t(
      "WiFi Connection Failed. Please check your password, SSID, and ensure it's a 2.4GHz network."
    );
  }
  return t("An error occurred. Please try again.");
}

async function submitForm() {
  attempted.value = true;
  if (ssidError.value || passwordError.value) {
    revealNotice();
    return;
  }

  phase.value = "connecting";
  failure.value = "";

  const formData = new URLSearchParams();
  formData.append("ssid", ssid.value);
  formData.append("password", needsPassword.value ? password.value : "");
  formData.append("deviceName", deviceName.value);
  formData.append("ipMode", ipMode.value);
  if (ipMode.value === "static") {
    formData.append("staticIp", staticIp.value.trim());
    formData.append("staticNetmask", staticNetmask.value.trim());
    formData.append("staticGateway", staticGateway.value.trim());
  }
  if (dnsServer.value.trim()) {
    formData.append("dnsServer", dnsServer.value.trim());
  }

  try {
    const response = await fetch("/save", {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
      },
      body: formData.toString(),
    });

    if (response.ok) {
      // mDNS hostname from the device name (lowercase, spaces to hyphens)
      savedHostname.value = deviceName.value.toLowerCase().replace(/\s+/g, "-");
      phase.value = "saved";
      setTimeout(() => {
        showNextSteps.value = true;
      }, 3000);
    } else {
      phase.value = "form";
      failure.value = failureMessage(response.status, await response.text());
      revealNotice();
    }
  } catch (_error) {
    phase.value = "form";
    failure.value = t(
      "Cannot reach the frame. Make sure your phone is still connected to its WiFi hotspot."
    );
    revealNotice();
  }
}
</script>

<template>
  <div class="pv">
    <main class="pv-shell">
      <!-- ───── Header ───── -->
      <header class="pv-hero">
        <svg class="pv-glyph" viewBox="0 0 72 52" aria-hidden="true">
          <rect x="1.5" y="1.5" width="69" height="49" rx="6" class="g-frame" />
          <rect x="8" y="8" width="56" height="36" rx="1.5" class="g-paper" />
          <circle cx="23" cy="20" r="4" class="g-ink-mid" />
          <path d="M8 44 L28 26 L40 36 L48 29 L64 44 Z" class="g-ink" />
        </svg>
        <h1>{{ t("PhotoFrame Setup") }}</h1>
        <p class="pv-lede">{{ t("Connect your PhotoFrame to WiFi") }}</p>
      </header>

      <!-- ───── Connecting ───── -->
      <section v-if="phase === 'connecting'" class="pv-state" role="status" aria-live="polite">
        <span class="pv-spinner" aria-hidden="true"></span>
        <h2>{{ t("Testing WiFi connection...") }}</h2>
        <p>{{ t("Keep this page open. It can take a little while.") }}</p>
      </section>

      <!-- ───── Saved ───── -->
      <section v-else-if="phase === 'saved'" class="pv-state" role="status" aria-live="polite">
        <svg class="pv-badge" viewBox="0 0 48 48" aria-hidden="true">
          <circle cx="24" cy="24" r="22" class="b-fill" />
          <path d="M14 25 L21 32 L34 17" class="b-mark" />
        </svg>
        <h2>{{ t("Setup complete") }}</h2>
        <p>
          {{
            t(
              'Credentials saved! Device will restart in 3 seconds and attempt to connect to "{ssid}".',
              { ssid }
            )
          }}
        </p>
        <ol v-if="showNextSteps" class="pv-steps">
          <li>{{ t("Close this page.") }}</li>
          <li>{{ t("Reconnect your phone to your usual WiFi network.") }}</li>
          <li>
            {{ t("Then open") }} <strong>http://{{ savedHostname }}.local</strong>
          </li>
        </ol>
      </section>

      <!-- ───── Form ───── -->
      <form v-else class="pv-form" novalidate @submit.prevent="submitForm">
        <!-- Network -->
        <section aria-labelledby="pv-net-h">
          <div class="pv-section-head">
            <h2 id="pv-net-h">{{ t("Network") }}</h2>
            <button type="button" class="pv-link" :disabled="scanning" @click="scanNetworks">
              {{ scanning ? t("Scanning…") : t("Rescan networks") }}
            </button>
          </div>

          <div class="pv-group" role="radiogroup" :aria-labelledby="'pv-net-h'">
            <div v-if="scanning && !networks.length" class="pv-row pv-row-note" role="status">
              <span class="pv-spinner pv-spinner-sm" aria-hidden="true"></span>
              <span>{{ t("Scanning for networks…") }}</span>
            </div>
            <div
              v-else-if="!networks.length"
              class="pv-row pv-row-note"
              :role="scanFailed ? 'alert' : null"
            >
              <svg class="pv-ico" viewBox="0 0 24 24" aria-hidden="true">
                <circle cx="12" cy="12" r="9.25" class="i-line" />
                <path d="M12 7.5 V13" class="i-line" />
                <circle cx="12" cy="16.5" r="1.1" class="i-dot" />
              </svg>
              <span>{{
                scanFailed
                  ? t("Could not scan for networks. You can type the name instead.")
                  : t("No networks found.")
              }}</span>
            </div>

            <button
              v-for="n in networks"
              :key="n.ssid"
              type="button"
              class="pv-row pv-row-btn"
              role="radio"
              :aria-checked="!manualEntry && ssid === n.ssid"
              @click="chooseNetwork(n)"
            >
              <svg class="pv-ico" viewBox="0 0 24 24" aria-hidden="true">
                <rect
                  v-for="i in 4"
                  :key="i"
                  :x="2 + (i - 1) * 5.5"
                  :y="20 - i * 4.5"
                  width="3.5"
                  :height="i * 4.5"
                  rx="1"
                  :class="i <= n.level ? 'i-solid' : 'i-faint'"
                />
              </svg>
              <span class="pv-row-main">
                <span class="pv-row-title">{{ n.ssid }}</span>
                <span class="pv-row-sub">{{ n.detail }}</span>
              </span>
              <svg v-if="n.secured" class="pv-ico pv-ico-sm" viewBox="0 0 24 24" aria-hidden="true">
                <rect x="5" y="10.5" width="14" height="10" rx="2.2" class="i-solid" />
                <path d="M8.5 10.5 V8 a3.5 3.5 0 0 1 7 0 V10.5" class="i-line" />
              </svg>
              <svg
                class="pv-tick"
                :class="{ on: !manualEntry && ssid === n.ssid }"
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path d="M5 12.5 L10 17.5 L19 7" />
              </svg>
            </button>

            <button
              type="button"
              class="pv-row pv-row-btn"
              role="radio"
              :aria-checked="manualEntry"
              @click="chooseOther"
            >
              <svg class="pv-ico" viewBox="0 0 24 24" aria-hidden="true">
                <path d="M12 5 V19 M5 12 H19" class="i-line" />
              </svg>
              <span class="pv-row-main">
                <span class="pv-row-title">{{ t("Other network…") }}</span>
                <span class="pv-row-sub">{{ t("Hidden or not listed") }}</span>
              </span>
              <svg
                class="pv-tick"
                :class="{ on: manualEntry }"
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path d="M5 12.5 L10 17.5 L19 7" />
              </svg>
            </button>

            <div v-if="manualEntry" class="pv-row pv-field">
              <label for="pv-ssid">{{ t("Network name") }}</label>
              <input
                id="pv-ssid"
                v-model="ssid"
                type="text"
                autocomplete="off"
                autocapitalize="none"
                autocorrect="off"
                spellcheck="false"
                :placeholder="t('Required')"
              />
            </div>
          </div>
        </section>

        <!-- Password -->
        <section v-if="needsPassword" aria-labelledby="pv-pw-h">
          <div class="pv-section-head">
            <h2 id="pv-pw-h">{{ t("WiFi Password") }}</h2>
          </div>
          <div class="pv-group">
            <div class="pv-row pv-field">
              <label for="pv-password">{{ t("Password") }}</label>
              <input
                id="pv-password"
                v-model="password"
                :type="showPassword ? 'text' : 'password'"
                autocomplete="off"
                autocapitalize="none"
                autocorrect="off"
                spellcheck="false"
                :aria-invalid="attempted && !!passwordError"
                :placeholder="selectedNetwork ? t('Required') : t('Optional')"
              />
              <button
                type="button"
                class="pv-link pv-link-inline"
                :aria-pressed="showPassword"
                @click="showPassword = !showPassword"
              >
                {{ showPassword ? t("Hide") : t("Show") }}
              </button>
            </div>
          </div>
          <p v-if="!selectedNetwork" class="pv-foot">{{ t("Leave blank for open networks") }}</p>
        </section>

        <!-- Device name -->
        <section aria-labelledby="pv-dev-h">
          <div class="pv-section-head">
            <h2 id="pv-dev-h">{{ t("Device Name") }}</h2>
          </div>
          <div class="pv-group">
            <div class="pv-row pv-field">
              <label for="pv-name">{{ t("Name") }}</label>
              <input
                id="pv-name"
                v-model="deviceName"
                type="text"
                autocomplete="off"
                autocapitalize="words"
                spellcheck="false"
              />
            </div>
          </div>
          <p class="pv-foot">{{ t("Used for mDNS hostname") }}</p>
        </section>

        <!-- Advanced -->
        <section>
          <div class="pv-group">
            <button
              type="button"
              class="pv-row pv-row-btn pv-disclosure"
              :aria-expanded="showAdvanced"
              aria-controls="pv-advanced"
              @click="showAdvanced = !showAdvanced"
            >
              <span class="pv-row-main">
                <span class="pv-row-title">{{ t("Advanced network settings") }}</span>
              </span>
              <svg
                class="pv-chev"
                :class="{ open: showAdvanced }"
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path d="M9 5.5 L15.5 12 L9 18.5" />
              </svg>
            </button>
          </div>

          <div v-show="showAdvanced" id="pv-advanced" class="pv-advanced">
            <div class="pv-segment" role="radiogroup" :aria-label="t('IP Configuration')">
              <button
                type="button"
                role="radio"
                :aria-checked="ipMode === 'dhcp'"
                @click="ipMode = 'dhcp'"
              >
                {{ t("Automatic (DHCP)") }}
              </button>
              <button
                type="button"
                role="radio"
                :aria-checked="ipMode === 'static'"
                @click="ipMode = 'static'"
              >
                {{ t("Static IP") }}
              </button>
            </div>

            <div class="pv-group">
              <template v-if="ipMode === 'static'">
                <div class="pv-row pv-field">
                  <label for="pv-ip">{{ t("IP Address") }}</label>
                  <input
                    id="pv-ip"
                    v-model="staticIp"
                    type="text"
                    inputmode="decimal"
                    autocomplete="off"
                    placeholder="192.168.1.50"
                  />
                </div>
                <div class="pv-row pv-field">
                  <label for="pv-mask">{{ t("Netmask") }}</label>
                  <input
                    id="pv-mask"
                    v-model="staticNetmask"
                    type="text"
                    inputmode="decimal"
                    autocomplete="off"
                  />
                </div>
                <div class="pv-row pv-field">
                  <label for="pv-gw">{{ t("Gateway") }}</label>
                  <input
                    id="pv-gw"
                    v-model="staticGateway"
                    type="text"
                    inputmode="decimal"
                    autocomplete="off"
                    placeholder="192.168.1.1"
                  />
                </div>
              </template>
              <div class="pv-row pv-field">
                <label for="pv-dns">{{ t("DNS Server") }}</label>
                <input
                  id="pv-dns"
                  v-model="dnsServer"
                  type="text"
                  inputmode="decimal"
                  autocomplete="off"
                  :placeholder="t('Optional')"
                />
              </div>
            </div>
            <p class="pv-foot">
              {{
                ipMode === "static"
                  ? t("Leave empty to use the gateway")
                  : t("Leave empty to use DHCP-provided DNS")
              }}
            </p>
          </div>
        </section>

        <!-- Messages: icon + words, never colour alone -->
        <div v-if="failure || formError" ref="notice" class="pv-notice" role="alert">
          <svg class="pv-ico" viewBox="0 0 24 24" aria-hidden="true">
            <circle cx="12" cy="12" r="9.25" class="i-line" />
            <path d="M12 7.5 V13" class="i-line" />
            <circle cx="12" cy="16.5" r="1.1" class="i-dot" />
          </svg>
          <div>
            <strong>{{ failure ? t("Could not connect") : t("Check your entries") }}</strong>
            <span>{{ failure || formError }}</span>
          </div>
        </div>

        <button type="submit" class="pv-primary">{{ t("Connect to WiFi") }}</button>
      </form>

      <footer class="pv-foot-bar">
        <button
          type="button"
          class="pv-link"
          :aria-pressed="locale === 'en'"
          @click="setLocale('en')"
        >
          English
        </button>
        <span aria-hidden="true">·</span>
        <button
          type="button"
          class="pv-link"
          :aria-pressed="locale === 'zh'"
          @click="setLocale('zh')"
        >
          中文
        </button>
      </footer>
    </main>
  </div>
</template>

<style scoped>
/* ── Tokens: black / white / grey only ── */
.pv {
  --bg: #f7f7f7;
  --surface: #ffffff;
  --fill: #f0f0f0;
  --ink: #111111;
  --ink-2: #666666;
  --ink-3: #999999;
  --line: #e0e0e0;
  --faint: #c9c9c9;
  --r-group: 12px;
  --r-ctl: 14px;

  min-height: 100vh;
  min-height: 100dvh;
  background: var(--bg);
  color: var(--ink);
  color-scheme: light;
  font-family:
    -apple-system, BlinkMacSystemFont, "SF Pro Text", "PingFang SC", "Helvetica Neue",
    "Noto Sans SC", system-ui, sans-serif;
  font-size: 17px;
  line-height: 1.35;
  -webkit-font-smoothing: antialiased;
  -webkit-text-size-adjust: 100%;
}

.pv *,
.pv *::before,
.pv *::after {
  box-sizing: border-box;
}

.pv-shell {
  max-width: 440px;
  margin: 0 auto;
  padding: max(24px, env(safe-area-inset-top)) max(16px, env(safe-area-inset-right))
    max(32px, env(safe-area-inset-bottom)) max(16px, env(safe-area-inset-left));
}

/* ── Hero ── */
.pv-hero {
  text-align: center;
  padding: 24px 0 28px;
}

.pv-glyph {
  width: 72px;
  height: 52px;
  margin-bottom: 20px;
}
.g-frame {
  fill: var(--surface);
  stroke: var(--ink);
  stroke-width: 3;
}
.g-paper {
  fill: var(--fill);
}
.g-ink-mid {
  fill: var(--faint);
}
.g-ink {
  fill: var(--ink-2);
}

.pv h1 {
  margin: 0;
  font-size: 28px;
  line-height: 1.2;
  font-weight: 700;
  letter-spacing: -0.01em;
}
.pv-lede {
  margin: 8px 0 0;
  color: var(--ink-2);
  font-size: 17px;
}

/* ── Sections ── */
.pv-form section {
  margin-bottom: 24px;
}
.pv-section-head {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  padding: 0 16px 6px;
}
.pv h2 {
  margin: 0;
  font-size: 13px;
  line-height: 1.3;
  font-weight: 600;
  letter-spacing: 0.02em;
  color: var(--ink-2);
}
.pv-foot {
  margin: 8px 16px 0;
  font-size: 13px;
  line-height: 1.35;
  color: var(--ink-2);
}

/* ── Grouped list ── */
.pv-group {
  background: var(--surface);
  border: 1px solid var(--line);
  border-radius: var(--r-group);
  overflow: hidden;
}
.pv-row {
  display: flex;
  align-items: center;
  gap: 12px;
  min-height: 52px;
  padding: 8px 16px;
}
.pv-group > .pv-row + .pv-row {
  border-top: 1px solid var(--line);
}
.pv-row-btn {
  width: 100%;
  border: 0;
  background: transparent;
  color: inherit;
  font: inherit;
  text-align: left;
  cursor: pointer;
  -webkit-tap-highlight-color: transparent;
}
.pv-row-btn:active {
  background: var(--fill);
}
.pv-row-note {
  color: var(--ink-2);
  font-size: 15px;
}
.pv-row-main {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
}
.pv-row-title {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.pv-row-sub {
  font-size: 13px;
  color: var(--ink-2);
}

/* ── Icons ── */
.pv-ico {
  flex: none;
  width: 24px;
  height: 24px;
}
.pv-ico-sm {
  width: 18px;
  height: 18px;
}
.i-solid {
  fill: var(--ink);
}
.i-faint {
  fill: var(--faint);
}
.i-line {
  fill: none;
  stroke: var(--ink);
  stroke-width: 2;
  stroke-linecap: round;
}
.i-dot {
  fill: var(--ink);
}
.pv-tick {
  flex: none;
  width: 22px;
  height: 22px;
  fill: none;
  stroke: var(--ink);
  stroke-width: 2.4;
  stroke-linecap: round;
  stroke-linejoin: round;
  visibility: hidden;
}
.pv-tick.on {
  visibility: visible;
}
.pv-chev {
  flex: none;
  width: 18px;
  height: 18px;
  fill: none;
  stroke: var(--ink-3);
  stroke-width: 2.4;
  stroke-linecap: round;
  stroke-linejoin: round;
  transition: transform 0.15s steps(3);
}
.pv-chev.open {
  transform: rotate(90deg);
}

/* ── Fields ── */
.pv-field label {
  flex: none;
  min-width: 96px;
  font-size: 17px;
  white-space: nowrap;
}
.pv-field input {
  flex: 1;
  min-width: 0;
  height: 44px;
  border: 0;
  outline: 0;
  background: transparent;
  color: var(--ink);
  font: inherit;
  text-align: right;
  caret-color: var(--ink);
  border-radius: 8px;
}
.pv-field input::placeholder {
  color: #767676;
}
.pv-field:focus-within {
  background: var(--fill);
}

/* ── Links / small buttons ── */
.pv-link {
  border: 0;
  background: none;
  padding: 0;
  color: var(--ink);
  font: inherit;
  font-size: 15px;
  font-weight: 600;
  text-decoration: underline;
  text-underline-offset: 3px;
  cursor: pointer;
  min-height: 24px;
}
.pv-link:disabled {
  color: var(--ink-3);
  text-decoration: none;
  cursor: default;
}
.pv-link-inline {
  flex: none;
  min-width: 44px;
  min-height: 44px;
  margin-right: -8px;
}

/* ── Advanced ── */
.pv-advanced {
  margin-top: 12px;
}
.pv-segment {
  display: flex;
  gap: 2px;
  padding: 2px;
  margin-bottom: 12px;
  background: var(--fill);
  border: 1px solid var(--line);
  border-radius: 10px;
}
.pv-segment button {
  flex: 1;
  min-height: 40px;
  border: 0;
  border-radius: 8px;
  background: transparent;
  color: var(--ink-2);
  font: inherit;
  font-size: 15px;
  cursor: pointer;
}
.pv-segment button[aria-checked="true"] {
  background: var(--ink);
  color: #fff;
  font-weight: 600;
}

/* ── Notice (error / validation) ── */
.pv-notice {
  display: flex;
  gap: 12px;
  align-items: flex-start;
  margin-bottom: 16px;
  padding: 14px 16px;
  background: var(--surface);
  border: 2px solid var(--ink);
  border-radius: var(--r-group);
  font-size: 15px;
}
.pv-notice > div {
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.pv-notice strong {
  font-weight: 600;
}
.pv-notice span {
  color: var(--ink-2);
}

/* ── Primary action ── */
.pv-primary {
  display: block;
  width: 100%;
  min-height: 52px;
  border: 0;
  border-radius: var(--r-ctl);
  background: var(--ink);
  color: #fff;
  font: inherit;
  font-weight: 600;
  cursor: pointer;
  -webkit-tap-highlight-color: transparent;
}
.pv-primary:active {
  background: #3a3a3a;
}

/* ── Progress / result states ── */
.pv-state {
  text-align: center;
  padding: 16px 8px 32px;
}
.pv-state h2 {
  margin: 20px 0 8px;
  font-size: 22px;
  line-height: 1.25;
  font-weight: 700;
  letter-spacing: 0;
  color: var(--ink);
}
.pv-state p {
  margin: 0 auto;
  max-width: 32ch;
  color: var(--ink-2);
  font-size: 15px;
  white-space: pre-line;
}
.pv-badge {
  width: 56px;
  height: 56px;
}
.b-fill {
  fill: var(--ink);
}
.b-mark {
  fill: none;
  stroke: #fff;
  stroke-width: 4;
  stroke-linecap: round;
  stroke-linejoin: round;
}
.pv-steps {
  margin: 24px 0 0;
  padding: 16px 16px 16px 36px;
  text-align: left;
  background: var(--surface);
  border: 1px solid var(--line);
  border-radius: var(--r-group);
  font-size: 15px;
  line-height: 1.5;
}
.pv-steps li + li {
  margin-top: 6px;
}
.pv-steps strong {
  overflow-wrap: anywhere;
}

/* Stepped spinner: reads as a page refresh rather than a smooth web animation. */
.pv-spinner {
  display: inline-block;
  width: 40px;
  height: 40px;
  border: 4px solid var(--line);
  border-top-color: var(--ink);
  border-radius: 50%;
  animation: pv-spin 0.9s steps(8) infinite;
}
.pv-spinner-sm {
  width: 20px;
  height: 20px;
  border-width: 3px;
}
@keyframes pv-spin {
  to {
    transform: rotate(360deg);
  }
}

/* ── Footer ── */
.pv-foot-bar {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 8px;
  margin-top: 32px;
  color: var(--ink-3);
}
.pv-foot-bar .pv-link {
  min-height: 44px;
  min-width: 44px;
  font-weight: 400;
  color: var(--ink-2);
  text-decoration: none;
}
.pv-foot-bar .pv-link[aria-pressed="true"] {
  color: var(--ink);
  font-weight: 600;
  text-decoration: underline;
}

/* ── Focus: a plain black ring, visible on every surface ── */
.pv button:focus-visible,
.pv input:focus-visible {
  outline: 2px solid var(--ink);
  outline-offset: 2px;
}
.pv-row-btn:focus-visible {
  outline-offset: -2px;
}

@media (prefers-reduced-motion: reduce) {
  .pv-spinner {
    animation-duration: 3.6s;
  }
  .pv-chev {
    transition: none;
  }
}

@media (min-width: 600px) {
  .pv-shell {
    padding-top: 56px;
  }
}
</style>
