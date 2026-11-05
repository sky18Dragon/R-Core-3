import { defineStore } from "pinia";
import { computed, ref } from "vue";
import { t } from "../i18n";
import { useSettingsStore } from "./settings.js";
import {
  addPreset,
  duplicateName,
  emptyDoc,
  migratedDoc,
  presetById,
  processingOf,
  removePreset,
  sameProcessingValues,
  setDefaultPreset,
  updatePreset,
} from "../utils/presets";

// The preset library on the frame (docs/PRD-PRESETS.md). Every change sends
// the whole document; when another browser changed it in between, the frame
// answers with its copy and the change is applied to that once more.
export const usePresetsStore = defineStore("presets", () => {
  const settingsStore = useSettingsStore();

  const doc = ref(emptyDoc());
  const loaded = ref(false);
  const available = ref(true); // false: no storage card, presets cannot be kept
  const saving = ref(false);

  const list = computed(() => doc.value.presets);
  const defaultId = computed(() => doc.value.default);
  const defaultPreset = computed(() => presetById(doc.value, doc.value.default));
  // The frame's default processing no longer matches the default preset (it
  // was changed some other way)
  const defaultModified = computed(
    () =>
      !!defaultPreset.value &&
      settingsStore.processingLoaded &&
      !sameProcessingValues(defaultPreset.value.params, settingsStore.params)
  );

  function byId(id) {
    return presetById(doc.value, id);
  }

  async function load() {
    try {
      const response = await fetch("/api/presets", { cache: "no-store" });
      if (response.status === 404) {
        available.value = false;
        return;
      }
      if (!response.ok) return;
      doc.value = await response.json();
      available.value = true;
      loaded.value = true;
    } catch {
      // Frame unreachable: keep what we have
    }
  }

  async function put(next) {
    const response = await fetch("/api/presets", {
      method: "PUT",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(next),
    });
    const body = await response.json().catch(() => null);
    return { status: response.status, body };
  }

  // Apply `change` (doc → doc) and save it; true when the frame kept it
  async function mutate(change) {
    saving.value = true;
    try {
      for (let attempt = 0; attempt < 2; attempt++) {
        const next = change(doc.value);
        if (next === doc.value) return false;
        const { status, body } = await put(next);
        if (status === 200 && body) {
          doc.value = body;
          return true;
        }
        if (status === 409 && body) {
          doc.value = body; // someone else saved first: redo on their copy
          continue;
        }
        return false;
      }
      return false;
    } catch {
      return false;
    } finally {
      saving.value = false;
    }
  }

  // A frame with no presets gets one from what it already uses
  async function ensureMigrated() {
    if (!loaded.value || !available.value || doc.value.presets.length) return;
    if (!settingsStore.processingLoaded) return;
    await mutate((d) => migratedDoc(d, settingsStore.params, t("My default")));
  }

  async function create({ name, params, source = "manual", makeDefault = false }) {
    let id = null;
    const ok = await mutate((d) => {
      const [next, newId] = addPreset(d, { name, params, source });
      id = newId;
      return makeDefault && newId ? setDefaultPreset(next, newId) : next;
    });
    if (ok && makeDefault) await applyDefaultToFrame();
    return ok ? id : null;
  }

  async function update(id, { name, params }) {
    const ok = await mutate((d) => updatePreset(d, id, { name, params }));
    if (ok && params && id === defaultId.value) await applyDefaultToFrame();
    return ok;
  }

  async function duplicate(id) {
    const source = byId(id);
    if (!source) return null;
    return create({
      name: duplicateName(doc.value, source.name, t("copy")),
      params: source.params,
      source: "copy",
    });
  }

  async function remove(id) {
    return mutate((d) => removePreset(d, id));
  }

  // The default's values become the frame's own processing default (the
  // layout stays as it is)
  async function applyDefaultToFrame() {
    const preset = defaultPreset.value;
    if (!preset) return false;
    Object.assign(settingsStore.params, preset.params);
    return settingsStore.saveSettings();
  }

  async function setDefault(id) {
    const ok = await mutate((d) => setDefaultPreset(d, id));
    return ok && (await applyDefaultToFrame());
  }

  // Processing values to render with for a preset id (the frame's default
  // processing when the id is unknown or presets are unavailable)
  function paramsFor(id) {
    const preset = byId(id) ?? defaultPreset.value;
    return preset ? { ...preset.params } : processingOf(settingsStore.params);
  }

  return {
    doc,
    loaded,
    available,
    saving,
    list,
    defaultId,
    defaultPreset,
    defaultModified,
    byId,
    load,
    ensureMigrated,
    create,
    update,
    duplicate,
    remove,
    setDefault,
    applyDefaultToFrame,
    paramsFor,
  };
});
