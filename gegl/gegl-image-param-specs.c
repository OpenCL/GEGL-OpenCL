#include    "gegl-image-param-specs.h"
#include    "gegl-utils.h"
#include    "gegl-image.h"
#include    "gegl-color-model.h"
#include    "gegl-color-model.h"
#include    "gegl-color-space.h"
#include    "gegl-channel-space.h"

GType GEGL_TYPE_PARAM_IMAGE = 0;

/* --- param spec functions --- */

static gboolean
param_spec_image_validate (GParamSpec *pspec,
                                GValue     *value)
{
  return FALSE;
}

void
gegl_image_param_spec_types_init (void)
{
  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecImage), /* instance_size */
        16,                              /* n_preallocs */
        NULL,                            /* instance_init */
        0x0,                             /* value_type */
        NULL,                            /* finalize */
        NULL,                            /* value_set_default */
        param_spec_image_validate,  /* value_validate */
        NULL,                            /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_IMAGE;
    GEGL_TYPE_PARAM_IMAGE = g_param_type_register_static ("GeglParamImage", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_IMAGE == g_type_from_name("GeglParamImage"));
  }
}

GParamSpec*
gegl_param_spec_image (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            GParamFlags  flags)
{
  GeglParamSpecImage *ispec;
  ispec = g_param_spec_internal (GEGL_TYPE_PARAM_IMAGE, 
                                 name, nick, blurb, flags);
  return G_PARAM_SPEC (ispec);
}
