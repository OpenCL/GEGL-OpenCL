#ifndef __GEGL_M_SOURCE_PARAM_SPECS_H__
#define __GEGL_M_SOURCE_PARAM_SPECS_H__

#include <glib-object.h>
#include "gegl-utils.h"

extern GType GEGL_TYPE_PARAM_M_SOURCE;

#define GEGL_IS_PARAM_SPEC_M_SOURCE(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_M_SOURCE))
#define GEGL_PARAM_SPEC_M_SOURCE(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_M_SOURCE, GeglParamSpecMSource))

typedef struct _GeglParamSpecMSource     GeglParamSpecMSource;
struct _GeglParamSpecMSource
{
  GParamSpec pspec;
};

GParamSpec*  gegl_param_spec_m_source   (const gchar     *name,
                                      const gchar     *nick,
                                      const gchar     *blurb,
                                      GParamFlags     flags);

void gegl_m_source_param_spec_types_init (void);

#endif /* __GEGL_M_SOURCE_PARAM_SPECS_H__ */
