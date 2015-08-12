#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include "lodepng.h"

#ifdef _WIN32
    #define DELIM '\\'
#else
#define DELIM '/'
#endif


static char const * const MEMORY_ERR = "Cannot allocate memory";
static size_t const MAX_FILENAME_LEN = 254;

enum _mode {
    MOD_NONE = 0,
    MOD_ENCODE = 1 << 0,
    MOD_DECODE = 1 << 1,
    MOD_UNSAFE = 1 << 2,
    MOD_LEGACY = 1 << 3,
};

void print_help(void);
void argcpy(char **rop);
void auto_mode(char * arg, char **input, enum _mode *mode);
char * cut_delim(char * str);

int valid_string(char const * x);

int encode_file(char const *filename, char const *storename, char const *output_file, int flags);
int decode_file(char const *filename, char *output_file, int flags);

void PNG_encode(const char *filename, const unsigned char *image, unsigned width, unsigned height);
unsigned char *PNG_decode(const char *filename, unsigned *width, unsigned *height);

extern int _optind;
extern char *_optarg;
int _getopt(int argc, char **argv, char * optstr);

#endif //UTIL_H_INCLUDED
