#ifndef __GEGL_PARAM_SPECS_H__
#define __GEGL_PARAM_SPECS_H__

#include <glib-object.h>
#include "gegl-utils.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel GeglColorModel;
#endif

#define GEGL_TYPE_PARAM_UINT8                 (gegl_param_spec_types[0])
#define GEGL_IS_PARAM_SPEC_UINT8(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_UINT8))
#define GEGL_PARAM_SPEC_UINT8(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_UINT8, GeglParamSpecUInt8))

#define GEGL_TYPE_PARAM_FLOAT                 (gegl_param_spec_types[1])
#define GEGL_IS_PARAM_SPEC_FLOAT(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_FLOAT))
#define GEGL_PARAM_SPEC_FLOAT(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_FLOAT, GeglParamSpecFloat))

#define GEGL_TYPE_PARAM_RGB_UINT8             (gegl_param_spec_types[2])
#define GEGL_IS_PARAM_SPEC_RGB_UINT8(pspec)   (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_RGB_UINT8))
#define GEGL_PARAM_SPEC_RGB_UINT8(pspec)      (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_RGB_UINT8, GeglParamSpecRgbUInt8))

#define GEGL_TYPE_PARAM_RGB_FLOAT             (gegl_param_spec_types[3])
#define GEGL_IS_PARAM_SPEC_RGB_FLOAT(pspec)   (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_RGB_FLOAT))
#define GEGL_PARAM_SPEC_RGB_FLOAT(pspec)      (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_RGB_FLOAT, GeglParamSpecRgbFloat))

#define GEGL_TYPE_PARAM_IMAGE_DATA            (gegl_param_spec_types[4])
#define GEGL_IS_PARAM_SPEC_IMAGE_DATA(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_IMAGE_DATA))
#define GEGL_PARAM_SPEC_IMAGE_DATA(pspec)     (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_IMAGE_DATA, GeglParamSpecImageData))

typedef struct _GeglParamSpecUInt8     GeglParamSpecUInt8;
struct _GeglParamSpecUInt8
{
  GParamSpec pspec;

  guint8 minimum;
  guint8 maximum;
  guint8 default_value;
};

typedef struct _GeglParamSpecFloat     GeglParamSpecFloat;
struct _GeglParamSpecFloat
{
  GParamSpec pspec;

  gfloat minimum;
  gfloat maximum;
  gfloat default_value;
};

typedef struct _GeglParamSpecRgbUInt8     GeglParamSpecRgbUInt8;
struct _GeglParamSpecRgbUInt8
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

typedef struct _GeglParamSpecRgbFloat     GeglParamSpecRgbFloat;
struct _GeglParamSpecRgbFloat
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

typedef struct _GeglParamSpecImageData       GeglParamSpecImageData;
struct _GeglParamSpecImageData
{
  GParamSpec pspec;

  GeglRect        rect;
  GeglColorModel   *color_model;
};


GParamSpec*  gegl_param_spec_uint8   (const gchar     *name,
                                      const gchar     *nick,
                                      const gchar     *blurb,
                                      guint8 min,
                                      guint8 max,
                                      guint8 default_value,
                                      GParamFlags     flags);

GParamSpec*  gegl_param_spec_float   (const gchar     *name,
                                      const gchar     *nick,
                                      const gchar     *blurb,
                                      gfloat min,
                                      gfloat max,
                                      gfloat default_value,
                                      GParamFlags     flags);

GParamSpec*  gegl_param_spec_rgb_uint8   (const gchar     *name,
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

GParamSpec*  gegl_param_spec_rgb_float   (const gchar     *name,
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

GParamSpec*  gegl_param_spec_image_data (const gchar     *name,
                                         const gchar     *nick,
                                         const gchar     *blurb,
                                         GeglRect        *rect,
                                         GeglColorModel    *color_model,
                                         GParamFlags     flags);


void gegl_param_spec_types_init (void);

#endif /* __GEGL_PARAM_SPECS_H__ */

