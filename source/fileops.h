#pragma once

#include <core.h>

#define fops_buffer_size 1024 * 10
char fops_buffer[fops_buffer_size];
usize fops_buffer_alloc_len;

void fops_read(const char *file_path);
void fops_read_bin(const char *file_path);
