#ifndef __GEGL_IMAGE_DATA_PARAM_SPECS_H__
#define __GEGL_IMAGE_DATA_PARAM_SPECS_H__

#include <glib-object.h>
#include "gegl-utils.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel GeglColorModel;
#endif

extern GType GEGL_TYPE_PARAM_IMAGE_DATA;

#define GEGL_IS_PARAM_SPEC_IMAGE_DATA(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_IMAGE_DATA))
#define GEGL_PARAM_SPEC_IMAGE_DATA(pspec)     (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_IMAGE_DATA, GeglParamSpecImageData))

typedef struct _GeglParamSpecImageData       GeglParamSpecImageData;
struct _GeglParamSpecImageData
{
  GParamSpec pspec;

  GeglRect        rect;
  GeglColorModel   *color_model;
};


GParamSpec*  gegl_param_spec_image_data (const gchar     *name,
                                         const gchar     *nick,
                                         const gchar     *blurb,
                                         GeglRect        *rect,
                                         GeglColorModel    *color_model,
                                         GParamFlags     flags);


void gegl_image_data_param_spec_types_init (void);

#endif /* __GEGL_IMAGE_DATA_PARAM_SPECS_H__ */
