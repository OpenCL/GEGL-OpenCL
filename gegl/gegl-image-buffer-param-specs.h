#ifndef __GEGL_IMAGE_BUFFER_PARAM_SPECS_H__
#define __GEGL_IMAGE_BUFFER_PARAM_SPECS_H__

#include <glib-object.h>
#include "gegl-utils.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel GeglColorModel;
#endif

extern GType GEGL_TYPE_PARAM_IMAGE_BUFFER;

#define GEGL_IS_PARAM_SPEC_IMAGE_BUFFER(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_IMAGE_BUFFER))
#define GEGL_PARAM_SPEC_IMAGE_BUFFER(pspec)     (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_IMAGE_BUFFER, GeglParamSpecImageBuffer))

typedef struct _GeglParamSpecImageBuffer       GeglParamSpecImageBuffer;
struct _GeglParamSpecImageBuffer
{
  GParamSpec pspec;
};


GParamSpec*  gegl_param_spec_image_buffer (const gchar     *name,
                                         const gchar     *nick,
                                         const gchar     *blurb,
                                         GParamFlags     flags);


void gegl_image_buffer_param_spec_types_init (void);

#endif /* __GEGL_IMAGE_BUFFER_PARAM_SPECS_H__ */
