#include "fileops.h"
#include <assert.h>

#include "hkArrayT-primitives.h"

void fops_read(const char *file_path) {
    FILE *fileptr;
    // fileptr = fopen(file_path, "r");

    fileptr = fopen(file_path, "r");
    fseek(fileptr, 0L, SEEK_END);
    u64 filesize = ftell(fileptr);
    fseek(fileptr, 0L, SEEK_SET);
    // printf("filesize = %lld\n", filesize);
    assert(filesize <= fops_buffer_size);
    if (fileptr != NULL) {
        size_t newlen = fread(fops_buffer, sizeof(char), fops_buffer_size, fileptr);
        if (ferror(fileptr) != 0) {
            fputs("error reading file!", stderr);
        } else {
            fops_buffer[newlen++] = '\0';
        }
    }
    fclose(fileptr);
    hkArrayi64 arr = hkArrayi64Create(8);
    hkArrayi64Destroy(&arr);
}
