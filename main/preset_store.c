#include "preset_store.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRESET_STORE_PATH_MAX 300
#define PRESET_STORE_VERSION 1

cJSON *preset_store_empty(void)
{
    cJSON *doc = cJSON_CreateObject();
    if (!doc) return NULL;
    if (!cJSON_AddNumberToObject(doc, "version", PRESET_STORE_VERSION) ||
        !cJSON_AddNumberToObject(doc, "rev", 0) || !cJSON_AddNullToObject(doc, "default") ||
        !cJSON_AddArrayToObject(doc, "presets")) {
        cJSON_Delete(doc);
        return NULL;
    }
    return doc;
}

int preset_store_rev(const cJSON *doc)
{
    const cJSON *rev = cJSON_GetObjectItemCaseSensitive(doc, "rev");
    return cJSON_IsNumber(rev) && rev->valuedouble >= 0 && rev->valuedouble < 2e9 ? rev->valueint
                                                                                 : 0;
}

void preset_store_set_rev(cJSON *doc, int rev)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(doc, "rev");
    if (item) cJSON_SetNumberValue(item, rev);
    else cJSON_AddNumberToObject(doc, "rev", rev);
}

static bool text_ok(const cJSON *item, size_t max)
{
    if (!cJSON_IsString(item) || !item->valuestring) return false;
    size_t length = strlen(item->valuestring);
    return length > 0 && length <= max;
}

static bool one_of(const char *value, const char *const *allowed)
{
    for (size_t i = 0; allowed[i]; ++i)
        if (!strcmp(value, allowed[i])) return true;
    return false;
}

// Processing values: finite numbers in a generous range (the page's sliders
// stay well inside it), the known modes and algorithms, and one switch.
static cJSON *checked_params(const cJSON *params, const char **error)
{
    static const char *numbers[] = {"exposure",  "saturation",        "contrast", "strength",
                                    "shadowBoost", "highlightCompress", "midpoint"};
    static const char *tone[] = {"contrast", "scurve", NULL};
    static const char *color[] = {"rgb", "lab", NULL};
    static const char *dither[] = {"floyd-steinberg", "stucki", "burkes", "sierra", NULL};
    static const struct {
        const char *key;
        const char *const *allowed;
    } strings[] = {{"toneMode", tone}, {"colorMethod", color}, {"ditherAlgorithm", dither}};

    if (!cJSON_IsObject(params)) {
        *error = "Preset params must be an object";
        return NULL;
    }
    cJSON *out = cJSON_CreateObject();
    if (!out) return NULL;
    for (size_t i = 0; i < sizeof(numbers) / sizeof(numbers[0]); ++i) {
        const cJSON *item = cJSON_GetObjectItemCaseSensitive(params, numbers[i]);
        if (!item) continue;
        if (!cJSON_IsNumber(item) || !isfinite(item->valuedouble) || item->valuedouble < 0 ||
            item->valuedouble > 10) {
            *error = "Preset number out of range";
            cJSON_Delete(out);
            return NULL;
        }
        cJSON_AddNumberToObject(out, numbers[i], item->valuedouble);
    }
    for (size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); ++i) {
        const cJSON *item = cJSON_GetObjectItemCaseSensitive(params, strings[i].key);
        if (!item) continue;
        if (!cJSON_IsString(item) || !item->valuestring ||
            !one_of(item->valuestring, strings[i].allowed)) {
            *error = "Preset mode or algorithm not supported";
            cJSON_Delete(out);
            return NULL;
        }
        cJSON_AddStringToObject(out, strings[i].key, item->valuestring);
    }
    const cJSON *compress = cJSON_GetObjectItemCaseSensitive(params, "compressDynamicRange");
    if (compress) {
        if (!cJSON_IsBool(compress)) {
            *error = "compressDynamicRange must be true or false";
            cJSON_Delete(out);
            return NULL;
        }
        cJSON_AddBoolToObject(out, "compressDynamicRange", cJSON_IsTrue(compress));
    }
    return out;
}

static cJSON *checked_preset(const cJSON *preset, const char **error)
{
    static const char *sources[] = {"tuning", "manual", "copy", "migrated", NULL};
    const cJSON *id = cJSON_GetObjectItemCaseSensitive(preset, "id");
    const cJSON *name = cJSON_GetObjectItemCaseSensitive(preset, "name");
    if (!cJSON_IsObject(preset) || !text_ok(id, PRESET_STORE_ID_MAX) ||
        !text_ok(name, PRESET_STORE_NAME_MAX)) {
        *error = "Each preset needs an id (1-32 bytes) and a name (1-64 bytes)";
        return NULL;
    }
    cJSON *params = checked_params(cJSON_GetObjectItemCaseSensitive(preset, "params"), error);
    if (!params) return NULL;
    cJSON *out = cJSON_CreateObject();
    if (!out) {
        cJSON_Delete(params);
        return NULL;
    }
    cJSON_AddStringToObject(out, "id", id->valuestring);
    cJSON_AddStringToObject(out, "name", name->valuestring);
    const cJSON *source = cJSON_GetObjectItemCaseSensitive(preset, "source");
    if (cJSON_IsString(source) && source->valuestring && one_of(source->valuestring, sources))
        cJSON_AddStringToObject(out, "source", source->valuestring);
    cJSON_AddItemToObject(out, "params", params);
    const char *times[] = {"created", "updated"};
    for (size_t i = 0; i < 2; ++i) {
        const cJSON *time = cJSON_GetObjectItemCaseSensitive(preset, times[i]);
        if (cJSON_IsNumber(time) && time->valuedouble > 0 && time->valuedouble < 1e11)
            cJSON_AddNumberToObject(out, times[i], floor(time->valuedouble));
    }
    return out;
}

