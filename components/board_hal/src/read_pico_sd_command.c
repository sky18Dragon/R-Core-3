#include <string.h>

#include "read_pico_sd.h"

read_pico_sd_command_t read_pico_sd_parser_feed(read_pico_sd_parser_t *parser, unsigned char byte)
{
    if (parser == NULL)
        return READ_PICO_SD_COMMAND_INVALID;
    if (byte == '\n') {
        read_pico_sd_command_t command = READ_PICO_SD_COMMAND_INVALID;
        if (!parser->rejected) {
            if (parser->length > 0 && parser->bytes[parser->length - 1] == '\r')
                --parser->length;
            parser->bytes[parser->length] = '\0';
            if (strcmp(parser->bytes, "status") == 0)
                command = READ_PICO_SD_COMMAND_STATUS;
            else if (strcmp(parser->bytes, "mount") == 0)
                command = READ_PICO_SD_COMMAND_MOUNT;
            else if (strcmp(parser->bytes, "file-test") == 0)
                command = READ_PICO_SD_COMMAND_FILE_TEST;
            else if (strcmp(parser->bytes, "file-check") == 0)
                command = READ_PICO_SD_COMMAND_FILE_CHECK;
            else if (strcmp(parser->bytes, "upload-test") == 0)
                command = READ_PICO_SD_COMMAND_UPLOAD_TEST;
            else if (strcmp(parser->bytes, "photo-check") == 0)
                command = READ_PICO_SD_COMMAND_PHOTO_CHECK;
            else if (strcmp(parser->bytes, "stop") == 0)
                command = READ_PICO_SD_COMMAND_STOP;
        }
        *parser = (read_pico_sd_parser_t) {0};
        return command;
    }
    if (byte == 0 || byte > 127 || (byte < 32 && byte != '\r') ||
        parser->length >= sizeof(parser->bytes) - 1)
        parser->rejected = true;
    if (!parser->rejected)
        parser->bytes[parser->length++] = (char) byte;
    return READ_PICO_SD_COMMAND_NONE;
}
