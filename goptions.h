#ifndef GFIO_OPTIONS_H
#define GFIO_OPTIONS_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

void gopt_get_options_window(GtkWidget *window, struct gfio_client *gc);
void gopt_init(void);
void gopt_exit(void);

#ifdef __cplusplus
}
#endif

#endif
