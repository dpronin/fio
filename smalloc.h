#ifndef FIO_SMALLOC_H
#define FIO_SMALLOC_H

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif

extern void *smalloc(size_t);
extern void *scalloc(size_t, size_t);
extern void sfree(void *);
extern char *smalloc_strdup(const char *);
extern void sinit(void);
extern void scleanup(void);
extern void smalloc_debug(size_t);

extern unsigned int smalloc_pool_size;

#ifdef __cplusplus
}
#endif

#endif
