#ifndef LINUX_DEV_LOOKUP
#define LINUX_DEV_LOOKUP

#ifdef __cplusplus
extern "C" {
#endif

int blktrace_lookup_device(const char *redirect, char *path, unsigned int maj,
			   unsigned int min);

#ifdef __cplusplus
}
#endif

#endif
