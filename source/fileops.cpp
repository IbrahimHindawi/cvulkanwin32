#include "fileops.h"
#include <assert.h>

void fops_read(const char *file_path) {
   FILE *fileptr;
   fileptr = fopen(file_path, "r");
   assert(fileptr != NULL);
   fseek(fileptr, 0L, SEEK_END);
   u64 filesize = ftell(fileptr);
   fseek(fileptr, 0L, SEEK_SET);
   // printf("filesize = %lld\n", filesize);
   assert(filesize <= fops_buffer_size);
   if (fileptr != NULL) {
       size_t newlen = fread(fops_buffer, sizeof(char), fops_buffer_size, fileptr);
       if (ferror(fileptr) != 0) {
           fputs("FileOps::Error reading file!", stderr);
       } else {
           fops_buffer[newlen++] = '\0';
       }
   }
   fclose(fileptr);
}

void fops_read_bin(const char *file_path) {
   FILE *fileptr;
   fileptr = fopen(file_path, "rb");
   assert(fileptr != NULL);
   fseek(fileptr, 0L, SEEK_END);
   u64 filesize = ftell(fileptr);
   fops_buffer_alloc_len = filesize;
   fseek(fileptr, 0L, SEEK_SET);
   printf("filesize = %lld\n", filesize);
   assert(filesize <= fops_buffer_size);
   if (fileptr != NULL) {
       size_t newlen = fread(fops_buffer, sizeof(char), fops_buffer_size, fileptr);
       if (ferror(fileptr) != 0) {
           fputs("FileOps::Error reading file!", stderr);
       } 
       // else {
       //     fops_buffer[newlen++] = '\0';
       // }
   }
   fclose(fileptr);
}
