#ifndef __GEGL_ARRAY_PARAM_SPECS_H__
#define __GEGL_ARRAY_PARAM_SPECS_H__

#include <glib-object.h>

extern GType GEGL_TYPE_PARAM_FLOAT_ARRAY;

#define GEGL_IS_PARAM_SPEC_FLOAT_ARRAY(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_FLOAT_ARRAY))
#define GEGL_PARAM_SPEC_FLOAT_ARRAY(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_FLOAT_ARRAY, GeglParamSpecFloatArray))

typedef struct _GeglParamSpecFloatArray     GeglParamSpecFloatArray;
struct _GeglParamSpecFloatArray
{
  GParamSpec pspec;
};


void gegl_array_param_spec_types_init (void);
GParamSpec* gegl_param_spec_float_array (const gchar *name, const gchar *nick, const gchar *blurb, GParamFlags  flags);

#endif /* __GEGL_ARRAY_PARAM_SPECS_H__ */

