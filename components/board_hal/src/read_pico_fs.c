#include "read_pico_fs.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "read_pico_sd.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
static portMUX_TYPE registry_mux = portMUX_INITIALIZER_UNLOCKED;
static void registry_lock(void) { portENTER_CRITICAL(&registry_mux); }
static void registry_unlock(void) { portEXIT_CRITICAL(&registry_mux); }
#else
// Host tests supply a real mutex; no RTOS or physical card is involved.
extern void read_pico_fs_test_lock(void);
extern void read_pico_fs_test_unlock(void);
#define registry_lock read_pico_fs_test_lock
#define registry_unlock read_pico_fs_test_unlock
#endif

FILE *__real_fopen(const char *path, const char *mode);
FILE *__real_fdopen(int fd, const char *mode);
int __real_fclose(FILE *file);
int __real_open(const char *path, int flags, ...);
int __real_close(int fd);
DIR *__real_opendir(const char *path);
int __real_closedir(DIR *dir);
int __real_stat(const char *path, struct stat *info);
int __real_mkdir(const char *path, mode_t mode);
int __real_unlink(const char *path);
int __real_rename(const char *from, const char *to);

enum { SLOT_COUNT = 32 };
typedef enum { FREE, RESERVED, OPEN, CHANGING, UNCERTAIN } slot_state_t;
typedef enum { FILE_HANDLE, FD_HANDLE, DIR_HANDLE } handle_kind_t;
typedef struct {
    slot_state_t state;
    handle_kind_t kind;
    uintptr_t key;
    bool key_live;
} slot_t;
static slot_t slots[SLOT_COUNT];

static bool storage_path(const char *path)
{
    const size_t n = sizeof(READ_PICO_SD_MOUNT_POINT) - 1;
    return path && strncmp(path, READ_PICO_SD_MOUNT_POINT, n) == 0 &&
           (path[n] == '/' || path[n] == '\0');
}

static bool begin_io(void)
{
    esp_err_t err = read_pico_sd_io_begin();
    if (err == ESP_OK)
        return true;
    errno = err == ESP_ERR_TIMEOUT ? EBUSY : ENODEV;
    return false;
}

#ifdef ESP_PLATFORM
// Unexpected open failures (anything but a missing file) must be explainable from the log.
static void note_open_failure(int error)
{
    if (error == ENOENT)
        return;
    int count[UNCERTAIN + 1] = {0};
    registry_lock();
    for (int i = 0; i < SLOT_COUNT; ++i)
        ++count[slots[i].state];
    registry_unlock();
    ESP_LOGW("read_pico_fs",
             "fopen failed: errno=%d (%s), slots free=%d reserved=%d open=%d changing=%d uncertain=%d",
             error, strerror(error), count[FREE], count[RESERVED], count[OPEN], count[CHANGING],
             count[UNCERTAIN]);
}
#else
static void note_open_failure(int error) { (void) error; }
#endif

static int reserve(void)
{
    if (!begin_io())
        return -1;
    int slot = -1;
    registry_lock();
    for (int i = 0; i < SLOT_COUNT; ++i)
        if (slots[i].state == FREE) {
            slots[i].state = RESERVED;
            slot = i;
            break;
        }
    registry_unlock();
    if (slot < 0) {
        read_pico_sd_io_end();
        errno = EMFILE;
    }
    return slot;
}

static void retire_recycled_key_locked(handle_kind_t kind, uintptr_t key)
{
    for (int i = 0; i < SLOT_COUNT; ++i)
        if ((slots[i].state == CHANGING || slots[i].state == UNCERTAIN) &&
            slots[i].kind == kind && slots[i].key == key)
            slots[i].key_live = false;
}

static void observe_untracked_open(handle_kind_t kind, uintptr_t key)
{
    // A non-storage open can also reuse a just-freed SD handle. Its eventual
    // close must pass through, not get mistaken for the old closing owner.
    registry_lock();
    retire_recycled_key_locked(kind, key);
    registry_unlock();
}

static void publish(int slot, handle_kind_t kind, uintptr_t key)
{
    registry_lock();
    retire_recycled_key_locked(kind, key);
    slots[slot] = (slot_t) {OPEN, kind, key, true};
    registry_unlock();
}

static void release(int slot)
{
    registry_lock();
    slots[slot] = (slot_t) {0};
    registry_unlock();
    // The physical object was already closed. Do not release during close.
    read_pico_sd_io_end();
}

// -1: untracked (non-SD); -2: already closing/uncertain, never retry it.
static int take(handle_kind_t kind, uintptr_t key)
{
    int slot = -1;
    registry_lock();
    for (int i = 0; i < SLOT_COUNT; ++i)
        if (slots[i].state != FREE && slots[i].state != RESERVED &&
            slots[i].key_live && slots[i].kind == kind && slots[i].key == key) {
            if (slots[i].state == OPEN) {
                slot = i;
                slots[i].state = CHANGING;
                break;
            }
            // libc may recycle a just-closed pointer/fd before its old wrapper
            // finishes bookkeeping. Prefer the newly published live handle;
            // an older CHANGING tombstone must not reject the new consumer.
            slot = -2;
        }
    registry_unlock();
    if (slot == -2)
        errno = EIO;
    return slot;
}

