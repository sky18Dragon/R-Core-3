import { describe, it, expect } from "vitest";
import {
  makeBaseName,
  classifyUploadFailure,
  isRetryableFailure,
  isImageFile,
} from "./uploadHelpers";

describe("makeBaseName", () => {
  it("is 12 chars, stable for the same input and differs by name or time", () => {
    const a = makeBaseName("IMG_1234.jpg", 1761800000000);
    expect(a).toMatch(/^[0-9a-f]{8}[0-9a-z]{4}$/);
    expect(makeBaseName("IMG_1234.jpg", 1761800000000)).toBe(a);
    expect(makeBaseName("IMG_1235.jpg", 1761800000000)).not.toBe(a);
    expect(makeBaseName("IMG_1234.jpg", 1761800000001)).not.toBe(a);
  });
  it("ignores the extension", () => {
    expect(makeBaseName("a.jpg", 5)).toBe(makeBaseName("a.png", 5));
  });
});

describe("classifyUploadFailure", () => {
  it("maps statuses and device messages", () => {
    expect(classifyUploadFailure(0)).toBe("unreachable");
    expect(classifyUploadFailure(400, "Invalid or oversized multipart upload")).toBe("too-large");
    expect(classifyUploadFailure(400, "Unsupported image format")).toBe("unsupported");
    expect(classifyUploadFailure(400, "Upload file signature mismatch")).toBe("unsupported");
    expect(classifyUploadFailure(415)).toBe("unsupported");
    expect(classifyUploadFailure(503, "System is still initializing")).toBe("busy");
    expect(classifyUploadFailure(500)).toBe("device");
    expect(classifyUploadFailure(401)).toBe("auth");
    expect(classifyUploadFailure(404)).toBe("other");
  });
});

describe("isRetryableFailure", () => {
  it("retries busy/device errors only", () => {
    expect(isRetryableFailure("busy")).toBe(true);
    expect(isRetryableFailure("device")).toBe(true);
    expect(isRetryableFailure("unsupported")).toBe(false);
    expect(isRetryableFailure("unreachable")).toBe(false);
  });
});

describe("isImageFile", () => {
  it("accepts by mime type or by extension", () => {
    expect(isImageFile({ type: "image/jpeg", name: "a" })).toBe(true);
    expect(isImageFile({ type: "", name: "IMG_1.HEIC" })).toBe(true);
    expect(isImageFile({ type: "application/pdf", name: "a.pdf" })).toBe(false);
  });
});
