#ifndef FIO_HWEIGHT_H
#define FIO_HWEIGHT_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned int hweight8(uint8_t w);
unsigned int hweight32(uint32_t w);
unsigned int hweight64(uint64_t w);

#ifdef __cplusplus
}
#endif

#endif
