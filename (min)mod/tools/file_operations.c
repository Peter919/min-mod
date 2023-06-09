#include <stdio.h>
#include "log.h"
#include "mem_tools.h"
#include "os.h"

const char * get_file_ext(const char * fname)
{
        const char * ext = strrchr(fname, '.');
        if (!ext) {
                LOG_ERROR("Couldn't find the file extension of \"%s\".\n", fname);
                return NULL;
        }
        return ext + 1;
}

char * get_parent_dir(const char * fpath)
{
        const char * last_sep;
        if (OS == OS_WINDOWS) {
                last_sep = strrchr(fpath, '\\');
        } else {
                last_sep = strrchr(fpath, '/');
        }

        ASSERT_OR_HANDLE(last_sep, NULL, "\"%s\" doesn't belong to a directory.");

        int last_sep_idx = last_sep - fpath;

        char * file_dir = ALLOC(char, last_sep_idx + 1);
        strncpy(file_dir, fpath, last_sep_idx);
        file_dir[last_sep_idx] = '\0';

        return file_dir;
}

static long get_file_length(FILE * fp)
{
        fseek(fp, 0, SEEK_END);
        long flength = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        return flength;
}

char * file_to_string(const char * fname)
{
        LOG_INFO("Moving the contents of \"%s\" to a string ...\n", fname);

        FILE * fp = fopen(fname, "r");
        if (!fp) {
                LOG_ERROR("Couldn't open \"%s\". Are you sure the file exists?\n", fname);
                return 0;
        }

        long flength = get_file_length(fp);

        // "true_flength" used in case the file contents take less space within
        // the program than without.
        char * fbuffer = ALLOC(char, flength + 1);
        long true_flength = fread(fbuffer, 1, flength, fp);
        fbuffer[true_flength] = 0;

        fclose(fp);

        char * fstring = ALLOC(char, strlen(fbuffer) + 1);
        strcpy(fstring, fbuffer);

        return fstring;
}
