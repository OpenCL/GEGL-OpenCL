#ifndef __GEGL_UTILS_H__
#define __GEGL_UTILS_H__

#include <gtk/gtk.h>
#include "gegl-types.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

#ifndef __TYPEDEF_GEGL_IMAGE_MANAGER__
#define __TYPEDEF_GEGL_IMAGE_MANAGER__
typedef struct _GeglImageManager  GeglImageManager;
#endif

void 
gegl_rect_set (GeglRect *r,
	           gint x,
	           gint y,
	           guint w,
	           guint h);
gboolean
gegl_rect_equal (GeglRect *r,
		 GeglRect *s); 
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
gboolean
gegl_rect_contains (GeglRect *r,
                    GeglRect *s);
gint
gegl_channel_data_type_bytes (GeglChannelDataType data);

GeglImageManager *
gegl_image_manager_instance (void);

GeglColorModel *
gegl_color_model_instance(GeglColorModelType type);

GeglColorModel *
gegl_color_model_instance1(GeglColorAlphaSpace color_alpha_space,
                           GeglChannelDataType data_type);

GeglColorModel *
gegl_color_model_instance2(GeglColorSpace color_space,
                           GeglChannelDataType data_type,
                           gboolean has_alpha);
void
gegl_init (int *argc, 
           char ***argv);

GeglColorAlphaSpace
gegl_utils_derived_color_alpha_space(GList *inputs);

GeglChannelDataType
gegl_utils_derived_channel_data_type(GList *inputs);

GeglColorSpace
gegl_utils_derived_color_space(GList *inputs);

#endif /* __GEGL_UTILS_H__ */
