#ifndef __GEGL_UTILS_H__
#define __GEGL_UTILS_H__

#include <gtk/gtk.h>
#include "gegl-types.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif


void 
gegl_rect_set (GeglRect *r,
	       gint x,
	       gint y,
	       guint w,
	       guint h);

void 
gegl_rect_copy (GeglRect *to,
	        GeglRect *from);
void 
gegl_rect_union (GeglRect *dest,
                 GeglRect *src1,
                 GeglRect *src2);

gboolean 
gegl_rect_intersect(GeglRect *dest,
                    GeglRect *src1,
                    GeglRect *src2);

GeglColorModel *
gegl_color_model_factory (GeglColorSpace space,
                          GeglChannelDataType data,
                          gboolean has_alpha);

#endif /* __GEGL_UTILS_H__ */
