#ifndef FIO_OUTPUT_BUFFER_H
#define FIO_OUTPUT_BUFFER_H

#include <stddef.h>

struct buf_output {
	char *buf;
	size_t buflen;
	size_t max_buflen;
};

#ifdef __cplusplus
extern "C" {
#endif

void buf_output_init(struct buf_output *out);
void buf_output_free(struct buf_output *out);
size_t buf_output_add(struct buf_output *out, const char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
