#ifndef GFIO_ERROR_H
#define GFIO_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

extern void gfio_report_error(struct gui_entry *ge, const char *format, ...);
extern void gfio_report_info(struct gui *ui, const char *title, const char *message);

#ifdef __cplusplus
}
#endif

#endif
