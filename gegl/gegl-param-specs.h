#ifndef __GEGL_PARAM_SPECS_H__
#define __GEGL_PARAM_SPECS_H__

#include <glib-object.h>
#include "gegl-utils.h"

#ifndef __TYPEDEF_GEGL_COLOR_MODEL__
#define __TYPEDEF_GEGL_COLOR_MODEL__
typedef struct _GeglColorModel GeglColorModel;
#endif

#define GEGL_TYPE_PARAM_TILE                 (gegl_param_spec_types[0])
#define GEGL_IS_PARAM_SPEC_TILE(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_TILE))
#define GEGL_PARAM_SPEC_TILE(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_TILE, GeglParamSpecTile))


typedef struct _GeglParamSpecTile       GeglParamSpecTile;
struct _GeglParamSpecTile
{
  GParamSpec pspec;

  GeglRect         rect;
  GeglColorModel   *color_model;
};

GParamSpec*  gegl_param_spec_tile   (const gchar     *name,
                                     const gchar     *nick,
                                     const gchar     *blurb,
                                     GeglRect        *rect,
                                     GeglColorModel  *color_model,
                                     GParamFlags     flags);
#endif /* __GEGL_PARAM_SPECS_H__ */
