#ifndef __GEGL_UTILS_H__
#define __GEGL_UTILS_H__

#include <glib-object.h>
#include "gegl-types.h"

void     gegl_rect_set          (GeglRect *r, gint x, gint y, guint w, guint h);
gboolean gegl_rect_equal        (GeglRect *r, GeglRect *s); 
void     gegl_rect_copy         (GeglRect *to, GeglRect *from);
void     gegl_rect_bounding_box (GeglRect *dest, GeglRect *src1, GeglRect *src2);
gboolean gegl_rect_intersect    (GeglRect *dest, GeglRect *src1, GeglRect *src2);
gboolean gegl_rect_contains     (GeglRect *r, GeglRect *s); 

gint                gegl_channel_data_type_bytes (GeglChannelDataType data);
void                gegl_init (int *argc, char ***argv); 
GeglColorAlphaSpace gegl_utils_derived_color_alpha_space(GList *inputs);
GeglChannelDataType gegl_utils_derived_channel_data_type(GList *inputs);
GeglColorSpace      gegl_utils_derived_color_space(GList *inputs);
GValue *            gegl_utils_construct_val (gchar *name, guint  n_props, GObjectConstructParam *props);

GType gegl_utils_get_filter_type();
void gegl_log(GLogLevelFlags level, gchar *file, gint line, gchar *function, gchar *format, ...);
void gegl_logv(GLogLevelFlags level, gchar *file, gint line, gchar *function, gchar *format, va_list args);

#define LOG_DEBUG(function, args...)  gegl_log(G_LOG_LEVEL_DEBUG,__FILE__,__LINE__,function,##args)
#define LOG_INFO(function, args...)  gegl_log(G_LOG_LEVEL_INFO,__FILE__,__LINE__,function,##args)
#define LOG_MSG(function, args...)  gegl_log(G_LOG_LEVEL_MESSAGE,__FILE__,__LINE__,function,##args)

#define GEGL_FLOAT_EPSILON                       (1e-5)
#define GEGL_FLOAT_IS_ZERO(value) (_gegl_float_epsilon_zero ((value)))
#define GEGL_FLOAT_EQUAL(v1, v2) (_gegl_float_epsilon_equal ((v1), (v2)))

static inline gint
_gegl_float_epsilon_zero (float value)
{
  return value > -GEGL_FLOAT_EPSILON && value < GEGL_FLOAT_EPSILON; 
}

static inline gint
_gegl_float_epsilon_equal (float v1, float v2)
{
  register float diff = v1 - v2;
  return diff > -GEGL_FLOAT_EPSILON && diff < GEGL_FLOAT_EPSILON; 
}

#endif /* __GEGL_UTILS_H__ */
