#include "read_pico_upload.h"

#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "esp_random.h"
#include "read_pico_sd.h"

static atomic_uint sequence;
static bool owned_path(const char *path)
{
    const char *root = READ_PICO_SD_MOUNT_POINT;
    size_t n = strlen(root);
    if (!path || strncmp(path, root, n) || (path[n] && path[n] != '/') || strchr(path, '\\'))
        return false;
    for (const char *p = path + n; *p;) {
        if (*p++ != '/')
            return false;
        const char *start = p;
        while (*p && *p != '/') {
            if ((unsigned char) *p < 32 || *p == 127 || strchr(":*?\"<>|", *p))
                return false;
            ++p;
        }
        size_t count = p - start;
        if (!count || (count == 1 && *start == '.') || (count == 2 && !memcmp(start, "..", 2)))
            return false;
    }
    return true;
}
static bool ready(void *context)
{
    (void) context;
    read_pico_sd_status_t s;
    return read_pico_sd_get_status(&s) == ESP_OK && s.present && s.mounted && s.media_valid &&
           !s.exit_failed;
}
static int create(void *c, const char *p)
{
    (void) c;
    return open(p, O_WRONLY | O_CREAT | O_EXCL, 0600);
}
static int put(void *c, int fd, const uint8_t *p, size_t n)
{
    (void) c;
    return (int) write(fd, p, n);
}
static int sync_file(void *c, int fd)
{
    (void) c;
    return fsync(fd);
}
static int close_file(void *c, int fd)
{
    (void) c;
    return close(fd);
}
static void uncertain(void *c)
{
    (void) c;
    read_pico_sd_note_file_close_failure();
}
static int publish(void *c, const char *from, const char *to)
{
    (void) c;
    // Only owned FAT /storage is supported. Locked IDF v6.1 vfs_fat_rename
    // holds its FS lock; FatFs f_rename rejects a different existing object
    // with FR_EXIST. This is NOT the overwrite behavior of POSIX rename.
    if (!owned_path(from) || !owned_path(to) || !strcmp(from, READ_PICO_SD_MOUNT_POINT) ||
        !strcmp(to, READ_PICO_SD_MOUNT_POINT)) {
        errno = EINVAL;
        return -1;
    }
    return rename(from, to);
}
static bool temp_name(const char *path)
{
    const char *base = path ? strrchr(path, '/') : NULL;
    static const char prefix[] = "/.rc3-upload-", suffix[] = ".part";
    size_t n = base ? strlen(base) : 0;
    return n > sizeof(prefix) - 1 + sizeof(suffix) - 1 &&
           !strncmp(base, prefix, sizeof(prefix) - 1) &&
           !strcmp(base + n - (sizeof(suffix) - 1), suffix);
}
static int remove_temp(void *c, const char *path)
{
    (void) c;
    if (!owned_path(path) || !temp_name(path)) {
        errno = EINVAL;
        return -1;
    }
    return unlink(path);
}
upload_file_io_t read_pico_upload_io(void)
{
    return (upload_file_io_t) {NULL,       ready,     create,  put, sync_file,
                               close_file, uncertain, publish, remove_temp};
}

bool read_pico_upload_begin(upload_file_t *f, const char *directory, char *path, size_t capacity,
                            size_t limit)
{
    if (!f || !directory || !path)
        return false;
    if (!owned_path(directory))
        return false;
    upload_file_io_t io = read_pico_upload_io();
    for (unsigned retry = 0; retry < 16; ++retry) {
        unsigned id = atomic_fetch_add_explicit(&sequence, 1, memory_order_relaxed);
        int length = snprintf(path, capacity, "%s/.rc3-upload-%08lx-%08x.part", directory,
                              (unsigned long) esp_random(), id);
        if (length < 0 || (size_t) length >= capacity)
            return false;
        errno = 0;
        if (upload_file_begin(f, &io, path, limit))
            return true;
        if (errno != EEXIST)
            return false;
    }
    return false;
}
