#ifndef CONFIG_HAVE_STRNDUP

#ifndef FIO_STRNDUP_LIB_H
#define FIO_STRNDUP_LIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

char *strndup(const char *s, size_t n);

#ifdef __cplusplus
}
#endif

#endif

#endif
