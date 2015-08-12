#include "util.h"

int _optind = 1;
char *_optarg = NULL;

void print_help() {
    puts("USAGE: snowcrash [OPTIONS] file");
    puts("Unless specified by -e or -d a file will be encoded if its suffix is '.png' or decoded otherwise");
    puts("\t-e [FILE]\t encode a file");
    puts("\t-d [FILE]\t decode a file");
    puts("\t-o [FILE]\t specify where to save the output");
    puts("\t-f [FILE]\t specify the filename to store instead of the actual filename \n\t\t\t (cannot be used with -d)");
    puts("\t-l       \t enable legacy mode for traditional snowcrash files (must provide -o)");
    puts("\t-u       \t ignore unusual characters warning");
    puts("");
    puts("EXAMPLES:");
#define HELP_FORMAT "\t%-38s - %s\n"
    printf(HELP_FORMAT, "snowcrash document.odt", "encodes document.odt to document.odt.png");
    printf(HELP_FORMAT, "snowcrash document.odt.png", "decodes document.odt to the file stored in the png");
    printf(HELP_FORMAT, "snowcrash -e tomato.jpg -f knife.jpg", "encodes tomato.jpg, storing knife.jpg as filename");
    printf(HELP_FORMAT, "snowcrash -d knife.jpg -o cucumber.xyz", "decodes knife.jpg to cucumber.xyz");
}

void argcpy(char **rop) {
    if(!(*rop = malloc(strlen(_optarg)+1))) {
        perror(MEMORY_ERR);
        exit(1);
    }
    strcpy(*rop, _optarg);
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
char * cut_delim(char * str) {
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

static int whitelisted_char(char x) { //whitelisted characters
    return isalpha(x) || isdigit(x) || strchr(" -,.()_", x) != NULL;
}

static int blacklisted_char(char x) {
    return strchr("/\\\"%!*~<>:|?", x) != NULL; //windows...
}

int valid_string(char const * x) {
    if(strcmp(x, ".") == 0 || strcmp(x, "..") == 0)
        return 2;
    for(; *x != '\0'; x++)
        if(blacklisted_char(*x))
            return 2;
        else if(!whitelisted_char(*x))
            return 1;
    return 0;
}

int _getopt(int argc, char **argv, char * optstr) {
    //search next option
    if (_optind >= argc || argv[_optind][0] != '-') {
        return -1;
    }
    char *option = strchr(optstr, argv[_optind][1]);
    //option not found
    if(!option || *option == '\0') {
        fprintf(stderr, "Invalid argument %s.\n", argv[_optind]);
        return 0;
    }
    //does option need args?
    if(*(option+1) == ':') {
        if(argc < _optind +1) {
            fprintf(stderr, "Option %s requires an argument.\n", argv[_optind]);
            return 0;
        }
        _optind++;
        _optarg = argv[_optind];
        _optind++;
    } else {
        _optind++;
    }
    return *option;
}

int encode_file(char const *filename, char const *storename, char const *output_file, int flags) {
    printf("Encoding to '%s'.\n", output_file);
    size_t storename_len = strlen(storename);
    if(storename_len > MAX_FILENAME_LEN) {
        fprintf(stderr, "Filename '%s' is too long.\n", storename);
        fprintf(stderr, "Please choose another one using the -f option.\n");
        return 1;
    }
    switch(valid_string(storename)) {
        case 2:
            fprintf(stderr, "Filename '%s' contains blacklisted characters.\n", storename);
            fprintf(stderr, "Please choose another one using the -f option.\n");
            return 1;
        case 1:
            fprintf(stderr, "Filename '%s' contains unusual characters.\n", storename);
            if(!(flags & MOD_UNSAFE)) {
                fprintf(stderr, "Supply -u to ignore this warning.\n");
                return 1;
            }
            break;
        default:
            break;
    }
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
    unsigned dimension;
    if(!(flags & MOD_LEGACY)) {
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
    if(!(flags & MOD_LEGACY)) {
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

int decode_file(char const *filename, char *output_file, int flags) {
    int out_file_provided = 0;
    if(output_file) {
        out_file_provided = 1;
        printf("Decoding to '%s'.\n", output_file);
    }
    unsigned width, height;
    unsigned char * img = PNG_decode(filename, &width, &height);
    size_t pos = 0;
    //load size
    uint64_t size = 0;
    if(!(flags & MOD_LEGACY)) {
        for (; pos < sizeof size; pos++) {
            size = size | (((uint64_t) img[pos]) << (8 * pos));
        }
        //load output file unless one is provided
        if (out_file_provided) {
            for (; img[pos] != '\0'; pos++); //skip stored filename
        } else {
            size_t filename_length = 0;
            //full input verification
#define DECODE_FILENAME_ERR_HELP_MSG "Use -o to set where to store the resulting file.\n"
            for(; img[pos+filename_length] != '\0' && filename_length <= MAX_FILENAME_LEN; filename_length++);
            if(filename_length > MAX_FILENAME_LEN) {
                fprintf(stderr, "Stored output filename is too long.\n");
                fprintf(stderr, "Try rerunning in legacy mode with -l or supply an output file using -o.\n");
                return 1;
            }
            switch(valid_string((char*)(img+pos))) {
                case 2:
                    fprintf(stderr, "Stored filename contains blacklisted characters.\n");
                    fprintf(stderr, DECODE_FILENAME_ERR_HELP_MSG);
                    return 1;
                case 1:
                    fprintf(stderr, "Stored filename contains unusual characters.\n");
                    if(!(flags & MOD_UNSAFE)) {
                        fprintf(stderr, DECODE_FILENAME_ERR_HELP_MSG);
                        fprintf(stderr, "Or supply -u to ignore this warning.\n");
                        return 1;
                    }
                    break;
                default:
                    break;
            }
            if (!(output_file = malloc(filename_length+1))) {
                perror(MEMORY_ERR);
                return 1;
            }
            strcpy(output_file, (char*)(img+pos));
            pos += filename_length;
        }
        pos++;
    } else {
        size = width * height * 4;
    }
    if(!out_file_provided) {
        printf("Decoding to '%s'.\n", output_file);
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
