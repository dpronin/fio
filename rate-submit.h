#ifndef FIO_RATE_SUBMIT
#define FIO_RATE_SUBMIT

#ifdef __cplusplus
extern "C" {
#endif

int rate_submit_init(struct thread_data *, struct sk_out *);
void rate_submit_exit(struct thread_data *);

#ifdef __cplusplus
}
#endif

#endif
