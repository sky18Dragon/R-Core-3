#pragma once
#include <stdbool.h>
#include "cJSON.h"
#ifdef __cplusplus
extern "C" {
#endif
// The user's processing presets (docs/PRD-PRESETS.md): named sets of image
// processing values kept on the storage card as one small JSON document,
//   {"version": 1, "rev": 7, "default": "p2" | null,
//    "presets": [{"id", "name", "source", "params": {...}, "created", "updated"}]}
// The page reads and writes the whole document; `rev` guards against two
// browsers overwriting each other. Portable C (cJSON + stdio) for host tests.
#define PRESET_STORE_FILENAME ".presets.json"
#define PRESET_STORE_MAX_BYTES (24 * 1024)
#define PRESET_STORE_MAX_PRESETS 20
#define PRESET_STORE_ID_MAX 32    // bytes, without the terminator
#define PRESET_STORE_NAME_MAX 64  // bytes of UTF-8, without the terminator

// An empty document: no presets, rev 0.
cJSON *preset_store_empty(void);

// A checked copy of a document sent by the page, keeping only known fields;
// NULL when anything is out of bounds (`error` says what, for the response).
// The copy carries the client's `rev` unchanged.
cJSON *preset_store_normalize(const cJSON *doc, const char **error);

int preset_store_rev(const cJSON *doc);
void preset_store_set_rev(cJSON *doc, int rev);

// Storage directory store. Loading never returns NULL: a missing or unreadable
// file gives an empty document (falling back to ".old" after an interrupted
// save). Saving writes ".new", parks the current file as ".old", then swaps.
cJSON *preset_store_load(const char *dir);
bool preset_store_save(const char *dir, const cJSON *doc);
#ifdef __cplusplus
}
#endif
