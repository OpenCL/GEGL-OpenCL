#include    "gegl-image-data-param-specs.h"
#include    "gegl-value-types.h"
#include    "gegl-utils.h"
#include    "gegl-image-data.h"
#include    "gegl-color-model.h"
#include    "gegl-color-model.h"
#include    "gegl-color-space.h"
#include    "gegl-data-space.h"

GType GEGL_TYPE_PARAM_IMAGE_DATA = 0;

/* --- param spec functions --- */

static gboolean
param_spec_image_data_validate (GParamSpec *pspec,
                                GValue     *value)
{
  return FALSE;
}

void
gegl_image_data_param_spec_types_init (void)
{
  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecImageData), /* instance_size */
        16,                              /* n_preallocs */
        NULL,                            /* instance_init */
        0x0,                             /* value_type */
        NULL,                            /* finalize */
        NULL,                            /* value_set_default */
        param_spec_image_data_validate,  /* value_validate */
        NULL,                            /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_IMAGE_DATA;
    GEGL_TYPE_PARAM_IMAGE_DATA = g_param_type_register_static ("GeglParamImageData", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_IMAGE_DATA == g_type_from_name("GeglParamImageData"));
  }
}

GParamSpec*
gegl_param_spec_image_data (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            GeglRect *rect,
                            GeglColorModel *color_model,
                            GParamFlags  flags)
{
  GeglParamSpecImageData *ispec;

  ispec = g_param_spec_internal (GEGL_TYPE_PARAM_IMAGE_DATA, 
                                 name, nick, blurb, flags);
  
  gegl_rect_copy(&ispec->rect, rect);
  ispec->color_model = color_model;
  
  return G_PARAM_SPEC (ispec);
}
