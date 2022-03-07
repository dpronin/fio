#ifndef _CONTAINER_OF_H
#define _CONTAINER_OF_H

#ifdef __cplusplus

#include <cstdlib>
#include <cstddef>

extern "C" {

#define container_of(ptr, type, member)  ({         \
    auto const *__mptr = static_cast<__typeof__( ((type *)0)->member ) const*>(ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})
}

#else // __cplusplus

#include <stdlib.h>
#include <stddef.h>

#define container_of(ptr, type, member)  ({			\
	const __typeof__( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#endif // _CONTAINER_OF_H
