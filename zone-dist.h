#ifndef FIO_ZONE_DIST_H
#define FIO_ZONE_DIST_H

#ifdef __cplusplus
extern "C" {
#endif

void td_zone_gen_index(struct thread_data *td);
void td_zone_free_index(struct thread_data *td);

#ifdef __cplusplus
}
#endif

#endif
