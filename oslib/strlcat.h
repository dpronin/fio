#ifndef CONFIG_STRLCAT

#ifndef FIO_STRLCAT_H
#define FIO_STRLCAT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t strlcat(char *dst, const char *src, size_t dsize);

#ifdef __cplusplus
}
#endif

#endif

#endif
