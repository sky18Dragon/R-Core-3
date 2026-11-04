#include "photo_meta.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PHOTO_META_PATH_MAX 300

static int normalize(int degrees) { return ((degrees % 360) + 360) % 360; }

bool photo_meta_up_valid(int up) { return up == 0 || up == 90 || up == 180 || up == 270; }

int photo_meta_up_for_upload(int rotation, bool portrait_source)
{
    int turned = normalize(rotation);
    if (portrait_source && (turned == 90 || turned == 270)) return normalize(360 - turned);
    return 0;
}

int photo_meta_turn(int up, int device) { return normalize(up - device); }

cJSON *photo_meta_parse(const char *text)
{
    cJSON *index = text ? cJSON_Parse(text) : NULL;
    if (!cJSON_IsObject(index)) {
        cJSON_Delete(index);
        index = cJSON_CreateObject();
    }
    return index;
}

static const cJSON *entry_of(const cJSON *index, const char *name)
{
    if (!index || !name) return NULL;
    const cJSON *entry = cJSON_GetObjectItemCaseSensitive(index, name);
    return cJSON_IsObject(entry) ? entry : NULL;
}

int photo_meta_up(const cJSON *index, const char *name)
{
    const cJSON *up = cJSON_GetObjectItemCaseSensitive(entry_of(index, name), "up");
    return cJSON_IsNumber(up) && photo_meta_up_valid(up->valueint) ? up->valueint : 0;
}

int64_t photo_meta_added(const cJSON *index, const char *name)
{
    const cJSON *added = cJSON_GetObjectItemCaseSensitive(entry_of(index, name), "added");
    return cJSON_IsNumber(added) && added->valuedouble > 0 ? (int64_t) added->valuedouble : 0;
}

bool photo_meta_set(cJSON *index, const char *name, int up, int64_t added_if_new)
{
    if (!cJSON_IsObject(index) || !name || !name[0] || !photo_meta_up_valid(up)) return false;
    int64_t added = photo_meta_added(index, name);
    if (!added) added = added_if_new > 0 ? added_if_new : 0;
    cJSON *entry = cJSON_CreateObject();
    if (!entry || !cJSON_AddNumberToObject(entry, "up", up) ||
        (added && !cJSON_AddNumberToObject(entry, "added", (double) added))) {
        cJSON_Delete(entry);
        return false;
    }
    if (cJSON_GetObjectItemCaseSensitive(index, name))
        return cJSON_ReplaceItemInObjectCaseSensitive(index, name, entry);
    return cJSON_AddItemToObject(index, name, entry);
}

void photo_meta_forget(cJSON *index, const char *name)
{
    if (index && name) cJSON_DeleteItemFromObjectCaseSensitive(index, name);
}

static bool album_file(char *out, size_t capacity, const char *album_dir, const char *suffix)
{
    int n = snprintf(out, capacity, "%s/" PHOTO_META_FILENAME "%s", album_dir, suffix);
    return n > 0 && (size_t) n < capacity;
}

static char *read_small_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    char *text = NULL;
    if (fseek(file, 0, SEEK_END) == 0) {
        long size = ftell(file);
        if (size >= 0 && size <= PHOTO_META_MAX_BYTES && fseek(file, 0, SEEK_SET) == 0) {
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

cJSON *photo_meta_load(const char *album_dir)
{
    char path[PHOTO_META_PATH_MAX];
    char *text = NULL;
    if (album_dir && album_file(path, sizeof(path), album_dir, "")) text = read_small_file(path);
    if (!text && album_dir && album_file(path, sizeof(path), album_dir, ".old"))
        text = read_small_file(path);
    cJSON *index = photo_meta_parse(text);
    free(text);
    return index;
}

bool photo_meta_save(const char *album_dir, const cJSON *index)
{
    char current[PHOTO_META_PATH_MAX], fresh[PHOTO_META_PATH_MAX], old[PHOTO_META_PATH_MAX];
    if (!album_dir || !cJSON_IsObject(index) || !album_file(current, sizeof(current), album_dir, "") ||
        !album_file(fresh, sizeof(fresh), album_dir, ".new") ||
        !album_file(old, sizeof(old), album_dir, ".old"))
        return false;
    char *text = cJSON_PrintUnformatted(index);
    if (!text) return false;
    size_t length = strlen(text);
    bool ok = length <= PHOTO_META_MAX_BYTES;
    FILE *file = ok ? fopen(fresh, "wb") : NULL;
    ok = file && fwrite(text, 1, length, file) == length;
    if (file && fclose(file) != 0) ok = false;
    free(text);
    if (!ok) {
        remove(fresh);
        return false;
    }
    // FAT cannot rename over an existing file: park the current index as ".old"
    // so a power cut at any point leaves one complete copy for photo_meta_load.
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

int photo_meta_lookup_up(const char *image_path)
{
    const char *slash = image_path ? strrchr(image_path, '/') : NULL;
    if (!slash || !slash[1] || (size_t) (slash - image_path) >= PHOTO_META_PATH_MAX - 32) return 0;
    char dir[PHOTO_META_PATH_MAX];
    memcpy(dir, image_path, (size_t) (slash - image_path));
    dir[slash - image_path] = '\0';
    cJSON *index = photo_meta_load(dir);
    int up = photo_meta_up(index, slash + 1);
    cJSON_Delete(index);
    return up;
}

void photo_meta_rotate180_4bpp(uint8_t *frame, size_t bytes)
{
    if (!frame) return;
    for (size_t i = 0, j = bytes; i < j--; ++i) {
        uint8_t a = frame[i], b = frame[j];
        frame[i] = (uint8_t) ((b << 4) | (b >> 4));
        frame[j] = (uint8_t) ((a << 4) | (a >> 4)); // a middle byte (i == j) swaps once
    }
}
