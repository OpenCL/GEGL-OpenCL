#include    "gegl-image-buffer-param-specs.h"
#include    "gegl-utils.h"
#include    "gegl-image-buffer.h"
#include    "gegl-color-model.h"
#include    "gegl-color-model.h"
#include    "gegl-color-space.h"
#include    "gegl-data-space.h"

GType GEGL_TYPE_PARAM_IMAGE_BUFFER = 0;

/* --- param spec functions --- */

static gboolean
param_spec_image_buffer_validate (GParamSpec *pspec,
                                GValue     *value)
{
  return FALSE;
}

void
gegl_image_buffer_param_spec_types_init (void)
{
  {
    static GParamSpecTypeInfo pspec_info = 
      {
        sizeof (GeglParamSpecImageBuffer), /* instance_size */
        16,                              /* n_preallocs */
        NULL,                            /* instance_init */
        0x0,                             /* value_type */
        NULL,                            /* finalize */
        NULL,                            /* value_set_default */
        param_spec_image_buffer_validate,  /* value_validate */
        NULL,                            /* values_cmp */
      };

    pspec_info.value_type = GEGL_TYPE_IMAGE_BUFFER;
    GEGL_TYPE_PARAM_IMAGE_BUFFER = g_param_type_register_static ("GeglParamImageBuffer", &pspec_info);
    g_assert (GEGL_TYPE_PARAM_IMAGE_BUFFER == g_type_from_name("GeglParamImageBuffer"));
  }
}

GParamSpec*
gegl_param_spec_image_buffer (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            GParamFlags  flags)
{
  GeglParamSpecImageBuffer *ispec;
  ispec = g_param_spec_internal (GEGL_TYPE_PARAM_IMAGE_BUFFER, 
                                 name, nick, blurb, flags);
  return G_PARAM_SPEC (ispec);
}