static int finish_close(int slot, int result)
{
    if (slot >= 0) {
        if (result == 0) {
            release(slot);
        } else {
            registry_lock();
            slots[slot].state = UNCERTAIN;
            registry_unlock();
            read_pico_sd_note_file_close_failure();
            // Keep the IO lease and never retry a possibly freed handle.
        }
    }
    return result;
}

// picolibc implements fopen() as open() + fdopen(). Both are wrapped, so the
// nested fdopen already published this FILE under the descriptor's lease.
// Publishing it a second time would leave one slot that no fclose() releases.
static bool file_already_tracked(int own, FILE *file)
{
    bool found = false;
    registry_lock();
    for (int i = 0; i < SLOT_COUNT && !found; ++i)
        found = i != own && slots[i].state == OPEN && slots[i].key_live &&
                slots[i].kind == FILE_HANDLE && slots[i].key == (uintptr_t) file;
    registry_unlock();
    return found;
}

FILE *__wrap_fopen(const char *path, const char *mode)
{
    if (!storage_path(path)) {
        FILE *file = __real_fopen(path, mode);
        if (file)
            observe_untracked_open(FILE_HANDLE, (uintptr_t) file);
        return file;
    }
    int slot = reserve();
    if (slot < 0) {
        int reserve_errno = errno;
        note_open_failure(reserve_errno);
        errno = reserve_errno;
        return NULL;
    }
    FILE *file = __real_fopen(path, mode);
    int open_errno = errno;
    if (file && file_already_tracked(slot, file)) {
        release(slot);
    } else if (file) {
        publish(slot, FILE_HANDLE, (uintptr_t) file);
    } else {
        release(slot);
        note_open_failure(open_errno);
        errno = open_errno;
    }
    return file;
}

FILE *__wrap_fdopen(int fd, const char *mode)
{
    int slot = take(FD_HANDLE, (uintptr_t) fd);
    if (slot == -2)
        return NULL;
    FILE *file = __real_fdopen(fd, mode);
    if (slot >= 0) {
        // fdopen failure leaves the descriptor owned by the caller.
        if (file)
            publish(slot, FILE_HANDLE, (uintptr_t) file);
        else
            publish(slot, FD_HANDLE, (uintptr_t) fd);
    } else if (file)
        observe_untracked_open(FILE_HANDLE, (uintptr_t) file);
    return file;
}

int __wrap_fclose(FILE *file)
{
    int slot = take(FILE_HANDLE, (uintptr_t) file);
    return slot == -2 ? EOF : finish_close(slot, __real_fclose(file));
}

int __wrap_open(const char *path, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = (mode_t) va_arg(args, int);
        va_end(args);
    }
    bool tracked = storage_path(path);
    int slot = tracked ? reserve() : -1;
    if (tracked && slot < 0)
        return -1;
    int fd = __real_open(path, flags, mode);
    if (slot >= 0) {
        if (fd >= 0)
            publish(slot, FD_HANDLE, (uintptr_t) fd);
        else
            release(slot);
    } else if (fd >= 0)
        observe_untracked_open(FD_HANDLE, (uintptr_t) fd);
    return fd;
}

int __wrap_close(int fd)
{
    int slot = take(FD_HANDLE, (uintptr_t) fd);
    return slot == -2 ? -1 : finish_close(slot, __real_close(fd));
}

DIR *__wrap_opendir(const char *path)
{
    if (!storage_path(path)) {
        DIR *dir = __real_opendir(path);
        if (dir)
            observe_untracked_open(DIR_HANDLE, (uintptr_t) dir);
        return dir;
    }
    int slot = reserve();
    if (slot < 0)
        return NULL;
    DIR *dir = __real_opendir(path);
    if (dir)
        publish(slot, DIR_HANDLE, (uintptr_t) dir);
    else
        release(slot);
    return dir;
}

int __wrap_closedir(DIR *dir)
{
    int slot = take(DIR_HANDLE, (uintptr_t) dir);
    return slot == -2 ? -1 : finish_close(slot, __real_closedir(dir));
}

static bool begin_path(const char *path, bool *tracked)
{
    *tracked = storage_path(path);
    return !*tracked || begin_io();
}

int __wrap_stat(const char *path, struct stat *info)
{
    bool tracked;
    if (!begin_path(path, &tracked))
        return -1;
    int result = __real_stat(path, info);
    if (tracked)
        read_pico_sd_io_end();
    return result;
}

int __wrap_mkdir(const char *path, mode_t mode)
{
    bool tracked;
    if (!begin_path(path, &tracked))
        return -1;
    int result = __real_mkdir(path, mode);
    if (tracked)
        read_pico_sd_io_end();
    return result;
}

int __wrap_unlink(const char *path)
{
    bool tracked;
    if (!begin_path(path, &tracked))
        return -1;
    int result = __real_unlink(path);
    if (tracked)
        read_pico_sd_io_end();
    return result;
}

int __wrap_rename(const char *from, const char *to)
{
    bool tracked = storage_path(from) || storage_path(to);
    if (tracked && !begin_io())
        return -1;
    int result = __real_rename(from, to);
    if (tracked)
        read_pico_sd_io_end();
    return result;
}
