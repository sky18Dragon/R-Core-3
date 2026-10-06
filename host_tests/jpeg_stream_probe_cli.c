#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "jpeg_file_probe.h"

typedef struct { const char *path; FILE *file; } source_t;
static bool ready(void *context) { return context != NULL; }
static int open_input(void *context)
{
    source_t *source = context;
    source->file = fopen(source->path, "rb");
    return source->file ? fileno(source->file) : -1;
}
static int64_t input_size(void *context, int fd)
{
    (void) context;
    struct stat info;
    return fstat(fd, &info) == 0 && S_ISREG(info.st_mode) ? info.st_size : -1;
}
static int read_input(void *context, int fd, uint8_t *data, size_t size)
{
    (void) fd;
    FILE *file = ((source_t *) context)->file;
    size_t got = fread(data, 1, size, file);
    return ferror(file) ? -1 : (int) got;
}
static int close_input(void *context, int fd)
{
    (void) fd;
    return fclose(((source_t *) context)->file);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: jpeg_stream_probe_cli <read-only JPEG path>\n");
        return 2;
    }
    source_t source = {.path = argv[1]};
    union { max_align_t alignment; uint8_t bytes[3100]; } work;
    const jpeg_file_probe_io_t io = {&source, ready, open_input, input_size, read_input, close_input};
    jpeg_file_probe_session_t session = {0};
    jpeg_file_probe_result_t file_result = jpeg_file_probe_run(&session, &io, work.bytes, sizeof(work.bytes));
    jpeg_stream_probe_result_t result = file_result.stream;
    printf("file=%s decode=%s decoder=%d source=%ux%u output=%ux%u scale=1/4 "
           "file_size=%zu bytes_read=%zu codec_read=%zu "
           "pixels=%zu blocks=%zu block_rgb_crc32=%08x close_failed=%d work=3100\n",
           jpeg_file_probe_error_name(file_result.error),
           jpeg_stream_probe_error_name(result.error), result.decoder_result,
           result.width, result.height, result.output_width, result.output_height,
           file_result.file_size, file_result.bytes_read, result.bytes_read,
           result.pixels, result.blocks, result.rgb_crc32, session.close_failed);
    return file_result.error == JPEG_FILE_OK ? 0 : 1;
}
