<script setup>
import { t } from "../i18n";
import { computed, onBeforeUnmount, ref, watch } from "vue";
import { getDitherOptions } from "@aitjcize/epaper-image-convert";
import { useAppStore, usePresetsStore, useSettingsStore, useUploadsStore } from "../stores";
import ProcessingControls from "./ProcessingControls.vue";
import SavePresetDialog from "./SavePresetDialog.vue";
import TestPhotoPanel from "./TestPhotoPanel.vue";
import { processingOf } from "../utils/presets";
import { hasTestPhoto, testFraming, testPhotoVersion, testSource } from "../utils/testPhoto";
import {
  FINAL_COUNT,
  MAX_CANDIDATES,
  describeDiff,
  nextRound,
  previousRound,
  reletter,
  remainingLetters,
  starterCandidates,
  toggleLetter,
} from "../utils/tuning";

// Finding a preset by elimination on the panel itself: each candidate gets a
// letter and is shown full screen in turn; the viewer strikes out the ones
// they do not want, the rest play again, and the last few are compared 2×2.
// The winner is kept as a preset (docs/PRD-PRESETS.md).
const props = defineProps({
  // Preset the candidates are built around (default preset when null)
  baseId: { type: String, default: null },
  // Set when tuning an existing preset: the result can update it
  updatingId: { type: String, default: null },
});
const emit = defineEmits(["close", "saved"]);

const appStore = useAppStore();
const settingsStore = useSettingsStore();
const uploads = useUploadsStore();
const presets = usePresetsStore();

const STORE_KEY = "photoframe.tuning";
const DWELL_OPTIONS = [10, 15, 20, 30, 60];

// ---- State (remembered in this browser, except the photo) ----------------
const base = ref(props.baseId ?? presets.defaultId);
const candidates = ref([]); // [{letter, params, name, isBase}]
const history = ref([]); // letters shown in each round; empty while setting up
const dropped = ref([]); // letters struck out in the current round
const dwell = ref(15);
const finalLetters = ref([]);

const basePreset = computed(() => presets.byId(base.value));
const baseParams = computed(() => ({
  ...settingsStore.params,
  ...(basePreset.value ? basePreset.value.params : {}),
}));

function resetList() {
  const others = presets.list
    .filter((p) => p.id !== base.value)
    .map((p) => ({ name: p.name, params: p.params }));
  candidates.value = starterCandidates(baseParams.value, basePreset.value?.name ?? null, others);
}

// Pick up an unfinished session around the same preset, else start fresh
function load() {
  try {
    const saved = JSON.parse(localStorage.getItem(STORE_KEY) || "null");
    if (DWELL_OPTIONS.includes(saved?.dwell)) dwell.value = saved.dwell;
    if (saved?.base === base.value && Array.isArray(saved.candidates) && saved.candidates.length) {
      candidates.value = saved.candidates.filter((c) => c && c.letter && c.params);
      history.value = Array.isArray(saved.history) ? saved.history : [];
      dropped.value = Array.isArray(saved.dropped) ? saved.dropped : [];
      return;
    }
  } catch {
    // Nothing saved or storage blocked
  }
  resetList();
}
load();

function persist() {
  try {
    localStorage.setItem(
      STORE_KEY,
      JSON.stringify({
        base: base.value,
        candidates: candidates.value,
        history: history.value,
        dropped: dropped.value,
        dwell: dwell.value,
      })
    );
  } catch {
    // Not remembered, still works
  }
}
watch([candidates, history, dropped, dwell, base], persist, { deep: true });

function changeBase(id) {
  base.value = id;
  resetList();
}

const presetItems = computed(() =>
  presets.list.map((p) => ({
    value: p.id,
    title: p.id === presets.defaultId ? `★ ${p.name}` : p.name,
  }))
);

const started = computed(() => history.value.length > 0);
const roundNumber = computed(() => history.value.length);
const pool = computed(() => history.value[history.value.length - 1] ?? []);
const remaining = computed(() => remainingLetters(history.value, dropped.value));
const droppedInRound = computed(() => pool.value.filter((l) => dropped.value.includes(l)));
const hasPhoto = computed(() => testPhotoVersion.value >= 0 && hasTestPhoto());

