#ifndef HTFILE_H_
#define HTFILE_H_

#include <stdarg.h>
#include <stdint.h>

#define SUCCESS 0
#define FAILURE -1

#define DEFAULT_PORT 8080
#define PORT_MAX 65535
#define IPADDR_LEN 15
#define FILENAME_MAXLEN 200

#define RETERROR(a, ...) do { fprintf(stderr, a, ## __VA_ARGS__); return(FAILURE); } while (0)

#define PRINTV(fmt, ...) do { if (opts->verbose) printf("(V) " fmt, ## __VA_ARGS__); } while (0)

#define USAGE_MSG   "Usage: %s [opt] <file1> [[file2] [file3 ] [...]]\n"    \
                    "Available options are:\n"                              \
                    "-v     verbose>\n"                                     \
                    "-p     Port to listen on\n"                            \
                    "-C     Do not stat() files until they are requested\n"

typedef struct argopt
		{
			_Bool verbose;
                        _Bool fcheck;
                        uint16_t port;
			int count;
			int files_num;
			char **files;
		} s_options;

extern s_options *opts;

extern void die(const char *msg, ...)
    __attribute__((format (printf, 1, 2)));



#endif /* HTFILE_H_ */
