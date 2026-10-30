import { ref } from "vue";

// The two main views of the page: "upload" (home) and "albums". The URL hash is
// the source of truth (#/albums), so back, refresh and bookmarks work. A hash is
// used rather than a path because the device serves the page only at "/".
export const VIEWS = ["upload", "albums"];

export function viewFromHash(hash) {
  return String(hash || "").replace(/^#\/?/, "") === "albums" ? "albums" : "upload";
}

export const currentView = ref(
  typeof window === "undefined" ? "upload" : viewFromHash(window.location.hash)
);

if (typeof window !== "undefined") {
  window.addEventListener("hashchange", () => {
    currentView.value = viewFromHash(window.location.hash);
  });
}

export function goTo(view) {
  if (!VIEWS.includes(view)) return;
  const hash = view === "albums" ? "#/albums" : "#/";
  if (window.location.hash !== hash) window.location.hash = hash;
  currentView.value = view;
}