function byLetter(letter) {
  return candidates.value.find((c) => c.letter === letter);
}

// ---- Describing a candidate ------------------------------------------------
const ditherTitles = Object.fromEntries(getDitherOptions().map((o) => [o.value, o.title]));

function keyTitle(key) {
  return {
    exposure: t("Exposure"),
    saturation: t("Saturation"),
    toneMode: t("Tone Mapping"),
    contrast: t("Contrast"),
    strength: t("Strength"),
    shadowBoost: t("Shadow Boost"),
    highlightCompress: t("Highlight Compress"),
    midpoint: t("Midpoint"),
    colorMethod: t("Color Matching"),
    ditherAlgorithm: t("Dithering Algorithm"),
    compressDynamicRange: t("Compress Dynamic Range"),
  }[key];
}

function valueText(key, value) {
  if (key === "toneMode") return value === "scurve" ? t("S-Curve") : t("Contrast");
  if (key === "ditherAlgorithm") return ditherTitles[value] ?? value;
  if (key === "colorMethod") return String(value).toUpperCase();
  if (typeof value === "boolean") return value ? t("on") : t("off");
  if (typeof value === "number") return value.toFixed(2);
  return String(value);
}

function titleOf(c) {
  if (c.isBase) return c.name ? t("Base: {name}", { name: c.name }) : t("Base");
  return c.name ?? t("Adjusted");
}

function detailOf(c) {
  const diff = describeDiff(c.params, baseParams.value, { grayscale: appStore.isGrayscale });
  if (c.isBase) return t("The preset these are built around");
  if (!diff.length) return t("Same as the base");
  return diff.map((d) => `${keyTitle(d.key)} ${valueText(d.key, d.value)}`).join(" · ");
}

// ---- Editing the list (before the first round) -----------------------------
const editing = ref(null); // {index, params}

function edit(index) {
  editing.value = { index, params: { ...candidates.value[index].params } };
}

function add() {
  if (candidates.value.length >= MAX_CANDIDATES) return;
  candidates.value = reletter([
    ...candidates.value,
    { params: { ...baseParams.value }, name: null, isBase: false },
  ]);
  edit(candidates.value.length - 1);
}

function remove(index) {
  candidates.value = reletter(candidates.value.filter((_, i) => i !== index));
}

function saveEdit() {
  const { index, params } = editing.value;
  const list = [...candidates.value];
  // An edited candidate is no longer the preset it came from
  list[index] = { ...list[index], params, name: list[index].isBase ? list[index].name : null };
  candidates.value = list;
  editing.value = null;
}

// ---- Playing a round on the frame ------------------------------------------
const index = ref(0);
const playing = ref(false);
const phase = ref("idle"); // "sending" | "showing" | "idle"
const roundDone = ref(false);
const shownAt = ref(0);
const now = ref(Date.now());
const sendError = ref("");
let timer = null;
let ticker = null;
let runId = 0;

const current = computed(() => byLetter(pool.value[index.value]));
const progress = computed(() =>
  phase.value === "showing" && playing.value
    ? Math.min(100, ((now.value - shownAt.value) / (dwell.value * 1000)) * 100)
    : 0
);

watch(
  [playing, phase],
  () => {
    clearInterval(ticker);
    if (playing.value && phase.value === "showing") {
      ticker = setInterval(() => (now.value = Date.now()), 250);
    }
  },
  { immediate: true }
);

function schedule() {
  clearTimeout(timer);
  const run = runId;
  timer = setTimeout(() => {
    if (run !== runId || !playing.value) return;
    if (index.value + 1 < pool.value.length) go(index.value + 1);
    else {
      playing.value = false;
      roundDone.value = true;
    }
  }, dwell.value * 1000);
}

async function go(i) {
  clearTimeout(timer);
  const run = ++runId;
  index.value = i;
  roundDone.value = false;
  finalLetters.value = [];
  sendError.value = "";
  const c = current.value;
  const source = testSource();
  if (!source || !c) return;
  phase.value = "sending";
  let result;
  try {
    // Let the status paint before the (synchronous) processing starts
    await new Promise((resolve) => setTimeout(resolve, 0));
    const sheet = await uploads.renderSheet(source, [c], {
      labels: [c.letter],
      framing: testFraming.value,
    });
    if (run !== runId) return;
    result = await uploads.displaySheet(sheet);
  } catch {
    result = { ok: false, message: t("Could not prepare the comparison") };
  }
  if (run !== runId) return;
  if (!result.ok) {
    phase.value = "idle";
    playing.value = false;
    sendError.value = result.message;
    return;
  }
  phase.value = "showing";
  shownAt.value = now.value = Date.now();
  if (playing.value) schedule();
}

