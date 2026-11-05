import { ref } from "vue";

// Where the page is: "photos" (home, every photo grouped by album), "albums"
// (the album list, or one album when `album` is set), "slideshow" and
// "presets" (the processing preset library; "#/tuning" from earlier versions
// lands there too). The URL
// hash is the source of truth (#/albums/Trips), so back, refresh and bookmarks
// work. A hash is used rather than a path because the device serves the page
// only at "/".
export const VIEWS = ["photos", "albums", "slideshow", "presets"];

export function routeFromHash(hash) {
  const parts = String(hash || "")
    .replace(/^#\/?/, "")
    .split("/");
  if (parts[0] === "albums") {
    let album = null;
    if (parts.length > 1 && parts[1]) {
      try {
        album = decodeURIComponent(parts.slice(1).join("/"));
      } catch {
        album = null;
      }
    }
    return { view: "albums", album };
  }
  if (parts[0] === "slideshow") return { view: "slideshow", album: null };
  if (parts[0] === "presets" || parts[0] === "tuning") return { view: "presets", album: null };
  return { view: "photos", album: null };
}

export function hashForRoute(view, album = null) {
  if (view === "albums") return album ? `#/albums/${encodeURIComponent(album)}` : "#/albums";
  if (view === "slideshow") return "#/slideshow";
  if (view === "presets") return "#/presets";
  return "#/";
}

const initial =
  typeof window === "undefined"
    ? { view: "photos", album: null }
    : routeFromHash(window.location.hash);

export const currentView = ref(initial.view);
// The open album on the albums view; null shows the album list.
export const currentAlbum = ref(initial.album);

if (typeof window !== "undefined") {
  window.addEventListener("hashchange", () => {
    const route = routeFromHash(window.location.hash);
    currentView.value = route.view;
    currentAlbum.value = route.album;
  });
}

export function goTo(view, album = null) {
  if (!VIEWS.includes(view)) return;
  const target = view === "albums" ? album : null;
  const hash = hashForRoute(view, target);
  if (window.location.hash !== hash) window.location.hash = hash;
  currentView.value = view;
  currentAlbum.value = target;
}

export function openAlbum(name) {
  goTo("albums", name);
}
