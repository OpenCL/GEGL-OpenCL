#ifndef __GEGL_PARAM_SPECS_H__
#define __GEGL_PARAM_SPECS_H__

#include <glib-object.h>

/* --- type macros --- */
#define GEGL_TYPE_PARAM_INT                  (gegl_param_spec_types[0])
#define GEGL_IS_PARAM_SPEC_INT(pspec)        (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_INT))
#define GEGL_PARAM_SPEC_INT(pspec)           (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_INT, GeglParamSpecInt))

#if 0
#define GEGL_TYPE_PARAM_IMAGE_DATA                 (gegl_param_spec_types[1])
#define GEGL_IS_PARAM_SPEC_IMAGE_DATA(pspec)       (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_IMAGE_DATA))
#define GEGL_PARAM_SPEC_IMAGE_DATA(pspec)          (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GEGL_TYPE_PARAM_IMAGE_DATA, GeglParamSpecTile))
#endif


/* --- typedefs & structures --- */
typedef struct _GeglParamSpecInt       GeglParamSpecInt;

struct _GeglParamSpecInt
{
  GParamSpecInt    pspec;
  gint             increment;
};

/* --- GeglParamSpec prototypes --- */
GParamSpec*  gegl_param_spec_int     (const gchar     *name,
                                      const gchar     *nick,
                                      const gchar     *blurb,
                                      gint            minimum,
                                      gint            maximum,
                                      gint            default_value,
                                      gint            increment,
                                      GParamFlags     flags);

#if 0
struct _GeglParamSpecImageData
{
  GParamSpec       pspec;

  GeglRect         rect;
  GeglColorModel   *color_model;
};

GParamSpec*  gegl_param_spec_image_data   (const gchar     *name,
                                           const gchar     *nick,
                                           const gchar     *blurb,
                                           GeglRect        *rect,
                                           GeglColorModel  *color_model,
                                           GParamFlags     flags);
#endif

#endif /* __GEGL_PARAM_SPECS_H__ */
