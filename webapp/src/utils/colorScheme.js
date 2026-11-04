import { ref } from "vue";

// Follows the system light/dark setting, live.
const query =
  typeof window !== "undefined" && window.matchMedia
    ? window.matchMedia("(prefers-color-scheme: dark)")
    : null;

export const prefersDark = ref(!!query?.matches);

query?.addEventListener?.("change", (event) => {
  prefersDark.value = event.matches;
});