function play() {
  if (!hasPhoto.value) return;
  playing.value = true;
  if (phase.value === "showing" && !roundDone.value) {
    shownAt.value = now.value = Date.now();
    schedule();
  } else if (phase.value !== "sending") {
    go(roundDone.value ? 0 : index.value);
  }
}

function pause() {
  playing.value = false;
  clearTimeout(timer);
}

function step(delta) {
  go(Math.min(pool.value.length - 1, Math.max(0, index.value + delta)));
}

function stopAll() {
  pause();
  runId++;
  phase.value = "idle";
  roundDone.value = false;
  index.value = 0;
}

function startRounds() {
  history.value = [candidates.value.map((c) => c.letter)];
  dropped.value = [];
  stopAll();
  play();
}

function drop(letter) {
  dropped.value = toggleLetter(dropped.value, letter);
}

function playRemaining() {
  const next = nextRound(history.value, dropped.value);
  if (!next) return;
  history.value = next;
  dropped.value = [];
  stopAll();
  play();
}

function backRound() {
  history.value = previousRound(history.value);
  dropped.value = [];
  stopAll();
  finalLetters.value = [];
}

function restart() {
  stopAll();
  history.value = [];
  dropped.value = [];
  finalLetters.value = [];
  // Written now: after saving, this view closes before a watcher would run
  persist();
}

const compareLetters = computed(() =>
  remaining.value.length >= 2 && remaining.value.length <= FINAL_COUNT ? remaining.value : []
);

const comparing = ref(false);
async function compareFinal() {
  const letters = compareLetters.value;
  const source = testSource();
  if (!letters.length || !source) return;
  pause();
  runId++;
  comparing.value = true;
  sendError.value = "";
  try {
    const sheet = await uploads.renderSheet(
      source,
      letters.map((l) => byLetter(l)),
      { labels: letters, framing: testFraming.value }
    );
    const result = await uploads.displaySheet(sheet);
    if (result.ok) {
      finalLetters.value = letters;
      phase.value = "idle";
    } else sendError.value = result.message;
  } catch {
    sendError.value = t("Could not prepare the comparison");
  } finally {
    comparing.value = false;
  }
}

// ---- Keeping the winner as a preset ----------------------------------------
const saving = ref(null); // the candidate being saved
const updatable = computed(() => {
  const preset = presets.byId(props.updatingId);
  return preset ? { id: preset.id, name: preset.name } : null;
});

function suggestedName(c) {
  if (c.name && !c.isBase) return c.name;
  const day = new Date();
  return t("Tuned {date}", { date: `${day.getMonth() + 1}-${day.getDate()}` });
}

function onSaved(id) {
  saving.value = null;
  restart();
  emit("saved", id);
}

onBeforeUnmount(() => {
  clearTimeout(timer);
  clearInterval(ticker);
});

const statusText = computed(() => {
  if (phase.value === "sending") return t("Sending to the frame…");
  if (roundDone.value) return t("Round finished");
  if (phase.value === "showing") return playing.value ? t("On the frame now") : t("Paused");
  return t("Not started");
});
</script>

