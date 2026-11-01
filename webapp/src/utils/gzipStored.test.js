// Stored-block gzip fallback (Safari < 16.4 has no CompressionStream) must be
// readable by zlib, which is what the frame uses to open .epdgz files.
import { describe, expect, it } from "vitest";
import { gunzipSync } from "node:zlib";
import { gzipStored } from "@aitjcize/epaper-image-convert";

function bytes(length) {
  const out = new Uint8Array(length);
  for (let i = 0; i < length; i++) out[i] = (i * 31 + (i >> 7)) & 0xff;
  return out;
}

describe("gzipStored", () => {
  it.each([0, 1, 65535, 65536, 131071, 415872])("round-trips %i bytes through zlib", (n) => {
    const input = bytes(n);
    const gz = gzipStored(input);
    expect(gz[0]).toBe(0x1f);
    expect(gz[1]).toBe(0x8b);
    expect(new Uint8Array(gunzipSync(gz))).toEqual(input);
    expect(gz.length).toBe(10 + n + Math.max(1, Math.ceil(n / 65535)) * 5 + 8);
  });
});
