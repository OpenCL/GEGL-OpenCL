#ifndef __GEGL_PIXEL_VALUE_TYPES_H__
#define __GEGL_PIXEL_VALUE_TYPES_H__

#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-color-model.h"

/* --- derived pixel types --- */
extern GType GEGL_TYPE_PIXEL_RGB_FLOAT;
extern GType GEGL_TYPE_PIXEL_RGBA_FLOAT;
extern GType GEGL_TYPE_PIXEL_GRAY_FLOAT;
extern GType GEGL_TYPE_PIXEL_GRAYA_FLOAT;

extern GType GEGL_TYPE_PIXEL_RGB_UINT8;
extern GType GEGL_TYPE_PIXEL_RGBA_UINT8;

/* --- type macros --- */
#define G_VALUE_HOLDS_PIXEL_RGB_FLOAT(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_PIXEL_RGB_FLOAT))
#define G_VALUE_HOLDS_PIXEL_RGBA_FLOAT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_PIXEL_RGBA_FLOAT))
#define G_VALUE_HOLDS_PIXEL_GRAY_FLOAT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_PIXEL_GRAY_FLOAT))
#define G_VALUE_HOLDS_PIXEL_GRAYA_FLOAT(value)  (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_PIXEL_GRAYA_FLOAT))

#define G_VALUE_HOLDS_PIXEL_RGB_UINT8(value)    (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_PIXEL_RGB_UINT8))
#define G_VALUE_HOLDS_PIXEL_RGBA_UINT8(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_PIXEL_RGBA_UINT8))

typedef struct _PixelValueInfo PixelValueInfo;
struct _PixelValueInfo
{
  gchar * channel_type_name; 
  gchar * color_space_name;
  gchar * color_model_name;
  gboolean has_alpha;
};

/* --- prototypes --- */
void gegl_pixel_value_types_init (void);
void gegl_pixel_value_transform_init(void);

void g_value_set_pixel_rgb_float (GValue *value, gfloat  r, gfloat  g, gfloat  b);
void g_value_get_pixel_rgb_float (const GValue *value, gfloat *r, gfloat *g, gfloat *b);

void g_value_set_pixel_rgba_float (GValue *value, gfloat  r, gfloat  g, gfloat  b, gfloat a);
void g_value_get_pixel_rgba_float (const GValue *value, gfloat *r, gfloat *g, gfloat *b, gfloat *a);

void g_value_set_pixel_gray_float (GValue *value, gfloat  gray);
void g_value_get_pixel_gray_float (const GValue *value, gfloat *gray);

void g_value_set_pixel_graya_float (GValue *value, gfloat  gray, gfloat  alpha);
void g_value_get_pixel_graya_float (const GValue *value, gfloat *gray, gfloat *alpha);

void g_value_set_pixel_rgb_uint8 (GValue *value, guint8  r, guint8  g, guint8  b);
void g_value_get_pixel_rgb_uint8 (const GValue *value, guint8 *r, guint8 *g, guint8 *b);

void g_value_set_pixel_rgba_uint8 (GValue *value, guint8  r, guint8  g, guint8  b, guint8 a);
void g_value_get_pixel_rgba_uint8 (const GValue *value, guint8 *r, guint8 *g, guint8 *b, guint8 *a);

gpointer g_value_pixel_get_data(GValue * value);
GeglColorModel * g_value_pixel_get_color_model(GValue * value);

#endif /* __GEGL_PIXEL_VALUE_TYPES_H__ */
