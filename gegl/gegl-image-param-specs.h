#ifndef __GEGL_IMAGE_PARAM_SPECS_H__
#define __GEGL_IMAGE_PARAM_SPECS_H__

#include <glib-object.h>
#include "gegl-utils.h"

extern GType GEGL_TYPE_PARAM_IMAGE;

#define GEGL_IS_PARAM_SPEC_IMAGE(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_IMAGE))
#define GEGL_PARAM_SPEC_IMAGE(pspec)     (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_IMAGE, GeglParamSpecImage))

typedef struct _GeglParamSpecImage       GeglParamSpecImage;
struct _GeglParamSpecImage
{
  GParamSpec pspec;
};


GParamSpec*  gegl_param_spec_image (const gchar     *name,
                                         const gchar     *nick,
                                         const gchar     *blurb,
                                         GParamFlags     flags);


void gegl_image_param_spec_types_init (void);

#endif /* __GEGL_IMAGE_PARAM_SPECS_H__ */
