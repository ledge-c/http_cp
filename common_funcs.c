#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "htfile.h"

int chomp(char *input)
{
    size_t tmp_len = strlen(input);
    if ((tmp_len > 0) && (input[tmp_len-1] == '\n'))
    {
        input[tmp_len-2] = '\0';
        return(SUCCESS);
    }
    else
    {
        PRINTV("chomp() did not find newline to cut\n");
        return(FAILURE);
    }
}

_Bool check_file(char *filename)
{
    static struct stat a;
    return(stat(filename, &a) == 0);
}

void die(const char *msg, ...)
{
        va_list params;
        va_start(params, msg);
        vfprintf(stderr, msg, params);
        va_end(params);
        exit(EXIT_FAILURE);
}
