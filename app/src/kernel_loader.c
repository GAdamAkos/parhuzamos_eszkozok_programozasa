#include "kernel_loader.h"

#include <stdio.h>
#include <stdlib.h>

/* Kernel forrásfájl teljes beolvasása memóriába. */
char* load_kernel_source(const char* path)
{
    FILE* file;
    long file_size;
    size_t read_size;
    char* source;

    file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open kernel source file: %s\n", path);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        fprintf(stderr, "Failed to seek to end of kernel source file: %s\n", path);
        return NULL;
    }

    file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        fprintf(stderr, "Failed to determine size of kernel source file: %s\n", path);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        fprintf(stderr, "Failed to seek to start of kernel source file: %s\n", path);
        return NULL;
    }

    source = (char*)malloc((size_t)file_size + 1);
    if (source == NULL) {
        fclose(file);
        fprintf(stderr, "Failed to allocate memory for kernel source.\n");
        return NULL;
    }

    read_size = fread(source, 1, (size_t)file_size, file);
    fclose(file);

    if (read_size != (size_t)file_size) {
        free(source);
        fprintf(stderr, "Failed to read complete kernel source file: %s\n", path);
        return NULL;
    }

    source[file_size] = '\0';
    return source;
}
