#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include "lodepng.h"

#ifdef _WIN32
#define DELIM '\\'
#else
#define DELIM '/'
#endif

#define MEMORY_ERR "Cannot allocate memory"

enum _mode {
    MOD_NONE = 0,
    MOD_ENCODE = 1 << 0,
    MOD_DECODE = 1 << 1,
    MOD_LEGACY = 1 << 2
};


static void print_help(void);
static void argcpy(char **rop);
static void auto_mode(char * arg, char **input, enum _mode *mode);
static char * cut_delim(char * str);

int encode_file(char const *filename, char const *storename, char const *output_file, int legacy);
int decode_file(char const *filename, char *output_file, int legacy);

void PNG_encode(const char *filename, const unsigned char *image, unsigned width, unsigned height);
unsigned char *PNG_decode(const char *filename, unsigned *width, unsigned *height);

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
    while(optind < argc) {
        if ((opt = getopt(argc, argv, "e:f:d:o:l")) != -1) {
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
                default:
                    fprintf(stderr, "Internal error.\n");
                    exit(1);
            }
        } else {
            auto_mode(argv[optind++], &input, &mode);
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
        fprintf(stderr, "You cannot use -f when decoding. If you're trying to encode please provide -e.");
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
        if(encode_file(input, name_to_store, output, mode & MOD_LEGACY)) {
            fprintf(stderr, "Error while encoding.\n");
        }
    } else if(mode & MOD_DECODE) {
        if(decode_file(input, output, mode & MOD_LEGACY)) {
            fprintf(stderr, "Error while decoding.\n");
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


void print_help() {
    puts("USAGE: snowcrash [OPTIONS] file");
    puts("Unless specified by -e or -d a file will be encoded if its suffix is '.png' or decoded otherwise");
    puts("\t-e [FILE]\t encode a file");
    puts("\t-d [FILE]\t decode a file");
    puts("\t-o [PATH]\t specify where to save the output");
    puts("\t-f [FILE]\t specify the filename to store instead of the actual filename \n\t\t\t (cannot be used with -d)");
    puts("\t-l       \t enable legacy mode for traditional snowcrash files (must provide -o)");
    puts("");
    puts("EXAMPLES:");
    #define HELP_FORMAT "\t%-38s - %s\n"
    printf(HELP_FORMAT, "snowcrash document.odt", "encodes document.odt to document.odt.png");
    printf(HELP_FORMAT, "snowcrash document.odt.png", "decodes document.odt to the file stored in the png");
    printf(HELP_FORMAT, "snowcrash -e tomato.jpg -f knife.jpg", "encodes tomato.jpg, storing knife.jpg as filename");
    printf(HELP_FORMAT, "snowcrash -d knife.jpg -o cucumber.xyz", "decodes knife.jpg to cucumber.xyz");
}

static void argcpy(char **rop) {
    if(!(*rop = malloc(strlen(optarg)+1))) {
        perror(MEMORY_ERR);
        exit(1);
    }
    strcpy(*rop, optarg);
}

void auto_mode(char *arg, char **input, enum _mode *mode) {
    size_t len = strlen(arg);
    if(!(*input = malloc(len + 1))) {
        perror(MEMORY_ERR);
        exit(1);
    }
    strcpy(*input, arg);
    if (len >= strlen(".png") && strcasecmp(*input + len - strlen(".png"), ".png") == 0) {
        *mode |= MOD_DECODE;
    } else {
        *mode |= MOD_ENCODE;
    }
}
//returns a string containing only the filename of a filepath
static char * cut_delim(char * str) {
    size_t len = strlen(str), i;
    for(i = len - 1; str[i] != DELIM && i > 0; i--);
    if(str[i] == DELIM) i++;
    char * rop;
    if(!(rop = malloc(len-i+1))) {
        perror(MEMORY_ERR);
        return 0;
    }
    strcpy(rop, str+i);
    return rop;
}

int encode_file(char const *filename, char const *storename, char const *output_file, int legacy) {
    printf("Encoding to '%s'.\n", output_file);
    FILE * fp = fopen(filename, "rb");
    if(!fp) {
        perror("Cannot open input");
        return 1;
    }
    //get filesize
    fseek(fp, 0, SEEK_END);
    uint64_t size = (uint64_t)(ftell(fp));
    fseek(fp, 0, SEEK_SET);

    //determine image dimensions (width = height)
    size_t storename_len = strlen(storename);
    unsigned dimension;
    if(!legacy) {
        dimension = (unsigned)(ceil(sqrt(ceil((size + sizeof size + strlen(storename) + 1)/4.0))));
    } else {
        dimension = (unsigned)(ceil(sqrt(ceil((size)/4.0))));
    }

    //allocate and zero-initialize space for the image
    unsigned char * content;
    if(!(content = calloc(dimension*dimension*4, 1))) {
        perror(MEMORY_ERR);
        return 1;
    }
    //stash filesize in content
    uint64_t pos = 0;
    if(!legacy) {
        for (; pos < sizeof size; pos++) {
            content[pos] = (unsigned char) ((size & ((((uint64_t) (1 << 8) - 1)) << (pos * 8))) >> (pos * 8));
        }
        //stash filename and '\0' in content
        for (size_t i = 0; i <= storename_len; i++, pos++) {
            content[pos] = (unsigned char) (storename[i]);
        }
    }
    //read full data file and encode it
    if(fread(content+pos, 1, size, fp) != size) {
        perror("Error reading input");
        return 1;
    }
    fclose(fp);
    PNG_encode(output_file, content, dimension, dimension);
    free(content);
    return 0;
}

int decode_file(char const *filename, char *output_file, int legacy) {
    printf("Decoding to '%s'.\n", output_file);
    int out_file_provided = (output_file) ? 1 : 0;
    unsigned width, height;
    unsigned char * img = PNG_decode(filename, &width, &height);
    size_t pos = 0;
    //load size
    uint64_t size = 0;
    if(!legacy) {
        for (; pos < sizeof size; pos++) {
            size = size | (((uint64_t) img[pos]) << (8 * pos));
        }
        //load output file unless one is provided
        if (out_file_provided) {
            for (; img[pos] != '\0'; pos++); //skip stored filename
        } else {
            size_t filename_length = 1, fileit;
            if (!(output_file = malloc(filename_length))) { //array list
                perror(MEMORY_ERR);
                return 1;
            }
            for (fileit = 0; img[pos] != '\0'; fileit++, pos++) {
                if (fileit >= filename_length) {
                    filename_length *= 2;
                    if (!(output_file = realloc(output_file, filename_length))) {
                        perror(MEMORY_ERR);
                        return 1;
                    }
                }
                output_file[fileit] = img[pos];
            }
            output_file[fileit] = '\0'; //add nul terminator
            if (!(output_file = realloc(output_file, fileit + 1))) { //trim char array back to actual size
                perror(MEMORY_ERR);
                return 1;
            }
            for (int i = 0; output_file[i] != '\0'; i++) {
                if (output_file[i] == DELIM) {
                    fprintf(stderr, "Input file contains a path delimiter and may be malicious.\n");
                    return 1;
                }
            }
        }
        pos++;
    } else {
        size = width * height * 4;
    }
    FILE * fp = fopen(output_file, "wb");
    fwrite(img+pos, 1, size, fp);
    fclose(fp);
    free(img);
    if(!out_file_provided) {
        free(output_file);
    }
    return 0;
}

//Thanks to the lodepng documentation for these examples
void PNG_encode(const char *filename, const unsigned char *image, unsigned width, unsigned height)
{
    /*Encode the image*/
    unsigned error = lodepng_encode32_file(filename, image, width, height);

    /*if there's an error, display it*/
    if(error) printf("error %u: %s\n", error, lodepng_error_text(error));
}

unsigned char *PNG_decode(const char *filename, unsigned *width, unsigned *height)
{
    unsigned error;
    unsigned char * image;

    error = lodepng_decode32_file(&image, width, height, filename);
    if(error) printf("error %u: %s\n", error, lodepng_error_text(error));

    return image;
}
