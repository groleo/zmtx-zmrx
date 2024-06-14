#include <glob.h>
#include <stdio.h>
#include <sys/stat.h>
#include <utime.h>
#include <string.h>

#include "fileio.h"

int use_aux = 0;

long fileio_get_modification_time(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
        return 0;
    struct stat s;
    fstat(fileno(fp), &s);
    return s.st_mtime;
}

void fileio_set_modification_time(const char *filename, long mdate)
{
    /*
     * set the time
     */
    struct utimbuf tv;
    tv.actime = (time_t)mdate;
    tv.modtime = (time_t)mdate;

    utime(filename, &tv);
}

long get_file_size(FILE *fp) {
    struct stat s;
    fstat(fileno(fp), &s);
    return s.st_size;
}

char *strip_path(char *path_in)
{
    // Nothing in particular to do for unix.
    return path_in;
}

int validate_device_choice(const char *choice)
{
    // All choices are valid at this point.
    return 1;
}


int get_matching_files(uint8_t *result, uint16_t result_size, int argc, char **argv)
{
    int count = 0;
    uint16_t remaining_size = result_size;
    for (int i = 0; i < argc; i++) {
        glob_t globbuf;
        glob(argv[i], GLOB_NOSORT, NULL, &globbuf);
        for (int k = 0; k < globbuf.gl_pathc; ++k)
        {
            uint8_t pathlen = strlen(globbuf.gl_pathv[k]);
            uint8_t required_size = 1/*pathlen*/ + 1/*0xff*/ + pathlen;
            if (required_size >= remaining_size)
            {
                fprintf(stderr, "Error: Too many filenames (not enough room in filename buffer).\r\n");
                return count;
            }
            *result++ = pathlen;

            strncpy((char *) result, globbuf.gl_pathv[k], pathlen);
            result += pathlen;
            count++;

        }
        globfree(&globbuf);
    }
    *result = 0xFF;
    return count;
}
