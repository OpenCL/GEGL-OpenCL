#ifndef __GEGL_PIXEL_VALUE_TYPES_H__
#define __GEGL_PIXEL_VALUE_TYPES_H__

#include <glib-object.h>
#include "gegl-types.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

/* --- derived pixel types --- */
extern GType GEGL_TYPE_PIXEL_RGB_FLOAT;
extern GType GEGL_TYPE_PIXEL_RGBA_FLOAT;
extern GType GEGL_TYPE_PIXEL_RGB_UINT8;
extern GType GEGL_TYPE_PIXEL_RGBA_UINT8;

/* --- type macros --- */
#define G_VALUE_HOLDS_PIXEL_RGB_FLOAT(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_PIXEL_RGB_FLOAT))
#define G_VALUE_HOLDS_PIXEL_RGBA_FLOAT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_PIXEL_RGBA_FLOAT))
#define G_VALUE_HOLDS_PIXEL_RGB_UINT8(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_PIXEL_RGB_UINT8))
#define G_VALUE_HOLDS_PIXEL_RGBA_UINT8(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_PIXEL_RGBA_UINT8))

/* --- prototypes --- */

void gegl_pixel_value_types_init (void);
void gegl_pixel_value_transform_init(void);

void g_value_set_pixel_rgb_float (GValue *value, gfloat  r, gfloat  g, gfloat  b);
void g_value_get_pixel_rgb_float (const GValue *value, gfloat *r, gfloat *g, gfloat *b);

void g_value_set_pixel_rgba_float (GValue *value, gfloat  r, gfloat  g, gfloat  b, gfloat a);
void g_value_get_pixel_rgba_float (const GValue *value, gfloat *r, gfloat *g, gfloat *b, gfloat *a);

void g_value_set_pixel_rgb_uint8 (GValue *value, guint8  r, guint8  g, guint8  b);
void g_value_get_pixel_rgb_uint8 (const GValue *value, guint8 *r, guint8 *g, guint8 *b);

void g_value_set_pixel_rgba_uint8 (GValue *value, guint8  r, guint8  g, guint8  b, guint8 a);
void g_value_get_pixel_rgba_uint8 (const GValue *value, guint8 *r, guint8 *g, guint8 *b, guint8 *a);

gpointer g_value_pixel_get_data(GValue * value);
GeglColorModel * g_value_pixel_get_color_model(GValue * value);

#endif /* __GEGL_PIXEL_VALUE_TYPES_H__ */
