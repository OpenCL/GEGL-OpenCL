#ifndef __GEGL_UTILS_H__
#define __GEGL_UTILS_H__

#include <glib-object.h>
#include "gegl-types.h"

#ifndef __TYPEDEF_GEGL_NODE__
#define __TYPEDEF_GEGL_NODE__
typedef struct _GeglNode  GeglNode;
#endif

#ifndef __TYPEDEF_GEGL_OP__
#define __TYPEDEF_GEGL_OP__
typedef struct _GeglOp  GeglOp;
#endif

void     gegl_rect_set          (GeglRect *r, gint x, gint y, guint w, guint h);
gboolean gegl_rect_equal        (GeglRect *r, GeglRect *s); 
gboolean gegl_rect_equal_coords (GeglRect *r, gint x, gint y, gint w, gint h);
void     gegl_rect_copy         (GeglRect *to, GeglRect *from);
void     gegl_rect_bounding_box (GeglRect *dest, GeglRect *src1, GeglRect *src2);
gboolean gegl_rect_intersect    (GeglRect *dest, GeglRect *src1, GeglRect *src2);
gboolean gegl_rect_contains     (GeglRect *r, GeglRect *s); 

void                gegl_dump_graph(GeglNode * root);
void                gegl_dump_graph_msg(gchar * msg, GeglNode * root); 

void gegl_log(GLogLevelFlags level, gchar *file, gint line, gchar *function, gchar *format, ...);
void gegl_logv(GLogLevelFlags level, gchar *file, gint line, gchar *function, gchar *format, va_list args);
void gegl_direct_log(GLogLevelFlags level, gchar *format, ...);
void gegl_direct_logv(GLogLevelFlags level, gchar *format, va_list args);

#define LOG_DEBUG(function, args...)  gegl_log(G_LOG_LEVEL_DEBUG,__FILE__,__LINE__,function,##args)
#define LOG_INFO(function, args...)  gegl_log(G_LOG_LEVEL_INFO,__FILE__,__LINE__,function,##args)
#define LOG_MSG(function, args...)  gegl_log(G_LOG_LEVEL_MESSAGE,__FILE__,__LINE__,function,##args)
#define LOG_DIRECT(args...)  gegl_direct_log(G_LOG_LEVEL_DEBUG,##args)

#define GEGL_FLOAT_EPSILON                       (1e-5)
#define GEGL_FLOAT_IS_ZERO(value) (_gegl_float_epsilon_zero ((value)))
#define GEGL_FLOAT_EQUAL(v1, v2) (_gegl_float_epsilon_equal ((v1), (v2)))

#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

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
