#ifndef __FILEIO_H__
#define __FILEIO_H__

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#define FILENAME_BUFFER_SIZE 8192

time_t fileio_get_modification_time(const char *filename);
void fileio_set_modification_time(const char *filename, time_t mdate);
char *strip_path(char *path_in);

int validate_device_choice(const char *choice);

long get_file_size(FILE *fp);

/// Populate array of filenames that match files or wildcard patterns in argv list.
/**
 *
 * @param result Buffer to contain matched filenames.
 * @param result_size The size of the matched filenames buffer in bytes.
 * @param argc Number of items in argv pattern list.
 * @param argv List of patterns or full files names to search.
 * @return Result buffer filled with found filenames.
 *         For each file name entry,
 *         the first byte is the length of the filename,
 *         followed by that many bytes (not zero terminated).
 *         The value 0xFF for the length marks the end of the list.
 *
 *         The total number of matches is returned.
 *
 * Filenames have driver letter, colon, base name, dot, extention. For example, B:RLX16.COM.
 */
int get_matching_files(uint8_t *result, uint16_t result_size, int argc, char **argv);

#endif
