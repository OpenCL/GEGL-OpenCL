#ifndef __GEGL_UTILS_H__
#define __GEGL_UTILS_H__

#include <glib-object.h>
#include "gegl-types.h"

void     gegl_rect_set          (GeglRect *r, gint x, gint y, guint w, guint h);
gboolean gegl_rect_equal        (GeglRect *r, GeglRect *s); 
gboolean gegl_rect_equal_coords (GeglRect *r, gint x, gint y, gint w, gint h);
void     gegl_rect_copy         (GeglRect *to, GeglRect *from);
void     gegl_rect_bounding_box (GeglRect *dest, GeglRect *src1, GeglRect *src2);
gboolean gegl_rect_intersect    (GeglRect *dest, GeglRect *src1, GeglRect *src2);
gboolean gegl_rect_contains     (GeglRect *r, GeglRect *s); 

void gegl_log_debug(gchar *function, gchar * format, ...);
void gegl_log_info(gchar *function, gchar * format, ...);
void gegl_log_message(gchar *function, gchar * format, ...);
void gegl_log_direct(gchar * format, ...);

#define GEGL_FLOAT_EPSILON                       (1e-5)
#define GEGL_FLOAT_IS_ZERO(value) (_gegl_float_epsilon_zero ((value)))
#define GEGL_FLOAT_EQUAL(v1, v2) (_gegl_float_epsilon_equal ((v1), (v2)))

#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

inline gint _gegl_float_epsilon_zero (float value);
inline gint _gegl_float_epsilon_equal (float v1, float v2);

#endif /* __GEGL_UTILS_H__ */
