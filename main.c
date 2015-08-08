#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <limits.h>
#include "util.h"

int main(int argc, char **argv)
{
    //sanity checks
    assert(sizeof (uint64_t) == 8);
    assert(CHAR_BIT == 8);

    enum _mode mode = MOD_NONE;
    char *input = NULL;
    char *name_to_store = NULL;
    char *output = NULL;

    int opt;
    while(_optind < argc) {
        if ((opt = _getopt(argc, argv, "e:f:d:o:lhu")) != -1) {
            switch (opt) {
                case 'e':
                    mode |= MOD_ENCODE;
                    argcpy(&input);
                    break;
                case 'f':
                    argcpy(&name_to_store);
                    break;
                case 'd':
                    mode |= MOD_DECODE;
                    argcpy(&input);
                    break;
                case 'o':
                    argcpy(&output);
                    break;
                case 'l':
                    mode |= MOD_LEGACY;
                    break;
                case 'h':
                    print_help();
                    return 0;
                case 'u':
                    mode |= MOD_UNSAFE;
                    break;
                default:
                    fprintf(stderr, "Internal error.\n");
                    return 1;
            }
        } else {
            auto_mode(argv[_optind++], &input, &mode);
        }
    }

    if(!mode) {
        print_help();
        exit(1);
    }
    if(((mode & MOD_ENCODE) && (mode & MOD_DECODE))) {
        fprintf(stderr, "You cannot use -e in conjunction with -d.\n");
        return 1;
    }
    if(((mode & MOD_DECODE) && (name_to_store))) {
        fprintf(stderr, "You cannot use -f when decoding.\n");
        fprintf(stderr, "If you're trying to encode please supply -e.\n");
        return 1;
    }
    if((mode & MOD_LEGACY) && !output) {
        fprintf(stderr, "Please supply an output file with -o.\n");
        return 1;
    }


    if(mode & MOD_ENCODE) {
        //do not store path delimiters in file
        if(name_to_store) {
            //old storename isn't needed
            char * tmp = name_to_store;
            name_to_store = cut_delim(name_to_store);
            free(tmp);
        } else {
            name_to_store = cut_delim(input);
        }
        if(!output) {
            if(!(output = malloc(strlen(name_to_store)+strlen(".png")+1))) {
                perror(MEMORY_ERR);
                return 1;
            }
            strcpy(output, name_to_store);
            strcat(output, ".png");
        }
        if(encode_file(input, name_to_store, output, mode)) {
            fprintf(stderr, "Error while encoding.\n");
            return 1;
        }
    } else if(mode & MOD_DECODE) {
        if(decode_file(input, output, mode)) {
            fprintf(stderr, "Error while decoding.\n");
            return 1;
        }
    }
    if(output) {
        free(output);
    }
    if(name_to_store) {
        free(name_to_store);
    }
    if(input) {
        free(input);
    }
    return 0;
}