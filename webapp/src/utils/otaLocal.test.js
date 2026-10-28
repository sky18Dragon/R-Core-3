import { describe, expect, it } from "vitest";
import { HEADER_BYTES, inspectFirmware } from "./otaLocal.js";

function image({ version = "v1.0+rplf", project = "esp32-photoframe", chip = 9, magic = 0xabcd5432 } = {}) {
  const bytes = new Uint8Array(256);
  const view = new DataView(bytes.buffer);
  bytes[0] = 0xe9;
  view.setUint16(12, chip, true);
  view.setUint32(32, magic, true);
  bytes.set(new TextEncoder().encode(version), 48);
  bytes.set(new TextEncoder().encode(project), 80);
  return bytes;
}

describe("inspectFirmware", () => {
  it("accepts a marked local-frame image and returns its version", () => {
    expect(inspectFirmware(image({ version: "dev-93b65f6+rplf" }), 1978880, 0x300000)).toEqual({
      ok: true,
      version: "dev-93b65f6+rplf",
    });
  });

  it("refuses each mismatch with a translatable reason", () => {
    const ok = image();
    expect(inspectFirmware(ok.subarray(0, HEADER_BYTES - 1), 2e6, 0x300000).ok).toBe(false);
    expect(inspectFirmware(ok, 0x300001, 0x300000).reason).toBe("Firmware is larger than the update slot");
    const badMagic = image();
    badMagic[0] = 0;
    expect(inspectFirmware(badMagic, 2e6, 0x300000).reason).toBe("Not a firmware image");
    expect(inspectFirmware(image({ chip: 0 }), 2e6, 0x300000).reason).toBe("Firmware is not for ESP32-S3");
    expect(inspectFirmware(image({ magic: 1 }), 2e6, 0x300000).reason).toBe("Not a firmware image");
    expect(inspectFirmware(image({ project: "other" }), 2e6, 0x300000).reason).toBe(
      "Firmware is not for this photo frame"
    );
    expect(inspectFirmware(image({ version: "v1.0" }), 2e6, 0x300000).reason).toBe(
      "Firmware is not a Read Pico local-frame build"
    );
    expect(inspectFirmware(image({ version: "a".repeat(32) }), 2e6, 0x300000).ok).toBe(false);
  });
});
