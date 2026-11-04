import { describe, it, expect } from "vitest";
import { routeFromHash, hashForRoute } from "./viewState";

describe("routeFromHash", () => {
  it("maps the hash to a view, defaulting to photos", () => {
    expect(routeFromHash("#/albums")).toEqual({ view: "albums", album: null });
    expect(routeFromHash("#albums")).toEqual({ view: "albums", album: null });
    expect(routeFromHash("#/slideshow")).toEqual({ view: "slideshow", album: null });
    expect(routeFromHash("#/")).toEqual({ view: "photos", album: null });
    expect(routeFromHash("")).toEqual({ view: "photos", album: null });
    expect(routeFromHash("#/unknown")).toEqual({ view: "photos", album: null });
    expect(routeFromHash(undefined)).toEqual({ view: "photos", album: null });
  });

  it("reads an album name, including encoded and odd characters", () => {
    expect(routeFromHash("#/albums/Trips")).toEqual({ view: "albums", album: "Trips" });
    expect(routeFromHash("#/albums/" + encodeURIComponent("家 2024/夏"))).toEqual({
      view: "albums",
      album: "家 2024/夏",
    });
    expect(routeFromHash("#/albums/%E0%A4%A")).toEqual({ view: "albums", album: null });
  });

  it("round-trips with hashForRoute", () => {
    for (const [view, album] of [
      ["photos", null],
      ["albums", null],
      ["albums", "Default"],
      ["albums", "a b&c#d"],
      ["slideshow", null],
    ]) {
      expect(routeFromHash(hashForRoute(view, album))).toEqual({ view, album });
    }
  });
});
