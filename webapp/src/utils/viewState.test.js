import { describe, it, expect } from "vitest";
import { viewFromHash } from "./viewState";

describe("viewFromHash", () => {
  it("maps the hash to a view, defaulting to upload", () => {
    expect(viewFromHash("#/albums")).toBe("albums");
    expect(viewFromHash("#albums")).toBe("albums");
    expect(viewFromHash("#/")).toBe("upload");
    expect(viewFromHash("")).toBe("upload");
    expect(viewFromHash("#/unknown")).toBe("upload");
    expect(viewFromHash(undefined)).toBe("upload");
  });
});
