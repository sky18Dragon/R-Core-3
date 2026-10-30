#include "upload_sweep.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SWEEP_PREFIX ".rc3-upload-"
#define SWEEP_SUFFIX ".part"
#define SWEEP_PATH_MAX 300

static bool hex_run(const char *p, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        char c = p[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    return true;
}

bool upload_sweep_is_temp_name(const char *leaf)
{
    const size_t prefix = sizeof(SWEEP_PREFIX) - 1, suffix = sizeof(SWEEP_SUFFIX) - 1;
    if (!leaf || strlen(leaf) != prefix + 8 + 1 + 8 + suffix) return false;
    const char *p = leaf + prefix;
    return !strncmp(leaf, SWEEP_PREFIX, prefix) && hex_run(p, 8) && p[8] == '-' &&
           hex_run(p + 9, 8) && !strcmp(p + 17, SWEEP_SUFFIX);
}

static bool join(char *out, const char *a, const char *b)
{
    int n = snprintf(out, SWEEP_PATH_MAX, "%s/%s", a, b);
    return n > 0 && n < SWEEP_PATH_MAX;
}

static void sweep_album(const char *album, unsigned max, upload_sweep_result_t *r)
{
    DIR *dir = opendir(album);
    if (!dir) return;
    struct dirent *entry;
    char path[SWEEP_PATH_MAX];
    while ((entry = readdir(dir)) != NULL) {
        if (!upload_sweep_is_temp_name(entry->d_name) || !join(path, album, entry->d_name)) continue;
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        if (r->removed >= max) {
            r->truncated = true;
            break;
        }
        if (unlink(path) == 0) {
            ++r->removed;
            r->bytes += (uint64_t) st.st_size;
        } else {
            ++r->failed;
        }
    }
    closedir(dir);
}

int upload_sweep_orphans(const char *root, unsigned max_removals, upload_sweep_result_t *out)
{
    upload_sweep_result_t r = {0};
    if (out) *out = r;
    DIR *dir = root ? opendir(root) : NULL;
    if (!dir) return -1;
    struct dirent *entry;
    char album[SWEEP_PATH_MAX];
    while (!r.truncated && (entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.' || !join(album, root, entry->d_name)) continue;
        struct stat st;
        if (stat(album, &st) != 0 || !S_ISDIR(st.st_mode)) continue;
        sweep_album(album, max_removals, &r);
    }
    closedir(dir);
    if (out) *out = r;
    return 0;
}
