#ifndef __GEGL_PIXEL_PARAM_SPECS_H__
#define __GEGL_PIXEL_PARAM_SPECS_H__

#include <glib-object.h>
#include "gegl-utils.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel GeglColorModel;
#endif

extern GType GEGL_TYPE_PARAM_PIXEL;
extern GType GEGL_TYPE_PARAM_PIXEL_RGB_UINT8;
extern GType GEGL_TYPE_PARAM_PIXEL_RGBA_UINT8;
extern GType GEGL_TYPE_PARAM_PIXEL_RGB_FLOAT;
extern GType GEGL_TYPE_PARAM_PIXEL_RGBA_FLOAT;

#define GEGL_IS_PARAM_SPEC_PIXEL(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_PIXEL))
#define GEGL_PARAM_SPEC_PIXEL(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_PIXEL, GeglParamSpecPixel))

#define GEGL_IS_PARAM_SPEC_PIXEL_RGB_UINT8(pspec)   (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_PIXEL_RGB_UINT8))
#define GEGL_PARAM_SPEC_PIXEL_RGB_UINT8(pspec)      (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_PIXEL_RGB_UINT8, GeglParamSpecPixelRgbUInt8))

#define GEGL_IS_PARAM_SPEC_PIXEL_RGBA_UINT8(pspec)   (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_PIXEL_RGBA_UINT8))
#define GEGL_PARAM_SPEC_PIXEL_RGBA_UINT8(pspec)      (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_PIXEL_RGBA_UINT8, GeglParamSpecPixelRgbaUInt8))

#define GEGL_IS_PARAM_SPEC_PIXEL_RGB_FLOAT(pspec)   (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_PIXEL_RGB_FLOAT))
#define GEGL_PARAM_SPEC_PIXEL_RGB_FLOAT(pspec)      (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_PIXEL_RGB_FLOAT, GeglParamSpecPixelRgbFloat))

#define GEGL_IS_PARAM_SPEC_PIXEL_RGBA_FLOAT(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_PIXEL_RGBA_FLOAT))
#define GEGL_PARAM_SPEC_PIXEL_RGBA_FLOAT(pspec)      (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_PIXEL_RGBA_FLOAT, GeglParamSpecPixelRgbaFloat))

typedef struct _GeglParamSpecPixel     GeglParamSpecPixel;
struct _GeglParamSpecPixel
{
  GParamSpec pspec;
};

typedef struct _GeglParamSpecPixelRgbUInt8     GeglParamSpecPixelRgbUInt8;
struct _GeglParamSpecPixelRgbUInt8
{
  GParamSpec pspec;

  guint8 min_red;
  guint8 max_red;
  guint8 min_green;
  guint8 max_green;
  guint8 min_blue;
  guint8 max_blue;

  guint8 default_red;
  guint8 default_green;
  guint8 default_blue;
};

typedef struct _GeglParamSpecPixelRgbaUInt8     GeglParamSpecPixelRgbaUInt8;
struct _GeglParamSpecPixelRgbaUInt8
{
  GParamSpec pspec;

  guint8 min_red;
  guint8 max_red;
  guint8 min_green;
  guint8 max_green;
  guint8 min_blue;
  guint8 max_blue;
  guint8 min_alpha;
  guint8 max_alpha;

  guint8 default_red;
  guint8 default_green;
  guint8 default_blue;
  guint8 default_alpha;
};

typedef struct _GeglParamSpecPixelRgbFloat     GeglParamSpecPixelRgbFloat;
struct _GeglParamSpecPixelRgbFloat
{
  GParamSpec pspec;

  gfloat min_red;
  gfloat max_red;
  gfloat min_green;
  gfloat max_green;
  gfloat min_blue;
  gfloat max_blue;

  gfloat default_red;
  gfloat default_green;
  gfloat default_blue;
};

typedef struct _GeglParamSpecPixelRgbaFloat     GeglParamSpecPixelRgbaFloat;
struct _GeglParamSpecPixelRgbaFloat
{
  GParamSpec pspec;

  gfloat min_red;
  gfloat max_red;
  gfloat min_green;
  gfloat max_green;
  gfloat min_blue;
  gfloat max_blue;
  gfloat min_alpha;
  gfloat max_alpha;

  gfloat default_red;
  gfloat default_green;
  gfloat default_blue;
  gfloat default_alpha;
};

GParamSpec*  gegl_param_spec_pixel   (const gchar     *name,
                                      const gchar     *nick,
                                      const gchar     *blurb,
                                      GParamFlags     flags);

GParamSpec*  gegl_param_spec_pixel_rgb_uint8   (const gchar     *name,
                                                const gchar     *nick,
                                                const gchar     *blurb,
                                                guint8 min_red, 
                                                guint8 max_red,
                                                guint8 min_green, 
                                                guint8 max_green,
                                                guint8 min_blue, 
                                                guint8 max_blue,
                                                guint8 default_red,
                                                guint8 default_green,
                                                guint8 default_blue,
                                                GParamFlags     flags);

GParamSpec*  gegl_param_spec_pixel_rgba_uint8   (const gchar     *name,
                                                 const gchar     *nick,
                                                 const gchar     *blurb,
                                                 guint8 min_red, 
                                                 guint8 max_red,
                                                 guint8 min_green, 
                                                 guint8 max_green,
                                                 guint8 min_blue, 
                                                 guint8 max_blue,
                                                 guint8 min_alpha, 
                                                 guint8 max_alpha,
                                                 guint8 default_red,
                                                 guint8 default_green,
                                                 guint8 default_blue,
                                                 guint8 default_alpha,
                                                 GParamFlags     flags);

GParamSpec*  gegl_param_spec_pixel_rgb_float   (const gchar     *name,
                                                const gchar     *nick,
                                                const gchar     *blurb,
                                                gfloat min_red, 
                                                gfloat max_red,
                                                gfloat min_green, 
                                                gfloat max_green,
                                                gfloat min_blue, 
                                                gfloat max_blue,
                                                gfloat default_red,
                                                gfloat default_green,
                                                gfloat default_blue,
                                                GParamFlags     flags);

GParamSpec*  gegl_param_spec_pixel_rgba_float   (const gchar     *name,
                                                 const gchar     *nick,
                                                 const gchar     *blurb,
                                                 gfloat min_red, 
                                                 gfloat max_red,
                                                 gfloat min_green, 
                                                 gfloat max_green,
                                                 gfloat min_blue, 
                                                 gfloat max_blue,
                                                 gfloat min_alpha, 
                                                 gfloat max_alpha,
                                                 gfloat default_red,
                                                 gfloat default_green,
                                                 gfloat default_blue,
                                                 gfloat default_alpha,
                                                 GParamFlags     flags);
void gegl_pixel_param_spec_types_init (void);

#endif /* __GEGL_PIXEL_PARAM_SPECS_H__ */

