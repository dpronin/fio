#ifndef FIO_FNV_H
#define FIO_FNV_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t fnv(const void *, uint32_t, uint64_t);

#ifdef __cplusplus
}
#endif

#endif
