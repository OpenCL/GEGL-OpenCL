#ifndef __GEGL_INPUT_PARAM_SPECS_H__
#define __GEGL_INPUT_PARAM_SPECS_H__

#include <glib-object.h>
#include "gegl-utils.h"

extern GType GEGL_TYPE_PARAM_INPUT;

#define GEGL_IS_PARAM_SPEC_INPUT(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_INPUT))
#define GEGL_PARAM_SPEC_INPUT(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_INPUT, GeglParamSpecInput))

typedef struct _GeglParamSpecInput     GeglParamSpecInput;
struct _GeglParamSpecInput
{
  GParamSpec pspec;
};

GParamSpec*  gegl_param_spec_input   (const gchar     *name,
                                      const gchar     *nick,
                                      const gchar     *blurb,
                                      GParamFlags     flags);

void gegl_input_param_spec_types_init (void);

#endif /* __GEGL_INPUT_PARAM_SPECS_H__ */