<template>
  <div class="tuning">
    <div class="head">
      <v-btn icon="mdi-arrow-left" variant="text" :aria-label="t('Back')" @click="emit('close')" />
      <h1 class="page-title">
        {{
          updatable ? t("Tune “{name}”", { name: updatable.name }) : t("Find a preset by tuning")
        }}
      </h1>
    </div>
    <p class="text-body-2 text-medium-emphasis intro">
      {{
        t(
          "Each setting gets a letter and is shown full screen on the frame in turn. Strike out the ones you do not want; the rest play again. When four are left, compare them on one screen and keep the best as a preset."
        )
      }}
    </p>

    <h2 class="section-title">{{ t("Test photo") }}</h2>
    <TestPhotoPanel
      :note="
        started
          ? t('Every setting uses this framing. A change shows from the next setting on.')
          : t('Every setting uses this framing, so they compare like for like.')
      "
    />

    <!-- Setting up the candidates -->
    <template v-if="!started">
      <div class="section-head">
        <h2 class="section-title">
          {{ t("Settings to try ({n})", { n: candidates.length }) }}
        </h2>
        <v-spacer />
        <v-btn variant="text" size="small" @click="resetList">{{ t("Reset list") }}</v-btn>
        <v-btn
          variant="text"
          size="small"
          :disabled="candidates.length >= MAX_CANDIDATES"
          @click="add"
        >
          <v-icon icon="mdi-plus" start />{{ t("Add") }}
        </v-btn>
      </div>
      <v-select
        v-if="presetItems.length"
        :model-value="base"
        :items="presetItems"
        :label="t('Start from preset')"
        variant="outlined"
        density="comfortable"
        hide-details
        class="base-select mb-2"
        @update:model-value="changeBase"
      />
      <v-list class="candidate-list" lines="two" bg-color="transparent">
        <v-list-item v-for="(c, i) in candidates" :key="c.letter" @click="edit(i)">
          <template #prepend>
            <span class="letter">{{ c.letter }}</span>
          </template>
          <v-list-item-title>{{ titleOf(c) }}</v-list-item-title>
          <v-list-item-subtitle>{{ detailOf(c) }}</v-list-item-subtitle>
          <template #append>
            <v-btn
              icon="mdi-delete-outline"
              variant="text"
              size="small"
              :disabled="candidates.length <= 2"
              :aria-label="t('Delete')"
              @click.stop="remove(i)"
            />
          </template>
        </v-list-item>
      </v-list>

      <div class="start-row">
        <v-select
          v-model="dwell"
          :items="DWELL_OPTIONS.map((s) => ({ value: s, title: t('{n} seconds', { n: s }) }))"
          :label="t('Time on the frame')"
          variant="outlined"
          density="comfortable"
          hide-details
          class="dwell-select"
        />
        <v-btn
          color="primary"
          variant="flat"
          size="large"
          :disabled="!hasPhoto || candidates.length < 2"
          @click="startRounds"
        >
          <v-icon icon="mdi-play" start />
          {{ t("Start round 1") }}
        </v-btn>
      </div>
      <div v-if="!hasPhoto" class="text-caption text-medium-emphasis mt-2">
        {{ t("Choose a test photo first.") }}
      </div>
    </template>

    <!-- Rounds -->
    <template v-else>
      <h2 class="section-title">
        {{ t("Round {r} · {n} settings", { r: roundNumber, n: pool.length }) }}
      </h2>

      <v-alert v-if="!hasPhoto" type="info" variant="tonal" density="compact" class="mb-4">
        {{ t("Choose the test photo again to continue.") }}
      </v-alert>

      <section v-if="current" class="now-card">
        <div class="now-letter" :class="{ 'now-letter--out': dropped.includes(current.letter) }">
          {{ current.letter }}
        </div>
        <div class="now-body">
          <div class="text-overline text-medium-emphasis">
            {{ statusText }} · {{ index + 1 }} / {{ pool.length }}
          </div>
          <div class="text-subtitle-1 font-weight-medium">{{ titleOf(current) }}</div>
          <div class="text-body-2 text-medium-emphasis">{{ detailOf(current) }}</div>
          <v-progress-linear
            :model-value="progress"
            :indeterminate="phase === 'sending'"
            height="4"
            rounded
            class="my-3"
          />
          <div class="now-controls">
            <v-btn
              icon="mdi-skip-previous"
              variant="text"
              :disabled="!hasPhoto || index === 0"
              :aria-label="t('Previous setting')"
              @click="step(-1)"
            />
            <v-btn
              v-if="playing"
              icon="mdi-pause"
              variant="tonal"
              :aria-label="t('Pause')"
              @click="pause"
            />
            <v-btn
              v-else
              icon="mdi-play"
              variant="tonal"
              :disabled="!hasPhoto"
              :aria-label="t('Play')"
              @click="play"
            />
            <v-btn
              icon="mdi-skip-next"
              variant="text"
              :disabled="!hasPhoto || index >= pool.length - 1"
              :aria-label="t('Next setting')"
              @click="step(1)"
            />
            <v-btn
              icon="mdi-refresh"
              variant="text"
              :disabled="!hasPhoto || phase === 'sending'"
              :title="t('Show this setting again')"
              :aria-label="t('Show this setting again')"
              @click="step(0)"
            />
            <v-spacer />
            <v-btn
              :variant="dropped.includes(current.letter) ? 'flat' : 'outlined'"
              :color="dropped.includes(current.letter) ? 'primary' : undefined"
              @click="drop(current.letter)"
            >
              <v-icon :icon="dropped.includes(current.letter) ? 'mdi-undo' : 'mdi-close'" start />
              {{
                dropped.includes(current.letter)
                  ? t("Bring {label} back", { label: current.letter })
                  : t("Drop {label}", { label: current.letter })
              }}
            </v-btn>
          </div>
        </div>
      </section>

      <v-alert v-if="sendError" type="error" variant="tonal" density="compact" class="mt-3">
        {{ sendError }}
      </v-alert>

      <div class="text-body-2 text-medium-emphasis mt-5 mb-2">
        {{ t("Tap letters to drop them or bring them back:") }}
      </div>
      <div class="letter-grid">
        <button
          v-for="(letter, i) in pool"
          :key="letter"
          type="button"
          class="letter-chip"
          :class="{
            'letter-chip--out': dropped.includes(letter),
            'letter-chip--current': i === index,
          }"
          :aria-pressed="dropped.includes(letter)"
          @click="drop(letter)"
        >
          {{ letter }}
        </button>
      </div>
      <div class="text-body-2 mt-3">
        {{
          droppedInRound.length
            ? t("Dropped {d}; {n} left: {letters}", {
                d: droppedInRound.length,
                n: remaining.length,
                letters: remaining.join(" "),
              })
            : t("Nothing dropped yet")
        }}
      </div>

      <div class="round-actions">
        <v-btn
          v-if="compareLetters.length"
          color="primary"
          variant="flat"
          :loading="comparing"
          :disabled="!hasPhoto"
          @click="compareFinal"
        >
          <v-icon icon="mdi-view-grid-outline" start />
          {{ t("Compare {letters} on one screen", { letters: compareLetters.join(" ") }) }}
        </v-btn>
        <v-btn
          :color="compareLetters.length ? undefined : 'primary'"
          :variant="compareLetters.length ? 'tonal' : 'flat'"
          :disabled="!hasPhoto || droppedInRound.length === 0 || remaining.length === 0"
          @click="playRemaining"
        >
          <v-icon icon="mdi-play-circle-outline" start />
          {{ t("Next round with the {n} left", { n: remaining.length }) }}
        </v-btn>
        <v-btn
          v-if="remaining.length === 1"
          variant="tonal"
          @click="saving = byLetter(remaining[0])"
        >
          <v-icon icon="mdi-content-save-outline" start />
          {{ t("Keep {label} as a preset", { label: remaining[0] }) }}
        </v-btn>
        <v-spacer />
        <v-btn v-if="roundNumber > 1" variant="text" @click="backRound">
          {{ t("Previous round") }}
        </v-btn>
        <v-btn variant="text" @click="restart">{{ t("Start over") }}</v-btn>
      </div>

      <!-- The final few, side by side on the frame -->
      <section v-if="finalLetters.length" class="final-card">
        <div class="text-subtitle-1 font-weight-medium mb-1">
          {{ t("On the frame now: {letters} side by side", { letters: finalLetters.join(" ") }) }}
        </div>
        <div class="text-body-2 text-medium-emphasis mb-3">
          {{ t("Pick the one to keep as a preset.") }}
        </div>
        <div class="final-grid">
          <div v-for="letter in finalLetters" :key="letter" class="final-item">
            <span class="letter">{{ letter }}</span>
            <div class="final-text">
              <div class="text-body-2 font-weight-medium">{{ titleOf(byLetter(letter)) }}</div>
              <div class="text-caption text-medium-emphasis">{{ detailOf(byLetter(letter)) }}</div>
            </div>
            <v-btn size="small" variant="tonal" @click="saving = byLetter(letter)">
              {{ t("Keep {label} as a preset", { label: letter }) }}
            </v-btn>
          </div>
        </div>
      </section>

      <div class="text-caption text-medium-emphasis mt-6">
        {{
          t(
            "Keep this page open while a round plays. If the frame changes photo on its own schedule, tap play to show the current setting again."
          )
        }}
      </div>
    </template>

    <!-- Editing one candidate -->
    <v-dialog
      :model-value="!!editing"
      max-width="640"
      scrollable
      @update:model-value="editing = null"
    >
      <v-card v-if="editing">
        <v-card-title>
          {{ t("Edit setting {label}", { label: candidates[editing.index]?.letter }) }}
        </v-card-title>
        <v-divider />
        <v-card-text>
          <ProcessingControls
            :params="editing.params"
            :show-presets="false"
            :show-layout="false"
            @update:params="editing.params = { ...editing.params, ...$event }"
          />
        </v-card-text>
        <v-divider />
        <v-card-actions>
          <v-spacer />
          <v-btn variant="text" @click="editing = null">{{ t("Cancel") }}</v-btn>
          <v-btn color="primary" variant="flat" @click="saveEdit">{{ t("Save") }}</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <SavePresetDialog
      :open="!!saving"
      :params="saving ? processingOf(saving.params) : null"
      :suggested-name="saving ? suggestedName(saving) : ''"
      source="tuning"
      :updatable="updatable"
      @close="saving = null"
      @saved="onSaved"
    />
  </div>
