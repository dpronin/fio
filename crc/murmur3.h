#ifndef FIO_MURMUR3_H
#define FIO_MURMUR3_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t murmurhash3(const void *key, uint32_t len, uint32_t seed);

#ifdef __cplusplus
}
#endif

#endif
