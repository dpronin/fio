#ifndef FIO_INET_ATON_LIB_H
#define FIO_INET_ATON_LIB_H

#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

int inet_aton(const char *cp, struct in_addr *inp);

#ifdef __cplusplus
}
#endif

#endif
