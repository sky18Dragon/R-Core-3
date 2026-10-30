#pragma once

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

// Linked only for Read Pico SD/photo/integration profiles. GNU link-wrap routes
// ordinary libc calls from the app and linked libraries here; other paths pass
// through. The registry is bounded, no heap, with no I/O inside its critical
// sections. It prevents teardown while opening, using or closing SD handles.
// It is NOT a photo-ownership policy or an atomic/durable publish transaction.
FILE *__wrap_fopen(const char *path, const char *mode);
FILE *__wrap_fdopen(int fd, const char *mode);
int __wrap_fclose(FILE *file);
int __wrap_open(const char *path, int flags, ...);
int __wrap_close(int fd);
DIR *__wrap_opendir(const char *path);
int __wrap_closedir(DIR *dir);
int __wrap_stat(const char *path, struct stat *info);
int __wrap_mkdir(const char *path, mode_t mode);
int __wrap_unlink(const char *path);
int __wrap_rename(const char *from, const char *to);

#ifdef __cplusplus
}
#endif
