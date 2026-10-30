#pragma once
// Host uses the locked esp_jpeg 1.3.1 external decoder, not ESP32-S3 ROM.
#define CONFIG_JD_SZBUF 512
#define CONFIG_JD_FORMAT 0
#define CONFIG_JD_USE_SCALE 1
#define CONFIG_JD_TBLCLIP 1
#define CONFIG_JD_FASTDECODE 0
