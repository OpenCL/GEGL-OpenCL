#ifndef __GEGL_UTILS_H__
#define __GEGL_UTILS_H__

#include <gtk/gtk.h>
#include "gegl-types.h"

void gegl_rect_set (GeglRect *r,
		gint x,
		gint y,
		guint w,
		guint h);

void gegl_rect_copy (GeglRect *to,
		     GeglRect *from);

#endif /* __GEGL_UTILS_H__ */
