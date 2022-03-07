#ifndef FIO_GETRUSAGE_H
#define FIO_GETRUSAGE_H

#include <sys/time.h>
#include <sys/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int fio_getrusage(struct rusage *ru);

#ifdef __cplusplus
}
#endif

#endif
