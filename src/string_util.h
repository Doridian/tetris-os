#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include "util.h"

size_t strlen(const char *str);
size_t strlcat(char *dst, const char *src, size_t size);
size_t strlcpy(char *dst, const char *src, size_t n);

#endif
