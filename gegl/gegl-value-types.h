#ifndef __GEGL_VALUE_TYPES_H__
#define __GEGL_VALUE_TYPES_H__

#include <glib-object.h>
#include "gegl-types.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel  GeglColorModel;
#endif

void gegl_value_types_init (void);
void gegl_value_transform_init(void);

/* --- fundamental types --- */
extern GType GEGL_TYPE_CHANNEL;
extern GType GEGL_TYPE_PIXEL;

/* --- derived types --- */
extern GType GEGL_TYPE_FLOAT;
extern GType GEGL_TYPE_UINT8;
/*extern GType GEGL_TYPE_RGBA_FLOAT;*/
extern GType GEGL_TYPE_RGB_FLOAT;
extern GType GEGL_TYPE_RGB_UINT8;

/* --- type macros --- */
#define G_VALUE_HOLDS_GEGL_UINT8(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_UINT8))
#define G_VALUE_HOLDS_GEGL_FLOAT(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_FLOAT))
/* #define G_VALUE_HOLDS_GEGL_RGBA_FLOAT(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_RGBA_FLOAT)) */
#define G_VALUE_HOLDS_GEGL_RGB_FLOAT(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_RGB_FLOAT))
#define G_VALUE_HOLDS_GEGL_RGB_UINT8(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_RGB_UINT8))

/* --- prototypes --- */
void g_value_set_gegl_uint8 (GValue *value, guint8  v_uint8);
guint8 g_value_get_gegl_uint8 (const GValue *value);

void g_value_set_gegl_float (GValue *value, gfloat  v_float);
gfloat g_value_get_gegl_float (const GValue *value);

/*void g_value_set_gegl_rgba_float (GValue *value, gfloat  r, gfloat  g, gfloat  b, gfloat  a); */
/*void g_value_get_gegl_rgba_float (const GValue *value, gfloat *r, gfloat *g, gfloat *b, gfloat *a); */

void g_value_set_gegl_rgb_float (GValue *value, gfloat  r, gfloat  g, gfloat  b);
void g_value_get_gegl_rgb_float (const GValue *value, gfloat *r, gfloat *g, gfloat *b);

void g_value_set_gegl_rgb_uint8 (GValue *value, guint8  r, guint8  g, guint8  b);
void g_value_get_gegl_rgb_uint8 (const GValue *value, guint8 *r, guint8 *g, guint8 *b);

gpointer g_value_pixel_get_data(GValue * value);
GeglColorModel * g_value_pixel_get_color_model(GValue * value);

void g_value_channel_print(GValue * value);

#endif /* __GEGL_VALUE_TYPES_H__ */