</template>

<style scoped>
.tuning {
  max-width: 820px;
}
.head {
  display: flex;
  align-items: center;
  gap: 4px;
  margin-left: -12px;
}
.page-title {
  font-size: 1.5rem;
  font-weight: 600;
}
.intro {
  max-width: 640px;
  margin-top: 4px;
}
.section-title {
  font-size: 1.05rem;
  font-weight: 600;
  margin: 32px 0 12px;
}
.section-head {
  display: flex;
  align-items: center;
  gap: 4px;
}
.base-select {
  max-width: 320px;
}
.candidate-list {
  padding: 0;
}
.letter {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  flex: none;
  width: 36px;
  height: 36px;
  margin-right: 16px;
  border-radius: 8px;
  font-weight: 700;
  border: 2px solid rgba(var(--v-theme-on-surface), 0.7);
}
.start-row {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 12px;
  margin-top: 16px;
}
.dwell-select {
  max-width: 200px;
}
.now-card {
  display: flex;
  gap: 20px;
  align-items: flex-start;
  padding: 16px;
  border-radius: 12px;
  border: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
}
.now-letter {
  flex: none;
  width: 88px;
  height: 88px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 3rem;
  font-weight: 700;
  border-radius: 12px;
  border: 3px solid rgba(var(--v-theme-on-surface), 0.8);
}
.now-letter--out {
  opacity: 0.35;
  text-decoration: line-through;
}
.now-body {
  flex: 1;
  min-width: 0;
}
.now-controls {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 4px;
}
.letter-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(48px, 1fr));
  gap: 8px;
}
.letter-chip {
  height: 48px;
  border-radius: 8px;
  font-size: 1.1rem;
  font-weight: 700;
  color: inherit;
  border: 1px solid rgba(var(--v-theme-on-surface), 0.4);
  background: transparent;
  cursor: pointer;
}
.letter-chip--current {
  outline: 2px solid rgb(var(--v-theme-on-surface));
  outline-offset: 1px;
}
.letter-chip--out {
  opacity: 0.35;
  text-decoration: line-through;
  border-style: dashed;
}
.round-actions {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px;
  margin-top: 20px;
}
.final-card {
  margin-top: 24px;
  padding: 16px;
  border-radius: 12px;
  border: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
}
.final-grid {
  display: grid;
  gap: 12px;
}
.final-item {
  display: flex;
  align-items: center;
  gap: 4px;
}
.final-text {
  flex: 1;
  min-width: 0;
}
@media (max-width: 600px) {
  .now-card {
    flex-direction: column;
    align-items: stretch;
  }
  .now-letter {
    width: 64px;
    height: 64px;
    font-size: 2.2rem;
  }
  .final-item {
    flex-wrap: wrap;
  }
}
</style>