cJSON *preset_store_normalize(const cJSON *doc, const char **error)
{
    const char *unused;
    if (!error) error = &unused;
    *error = NULL;
    const cJSON *presets = cJSON_GetObjectItemCaseSensitive(doc, "presets");
    if (!cJSON_IsObject(doc) || !cJSON_IsArray(presets)) {
        *error = "Need an object with a presets array";
        return NULL;
    }
    if (cJSON_GetArraySize(presets) > PRESET_STORE_MAX_PRESETS) {
        *error = "At most 20 presets";
        return NULL;
    }
    cJSON *out = preset_store_empty();
    if (!out) return NULL;
    preset_store_set_rev(out, preset_store_rev(doc));
    cJSON *list = cJSON_GetObjectItemCaseSensitive(out, "presets");
    const cJSON *preset;
    cJSON_ArrayForEach(preset, presets)
    {
        cJSON *checked = checked_preset(preset, error);
        if (!checked) {
            cJSON_Delete(out);
            return NULL;
        }
        const char *id = cJSON_GetObjectItemCaseSensitive(checked, "id")->valuestring;
        const cJSON *seen;
        cJSON_ArrayForEach(seen, list)
        {
            if (!strcmp(cJSON_GetObjectItemCaseSensitive(seen, "id")->valuestring, id)) {
                *error = "Preset ids must be unique";
                cJSON_Delete(checked);
                cJSON_Delete(out);
                return NULL;
            }
        }
        cJSON_AddItemToArray(list, checked);
    }
    const cJSON *def = cJSON_GetObjectItemCaseSensitive(doc, "default");
    if (def && !cJSON_IsNull(def)) {
        bool found = false;
        const cJSON *item;
        cJSON_ArrayForEach(item, list)
        {
            if (cJSON_IsString(def) &&
                !strcmp(cJSON_GetObjectItemCaseSensitive(item, "id")->valuestring,
                        def->valuestring))
                found = true;
        }
        if (!found) {
            *error = "The default must name one of the presets";
            cJSON_Delete(out);
            return NULL;
        }
        cJSON_ReplaceItemInObjectCaseSensitive(out, "default", cJSON_CreateString(def->valuestring));
    }
    return out;
}

static bool store_file(char *out, size_t capacity, const char *dir, const char *suffix)
{
    int n = snprintf(out, capacity, "%s/" PRESET_STORE_FILENAME "%s", dir, suffix);
    return n > 0 && (size_t) n < capacity;
}

static char *read_small_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    char *text = NULL;
    if (fseek(file, 0, SEEK_END) == 0) {
        long size = ftell(file);
        if (size > 0 && size <= PRESET_STORE_MAX_BYTES && fseek(file, 0, SEEK_SET) == 0) {
            text = malloc((size_t) size + 1);
            if (text && fread(text, 1, (size_t) size, file) == (size_t) size) {
                text[size] = '\0';
            } else {
                free(text);
                text = NULL;
            }
        }
    }
    fclose(file);
    return text;
}

static cJSON *parse_stored(const char *text)
{
    cJSON *raw = text ? cJSON_Parse(text) : NULL;
    // A stored document is checked like a sent one; anything unreadable is empty
    cJSON *doc = raw ? preset_store_normalize(raw, NULL) : NULL;
    cJSON_Delete(raw);
    return doc;
}

cJSON *preset_store_load(const char *dir)
{
    char path[PRESET_STORE_PATH_MAX];
    cJSON *doc = NULL;
    for (int attempt = 0; attempt < 2 && !doc; ++attempt) {
        if (!dir || !store_file(path, sizeof(path), dir, attempt ? ".old" : "")) break;
        char *text = read_small_file(path);
        doc = parse_stored(text);
        free(text);
    }
    return doc ? doc : preset_store_empty();
}

bool preset_store_save(const char *dir, const cJSON *doc)
{
    char current[PRESET_STORE_PATH_MAX], fresh[PRESET_STORE_PATH_MAX], old[PRESET_STORE_PATH_MAX];
    if (!dir || !cJSON_IsObject(doc) || !store_file(current, sizeof(current), dir, "") ||
        !store_file(fresh, sizeof(fresh), dir, ".new") || !store_file(old, sizeof(old), dir, ".old"))
        return false;
    char *text = cJSON_PrintUnformatted(doc);
    if (!text) return false;
    size_t length = strlen(text);
    bool ok = length <= PRESET_STORE_MAX_BYTES;
    FILE *file = ok ? fopen(fresh, "wb") : NULL;
    ok = file && fwrite(text, 1, length, file) == length;
    if (file && fclose(file) != 0) ok = false;
    free(text);
    if (!ok) {
        remove(fresh);
        return false;
    }
    // FAT cannot rename over an existing file: park the current document as
    // ".old" so a power cut at any point leaves one complete copy to load.
    remove(old);
    bool had_current = rename(current, old) == 0;
    if (rename(fresh, current) != 0) {
        if (had_current) rename(old, current);
        remove(fresh);
        return false;
    }
    remove(old);
    return true;
}
